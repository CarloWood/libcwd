// $Header$
//
// @Copyright (C) 2002 - 2007, 2023  Carlo Wood.
//
// pub   dsa3072/C155A4EEE4E527A2 2018-08-16 Carlo Wood (CarloWood on Libera) <carlo@alinoe.com>
// fingerprint: 8020 B266 6305 EE2F D53E  6827 C155 A4EE E4E5 27A2
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#undef LIBCWD_DEBUGBFD		// Define to add debug code for this file.

#include "sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include <cstdarg>
#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <link.h>
#include <new>
#include <string>
#include <fstream>
#include <sys/param.h>          // Needed for realpath(3)
#include <unistd.h>             // Needed for getpid(2) and realpath(3)
#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <stdlib.h>             // Needed for realpath(3)
#endif
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include <map>
#endif
#include <algorithm>
#include <iomanip>
#include "cwd_debug.h"
#include "ios_base_Init.h"
#include "exec_prog.h"
#include "match.h"
#include "cwd_dwarf.h"
#include <libcwd/class_object_file.h>
#include <libcwd/class_channel.h>
#include <libcwd/private_string.h>
#include <libcwd/core_dump.h>

#ifndef LIBCWD_PRIVATE_MUTEX_INSTANCES_H
#include "libcwd/private_mutex_instances.h"
#endif
#include "libcwd/debug.h"
#include "elfutils/known-dwarf.h"
#include <elf.h>
#include <functional>
#if CWDEBUG_DEBUG
#include "libcwd/buf2str.h"
#endif

namespace libcwd {
namespace _private_ {

extern void demangle_symbol(char const* in, _private_::internal_string& out);
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
extern char* pool_allocator_lock_symbol_ptr;
#endif
#if CWDEBUG_ALLOC
extern void remove_type_info_references(object_file_ct const* object_file_ptr LIBCWD_COMMA_TSD_PARAM);
#endif

} // namespace _private_

// New debug channel.
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

/** \} */
} // namespace dc
} // namespace channels

using _private_::set_alloc_checking_on;
using _private_::set_alloc_checking_off;

#ifdef HAVE_DLOPEN
extern "C" {
  // dlopen and dlclose function pointers into libdl.
  static union { void* symptr; void* (*func)(char const*, int); } real_dlopen;
  static union { void* symptr; int (*func)(void*); } real_dlclose;
}
#endif

char const* const location_ct::S_uninitialized_location_ct_c = "<uninitialized location_ct>";
char const* const location_ct::S_pre_ios_initialization_c = "<pre ios initialization>";
char const* const location_ct::S_pre_libcwd_initialization_c = "<pre libcwd initialization>";
char const* const location_ct::S_cleared_location_ct_c = "<cleared location ct>";

namespace dwarf {

// Detect if this libcwd library is static or not.
#ifdef __PIC__
constexpr bool statically_linked = false;
#else
constexpr bool statically_linked = true;
#endif

bool is_group_member(gid_t gid)
{
#if defined(HAVE_GETGID) && defined(HAVE_GETEGID)
  if (gid == getgid() || gid == getegid())
#endif
    return true;

#ifdef HAVE_GETGROUPS
  int ngroups = 0;
  int default_group_array_size = 0;
  getgroups_t* group_array = (getgroups_t*)nullptr;

  while (ngroups == default_group_array_size)
  {
    default_group_array_size += 64;
    group_array = (getgroups_t*)realloc(group_array, default_group_array_size * sizeof(getgroups_t));
    ngroups = getgroups(default_group_array_size, group_array);
  }

  if (ngroups > 0)
    for (int i = 0; i < ngroups; i++)
      if (gid == group_array[i])
      {
        free(group_array);
        return true;
      }

  free(group_array);
#endif // HAVE_GETGROUPS

  return false;
}

//
// Find the full path to the current running process.
// This needs to work before we reach main, therefore
// it uses the /proc filesystem.  In order to be as
// portable as possible the most general trick is used:
// reading "/proc/PID/cmdline".  It expects that the
// name of the program (argv[0] in main()) is returned
// as a zero terminated string when reading this file.
//
// If "/proc/PID/cmdline" doesn't exist, then we run
// `ps' in order to find the name of the running
// program.
//
void ST_get_full_path_to_executable(_private_::internal_string& result LIBCWD_COMMA_TSD_PARAM)
{
  _private_::string argv0;		// Like main()s argv[0], thus must be zero terminated.
  char buf[11];
  char* p = &buf[10];
  *p = 0;
  int pid = getpid();
  do { *--p = '0' + (pid % 10); } while ((pid /= 10) > 0);
  size_t const max_proc_path = sizeof("/proc/2147483647/cmdline\0");
  char proc_path[max_proc_path];
  strcpy(proc_path, "/proc/");
  strcpy(proc_path + 6, p);
  strcat(proc_path, "/cmdline");
  std::ifstream proc_file(proc_path);

  if (proc_file)
  {
    proc_file >> argv0;
    proc_file.close();
  }
  else
    DoutFatal(dc::fatal, "Can't find \"" << proc_path << "\". Try -DEnableLibcwdLocation:BOOL=OFF.");

  argv0 += '\0';
  // argv0 now contains a zero terminated string optionally followed by the command line
  // arguments (which is why we can't use argv0.find('/') here), all seperated by the 0 character.
  if (!strchr(argv0.data(), '/'))
  {
    _private_::string prog_name(argv0);
    _private_::string path_list(getenv("PATH"));
    _private_::string::size_type start_pos = 0, end_pos;
    _private_::string path;
    struct stat finfo;
    for (;;)
    {
      end_pos = path_list.find(':', start_pos);
      path = path_list.substr(start_pos, end_pos - start_pos) + '/' + prog_name + '\0';
      if (stat(path.data(), &finfo) == 0 && !S_ISDIR(finfo.st_mode))
      {
        uid_t user_id = geteuid();
        if ((user_id == 0 && (finfo.st_mode & 0111)) ||
	  (user_id == finfo.st_uid && (finfo.st_mode & 0100)) ||
	  (is_group_member(finfo.st_gid) && (finfo.st_mode & 0010)) ||
	  (finfo.st_mode & 0001))
        {
	argv0 = path;
	break;
        }
      }
      if (end_pos == _private_::string::npos)
        break;
      start_pos = end_pos + 1;
    }
  }

  char full_path_buf[MAXPATHLEN];
  char* full_path = realpath(argv0.data(), full_path_buf);

  if (!full_path)
    DoutFatal(dc::fatal|error_cf, "realpath(\"" << argv0.data() << "\", full_path_buf)");

  Dout(dc::debug, "Full path to executable is \"" << full_path << "\".");
  set_alloc_checking_off(LIBCWD_TSD);
  result.assign(full_path);
  result += '\0';				// Make string null terminated so we can use data().
  set_alloc_checking_on(LIBCWD_TSD);
}

// Magic numbers for "unknown" load address and "executable" address.
constexpr uintptr_t unknown_base_addr = -1;
constexpr uintptr_t executable_base_addr = -2;

static bool WST_initialized = false;                      // MT: Set here to false, set to `true' once in `cwbfd::ST_init'.
struct objfile_ct;

class symbol_ct
{
 private:
  Dwarf_Addr real_start_;               // Key
  Dwarf_Addr real_end_;

  mutable objfile_ct* objfile_;         // Data
  mutable Dwarf_Die die_;
  mutable char const* cu_name_;
  mutable char const* func_name_;
  mutable char const* linkage_name_;

 public:
  inline symbol_ct(Dwarf_Addr start, Dwarf_Addr end,
      objfile_ct* objfile, Dwarf_Die const* die, char const* cu_name, char const* func_name, char const* linkage_name);
  // Create a symbol from an .symtab entry.
  symbol_ct(Dwarf_Addr start, Dwarf_Addr end, objfile_ct* objfile, char const* linkage_name);
  // Create a dummy symbol to be used as search key.
  symbol_ct(Dwarf_Addr start) : real_start_(start), real_end_(start + 1) { }

  Dwarf_Addr real_start() const { return real_start_; }
  Dwarf_Addr real_end() const { return real_end_; }
  objfile_ct const* objfile() const { return objfile_; }
  Dwarf_Die const* die() const { return &die_; }
  bool diecu(Dwarf_Die* cu_die_out) const
  {
    // die_.addr should only be NULL when the .symtab constructor was used, in which case die_ is uninitialized.
    return die_.addr ? dwarf_diecu(&die_, cu_die_out, NULL, NULL) != nullptr : false;
  }
  char const* cu_name() const { return cu_name_; }
  char const* func_name() const { return func_name_; }
  char const* linkage_name() const { return linkage_name_; }
  char const* name() const { return linkage_name_ ? linkage_name_ : func_name_; }

