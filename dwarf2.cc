#include "cwd_sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include "cwd_dwarf2.h"
#include <libcwd/ObjectFileName.h>
#include "libcwd/debug.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <elf.h>
#include <filesystem>
#include <fcntl.h>
#include <link.h>
#include <limits>
#include <string>
#include <elfutils/libdw.h>

namespace libcwd::dwarf2 {

// Detect if this libcwd library is static or not.
#ifdef __PIC__
constexpr bool statically_linked = false;
#else
constexpr bool statically_linked = true;
#endif
static bool WST_initialized = false;                      // MT: Set here to false, set to `true' once in `ST_init'.

//static
ObjectFileBase::object_files_t ObjectFileBase::s_object_files_;

// class ObjectFile
//
// Represents a single object file: either the executable or a shared library.
// The base class `ObjectFileBase` has a protected static member `s_object_files_`
// that is a read-write lock protected std::map<uintptr_t, PTLoadSegment*>.
//
// The get access to the list you need to read and/or write lock it.
//
// To obtain a read lock, create a stack variable,
//
//   object_files_t::rat object_files_r(s_object_files_);       // Scoped read-lock for s_object_files_.
//
// Use object_files_r-> for read access (returns a std::map<uintptr_t, PTLoadSegment*> const*).
//
// To obtain a write lock, create a stack variable,
//
//   object_files_t::wat object_files_w(s_object_files_);       // Scoped write-lock for s_object_files_.
//
// Use object_files_w-> for write access (returns a std::map<uintptr_t, PTLoadSegment*>*).
//
class ObjectFile : public ObjectFileBase
{
  static std::atomic_bool s_object_files_initialized_;

 private:
  uintptr_t const lbase_;
  Dwarf* dwarf_handle_{nullptr};
  int dwarf_fd_{-1};

 public:
  ObjectFile(char const* filename, uintptr_t lbase);
  ~ObjectFile();

  // Accessors.
  uintptr_t lbase() const { return lbase_; }
  bool is_initialized() const { return dwarf_fd_ != -1; }

  // Information passed to the cb_dl_iterate_phdr call back function.
  struct CallBackData {
    object_files_t::wat object_files_w;         // Write Access Type to ObjectFileBase::s_object_files_.
    std::string fullpath_;                      // The full path to the current object file being processed.

    CallBackData(object_files_t& object_files, std::string const& fullpath) : object_files_w(object_files), fullpath_(fullpath) { }
  };

 private:
  // Called by constructor.
  void load_symbols();

  void open_dwarf(LIBCWD_TSD_PARAM);
  void close_dwarf(LIBCWD_TSD_PARAM);

  friend bool dwarf2::ST_init(LIBCWD_TSD_PARAM);
  static void load_object_files();
  static int cb_dl_iterate_phdr(dl_phdr_info* info, size_t size, void* call_back_data);
  static bool already_loaded(uintptr_t lbase, object_files_t::wat const& object_files_w);
};

ObjectFile::ObjectFile(char const* filename, uintptr_t lbase) : ObjectFileBase(filename), lbase_(lbase)
{
  Dout(dc::bfd, "dwarf2: new ObjectFile \"" << filename << "\" with load base 0x" << std::hex << lbase);
  load_symbols();
}

ObjectFile::~ObjectFile()
{
  Dout(dc::bfd, "dwarf2: destroying ObjectFile \"" << object_file_name_.filepath() << "\".");

  LIBCWD_TSD_DECLARATION;
  close_dwarf(LIBCWD_TSD);
}

//static
std::atomic_bool ObjectFile::s_object_files_initialized_ = false;

// class PTLoadSegment
//
// Represents one loadable runtime segment of an ObjectFile.  Instances are
// created while holding ObjectFileBase::s_object_files_' write lock during
// initialization and then treated as immutable; later address lookup can read
// the segment start/end/flags and follow object_file() without taking a
// per-segment or per-object lock.
class PTLoadSegment
{
 private:
  ObjectFile const* object_file_;
  uintptr_t start_addr_;
  uintptr_t end_addr_;
  ElfW(Word) flags_;

 public:
  PTLoadSegment(ObjectFile const* object_file, uintptr_t start_addr, uintptr_t end_addr, ElfW(Word) flags) :
    object_file_(object_file), start_addr_(start_addr), end_addr_(end_addr), flags_(flags) { }

