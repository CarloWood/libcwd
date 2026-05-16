#include "cwd_sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include "cwd_dwarf2.h"
#include <libcwd/ObjectFileName.h>
#include "libcwd/debug.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <dlfcn.h>
#include <elf.h>
#include <elfutils/libdw.h>
#include <filesystem>
#include <fcntl.h>
#include <link.h>
#include <limits>
#include <map>
#include <string>
#ifdef HAVE_DLOPEN
#include <mutex>
#endif

namespace libcwd {

// New debug channels.
namespace channels {
namespace dc {
/** \addtogroup group_default_dc */
/** \{ */

/** The BFD channel. */
channel_ct bfd
#ifndef HIDE_FROM_DOXYGEN
  ("BFD")
#endif
  ;

// Internal debug channel.
channel_ct dwarf
#ifndef HIDE_FROM_DOXYGEN
  ("DWARF")
#endif
  ;

/** \} */
} // namespace dc
} // namespace channels

namespace dwarf2 {

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
// that is a read-write lock protected std::map<uintptr_t, PTLoadSegment>.
//
// The get access to the list you need to read and/or write lock it.
//
// To obtain a read lock, create a stack variable,
//
//   object_files_t::rat object_files_r(s_object_files_);       // Scoped read-lock for s_object_files_.
//
// Use object_files_r-> for read access (returns a std::map<uintptr_t, PTLoadSegment> const*).
//
// To obtain a write lock, create a stack variable,
//
//   object_files_t::wat object_files_w(s_object_files_);       // Scoped write-lock for s_object_files_.
//
// Use object_files_w-> for write access (returns a std::map<uintptr_t, PTLoadSegment>*).
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
    std::string executable_path_;               // The full path to the current executable.
    // Used for targeted dlopen-driven object discovery:
    uintptr_t target_lbase_;                    // When non-zero, create an ObjectFile only for the DSO with this load base.
    mutable ObjectFile const* object_file_{};   // Set to the newly created ObjectFile for target_lbase_.

    CallBackData(object_files_t& object_files, std::string const& executable_path, uintptr_t target_lbase = 0) :
      object_files_w(object_files), executable_path_(executable_path), target_lbase_(target_lbase) { }
  };

 private:
  // Called by constructor.
  void load_symbols();

  void open_dwarf(LIBCWD_TSD_PARAM);
  void close_dwarf(LIBCWD_TSD_PARAM);

  friend bool dwarf2::ST_init(LIBCWD_TSD_PARAM);
  static void register_initial_object_files();
  static ObjectFile const* iterate_program_headers(CallBackData const& data);
  static int cb_dl_iterate_phdr(dl_phdr_info* info, size_t size, void* call_back_data);
  static ObjectFile const* find_registered_object_file(uintptr_t lbase, object_files_t::wat const& object_files_w);

 public:
  // Called from dlopen.
  static ObjectFile const* register_object_file_at_lbase(uintptr_t lbase);
  // Called from dlclose.
  static void unregister_object_file_ranges(ObjectFile const* object_file);
};

ObjectFile::ObjectFile(char const* filename, uintptr_t lbase) : ObjectFileBase(filename), lbase_(lbase)
{
  Dout(dc::dwarf, "new ObjectFile \"" << filename << "\" with load base 0x" << std::hex << lbase);
  load_symbols();
}

ObjectFile::~ObjectFile()
{
  Dout(dc::dwarf, "destroying ObjectFile \"" << object_file_name_.filepath() << "\".");

  LIBCWD_TSD_DECLARATION;
  close_dwarf(LIBCWD_TSD);
}

//static
std::atomic_bool ObjectFile::s_object_files_initialized_ = false;

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

struct ForceLoadingDebugOutput
{
  // LIBCWD_NO_STARTUP_MSGS deliberately suppresses even the loading trace that
  // LIBCWD_PRINT_LOADING would otherwise force on.
  //
  // Do we need debug output regarding the loading of object files and their symbols?
  bool const forced_loading_output{_private_::always_print_loading && !_private_::suppress_startup_msgs};

  libcwd::debug_ct::OnOffState do_state;
  libcwd::channel_ct::OnOffState bfd_state;
  libcwd::channel_ct::OnOffState dwarf_state;

  ForceLoadingDebugOutput()
  {
    if (forced_loading_output)
    {
      Debug(libcw_do.force_on(do_state));
      Debug(dc::bfd.force_on(bfd_state, "BFD"));
      Debug(dc::dwarf.force_on(dwarf_state, "DWARF"));
    }
  }

  ~ForceLoadingDebugOutput()
  {
    if (forced_loading_output)
    {
      Debug(dc::dwarf.restore(dwarf_state));
      Debug(dc::bfd.restore(bfd_state));
      Debug(libcw_do.restore(do_state));
    }
  }
};

} // namespace