  void set_func_name(char const* func_name) const { func_name_ = func_name; }
  void set_linkage_name(char const* linkage_name) const { linkage_name_ = linkage_name; }
  void set_real_end(Dwarf_Addr real_end) { real_end_ = real_end; }

  bool operator==(symbol_ct const&) const { DoutFatal(dc::core, "Calling operator=="); }
  friend struct symbol_key_greater;
};

struct symbol_key_greater
{
  // Returns true when the start of a lays beyond the end of b (ie, no overlap).
  bool operator()(symbol_ct const& a, symbol_ct const& b) const
  {
    return a.real_start() >= b.real_end();
  }
};

#if CWDEBUG_ALLOC
using function_symbols_ct = std::set<symbol_ct, symbol_key_greater, _private_::object_files_allocator::rebind<symbol_ct>::other>;
#else
using function_symbols_ct = std::set<symbol_ct, symbol_key_greater>;
#endif

class Elf;

// All allocations related to objfile_ct must be `internal'.
class objfile_ct : public objfiles_ct
{
  friend class libcwd::location_ct;

 private:
  Dwarf* dwarf_handle_;
  int dwarf_fd_;
  uintptr_t M_lbase;
  uintptr_t M_start_addr;
  uintptr_t M_end_addr;
  function_symbols_ct M_function_symbols;

 public:
  objfile_ct(char const* filename, uintptr_t base_addr, uintptr_t start_addr, uintptr_t end_addr);
  ~objfile_ct();

  bool initialize(char const* filename LIBCWD_COMMA_ALLOC_OPT(bool is_libc) LIBCWD_COMMA_TSD_PARAM);
  void deinitialize(LIBCWD_TSD_PARAM);
  uintptr_t get_lbase() const { return M_lbase; }

  uintptr_t get_start_addr() const { return M_start_addr; }
  uintptr_t get_end_addr() const { return M_end_addr; }

  symbol_ct const* find_symbol(symbol_ct const& search_key) const;

  bool is_initialized() const
  {
    return dwarf_fd_ != -1;
  }

 private:
  void extract_and_add_function_symbols(char const* cu_name, Elf& elf, Dwarf_Die* die_ptr LIBCWD_COMMA_TSD_PARAM);
  void open_dwarf(LIBCWD_TSD_PARAM);
  void close_dwarf(LIBCWD_TSD_PARAM);
};

struct object_file_greater {
  bool operator()(objfiles_ct const* a, objfiles_ct const* b) const
  {
    return static_cast<objfile_ct const*>(a)->get_lbase() > static_cast<objfile_ct const*>(b)->get_lbase();
  }
};

symbol_ct::symbol_ct(Dwarf_Addr start, Dwarf_Addr end,
    objfile_ct* objfile, Dwarf_Die const* die, char const* cu_name, char const* func_name, char const* linkage_name) :
    real_start_(start + objfile->get_lbase()), real_end_(end + objfile->get_lbase()),
    objfile_(objfile), die_(*die), cu_name_(cu_name), func_name_(func_name), linkage_name_(linkage_name)
{
}

symbol_ct::symbol_ct(Dwarf_Addr start, Dwarf_Addr end, objfile_ct* objfile, char const* linkage_name) :
    real_start_(start + objfile->get_lbase()), real_end_(end + objfile->get_lbase()),
    objfile_(objfile), die_{}, cu_name_(nullptr), func_name_(nullptr), linkage_name_(linkage_name)
{
}

objfile_ct* load_object_file(char const* name, uintptr_t base_addr, uintptr_t start_addr, uintptr_t end_addr, bool initialized = false)
{
  static bool WST_initialized = false;
  LIBCWD_TSD_DECLARATION;
  // If dlopen is called as very first function (instead of malloc), we get here
  // as first function inside libcwd. In that case, initialize libcwd first.
  if (!WST_initialized)
  {
    if (initialized)
      WST_initialized = true;
    else if (!ST_init(LIBCWD_TSD))
      return nullptr;
  }
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
  objfile_ct* object_file;
  char const* slash = strrchr(name, '/');
  if (!slash)
    slash = name - 1;
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
  bool is_libstdcpp;
  is_libstdcpp = (strncmp("libstdc++.so", slash + 1, 12) == 0);
#endif
#if CWDEBUG_ALLOC
  bool is_libc = (strncmp("libc.so", slash + 1, 7) == 0);
#endif
  bool failure;
  LIBCWD_DEFER_CANCEL;
  DWARF_ACQUIRE_WRITE_LOCK;
  set_alloc_checking_off(LIBCWD_TSD);
  object_file = new objfile_ct(name, base_addr, start_addr, end_addr);		// LEAK6
  set_alloc_checking_on(LIBCWD_TSD);
  DWARF_RELEASE_WRITE_LOCK;
  failure = object_file->initialize(name LIBCWD_COMMA_ALLOC_OPT(is_libc) LIBCWD_COMMA_TSD);
  LIBCWD_RESTORE_CANCEL;
  if (failure)
  {
    set_alloc_checking_off(LIBCWD_TSD);
    delete object_file;
    set_alloc_checking_on(LIBCWD_TSD);
    return nullptr;
  }
  return object_file;
}

static int dl_iterate_phdr_callback(dl_phdr_info* info, size_t size, void* fullpath)
{
  uintptr_t base_address = info->dlpi_addr;
  uintptr_t start_addr = (uintptr_t)0 - 1;
  uintptr_t end_addr = 0;
  for (int i = 0; i < info->dlpi_phnum; ++i)
  {
    if (info->dlpi_phdr[i].p_type == PT_LOAD && (info->dlpi_phdr[i].p_flags & PF_X))
    {
      if (end_addr != 0)
        // This isn't really a problem. All we need an address range that uniquely determines this object file.
        Dout(dc::warning, "Object file \"" << info->dlpi_name << "\" has more than one executable PT_LOAD segments!");

      uintptr_t current_start_addr = base_address + info->dlpi_phdr[i].p_vaddr;
      uintptr_t current_end_addr = current_start_addr + info->dlpi_phdr[i].p_memsz;

      start_addr = std::min(start_addr, current_start_addr);
      end_addr = std::max(end_addr, current_end_addr);
    }
  }
  if (end_addr != 0)
  {
    [[maybe_unused]] objfile_ct* objfile;
    if (info->dlpi_name[0] == 0)
      objfile = load_object_file(reinterpret_cast<char const*>(fullpath), base_address, start_addr, end_addr, true);
    else if (info->dlpi_name[0] == '/' || info->dlpi_name[0] == '.')
      objfile = load_object_file(info->dlpi_name, base_address, start_addr, end_addr);
  }
  return 0;
}

static int dl_iterate_phdr_callback2(dl_phdr_info* info, size_t size, void* addr)
{
  uintptr_t int_addr = reinterpret_cast<uintptr_t>(addr);
  uintptr_t base_address = info->dlpi_addr;
  uintptr_t start_addr = (uintptr_t)0 - 1;
  uintptr_t end_addr = 0;
  for (int i = 0; i < info->dlpi_phnum; ++i)
  {
    if (info->dlpi_phdr[i].p_type == PT_LOAD && (info->dlpi_phdr[i].p_flags & PF_X))
    {
      if (end_addr != 0)
        // This isn't really a problem. All we need an address range that uniquely determines this object file.
        Dout(dc::warning, "Object file \"" << info->dlpi_name << "\" has more than one executable PT_LOAD segments!");

      uintptr_t current_start_addr = base_address + info->dlpi_phdr[i].p_vaddr;
      uintptr_t current_end_addr = current_start_addr + info->dlpi_phdr[i].p_memsz;

      start_addr = std::min(start_addr, current_start_addr);
      end_addr = std::max(end_addr, current_end_addr);
    }
  }
  bool found = start_addr <= int_addr && int_addr < end_addr;
  if (found)
  {
    // We already have the executable; only check for shared libraries.
    if (info->dlpi_name[0] == '/' || info->dlpi_name[0] == '.')
      load_object_file(info->dlpi_name, base_address, start_addr, end_addr);
    return 1;
  }
  // Continue with the next object file.
  return 0;
}

bool ST_init(LIBCWD_TSD_PARAM)
{
  static bool WST_being_initialized = false;
  // This should catch it when we call new or malloc while 'internal'.
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

#if CWDEBUG_DEBUG && CWDEBUG_ALLOC
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
#if CWDEBUG_ALLOC
  // Initialize the malloc library if not done yet.
  init_debugmalloc();
#endif

  // ****************************************************************************
  // Start INTERNAL!
  set_alloc_checking_off(LIBCWD_TSD);

//  new (fake_ST_shared_libs) ST_shared_libs_vector_ct;

  libcwd::debug_ct::OnOffState state;
  libcwd::channel_ct::OnOffState state2;
  if (_private_::always_print_loading && !_private_::suppress_startup_msgs)
  {
    // We want debug output to BFD
    Debug( libcw_do.force_on(state) );
    Debug( dc::bfd.force_on(state2, "BFD") );
  }

  // Initialize object files list, we don't really need the
  // write lock because this function is Single Threaded.
  LIBCWD_DEFER_CANCEL;
  DWARF_ACQUIRE_WRITE_LOCK;
  new (&NEEDS_WRITE_LOCK_object_files()) object_files_ct;
  DWARF_RELEASE_WRITE_LOCK;
  set_alloc_checking_on(LIBCWD_TSD);
  LIBCWD_RESTORE_CANCEL;
  set_alloc_checking_off(LIBCWD_TSD);

  // Start a new scope for 'fullpath': it needs to be destructed while internal.
  {
    // Get the full path and name of executable
    _private_::internal_string fullpath;

    set_alloc_checking_on(LIBCWD_TSD);

    // End INTERNAL!
    // ****************************************************************************

    ST_get_full_path_to_executable(fullpath LIBCWD_COMMA_TSD);
        // Result is '\0' terminated so we can use data() as a C string.

    // Load executable and shared objects.
    DWARF_INITIALIZE_LOCK;

    if (!statically_linked)
    {
      // Call load_object_file for everything..
      dl_iterate_phdr(dl_iterate_phdr_callback, fullpath.data());

      LIBCWD_DEFER_CANCEL;
      DWARF_ACQUIRE_WRITE_LOCK;
      set_alloc_checking_off(LIBCWD_TSD);
      NEEDS_WRITE_LOCK_object_files().sort(object_file_greater());
      set_alloc_checking_on(LIBCWD_TSD);
      DWARF_RELEASE_WRITE_LOCK;
      LIBCWD_RESTORE_CANCEL;
    }

    if (_private_::always_print_loading)
    {
      Debug( dc::bfd.restore(state2) );
      Debug( libcw_do.restore(state) );
    }

    WST_initialized = true;			// MT: Safe, this function is Single Threaded.

    set_alloc_checking_off(LIBCWD_TSD);
  } // Destruct fullpath
  set_alloc_checking_on(LIBCWD_TSD);

  return true;
}

char objfiles_ct::ST_list_instance[sizeof(object_files_ct)] __attribute__((__aligned__));

objfile_ct::objfile_ct(char const* filename, uintptr_t base_addr, uintptr_t start_addr, uintptr_t end_addr) :
  objfiles_ct(filename), dwarf_fd_(-1), dwarf_handle_(nullptr), M_lbase(base_addr), M_start_addr(start_addr), M_end_addr(end_addr)
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( __libcwd_tsd.internal );
#if CWDEBUG_DEBUGT
  LIBCWD_ASSERT( _private_::is_locked(object_files_instance) );
#endif
#endif
}

