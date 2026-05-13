#include "cwd_sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include "cwd_dwarf2.h"
#include <libcwd/ObjectFileName.h>
#include "cwd_dwarf.h"
#include <elf.h>
#include <fcntl.h>
#include <filesystem>
#include <cstdint>
#include <string>

namespace libcwd::dwarf2 {

//static
ObjectFileBase::object_files_t ObjectFileBase::s_object_files_;

// class ObjectFile
//
// Represents a single object file: either the executable or a shared library.
// The base class `ObjectFileBase` has a protected static member `s_object_files_`
// that is a read-write lock protected std::list<ObjectFile*>.
//
// The get access to the list you need to read and/or write lock it.
//
// To obtain a read lock, create a stack variable,
//
//   object_files_t::rat object_files_r(s_object_files_);       // Scoped read-lock for s_object_files_.
//
// Use object_files_r-> for read access (returns a std::list<ObjectFile*> const*).
//
// To obtain a write lock, create a stack variable,
//
//   object_files_t::wat object_files_w(s_object_files_);       // Scoped write-lock for s_object_files_.
//
// Use object_files_w-> for write access (returns a std::list<ObjectFile*>*).
//
class ObjectFile : public ObjectFileBase
{
 private:
  Dwarf* dwarf_handle_{nullptr};
  int dwarf_fd_{-1};
  uintptr_t lbase_;
  uintptr_t start_addr_;
  uintptr_t end_addr_;

 public:
  ObjectFile(char const* filename, uintptr_t lbase, uintptr_t start_addr, uintptr_t end_addr);
  ~ObjectFile();

  uintptr_t get_lbase() const { return lbase_; }

  uintptr_t get_start_addr() const { return start_addr_; }
  uintptr_t get_end_addr() const { return end_addr_; }
  bool is_initialized() const { return dwarf_fd_ != -1; }

 private:
  void open_dwarf(LIBCWD_TSD_PARAM);
  void close_dwarf(LIBCWD_TSD_PARAM);
};

ObjectFile::ObjectFile(char const* filename, uintptr_t lbase, uintptr_t start_addr, uintptr_t end_addr) :
  ObjectFileBase(filename), lbase_(lbase), start_addr_(start_addr), end_addr_(end_addr)
{
}

ObjectFile::~ObjectFile()
{
  LIBCWD_TSD_DECLARATION;
  close_dwarf(LIBCWD_TSD);
}

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

} // namespace

void ObjectFile::open_dwarf(LIBCWD_TSD_PARAM)
{
  LIBCWD_ASSERT(dwarf_fd_ == -1 && dwarf_handle_ == nullptr);

  std::string debug_info_path = get_debug_info_path(object_file_name_.filepath() LIBCWD_COMMA_TSD);

  bool different_symbols_path = debug_info_path != object_file_name_.filepath();
  Dout(dc::bfd|continued_cf, "Loading debug info " << (different_symbols_path ? "for " : "from ") << object_file_name_.filepath());
  if (different_symbols_path)
    Dout(dc::continued, " (from " << debug_info_path << ")");
  Dout(dc::continued|flush_cf, " (" << (void*)lbase_ << " [" << (void*)start_addr_ << '-' << (void*)end_addr_ << "])... ");

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

#endif // CWDEBUG_LOCATION