//static
void ObjectFile::register_initial_object_files()
{
  ForceLoadingDebugOutput scoped_;

  // Initialize object files list, we don't really need the
  // write lock because this function is Single Threaded.
  //
  // Start a new scope for the write lock.
  {
    CallBackData const data{s_object_files_, current_executable_path()};

    // This is the initial population pass for the dwarf2 object/segment cache.
    // Later dlopen integration may need duplicate detection, but at this point
    // no other path should have inserted PT_LOAD segments yet.
    LIBCWD_ASSERT(data.object_files_w->empty());

    // Load executable and shared objects.
    if (!statically_linked)
    {
      // Iterate over all currently loaded object files and create new ObjectFile's for them.
      iterate_program_headers(data);
    }

    s_object_files_initialized_ = true;
  } // Unlock s_object_files_.
}

//static
ObjectFile const* ObjectFile::iterate_program_headers(CallBackData const& data)
{
  // From https://man7.org/linux/man-pages/man3/dl_iterate_phdr.3.html:
  //
  // The dl_iterate_phdr() function walks through the list of an application's shared objects and
  // calls the function cb_dl_iterate_phdr once for each object, until either all shared objects
  // have been processed or cb_dl_iterate_phdr returns a nonzero value.
  //
  // A non-zero data.target_lbase_ makes the callback stop after the matching object;
  // zero means enumerate all currently loaded objects.
  //
  dl_iterate_phdr(cb_dl_iterate_phdr, const_cast<CallBackData*>(&data));
  return data.object_file_;
}

//static
int ObjectFile::cb_dl_iterate_phdr(dl_phdr_info* info, size_t size, void* call_back_data)
{
  CallBackData const* data = static_cast<CallBackData const*>(call_back_data);
  uintptr_t const lbase = static_cast<uintptr_t>(info->dlpi_addr);
  bool const target_lbase_only = data->target_lbase_ != 0;

  // If this is not the target object; advance to the next.
  if (target_lbase_only && lbase != data->target_lbase_)
    return 0;

  // dl_iterate_phdr reports an empty name for the main executable.
  // Use the already resolved /proc/self/exe path in that case so ObjectFileName remains
  // useful for diagnostics and later public location data.
  char const* object_filename = (info->dlpi_name && info->dlpi_name[0] != '\0') ? info->dlpi_name : data->executable_path_.c_str();

  // Create new ObjectFile.
  data->object_file_ = new ObjectFile(object_filename, lbase);

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

    auto const ibp = data->object_files_w->try_emplace(segment_end, data->object_file_, segment_start, segment_end, phdr.p_flags);
    // Make sure the new segment doesn't overlap with an already existing one.
    LIBCWD_ASSERT(ibp.second &&
        (ibp.first == data->object_files_w->begin() ||
            std::prev(ibp.first)->first <= segment_start) &&
        (std::next(ibp.first) == data->object_files_w->end() ||
            std::next(ibp.first)->second.start_addr() >= segment_end));

    Dout(dc::dwarf, "    map[" << (void*)segment_end << "] = PTLoadSegment " << ibp.first->second <<
        " [" << (void*)segment_start << '-' << (void*)segment_end << ") flags=" << flags_to_string(phdr.p_flags) <<
        " object=" << data->object_file_);
  }

  // Stop iterating iff there was only one target.
  return target_lbase_only;
}

//static
ObjectFile const* ObjectFile::find_registered_object_file(uintptr_t lbase, object_files_t::wat const& object_files_w)
{
  // The writable map is keyed by PT_LOAD end address and can contain multiple
  // segments for the same object file.  Treat any segment whose owning
  // ObjectFile has the requested load base as proof that the object was already
  // registered; this prevents duplicate ObjectFile creation when loader
  // enumeration reports a DSO that was discovered through an earlier path.
  for (auto const& [segment_end, segment] : *object_files_w)
  {
    ObjectFile const* object_file = segment.object_file();
    if (object_file->lbase() == lbase)
      return object_file;
  }
  return nullptr;
}

//static
ObjectFile const* ObjectFile::register_object_file_at_lbase(uintptr_t lbase)
{
  // Locks s_object_files_.
  CallBackData const data{s_object_files_, current_executable_path(), lbase};

  // If this object file was already loaded before, then return that instead of creating a new ObjectFile.
  ObjectFile const* existing_object_file = find_registered_object_file(lbase, data.object_files_w);

  // Otherwise iterate over all currently loaded object files to find the just dynamically
  // opened object file, loaded at data.target_lbase_, and create a new ObjectFile for it.
  return existing_object_file ? existing_object_file : iterate_program_headers(data);
}