objfile_ct::~objfile_ct()
{
  LIBCWD_TSD_DECLARATION;
  set_alloc_checking_on(LIBCWD_TSD);
  close_dwarf(LIBCWD_TSD);
  set_alloc_checking_off(LIBCWD_TSD);
}

#if CWDEBUG_DEBUG

#define DWARF_ONE_KNOWN_DW_TAG(tag_, tag_name_) \
  case tag_name_: \
    tag_name = #tag_name_; \
    break;

static void print_die_info(Dwarf_Die* die)
{
  // 1. Print DIE Tag.
  char const* tag_name = "<unknown>";
  Dwarf_Half tag = dwarf_tag(die);
  switch (tag)
  {
    DWARF_ALL_KNOWN_DW_TAG
  }
  Dout(dc::bfd, "DIE Tag: " << tag_name);

  // 2. Print DIE Offset.
  Dwarf_Off die_offset = dwarf_dieoffset(die);
  Dout(dc::bfd, "DIE Offset: 0x" << std::hex << die_offset);

  // 3. Print DIE Name (if available).
  char const* name = dwarf_diename(die);
  if (name) {
    Dout(dc::bfd, "DIE Name: " << name);
  }

  // 4. Print Parent DIE Offset (if applicable).
  Dwarf_Die parent_die;
  if (dwarf_diecu(die, &parent_die, nullptr, nullptr))
  {
    die_offset = dwarf_dieoffset(&parent_die);
    Dout(dc::bfd, "Parent DIE Offset: 0x" << std::hex << die_offset);
  }
}

void dump_shdr(Elf64_Shdr const* shdr)
{
  Dout(dc::bfd,
       "{sh_name="      << shdr->sh_name << std::hex <<   // Section name (string tbl index).
      ", sh_type=0x"    << shdr->sh_type <<               // Section type.
      ", sh_flags=0x"   << shdr->sh_flags <<              // Section flags.
      ", sh_addr=0x"    << shdr->sh_addr <<               // Section virtual addr at execution.
      ", sh_offset=0x"  << shdr->sh_offset << std::dec << // Section file offset.
      ", sh_size="      << shdr->sh_size <<               // Section size in bytes.
      ", sh_link="      << shdr->sh_link <<               // Link to another section.
      ", sh_info="      << shdr->sh_info <<               // Additional section information.
      ", sh_addralign=" << shdr->sh_addralign <<          // Section alignment.
      ", sh_entsize="   << shdr->sh_entsize << '}');      // Entry size if section holds table.
}
#endif // CWDEBUG_DEBUG

struct SymbolNameCompare
{
  bool operator()(char const* name1, char const* name2) const
  {
    return strcmp(name1, name2) < 0;
  }
};

class Elf
{
 private:
  GElf_Addr lbase_;
  int fd_;
  ::Elf* elf_;
  size_t shstrndx_;
#if CWDEBUG_DEBUG
  char const* filepath_;
#endif

  struct Range
  {
    GElf_Addr start_;
    GElf_Addr end_;
  };

  using relocation_map_ct = std::map<char const*, Range, SymbolNameCompare
#if CWDEBUG_ALLOC
                      , _private_::internal_allocator::rebind<std::pair<char const* const, Range>>::other
#endif
                      >;

  relocation_map_ct relocation_map_;

 public:
  Elf(char const* filepath, GElf_Addr lbase) :
    lbase_(lbase)
#if CWDEBUG_DEBUG
    , filepath_(filepath)
#endif
  {
    // Open the object file for reading.
    if ((fd_ = open(filepath, O_RDONLY)) == -1)
      Dout(dc::warning|error_cf, "Failed to open file \"" << filepath << "\"");
    // Open an ELF descriptor for reading.
    else if (!(elf_ = elf_begin(fd_, ELF_C_READ, NULL)))
    {
      ::close(fd_);
      Dout(dc::warning, filepath << ": elf_begin returned NULL: " << elf_errmsg(-1));
    }
    // Check if it is an ELF file.
    else if (elf_kind(elf_) != ELF_K_ELF)
      Dout(dc::bfd, filepath << ": skipping, not an ELF file.");
    // Get the section header string table index.
    else if (elf_getshdrstrndx(elf_, &shstrndx_) != 0)
      Dout(dc::warning, filepath << ": elf_getshdrstrndx couldn't find the section header string index: " << elf_errmsg(-1));
    else
      // Success.
      return;
    throw std::runtime_error("error");
  }

  ~Elf()
  {
    elf_end(elf_);
    ::close(fd_);
    LIBCWD_TSD_DECLARATION;
    {
      relocation_map_ct empty_dummy;
      std::swap(relocation_map_, empty_dummy);
#if CWDEBUG_ALLOC
      __libcwd_tsd.internal = 1;
#endif
    }
#if CWDEBUG_ALLOC
    __libcwd_tsd.internal = 0;
#endif
  }

  int process_relocs();
  void add_symtab(std::function<void(Dwarf_Addr start, Dwarf_Addr end, char const* linkage_name LIBCWD_COMMA_TSD_PARAM)> symbol_cb) const;
  void add_backup(char const* linkage_name, GElf_Addr start, GElf_Addr end);
  bool apply_relocation(char const* linkage_name, GElf_Addr& start_out, GElf_Addr& end_out) const;

#if CWDEBUG_DEBUG
  // Print all section header names (for debugging purposes).
  void print_section_header_names() const
  {
    Dout(dc::bfd, "Section headers of \"" << filepath_ << "\":");
    int count = 0;
    Elf_Scn* section = nullptr;
    while ((section = elf_nextscn(elf_, section)) != nullptr)
    {
      GElf_Shdr shdr;
      if (gelf_getshdr(section, &shdr))
      {
        ++count;
        //dump_shdr(&shdr);
        char const* section_name = elf_strptr(elf_, shstrndx_, shdr.sh_name);
        Dout(dc::bfd, " [" << count << "] \"" << section_name << "\"");
      }
    }
  }
#endif // CWDEBUG_DEBUG