  ObjectFile const* object_file() const { return object_file_; }
  uintptr_t start_addr() const { return start_addr_; }
  uintptr_t end_addr() const { return end_addr_; }
  ElfW(Word) flags() const { return flags_; }
};

namespace {

std::string read_build_id(std::string const& object_file LIBCWD_COMMA_TSD_PARAM)
{
  // Determine the working version (the ELF version supported by both, the libelf library and this program).
  if (elf_version(EV_CURRENT) == EV_NONE)
  {
    DoutFatal(dc::fatal, "Warning: libelf is out of date. Can't read build-id of \"" << object_file << "\" (or any object file).");
  }
  else
  {
    // Open the ELF object file.
    int fd = open(object_file.c_str(), O_RDONLY);
    if (fd == -1)
      Dout(dc::warning, "failed to open file \"" << object_file << "\"");
    else
    {
      // Open an ELF descriptor for reading.
      ::Elf* e = elf_begin(fd, ELF_C_READ, NULL);
      if (!e)
        Dout(dc::warning, "elf_begin returned NULL for \"" << object_file << "\": " << elf_errmsg(-1));
      else
      {
        if (elf_kind(e) != ELF_K_ELF)
        {
          //std::cout << object_file << ": skipping, not an ELF file." << std::endl;
        }
        else
        {
#if 0
          // Get the string table section index.
          size_t shstrndx;
          if (elf_getshdrstrndx(e, &shstrndx) != 0)
          {
            std::cerr << "elf_getshdrstrndx() failed: " << elf_errmsg(-1) << std::endl;
            return "";
          }
#endif
          // Run over all sections in the ELF file.
          Elf_Scn* scn = nullptr;
          while ((scn = elf_nextscn(e, scn)) != nullptr)
          {
            // Get the section header.
            GElf_Shdr shdr;
            gelf_getshdr(scn, &shdr);
            if (shdr.sh_type == SHT_NOTE)
            {
#if 0
              // Get the name of the section
              char const* name = elf_strptr(e, shstrndx, shdr.sh_name);
              std::cout << "Section: " << name << std::endl;
#endif
              Elf_Data* data = elf_getdata(scn, NULL);
              GElf_Nhdr nhdr;
              size_t name_offset, desc_offset, offset = 0;
              while ((offset = gelf_getnote(data, offset, &nhdr, &name_offset, &desc_offset)) > 0)
              {
                if (nhdr.n_type == NT_GNU_BUILD_ID && nhdr.n_namesz == 4 && strncmp((char*)data->d_buf + name_offset, "GNU", 4) == 0)
                {
                  unsigned char *desc = (unsigned char *)data->d_buf + desc_offset;
                  std::string result;
                  for (int i = 0; i < nhdr.n_descsz; ++i)
                  {
                    unsigned int val = (unsigned int)desc[i] & 0xffU;
                    static char const* hexchar = "0123456789abcdef";
                    for (int digit = 16; digit; digit /= 16)
                    {
                      result += hexchar[val / digit];
                      val %= digit;
                    }
                  }
                  //std::cout << "build-id of " << object_file << " is " << oss.str() << std::endl;
                  return result;
                }
              }
            }
          }
        }
        elf_end(e);
      }
      close(fd);
    }
  }
  return {};
}

std::filesystem::path get_debug_info_path(std::string object_file LIBCWD_COMMA_TSD_PARAM)
{
  using namespace std::filesystem;
  path build_id_dir = "/usr/lib/debug/.build-id";
  std::error_code ec;
  file_status sr = status(build_id_dir, ec);

  path debug_info_path;
  std::string build_id;
  if (!ec && is_directory(sr))
  {
    // Get the build-id, if any.
    build_id = read_build_id(object_file LIBCWD_COMMA_TSD);
    if (build_id.length() > 2)
      debug_info_path = build_id_dir / build_id.substr(0, 2) / (build_id.substr(2) + ".debug");
  }
  else
    debug_info_path = "/usr/lib/debug" + object_file + ".debug";
  sr = status(debug_info_path, ec);
  if (!ec)
  {
    // Try to find a debug_info file already downloaded by debuginfod.
    char const* cache_path_envvar;
    debug_info_path = object_file;
    for (int path_attempt = 0; path_attempt < 4; ++path_attempt)
    {
      switch (path_attempt)
      {
        case 0:
          cache_path_envvar = getenv("DEBUGINFOD_CACHE_PATH");
          break;
        case 1:
          cache_path_envvar = getenv("XDG_CACHE_HOME");
          break;
        default:
          cache_path_envvar = getenv("HOME");
          break;
      }
      if (cache_path_envvar == nullptr)
        continue;
      path cache_path = cache_path_envvar;
      switch (path_attempt)
      {
        case 0:
          break;
        case 1:
          cache_path = cache_path / "debuginfod_client/";
          break;
        case 2:
          cache_path = cache_path / ".cache/debuginfod_client/";
          break;
        case 3:
          cache_path = cache_path / ".debuginfod_client_cache/";
          break;
      }
      cache_path = cache_path / build_id / "debuginfo";
      sr = status(cache_path, ec);
      if (!ec && exists(sr))
      {
        debug_info_path = cache_path;
        break;
      }
    }
  }
  return debug_info_path;
}

// Find the full path to the current running process.
// This needs to work before we reach main, therefore it uses the /proc filesystem.
std::string current_executable_path()
{
  std::string result(256, '\0');

  for (;;)
  {
    ssize_t const n = ::readlink("/proc/self/exe", result.data(), result.size());

    if (n < 0)
      throw std::runtime_error(std::string("readlink(/proc/self/exe) failed: ") + std::strerror(errno));

    if (static_cast<size_t>(n) < result.size())
    {
      result.resize(static_cast<size_t>(n));
      return result;
    }

    result.resize(result.size() * 2);
  }
}

// Return a compact textual representation of ELF program-header permission flags.
// The result is used only for diagnostics while building the dwarf2 object/segment cache.
// Output character positions correspond to respectively PF_R, PF_W, PF_X; a dash is
// printed if that permission bit is absent.
char const* flags_to_string(ElfW(Word) flags)
{
  static thread_local char result[4];
  result[0] = (flags & PF_R) ? 'R' : '-';
  result[1] = (flags & PF_W) ? 'W' : '-';
  result[2] = (flags & PF_X) ? 'X' : '-';
  result[3] = '\0';
  return result;
}

} // namespace

//static
void ObjectFile::load_object_files()
{
  // LIBCWD_NO_STARTUP_MSGS deliberately suppresses even the loading trace that
  // LIBCWD_PRINT_LOADING would otherwise force on.
  //
  // Do we need debug output regarding the loading of object files and their symbols?
  bool const forced_loading_output = _private_::always_print_loading && !_private_::suppress_startup_msgs;

  // If so, store previous state of libcw_do and dc::bfd.
  libcwd::debug_ct::OnOffState state;
  libcwd::channel_ct::OnOffState state2;
  if (forced_loading_output)
  {
    Debug(libcw_do.force_on(state));
    Debug(dc::bfd.force_on(state2, "BFD"));
  }

  // Initialize object files list, we don't really need the
  // write lock because this function is Single Threaded.
  //
  // Start a new scope for the write lock.
  {
    CallBackData data{s_object_files_, current_executable_path()};

    // Load executable and shared objects.
    if (!statically_linked)
    {
      // Iterate over all currently loaded object files.
      dl_iterate_phdr(cb_dl_iterate_phdr, &data);
    }

    s_object_files_initialized_ = true;
  } // Unlock s_object_files_.

  if (forced_loading_output)
  {
    Debug(dc::bfd.restore(state2));
    Debug(libcw_do.restore(state));
  }
}

//static
int ObjectFile::cb_dl_iterate_phdr(dl_phdr_info* info, size_t size, void* call_back_data)
{
  CallBackData const* data = static_cast<CallBackData const*>(call_back_data);

  // dl_iterate_phdr reports an empty name for the main executable.
  // Use the already resolved /proc/self/exe path in that case so ObjectFileName remains
  // useful for diagnostics and later public location data.
  char const* object_filename = (info->dlpi_name && info->dlpi_name[0] != '\0') ? info->dlpi_name : data->fullpath_.c_str();

  // Create new ObjectFile.
  ObjectFile const* object_file = new ObjectFile(object_filename, static_cast<uintptr_t>(info->dlpi_addr));

  // Insert one immutable PTLoadSegment per loadable segment into the end-keyed map.
  // The map owns the discoverable active address ranges; the pointed-to objects are intentionally kept stable
  // for the process lifetime until later dlclose/tombstone semantics are designed.
  for (ElfW(Half) phdr_index = 0; phdr_index < info->dlpi_phnum; ++phdr_index)
  {
    ElfW(Phdr) const& phdr = info->dlpi_phdr[phdr_index];
    if (phdr.p_type != PT_LOAD ||
        // The Elf-64 Object File Format specifies that p_memsz may be zero (https://man7.org/linux/man-pages/man5/elf.5.html)
        phdr.p_memsz == 0)
      continue;

    uintptr_t const segment_start = static_cast<uintptr_t>(info->dlpi_addr + phdr.p_vaddr);
    uintptr_t const segment_end = segment_start + static_cast<uintptr_t>(phdr.p_memsz);
    LIBCWD_ASSERT(segment_end > segment_start);

    PTLoadSegment const* segment = new PTLoadSegment(object_file, segment_start, segment_end, phdr.p_flags);
    auto const inserted = data->object_files_w->emplace(segment_end, segment);
    if (!inserted.second)
    {
      Dout(dc::warning, "Ignoring duplicate PT_LOAD end address " << (void*)segment_end << " for \"" << object_filename << "\"");
      delete segment;
    }
    else
    {
      Dout(dc::bfd, "dwarf2:     map[" << (void*)segment_end << "] = PTLoadSegment " << segment <<
          " [" << (void*)segment_start << '-' << (void*)segment_end << ") flags=" << flags_to_string(phdr.p_flags) <<
          " object=" << object_file);
    }
  }

  return 0;
}

//static
bool ObjectFile::already_loaded(uintptr_t lbase, object_files_t::wat const& object_files_w)
{
}

void ObjectFile::load_symbols()
{
}

bool ST_init(LIBCWD_TSD_PARAM)
{
  static bool WST_being_initialized = false;
  // Detect recursive initialization.
  if (WST_being_initialized)
    return false;
  WST_being_initialized = true;

  // This must be called before calling ST_init().
  if (!libcw_do.NS_init(LIBCWD_TSD))
    return false;

  // MT: We assume this is called before reaching main().
  //     Therefore, no synchronisation is required.
#if CWDEBUG_DEBUG && LIBCWD_THREAD_SAFE
  if (_private_::WST_multi_threaded)
    core_dump();
#endif

  ObjectFile::load_object_files();

  return true;
}

void ObjectFile::open_dwarf(LIBCWD_TSD_PARAM)
{
  LIBCWD_ASSERT(dwarf_fd_ == -1 && dwarf_handle_ == nullptr);

  std::string debug_info_path = get_debug_info_path(object_file_name_.filepath() LIBCWD_COMMA_TSD);

  bool different_symbols_path = debug_info_path != object_file_name_.filepath();
  Dout(dc::bfd|continued_cf, "Loading debug info " << (different_symbols_path ? "for " : "from ") << object_file_name_.filepath());
  if (different_symbols_path)
    Dout(dc::continued, " (from " << debug_info_path << ")");
  Dout(dc::continued|flush_cf, " (" << (void*)lbase_ << ")... ");

  dwarf_fd_ = open(debug_info_path.c_str(), O_RDONLY);

  if (dwarf_fd_ == -1)
  {
    Dout(dc::finish|error_cf, "failed to open");
    return;
  }

  dwarf_handle_ = dwarf_begin(dwarf_fd_, DWARF_C_READ);
  if (!dwarf_handle_)
  {
    Dout(dc::finish, "failed to obtain DWARF handle: " << dwarf_errmsg(-1));
    close(dwarf_fd_);
    dwarf_fd_ = -1;
    return;
  }

  Dout(dc::finish, "done");
}

void ObjectFile::close_dwarf(LIBCWD_TSD_PARAM)
{
  if (dwarf_handle_)
  {
    dwarf_end(dwarf_handle_);
    dwarf_handle_ = nullptr;
  }

  if (dwarf_fd_ != -1)
  {
    close(dwarf_fd_);
    dwarf_fd_ = -1;
  }
}

} // namespace libcwd::dwarf2

namespace libcwd {

ObjectFileName::ObjectFileName(char const* filepath) : no_debug_line_sections_(false)
{
  LIBCWD_TSD_DECLARATION;
  filepath_ = strcpy((char*)malloc(strlen(filepath) + 1), filepath);	// LEAK8
  filename_ = strrchr(filepath_, '/') + 1;
  if (filename_ == (char const*)1)
    filename_ = filepath_;
}

} // namespace libcwd

#endif // CWDEBUG_LOCATION