//static
void ObjectFile::unregister_object_file_ranges(ObjectFile const* object_file)
{
  object_files_t::wat object_files_w(s_object_files_);
  for (auto iter = object_files_w->begin(); iter != object_files_w->end(); )
  {
    PTLoadSegment const& segment = iter->second;
    if (segment.object_file() == object_file)
    {
      Dout(dc::dwarf, "removing map[" << (void*)iter->first << "] = PTLoadSegment " << segment <<
          " for object " << object_file << " (\"" << object_file->object_file_name().filename() << "\")");
      iter = object_files_w->erase(iter);
    }
    else
      ++iter;
  }
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

  ObjectFile::register_initial_object_files();

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

#ifdef HAVE_DLOPEN
namespace {

// Real dynamic-loader entry points used by the exported wrappers below.  The
// wrappers are responsible for keeping dwarf2's loaded-segment map synchronized
// with libraries added by dlopen and, eventually, removed by dlclose.
extern "C" {
static union { void* symptr; void* (*func)(char const*, int); } real_dlopen;
static union { void* symptr; int (*func)(void*); } real_dlclose;
}
std::once_flag initialize_real_dlopen;
std::once_flag initialize_real_dlclose;

struct DynamicLoaderRecord
{
  int refcount_;
  int flags_;
  ObjectFile const* object_file_;

  DynamicLoaderRecord(ObjectFile const* object_file, int flags) : refcount_(1), flags_(flags), object_file_(object_file) { }
};

using dynamic_loader_records_t = threadsafe::Unlocked<std::map<void*, DynamicLoaderRecord, std::less<void*>>, threadsafe::policy::Primitive<std::mutex>>;

dynamic_loader_records_t& dlopen_map()
{
  static dynamic_loader_records_t* map = new dynamic_loader_records_t;  // Intentionally leaked for late DSO teardown.
  return *map;
}

} // namespace
#endif // HAVE_DLOPEN

} // namespace dwarf2
} // namespace libcwd

#ifdef HAVE_DLOPEN
extern "C" {

void* dlopen(char const* name, int flags)
{
  // No need to register if no name is given.
  bool const need_register_object_file = name && *name;

  // Ensure the initial dwarf2 object/segment cache exists before the new DSO is mapped.
  // That makes the following post-dlopen pass responsible only for the just-loaded link_map entry, and avoids duplicate startup registration.
  if (need_register_object_file)
    libcwd::initialize();

  using namespace libcwd::dwarf2;

  // Initialize real_dlopen if that wasn't done yet.
  std::call_once(initialize_real_dlopen, [](){ real_dlopen.symptr = dlsym(RTLD_NEXT, "dlopen"); });

  void* handle = real_dlopen.func(name, flags);
  if (statically_linked)
  {
    Dout(dc::warning, "Calling dlopen(3) from statically linked application; this is not going to work if the loaded module uses libcwd too or when it allocates any memory!");
    return handle;
  }
  if (handle == nullptr)
    return handle;
#ifdef RTLD_NOLOAD
  if ((flags & RTLD_NOLOAD))
    return handle;
#endif

  // Lock dlopen_map.
  dynamic_loader_records_t::wat dynamic_loader_records_w(dlopen_map());

  // If the record already exist, increment DynamicLoaderRecord::refcount_ and return.
  {
    auto iter = dynamic_loader_records_w->find(handle);
    if (iter != dynamic_loader_records_w->end())
    {
      ++iter->second.refcount_;
      return handle;
    }
  }

  if (need_register_object_file)
  {
    struct link_map* link_map;
    if (::dlinfo(handle, RTLD_DI_LINKMAP, &link_map) == 0)
    {
      ForceLoadingDebugOutput scoped_;
      ObjectFile const* object_file = ObjectFile::register_object_file_at_lbase(static_cast<uintptr_t>(link_map->l_addr));
      // NULL would mean that there is no DSO at the `l_addr` that `dlinfo` just returned.
      // That shouldn't be possible because no thread can have dlclose-d it without already haven gotten the handle that we didn't even return yet.
      LIBCWD_ASSERT(object_file);
      dynamic_loader_records_w->emplace(handle, DynamicLoaderRecord{object_file, flags});
    }
  }

  // Unlock dlopen_map and return handle.
  return handle;
}

int dlclose(void* handle)
{
  using namespace libcwd::dwarf2;

  // Initialize real_dlclose if that wasn't done yet.
  std::call_once(initialize_real_dlclose, [](){ real_dlclose.symptr = dlsym(RTLD_NEXT, "dlclose"); });

  int error = real_dlclose.func(handle);
  if (error != 0 || statically_linked)
    return error;

  // Lock dlopen_map.
  dynamic_loader_records_t::wat dynamic_loader_records_w(dlopen_map());

  auto iter = dynamic_loader_records_w->find(handle);
  if (iter != dynamic_loader_records_w->end() && --iter->second.refcount_ == 0)
  {
#ifdef RTLD_NODELETE
    if (!(iter->second.flags_ & RTLD_NODELETE))
#endif
    {
      ForceLoadingDebugOutput scoped_;
      ObjectFile::unregister_object_file_ranges(iter->second.object_file_);
    }
    dynamic_loader_records_w->erase(iter);
  }

  // Unlock dlopen_map and return success.
  return 0;
}

} // extern "C"
#endif // HAVE_DLOPEN

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