 private:
  void add_symbol(GElf_Sym* sym, size_t symstrndx, bool is_reloc, GElf_Sxword r_addend = 0);
  void process_relocs_x(GElf_Shdr* shdr, Elf_Data* symdata, Elf_Data *xndxdata, size_t symstrndx,
      GElf_Addr r_offset, GElf_Xword r_info, GElf_Sxword r_addend);
  void process_relocs_rel(GElf_Shdr* shdr, Elf_Data* data, Elf_Data* symdata, Elf_Data* xndxdata, size_t symstrndx);
  void process_relocs_rela(GElf_Shdr* shdr, Elf_Data* data, Elf_Data* symdata, Elf_Data* xndxdata, size_t symstrndx);
};

void Elf::add_symbol(GElf_Sym* sym, size_t symstrndx, bool is_reloc, GElf_Sxword r_addend)
{
  char const* name = elf_strptr(elf_, symstrndx, sym->st_name);
  GElf_Addr start = sym->st_value + r_addend;
  GElf_Addr end = start + sym->st_size;

  if (GELF_ST_TYPE(sym->st_info) == STT_FUNC && sym->st_shndx != SHN_UNDEF && sym->st_size > 0)
  {
#if CWDEBUG_DEBUG
    Dout(dc::bfd|continued_cf, "Adding \"" << libcwd::buf2str(name, std::strlen(name)) <<
        "\" range [0x" << std::hex << (lbase_ + start) << "-0x" << (lbase_ + end) << "] to relocation_map_...");
#endif
    LIBCWD_TSD_DECLARATION;
#if CWDEBUG_ALLOC
    __libcwd_tsd.internal = 1;
#endif
#if CWDEBUG_DEBUG
    auto ibp =
#endif
    relocation_map_.emplace(std::make_pair(name, Range{start, end}));
#if CWDEBUG_ALLOC
    __libcwd_tsd.internal = 0;
#endif
#if CWDEBUG_DEBUG
    if (ibp.second)
      Dout(dc::finish, " done");
    else if (is_reloc || start != ibp.first->second.start_ || end > ibp.first->second.end_)
      Dout(dc::finish, " failed! Collision with \"" << ibp.first->first << std::hex <<
          "\" [0x" << (lbase_ + ibp.first->second.start_) << "-0x" << (lbase_ + ibp.first->second.end_) << "]");
    else
      Dout(dc::finish, " existed");
#endif
  }
}

void Elf::process_relocs_x(GElf_Shdr* shdr, Elf_Data* symdata, Elf_Data *xndxdata, size_t symstrndx,
    GElf_Addr r_offset, GElf_Xword r_info, GElf_Sxword r_addend)
{
  Elf32_Word xndx;
  GElf_Sym symmem;
  GElf_Sym* sym = gelf_getsymshndx(symdata, xndxdata, GELF_R_SYM(r_info), &symmem, &xndx);
  if (!sym)
  {
#if CWDEBUG_DEBUG
    Dout(dc::bfd, filepath_ << ": gelf_getsymshndx: INVALID SYMBOL");
#else
    Dout(dc::bfd, "gelf_getsymshndx: INVALID SYMBOL");
#endif
    return;
  }
  else if (GELF_ST_TYPE(sym->st_info) == STT_SECTION)
  {
    return;
#if 0
    // The symbol refers to a section header.
    GElf_Shdr destshdr_mem;
    GElf_Shdr* destshdr;
    destshdr = gelf_getshdr(elf_getscn(elf_, sym->st_shndx == SHN_XINDEX ? xndx : sym->st_shndx), &destshdr_mem);

    if (shdr == nullptr || destshdr == nullptr)
      Dout(dc::bfd, "INVALID SECTION");
    else
      Dout(dc::bfd, "shstrndx: " << shstrndx_ << ": value: 0x" << std::hex << sym->st_value << ": " << elf_strptr(elf_, shstrndx_, destshdr->sh_name));
#endif
  }

  // Found a symbol.
  add_symbol(sym, symstrndx, true, r_addend);
}

void Elf::add_symtab(std::function<void(Dwarf_Addr start, Dwarf_Addr end, char const* linkage_name LIBCWD_COMMA_TSD_PARAM)> symbol_cb) const
{
  LIBCWD_TSD_DECLARATION;
  for (relocation_map_ct::value_type const& value : relocation_map_)
    symbol_cb(value.second.start_, value.second.end_, value.first LIBCWD_COMMA_TSD);
}

void Elf::process_relocs_rel(GElf_Shdr* shdr, Elf_Data* data, Elf_Data* symdata, Elf_Data* xndxdata, size_t symstrndx)
{
  size_t sh_entsize = gelf_fsize(elf_, ELF_T_REL, 1, EV_CURRENT);
  int nentries = shdr->sh_size / sh_entsize;

  for (int cnt = 0; cnt < nentries; ++cnt)
  {
    GElf_Rel relmem;
    GElf_Rel* rel;

    rel = gelf_getrel(data, cnt, &relmem);
    if (rel != nullptr)
      process_relocs_x(shdr, symdata, xndxdata, symstrndx, rel->r_offset, rel->r_info, 0);
  }
}

void Elf::process_relocs_rela(GElf_Shdr* shdr, Elf_Data* data, Elf_Data* symdata, Elf_Data* xndxdata, size_t symstrndx)
{
  size_t sh_entsize = gelf_fsize(elf_, ELF_T_RELA, 1, EV_CURRENT);
  int nentries = shdr->sh_size / sh_entsize;

  for (int cnt = 0; cnt < nentries; ++cnt)
  {
    GElf_Rela relmem;
    GElf_Rela* rel;

    rel = gelf_getrela(data, cnt, &relmem);
    if (rel != nullptr)
      process_relocs_x(shdr, symdata, xndxdata, symstrndx, rel->r_offset, rel->r_info, rel->r_addend);
  }
}

int Elf::process_relocs()
{
  Elf_Scn* scn = nullptr;
  while ((scn = elf_nextscn(elf_, scn)) != nullptr)
  {
    GElf_Shdr shdr_mem;
    GElf_Shdr* shdr = gelf_getshdr(scn, &shdr_mem);
    if (shdr == nullptr)
    {
#if CWDEBUG_DEBUG
      Dout(dc::warning, filepath_ << ": gelf_getshdr returned NULL");
#else
      Dout(dc::warning, "gelf_getshdr returned NULL!");
#endif
      // Looks like a fatal error, but try to continue anyway.
      continue;
    }
    if (shdr->sh_type == SHT_REL || shdr->sh_type == SHT_RELA)
    {
#if 0
      GElf_Shdr destshdr_mem;
      GElf_Shdr* destshdr = gelf_getshdr(elf_getscn(elf_, shdr->sh_info), &destshdr_mem);
      if (destshdr == nullptr)
        continue;
#endif
      // Get the data of the section.
      Elf_Data* data = elf_getdata(scn, nullptr);
      if (data == nullptr)
        continue;
      // Get the symbol table information.
      Elf_Scn* symscn = elf_getscn(elf_, shdr->sh_link);
      GElf_Shdr symshdr_mem;
      GElf_Shdr* symshdr = gelf_getshdr(symscn, &symshdr_mem);
      Elf_Data* symdata = elf_getdata(symscn, nullptr);
      if (symshdr == nullptr || symdata == nullptr)
        continue;
      // Search for the optional extended section index table.
      Elf_Data* xndxdata = nullptr;
      Elf_Scn* xndxscn = nullptr;
      while ((xndxscn = elf_nextscn(elf_, xndxscn)) != nullptr)
      {
        GElf_Shdr xndxshdr_mem;
        GElf_Shdr* xndxshdr;

        xndxshdr = gelf_getshdr(xndxscn, &xndxshdr_mem);
        if (xndxshdr != nullptr && xndxshdr->sh_type == SHT_SYMTAB_SHNDX && xndxshdr->sh_link == elf_ndxscn(symscn))
        {
          // Found it.
          xndxdata = elf_getdata(xndxscn, nullptr);
          break;
        }
      }

      if (shdr->sh_type == SHT_REL)
        process_relocs_rel(shdr, data, symdata, xndxdata, symshdr->sh_link);
      else
        process_relocs_rela(shdr, data, symdata, xndxdata, symshdr->sh_link);
    }
  }

  //Dout(dc::bfd, "Adding .symtab:");
  scn = nullptr;
  while ((scn = elf_nextscn(elf_, scn)) != nullptr)
  {
    GElf_Shdr shdr_mem;
    GElf_Shdr* shdr = gelf_getshdr(scn, &shdr_mem);
    if (shdr == nullptr)
    {
#if CWDEBUG_DEBUG
      Dout(dc::warning, filepath_ << ": gelf_getshdr returned NULL");
#else
      Dout(dc::warning, "gelf_getshdr returned NULL!");
#endif
      return -1;
    }
    if (shdr->sh_type == SHT_SYMTAB)
    {
      // Get the data of the section.
      Elf_Data* data = elf_getdata(scn, nullptr);
      if (data == nullptr)
        continue;

      // Calculate the number of symbol entries in the .symtab section.
      size_t num_syms = shdr->sh_size / shdr->sh_entsize;

      // Process each symbol entry in the .symtab section.
      for (size_t i = 0; i < num_syms; ++i)
      {
        GElf_Sym sym_mem;
        GElf_Sym* sym = gelf_getsym(data, static_cast<int>(i), &sym_mem);
        if (sym != nullptr)
          add_symbol(sym, shdr->sh_link, false);
      }
      break;  // We found and processed the .symtab section, so exit the loop.
    }
  }

  // Success.
  return 0;
}

void Elf::add_backup(char const* linkage_name, GElf_Addr start, GElf_Addr end)
{
  auto it = relocation_map_.find(linkage_name);
  if (it == relocation_map_.end())
  {
    LIBCWD_TSD_DECLARATION;
#if CWDEBUG_ALLOC
    __libcwd_tsd.internal = 1;
#endif
    relocation_map_.emplace(std::make_pair(linkage_name, Range{start, end}));
#if CWDEBUG_ALLOC
    __libcwd_tsd.internal = 0;
#endif
  }
}

bool Elf::apply_relocation(char const* linkage_name, GElf_Addr& start_out, GElf_Addr& end_out) const
{
  auto it = relocation_map_.find(linkage_name);
  if (it == relocation_map_.end())
    return false;

  start_out = it->second.start_;
  end_out = it->second.end_;

  return true;
}

void objfile_ct::extract_and_add_function_symbols(char const* cu_name, Elf& elf, Dwarf_Die* die_ptr LIBCWD_COMMA_TSD_PARAM)
{
  //=====================================================================
  // Iterate over all children of the compilation unit.
  //

  Dwarf_Die child_die;
  if (dwarf_child(die_ptr, &child_die) == 0)   // Get the first child of this DIE, if any.
  {
    do
    {
      // Recursively iterate over all children of every child, if any.
      if (dwarf_haschildren(&child_die))
        extract_and_add_function_symbols(cu_name, elf, &child_die LIBCWD_COMMA_TSD);

      //===================================================================
      // Find all function DIE's with a name.
      //

      // We are only interested in DIE that represents a function.
      if (dwarf_tag(&child_die) == DW_TAG_subprogram)
      {
        Dwarf_Attribute attr;
        bool is_true;

        // Declarations are DW_TAG_subprogram too; skip all declarations; we are only interested in definitions.
        if (dwarf_attr(&child_die, DW_AT_declaration, &attr) &&
            dwarf_formflag(&attr, &is_true) == 0 && is_true)
          continue;

        // If the declaration is deleted; don't bother with it.
        if (dwarf_attr(&child_die, DW_AT_deleted, &attr) &&
            dwarf_formflag(&attr, &is_true) == 0 && is_true)
          continue;

        // We STRONGLY prefer mangled names in the case of C++ functions. Try to obtain it.
        char const* linkage_name = nullptr;
        Dwarf_Die referenced_die;
        for (Dwarf_Die* die_ptr = &child_die;; die_ptr = &referenced_die)
        {
          if (dwarf_attr(die_ptr, DW_AT_MIPS_linkage_name, &attr) ||
              dwarf_attr(die_ptr, DW_AT_linkage_name, &attr))
          {
            linkage_name = dwarf_formstring(&attr);
            break;
          }
          else if ((
                !dwarf_attr(die_ptr, DW_AT_specification, &attr) &&
                !dwarf_attr(die_ptr, DW_AT_abstract_origin, &attr)) ||
              !dwarf_formref_die(&attr, &referenced_die))
            break;
        }

        // Not sure if it can happen that such a function DIE has no name, but if that is the case then
        // pretend that that isn't a function.
        char const* func_name = dwarf_diename(&child_die);
        if ((!func_name || !*func_name) && !linkage_name)
          continue;

        //=================================================================
        // Run over all address ranges owned by the function DIE.
        //

        Dwarf_Addr lbase;
        Dwarf_Addr start;
        Dwarf_Addr end;
        ptrdiff_t offset = 0;
        while ((offset = dwarf_ranges(&child_die, offset, &lbase, &start, &end)) > 0)
        {
          if (end != (Dwarf_Addr)-1)
          {
            bool is_relocated = false;
            char const* name = linkage_name ? linkage_name : func_name;
            if (!start)
            {
              if (!name)        // Is this even possible?
              {
                Dout(dc::warning, "Zero low_pc and no name for symbol in " <<
                    M_object_file.filepath() << " / " << (cu_name ? cu_name : "<null>"));
#if CWDEBUG_DEBUG
                print_die_info(&child_die);
#endif
                continue;
              }
              if (elf.apply_relocation(name, start, end))
              {
                is_relocated = true;
#if CWDEBUG_DEBUG
                Dout(dc::bfd, "Relocated \"" << name << "\" to " <<
                    (void*)(M_lbase + start) << "-" << (void*)(M_lbase + end) << " of " <<
                    M_object_file.filepath() << " / " << (cu_name ? cu_name : "<null>"));
#endif
              }
              else
              {
#if CWDEBUG_DEBUG       // I have no idea why this would be "normal"... but it seems to be the case for /usr/lib/libstdc++.so.6.
                // This library has symbols in its .debug_info section with a pc_low of zero, that do not appear in the .symtab.
                Dout(dc::warning, "Symbol not in .symtab: \"" << name << "\" of " <<
                    M_object_file.filepath() << " / " << (cu_name ? cu_name : "<null>"));
#endif
                continue;       // relocation failed (symbol not in .symtab).
              }
            }
            LIBCWD_ASSERT(start != 0 && end > start);

            DWARF_ACQUIRE_WRITE_LOCK;
#if CWDEBUG_ALLOC
            __libcwd_tsd.internal = 1;
#endif
            auto ibp = M_function_symbols.emplace(start, end, this, &child_die, cu_name, func_name, linkage_name);
#if CWDEBUG_ALLOC
            __libcwd_tsd.internal = 0;
#endif
            if (ibp.second)
            {
#if CWDEBUG_DEBUG
              Dout(dc::bfd|continued_cf, "Successfully inserted")
#endif
              ;
              // This is probably never needed, but since we have this information anyway
              // add it to the "relocation" table - so that any other DIE with a low_pc of zero
              // will pick it up if this information is missing everywhere else.
              if (!is_relocated)
                elf.add_backup(name, start, end);
            }
            else if (M_lbase + start == ibp.first->real_start())
            {
              // If our func_name is different from the DIE name, then we're probably an alias.
              // Let store the alias name because it seems typically to be the name that users should use.
              if (std::strcmp(dwarf_diename(&child_die), func_name) != 0)
              {
                if (linkage_name)
                  ibp.first->set_linkage_name(linkage_name);
                if (func_name)
                  ibp.first->set_func_name(func_name);
              }
              if (M_lbase + end == ibp.first->real_end())
              {
#if CWDEBUG_DEBUG
                Dout(dc::bfd|continued_cf, "Duplicate")
#endif
                ;
              }
              else if (is_relocated)
              {
#if CWDEBUG_DEBUG
                Dout(dc::bfd|continued_cf, "Duplicate with different end");
#endif
                // Apparently it happens... always believe the relocation value.
                // Although we're changing the key of the set here, it shouldn't reorder the element...
                const_cast<symbol_ct&>(*ibp.first).set_real_end(M_lbase + end);
              }
            }
            else
            {
#if CWDEBUG_DEBUG
              Dout(dc::bfd|continued_cf, "Failed to insert: overlapping range with different start address!");
#else
              Dout(dc::warning, "Failed to insert: overlapping range with different start address! \"" <<
                  (linkage_name ? linkage_name : func_name) << "\", range " <<
                  (void*)(M_lbase + start) << "-" << (void*)(M_lbase + end) << " of " <<
                  M_object_file.filepath() << " / " << (cu_name ? cu_name : "<null>"));
#endif
            }
#if CWDEBUG_DEBUG
            Dout(dc::finish, " \"" << (linkage_name ? linkage_name : func_name) << "\", range " <<
                (void*)(M_lbase + start) << "-" << (void*)(M_lbase + end) << " of " <<
                M_object_file.filepath() << " / " << (cu_name ? cu_name : "<null>"));
            if (!ibp.second)
              print_die_info(&child_die);
#endif

            LIBCWD_ASSERT(M_lbase + start == ibp.first->real_start());

            DWARF_RELEASE_WRITE_LOCK;
          }
        }

        //
        //=================================================================
      }
      //===================================================================
      // Continue with the next child DIE of the compilation unit.
    }
    while (dwarf_siblingof(&child_die, &child_die) == 0);
  }
}

bool objfile_ct::initialize(char const* filename LIBCWD_COMMA_ALLOC_OPT(bool is_libc) LIBCWD_COMMA_TSD_PARAM)
{
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( __libcwd_tsd.internal == 0 );
#if CWDEBUG_DEBUGT
  LIBCWD_ASSERT( _private_::locked_by[object_files_instance] != __libcwd_tsd.tid );
  LIBCWD_ASSERT( __libcwd_tsd.rdlocked_by1[object_files_instance] != __libcwd_tsd.tid && __libcwd_tsd.rdlocked_by2[object_files_instance] != __libcwd_tsd.tid );
#endif
#endif

  DWARF_ACQUIRE_WRITE_LOCK;
  bool already_exists = false;
  for (object_files_ct::iterator iter = NEEDS_WRITE_LOCK_object_files().begin();
       iter != NEEDS_WRITE_LOCK_object_files().end(); ++iter)
  {
    if (static_cast<objfile_ct const*>(*iter)->get_lbase() == get_lbase())
    {
      Dout(dc::bfd, "objfile_ct::initialize(\"" << get_object_file()->filepath() << "\"): already loaded as \"" <<
          (*iter)->get_object_file()->filepath() << "\"");
      already_exists = true;
      break;
    }
  }
  DWARF_RELEASE_WRITE_LOCK;

  if (!already_exists)
  {
    open_dwarf(LIBCWD_TSD);

    //===========================================================================
    // Open the ELF file for relocation info.
    //
    try
    {
      Elf elf(M_object_file.filepath(), get_lbase());
      elf.process_relocs();

      if (dwarf_handle_)
      {
        //===========================================================================
        // Iterate over all compilation units of the given object file.
        //
        Dwarf_Off offset = 0;
        Dwarf_Off next_offset;
        size_t hsize;
        while (dwarf_nextcu(dwarf_handle_, offset, &next_offset, &hsize, nullptr, nullptr, nullptr) == 0)
        {
          //=========================================================================
          // Find the compilation unit Debug Information Entry (DIE).
          //
          Dwarf_Die cu_die;
          Dwarf_Die* cu_die_ptr = dwarf_offdie(dwarf_handle_, offset + hsize, &cu_die);
          if (cu_die_ptr)
          {
            //=======================================================================
            // Compilation unit name
            //
            char const* cu_name = dwarf_diename(cu_die_ptr);
#if CWDEBUG_DEBUG
            if (cu_name)
              Dout(dc::bfd, "Found compilation unit: \"" << cu_name << "\" (" << (void*)cu_name << ")");
            else
              Dout(dc::bfd, "DIE name not found.");
#endif

            extract_and_add_function_symbols(cu_name, elf, cu_die_ptr LIBCWD_COMMA_TSD);
          }
          else
            Dout(dc::bfd, get_object_file()->filepath() << ": dwarf_offdie failed: " << dwarf_errmsg(-1));

          //=========================================================================
          // Next compilation unit.
          //
          // Update the offset for the next iteration.
          offset = next_offset;
        }
      }

      elf.add_symtab([this](Dwarf_Addr start, Dwarf_Addr end, char const* linkage_name LIBCWD_COMMA_TSD_PARAM){
        // If the symbol table contains a function with an address range that does not overlap with
        // an already stored address range from .debug_info - then add it. I expect this to only
        // add functions when there wasn't a .debug_info in the first place.
#if CWDEBUG_ALLOC
        __libcwd_tsd.internal = 1;
#endif
        auto ibp = M_function_symbols.emplace(start, end, this, linkage_name);
#if CWDEBUG_ALLOC
        __libcwd_tsd.internal = 0;
#endif
#if CWDEBUG_DEBUG
        if (dwarf_handle_ && ibp.second)
          Dout(dc::bfd, "Added symbol from .symtab that does not exist in .debug_info: \"" <<
              linkage_name << "\" [" << (void*)(M_lbase + start) << "-" << (void*)(M_lbase + end) << "]");
#endif
      });
    }
    catch (std::exception const&)
    {
#if CWDEBUG_DEBUGM
      LIBCWD_ASSERT( __libcwd_tsd.internal == 0 );
#endif
      // A fatal error occurred.
      return true;
    }
    //
    //===========================================================================
  }

  if (!already_exists)
  {
    DWARF_ACQUIRE_WRITE_LOCK;
#if CWDEBUG_ALLOC
    __libcwd_tsd.internal = 1;
#endif
    NEEDS_WRITE_LOCK_object_files().push_back(this);
#if CWDEBUG_ALLOC
    __libcwd_tsd.internal = 0;
#endif
    DWARF_RELEASE_WRITE_LOCK;
  }

#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( __libcwd_tsd.internal == 0 );
#endif
  return already_exists;
}

_private_::string read_build_id(_private_::string const& object_file LIBCWD_COMMA_TSD_PARAM)
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
    {
#if CWDEBUG_ALLOC
      if (!__libcwd_tsd.inside_malloc_or_free)
#endif
        Dout(dc::warning, "failed to open file \"" << object_file << "\"");
    }
    else
    {
      // Open an ELF descriptor for reading.
      ::Elf* e = elf_begin(fd, ELF_C_READ, NULL);
      if (!e)
      {
#if CWDEBUG_ALLOC
        if (!__libcwd_tsd.inside_malloc_or_free)
#endif
          Dout(dc::warning, "elf_begin returned NULL for \"" << object_file << "\": " << elf_errmsg(-1));
      }
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
                  _private_::string result;
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

_private_::string get_debug_info_path(_private_::string object_file LIBCWD_COMMA_TSD_PARAM)
{
  _private_::string debug_info_path;

  char const* build_id_dir = "/usr/lib/debug/.build-id";
  struct stat sb;
  int sr = stat(build_id_dir, &sb);
  _private_::string build_id;
  if (sr == 0 && S_ISDIR(sb.st_mode))
  {
    // Get the build-id, if any.
    build_id = read_build_id(object_file LIBCWD_COMMA_TSD);
    if (build_id.length() > 2)
    {
      debug_info_path = build_id_dir;
      debug_info_path += '/';
      debug_info_path += build_id.substr(0, 2);
      debug_info_path += '/';
      debug_info_path += build_id.substr(2);
      debug_info_path += ".debug";
    }
  }
  else
    debug_info_path = "/usr/lib/debug" + object_file + ".debug";
  sr = stat(debug_info_path.c_str(), &sb);
  if (sr != 0)
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
      _private_::string cache_path = cache_path_envvar;
      switch (path_attempt)
      {
        case 0:
          break;
        case 1:
          cache_path += "/debuginfod_client/";
          break;
        case 2:
          cache_path += "/.cache/debuginfod_client/";
          break;
        case 3:
          cache_path += "/.debuginfod_client_cache/";
          break;
      }
      cache_path += build_id;
      cache_path += "/debuginfo";
      if (stat(cache_path.c_str(), &sb) == 0)
      {
        debug_info_path = cache_path;
        break;
      }
    }
  }
  return debug_info_path;
}

void objfile_ct::open_dwarf(LIBCWD_TSD_PARAM)
{
  LIBCWD_ASSERT(dwarf_fd_ == -1 && dwarf_handle_ == nullptr);
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif

  _private_::string debug_info_path = get_debug_info_path(M_object_file.filepath() LIBCWD_COMMA_TSD);

  bool different_symbols_path = debug_info_path != M_object_file.filepath();
  Dout(dc::bfd|continued_cf, "Loading debug info " << (different_symbols_path ? "for " : "from ") << M_object_file.filepath());
  if (different_symbols_path)
    Dout(dc::continued, " (from " << debug_info_path << ")");
  if (M_lbase != unknown_base_addr && M_lbase != executable_base_addr)
    Dout(dc::continued, " (" << (void*)M_lbase << " [" << (void*)M_start_addr << '-' << (void*)M_end_addr << "])");
  Dout(dc::continued|flush_cf, "... ");

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

symbol_ct const* objfile_ct::find_symbol(symbol_ct const& search_key) const
{
  auto iter = M_function_symbols.find(search_key);
  if (iter == M_function_symbols.end())
    return nullptr;
  return &*iter;
}

void objfile_ct::close_dwarf(LIBCWD_TSD_PARAM)
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

void objfile_ct::deinitialize(LIBCWD_TSD_PARAM)
{
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
#if CWDEBUG_DEBUGT
  LIBCWD_ASSERT( _private_::locked_by[object_files_instance] != __libcwd_tsd.tid );
  LIBCWD_ASSERT( __libcwd_tsd.rdlocked_by1[object_files_instance] != __libcwd_tsd.tid && __libcwd_tsd.rdlocked_by2[object_files_instance] != __libcwd_tsd.tid );
#endif
#endif
#if CWDEBUG_ALLOC
  _private_::remove_type_info_references(&M_object_file LIBCWD_COMMA_TSD);
#endif
  LIBCWD_DEFER_CANCEL;
  DWARF_ACQUIRE_WRITE_LOCK;
  set_alloc_checking_off(LIBCWD_TSD);
  object_files_ct::iterator iter(find(NEEDS_WRITE_LOCK_object_files().begin(),
                                      NEEDS_WRITE_LOCK_object_files().end(), this));
  if (iter != NEEDS_WRITE_LOCK_object_files().end())
    NEEDS_WRITE_LOCK_object_files().erase(iter);
  set_alloc_checking_on(LIBCWD_TSD);
  DWARF_RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;

  close_dwarf(LIBCWD_TSD);
}

objfile_ct* NEEDS_READ_LOCK_find_object_file(uintptr_t addr)
{
  object_files_ct::const_iterator i(NEEDS_READ_LOCK_object_files().begin());
  for(; i != NEEDS_READ_LOCK_object_files().end(); ++i)
    if (static_cast<objfile_ct const*>(*i)->get_start_addr() < addr && addr < static_cast<objfile_ct const*>(*i)->get_end_addr())
      break;
  return (i != NEEDS_READ_LOCK_object_files().end()) ? static_cast<objfile_ct*>(*i) : nullptr;
}

objfile_ct* get_object_file(void const* addr LIBCWD_COMMA_TSD_PARAM)
{
  objfile_ct* object_file;
  uintptr_t int_addr = reinterpret_cast<uintptr_t>(addr);

  LIBCWD_DEFER_CANCEL;
  DWARF_ACQUIRE_READ_LOCK;
  object_file = NEEDS_READ_LOCK_find_object_file(int_addr);
  DWARF_RELEASE_READ_LOCK;

  if (!object_file && !statically_linked)
  {
    // The const_cast is OK because dl_iterate_phdr_callback2 does not write to addr.
    dl_iterate_phdr(dl_iterate_phdr_callback2, const_cast<void*>(addr));

    LIBCWD_DEFER_CANCEL;
    DWARF_ACQUIRE_WRITE_LOCK;
    set_alloc_checking_off(LIBCWD_TSD);
    NEEDS_WRITE_LOCK_object_files().sort(object_file_greater());
    set_alloc_checking_on(LIBCWD_TSD);
    object_file = NEEDS_READ_LOCK_find_object_file(int_addr);
    DWARF_RELEASE_WRITE_LOCK;
    LIBCWD_RESTORE_CANCEL;
  }

  LIBCWD_RESTORE_CANCEL;

  return object_file;
}

} // namespace dwarf

/** \addtogroup group_locations */
/** \{ */

char const* const unknown_function_c = "<unknown function>";

/**
 * \brief Find the mangled function name of the address \a addr.
 *
 * \returns the same pointer that is returned by location_ct::mangled_function_name() on success,
 * otherwise \ref unknown_function_c is returned.
 */
char const* pc_mangled_function_name(void const* addr)
{
  using namespace dwarf;

  LIBCWD_TSD_DECLARATION;

  if (!WST_initialized)	// `WST_initialized' is only false when we are still Single Threaded.
  {
    if (!ST_init(LIBCWD_TSD))
      return unknown_function_c;
  }

  objfile_ct* object_file = get_object_file(addr LIBCWD_COMMA_TSD);
  if (!object_file)
    return unknown_function_c;

  symbol_ct search_key(reinterpret_cast<uintptr_t>(addr));
  symbol_ct const* symbol = object_file->find_symbol(search_key);
  if (!symbol)
    return unknown_function_c;

  return symbol->name();
}

/** \} */	// End of group 'group_locations'.

#if CWDEBUG_ALLOC
struct bfd_location_ct : public location_ct {
  friend _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, bfd_location_ct const& data);
};

_private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, bfd_location_ct const& location)
{
  if (location.M_known)
    os << location.M_filename << ':' << location.M_line;
  else
    os << "<unknown location>";
  return os;
}
#else
using bfd_location_ct = location_ct;
#endif

object_file_ct::object_file_ct(char const* filepath) :
#if CWDEBUG_ALLOC
    M_hide(false),
#endif
    M_no_debug_line_sections(false)
{
  LIBCWD_TSD_DECLARATION;
  set_alloc_checking_off(LIBCWD_TSD);
  M_filepath = strcpy((char*)malloc(strlen(filepath) + 1), filepath);	// LEAK8
  set_alloc_checking_on(LIBCWD_TSD);
  M_filename = strrchr(M_filepath, '/') + 1;
  if (M_filename == (char const*)1)
    M_filename = M_filepath;
}

//
// location_ct::M_pc_location
//
// Find source file, (mangled) function name and line number of the address `addr'.
//
// Like `pc_function', this function looks up the symbol (function) that
// belongs to the address `addr' and stores the pointer to the name of that symbol
// in the member `M_func'.  When no symbol could be found then `M_func' is set to
// `libcwd::unknown_function_c'.
//
// If a symbol is found then this function attempts to lookup source file and line number
// nearest to the given address.  The result - if any - is put into `M_filepath' (source
// file) and `M_line' (line number), and `M_filename' is set to point to the filename
// part of `M_filepath'.
//
void location_ct::M_pc_location(void const* addr LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_ASSERT( !M_known );

  using namespace dwarf;

  if (!WST_initialized)
  {
    // MT: `WST_initialized' is only false when we're still Single Threaded.
    //     Therefore it is safe to call ST_* functions.

#if CWDEBUG_ALLOC && LIBCWD_IOSBASE_INIT_ALLOCATES
    if (!_private_::WST_ios_base_initialized && _private_::inside_ios_base_Init_Init())
    {
      M_object_file = nullptr;
      M_func = S_pre_ios_initialization_c;
      M_initialization_delayed = addr;
      return;
    }
#endif
    if (!ST_init(LIBCWD_TSD))	// Initialization of BFD code fails?
    {
      M_object_file = nullptr;
      M_func = S_pre_libcwd_initialization_c;
      M_initialization_delayed = addr;
      return;
    }
  }

#if LIBCWD_THREAD_SAFE
  if (__libcwd_tsd.pthread_lock_interface_is_locked)
  {
    // Some time, in another universe, this might really happen:
    //
    // We get here when we are writing debug output (and therefore set the lock)
    // and the streambuf of the related ostream overflows, this use basic_string
    // which uses the pool allocator which might just at that moment also run
    // out of memory and therefore call malloc(3).  And when THAT happens for the
    // first time, so the location from which that is called is not yet in the
    // location cache... then we get here.
    // We cannot obtain the object_files_instance lock now because another thread
    // might have obtained that already when calling malloc(3), doing a location
    // lookup for that and then realizing that no debug info was read yet for
    // the library that did that malloc(3) call and therefore wanting to print
    // debug output (Loading debug info from...) causing a dead-lock.
    //
    // PS We DID get here - 0.99.45, compiled with gcc 3.4.6, while running
    // threads_threads_shared from dejagnu (2006/11/08). I also had to stand
    // on one leg and wave high left.
    M_object_file = nullptr;
    M_func = S_pre_libcwd_initialization_c;	// Not really true, but this hardly ever happens in the first place.
    M_initialization_delayed = addr;
    return;
  }
#endif

  objfile_ct* object_file = get_object_file(addr LIBCWD_COMMA_TSD);

  M_initialization_delayed = nullptr;
  if (!object_file)
  {
    Dout(dc::bfd, "No object file for address " << addr);
    M_object_file = nullptr;
    M_func = unknown_function_c;
    M_unknown_pc = addr;
    return;
  }

  M_object_file = object_file->get_object_file();

  uintptr_t int_addr = reinterpret_cast<uintptr_t>(addr);
  symbol_ct search_key(int_addr);
  symbol_ct const* symbol = object_file->find_symbol(search_key);
  if (!symbol)
  {
    M_func = unknown_function_c;
    M_unknown_pc = addr;
    return;
  }

  M_func = symbol->name();

  Dwarf_Die cu_die;
  Dwarf_Line* line;
  if (!symbol->diecu(&cu_die) || !(line = dwarf_getsrc_die(&cu_die, int_addr - object_file->get_lbase())))
  {
    M_known = false;
    return;
  }

  int lineno = 0;
  if (dwarf_lineno(line, &lineno) != 0)
    Dout(dc::bfd, "dwarf_line failed for address " << addr << ": " << dwarf_errmsg(-1));
  M_line = lineno;

  char const* srcfile;
  if ((srcfile = dwarf_linesrc(line, nullptr, nullptr)) == nullptr)
    Dout(dc::bfd, "dwarf_linesrc failed for address " << addr << ": " << dwarf_errmsg(-1));
  else
  {
    size_t len = strlen(srcfile);
    set_alloc_checking_off(LIBCWD_TSD);
    M_filepath = lockable_auto_ptr<char, true>(new char [len + 1]);	// LEAK5
    set_alloc_checking_on(LIBCWD_TSD);
    strcpy(M_filepath.get(), srcfile);
    M_known = true;
    M_filename = strrchr(M_filepath.get(), '/') + 1;
    if (M_filename == (char const*)1)
      M_filename = M_filepath.get();
  }
}

/**
 * \brief Reset this location object (frees memory).
 */
void location_ct::clear()
{
  if (M_known)
  {
    M_known = false;
#if CWDEBUG_ALLOC
    M_hide = _private_::filtered_location;
#endif
    if (M_filepath.is_owner())
    {
      LIBCWD_TSD_DECLARATION;
      set_alloc_checking_off(LIBCWD_TSD);
      M_filepath.reset();
      set_alloc_checking_on(LIBCWD_TSD);
    }
  }
  M_object_file = nullptr;
  M_func = S_cleared_location_ct_c;
}

location_ct::location_ct(location_ct const &prototype)
#if CWDEBUG_ALLOC
    : M_hide(_private_::new_location)
#endif
{
  if ((M_known = prototype.M_known))
  {
    M_filepath = prototype.M_filepath;
    M_filename = prototype.M_filename;
    M_line = prototype.M_line;
  }
  else
    M_initialization_delayed = prototype.M_initialization_delayed;
  M_object_file = prototype.M_object_file;
  M_func = prototype.M_func;
#if CWDEBUG_ALLOC
  M_hide = prototype.M_hide;
#endif
}

location_ct& location_ct::operator=(location_ct const &prototype)
{
  if (this != &prototype)
  {
    clear();
    if ((M_known = prototype.M_known))
    {
      M_filepath = prototype.M_filepath;
      M_filename = prototype.M_filename;
      M_line = prototype.M_line;
    }
    else
      M_initialization_delayed = prototype.M_initialization_delayed;
    M_object_file = prototype.M_object_file;
    M_func = prototype.M_func;
#if CWDEBUG_ALLOC
    M_hide = prototype.M_hide;
#endif
  }
  return *this;
}

} // namespace libcwd

#ifdef HAVE_DLOPEN
using namespace libcwd;

struct dlloaded_st {
  dwarf::objfile_ct* M_object_file;
  int M_flags;
  int M_refcount;
  dlloaded_st(dwarf::objfile_ct* object_file, int flags) : M_object_file(object_file), M_flags(flags), M_refcount(1) { }
};

namespace libcwd::_private_ {

using dlopen_map_ct = std::map<void*, dlloaded_st, std::less<void*>
#if CWDEBUG_ALLOC
                      , internal_allocator::rebind<std::pair<void* const, dlloaded_st> >::other
#endif
                      >;
static dlopen_map_ct* dlopen_map;

#if LIBCWD_THREAD_SAFE
void dlopenclose_cleanup(void* arg)
{
#if CWDEBUG_ALLOC
  TSD_st& __libcwd_tsd = (*static_cast<TSD_st*>(arg));
  // We can get here from DoutFatal, in which case alloc checking is already turned on.
  if (__libcwd_tsd.internal)
    set_alloc_checking_on(LIBCWD_TSD);
#endif
  rwlock_tct<object_files_instance>::cleanup(arg);
}

void dlopen_map_cleanup(void* arg)
{
#if CWDEBUG_ALLOC
  TSD_st& __libcwd_tsd = (*static_cast<TSD_st*>(arg));
  if (__libcwd_tsd.internal)
    set_alloc_checking_on(LIBCWD_TSD);
#else
  (void)arg;	// Suppress unused warning.
#endif
  DLOPEN_MAP_RELEASE_LOCK;
}
#endif

} // namespace libcwd::_private_

extern "C" {

  void* dlopen(char const* name, int flags)
  {
#if CWDEBUG_DEBUGM
    LIBCWD_TSD_DECLARATION;
    LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
    // One time initialization.
    if (!real_dlopen.symptr)
      real_dlopen.symptr = dlsym(RTLD_NEXT, "dlopen");
    // Call the real dlopen.
    void* handle = real_dlopen.func(name, flags);
    if (libcwd::dwarf::statically_linked)
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
#if !CWDEBUG_DEBUGM
    LIBCWD_TSD_DECLARATION;
#endif
    LIBCWD_DEFER_CLEANUP_PUSH(libcwd::_private_::dlopen_map_cleanup, &__libcwd_tsd);
    DLOPEN_MAP_ACQUIRE_LOCK;
    if (!libcwd::_private_::dlopen_map)
    {
      set_alloc_checking_off(LIBCWD_TSD);
      libcwd::_private_::dlopen_map = new libcwd::_private_::dlopen_map_ct;	// LEAK52
      set_alloc_checking_on(LIBCWD_TSD);
    }
    libcwd::_private_::dlopen_map_ct::iterator iter(libcwd::_private_::dlopen_map->find(handle));
    if (iter != libcwd::_private_::dlopen_map->end())
      ++(*iter).second.M_refcount;
    else
    {
      dwarf::objfile_ct* object_file;
#ifdef HAVE__DL_LOADED
      if (name)
      {
	name = ((link_map*)handle)->l_name;	// This is dirty, but its the only reasonable way to get
						// the full path to the loaded library.
      }
#endif
      // Don't call dwarf::load_object_file when dlopen() was called with nullptr as argument.
      if (name && *name)
      {
	object_file = dwarf::load_object_file(name, dwarf::unknown_base_addr, 0, 0);
	if (object_file)
	{
	  LIBCWD_DEFER_CANCEL;
	  DWARF_ACQUIRE_WRITE_LOCK;
	  set_alloc_checking_off(LIBCWD_TSD);
	  dwarf::NEEDS_WRITE_LOCK_object_files().sort(dwarf::object_file_greater());
	  set_alloc_checking_on(LIBCWD_TSD);
	  DWARF_RELEASE_WRITE_LOCK;
	  LIBCWD_RESTORE_CANCEL;
	  set_alloc_checking_off(LIBCWD_TSD);
	  libcwd::_private_::dlopen_map->insert(std::pair<void* const, dlloaded_st>(handle, dlloaded_st(object_file, flags)));
	  set_alloc_checking_on(LIBCWD_TSD);
	}
      }
    }
    DLOPEN_MAP_RELEASE_LOCK;
    LIBCWD_CLEANUP_POP_RESTORE(false);
    return handle;
  }

  int dlclose(void* handle)
  {
    LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
    LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
    // One time initialization.
    if (!real_dlclose.symptr)
      real_dlclose.symptr = dlsym(RTLD_NEXT, "dlclose");
    // Block until printing of allocations is finished because it might be that
    // those allocations have type_info pointers to shared objectst that will be
    // removed by dlclose, causing a core dump in list_allocations_on in the other
    // thread.
    int ret;
    LIBCWD_DEFER_CLEANUP_PUSH(&_private_::mutex_tct<dlclose_instance>::cleanup, &__libcwd_tsd);
    DLCLOSE_ACQUIRE_LOCK;
    ret = real_dlclose.func(handle);
    DLCLOSE_RELEASE_LOCK;
    LIBCWD_CLEANUP_POP_RESTORE(false);
    if (ret != 0 || libcwd::dwarf::statically_linked)
      return ret;
    LIBCWD_DEFER_CLEANUP_PUSH(_private_::dlopen_map_cleanup, &__libcwd_tsd);
    DLOPEN_MAP_ACQUIRE_LOCK;
    libcwd::_private_::dlopen_map_ct::iterator iter(libcwd::_private_::dlopen_map->find(handle));
    if (iter != libcwd::_private_::dlopen_map->end())
    {
      if (--(*iter).second.M_refcount == 0)
      {
#ifdef RTLD_NODELETE
	if (!((*iter).second.M_flags & RTLD_NODELETE))
	{
	  (*iter).second.M_object_file->deinitialize(LIBCWD_TSD);
	  // M_object_file is not deleted because location_ct objects still point to it.
	}
#endif
	set_alloc_checking_off(LIBCWD_TSD);
	libcwd::_private_::dlopen_map->erase(iter);
	set_alloc_checking_on(LIBCWD_TSD);
      }
    }
    DLOPEN_MAP_RELEASE_LOCK;
    LIBCWD_CLEANUP_POP_RESTORE(false);
    return ret;
  }

} // extern "C"

#endif // HAVE_DLOPEN

namespace libcwd {

void location_ct::print_filepath_on(std::ostream& os) const
{
  LIBCWD_ASSERT( M_known );
  os << M_filepath.get();
}

void location_ct::print_filename_on(std::ostream& os) const
{
  LIBCWD_ASSERT( M_known );
  os << M_filename;
}

} // namespace libcwd

#endif // CWDEBUG_LOCATION
