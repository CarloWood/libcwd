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
#include <cstdio>		// Needed for vsnprintf.
#include <algorithm>
#ifdef LIBCWD_DEBUGBFD
#include <iomanip>
#endif
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
  getgroups_t* group_array = (getgroups_t*)NULL;

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
static char unknown_l_addr_c;
static char executable_l_addr_c;
constexpr void* unknown_l_addr = &unknown_l_addr_c;
constexpr void* executable_l_addr = &executable_l_addr_c;

static bool WST_initialized = false;                      // MT: Set here to false, set to `true' once in `cwbfd::ST_init'.
struct objfile_ct;

struct object_file_greater {
  bool operator()(objfile_ct const* a, objfile_ct const* b) const { return a->get_lbase() > b->get_lbase(); }
};

objfile_ct* load_object_file(char const* name, void* l_addr, void const* end, bool initialized = false)
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
      return NULL;
  }
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
  if (l_addr == unknown_l_addr)
    Dout(dc::bfd|continued_cf|flush_cf, "Loading debug symbols from " << name << ' ');
  else if (l_addr == executable_l_addr)
    Dout(dc::bfd|continued_cf|flush_cf, "Loading debug symbols from " << name << "... ");
  else
    Dout(dc::bfd|continued_cf|flush_cf, "Loading debug symbols from " << name << " (" << l_addr << ") ... ");
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
  bool already_exists;
  LIBCWD_DEFER_CANCEL;
  DWARF_ACQUIRE_WRITE_LOCK;
  set_alloc_checking_off(LIBCWD_TSD);
  object_file = new objfile_ct(name, l_addr, end);		// LEAK6
  DWARF_RELEASE_WRITE_LOCK;
  already_exists =
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
      object_file->initialize(name LIBCWD_COMMA_ALLOC_OPT(is_libc), is_libstdcpp LIBCWD_COMMA_TSD);
#else
      object_file->initialize(name LIBCWD_COMMA_ALLOC_OPT(is_libc) LIBCWD_COMMA_TSD);
#endif
  set_alloc_checking_on(LIBCWD_TSD);
  LIBCWD_RESTORE_CANCEL;
  if (!already_exists)
  {
    Dout(dc::finish, "done");
  }
  else
  {
    Dout(dc::finish, "Already loaded");
    set_alloc_checking_off(LIBCWD_TSD);
    delete object_file;
    set_alloc_checking_on(LIBCWD_TSD);
    return NULL;
  }
  return object_file;
}

static int dl_iterate_phdr_callback(dl_phdr_info* info, size_t size, void* fullpath)
{
  uintptr_t start_addr = info->dlpi_addr + info->dlpi_phdr[0].p_vaddr;
  uintptr_t end_addr = start_addr + info->dlpi_phdr[0].p_memsz;
  objfile_ct* objfile;
  if (info->dlpi_name[0] == 0)
  {
#if 0
    //FIXME: is this still correct when using dl_iterate_phdr?
    // It seems that the executable is loaded somewhere with offset 0x555555554000 (without ASLR).
    // non-PIE code seems to be loaded at very low addresses.
    bool is_pie = info->dlpi_addr >= 0x500000000000;     // Dirty heuristics.
    // If the executable is not PIE then executable_l_addr should be passed instead here.
    objfile = load_object_file(fullpath.data(), is_pie ? reinterpret_cast<void*>(l->l_addr) : executable_l_addr, true);
#endif
    objfile = load_object_file(reinterpret_cast<char const*>(fullpath), reinterpret_cast<void*>(start_addr), reinterpret_cast<void*>(end_addr), true);
  }
  else if (info->dlpi_name[0] == '/' || info->dlpi_name[0] == '.')
    objfile = load_object_file(info->dlpi_name, reinterpret_cast<void*>(start_addr), reinterpret_cast<void*>(end_addr));
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

char objfile_ct::ST_list_instance[sizeof(object_files_ct)] __attribute__((__aligned__));

objfile_ct::objfile_ct(char const* filename, void* base, void const* end) : M_lbase(base), M_end(end), M_object_file(filename)
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( __libcwd_tsd.internal );
#if CWDEBUG_DEBUGT
  LIBCWD_ASSERT( _private_::is_locked(object_files_instance) );
#endif
#endif
}

bool objfile_ct::initialize(char const* filename LIBCWD_COMMA_ALLOC_OPT(bool is_libc) LIBCWD_COMMA_TSD_PARAM)
{
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( __libcwd_tsd.internal == 1 );
#if CWDEBUG_DEBUGT
  LIBCWD_ASSERT( _private_::locked_by[object_files_instance] != __libcwd_tsd.tid );
  LIBCWD_ASSERT( __libcwd_tsd.rdlocked_by1[object_files_instance] != __libcwd_tsd.tid && __libcwd_tsd.rdlocked_by2[object_files_instance] != __libcwd_tsd.tid );
#endif
#endif
  DWARF_ACQUIRE_WRITE_LOCK;
  bool already_exists = false;
#if CWDEBUG_ALLOC
  __libcwd_tsd.internal = 0;
#endif
  Dout(dc::bfd, "Adding \"" << get_object_file()->filepath() << "\", load address " <<
      get_lbase() /*<< ", start " << get_start() << " and end " << (void*)((size_t)get_start() + size())*/);
  for (object_files_ct::iterator iter = NEEDS_WRITE_LOCK_object_files().begin();
       iter != NEEDS_WRITE_LOCK_object_files().end(); ++iter)
  {
    if ((*iter)->get_lbase() == get_lbase())
    {
//      LIBCWD_ASSERT((*iter)->size() == size());
      Dout(dc::bfd, "Already loaded as \"" << (*iter)->get_object_file()->filepath() << "\"");
      already_exists = true;
      break;
    }
  }
#if CWDEBUG_ALLOC
  __libcwd_tsd.internal = 1;
#endif
  if (!already_exists)
    NEEDS_WRITE_LOCK_object_files().push_back(this);

  DWARF_RELEASE_WRITE_LOCK;
  return already_exists;
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
}

objfile_ct* NEEDS_READ_LOCK_find_object_file(void const* addr)
{
  object_files_ct::const_iterator i(NEEDS_READ_LOCK_object_files().begin());
  for(; i != NEEDS_READ_LOCK_object_files().end(); ++i)
    if ((*i)->get_start() < addr && addr < (*i)->get_end())
      break;
  return (i != NEEDS_READ_LOCK_object_files().end()) ? (*i) : NULL;
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

  if (!WST_initialized)	// `WST_initialized' is only false when we are still Single Threaded.
  {
    LIBCWD_TSD_DECLARATION;
    if (!ST_init(LIBCWD_TSD))
      return unknown_function_c;
  }

#if 0
  symbol_ct const* symbol;
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  LIBCWD_DEFER_CANCEL;
  BFD_ACQUIRE_READ_LOCK;
  symbol = pc_symbol(addr, NEEDS_READ_LOCK_find_object_file(addr));
  BFD_RELEASE_READ_LOCK;
  LIBCWD_RESTORE_CANCEL;

  if (!symbol)
    return unknown_function_c;

  return symbol->get_symbol()->name;
#endif
  // FIXME: implement the above.
  return unknown_function_c;
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
      M_object_file = NULL;
      M_func = S_pre_ios_initialization_c;
      M_initialization_delayed = addr;
      return;
    }
#endif
    if (!ST_init(LIBCWD_TSD))	// Initialization of BFD code fails?
    {
      M_object_file = NULL;
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
    M_object_file = NULL;
    M_func = S_pre_libcwd_initialization_c;	// Not really true, but this hardly ever happens in the first place.
    M_initialization_delayed = addr;
    return;
  }
#endif

  objfile_ct* object_file;
  LIBCWD_DEFER_CANCEL;
  DWARF_ACQUIRE_READ_LOCK;
  object_file = NEEDS_READ_LOCK_find_object_file(addr);
  DWARF_RELEASE_READ_LOCK;
#ifdef HAVE__DL_LOADED
  if (!object_file && !statically_linked)
  {
    //FIXME: implement this:
    assert(false);
#if 0
    // Try to load everything again... previous loaded libraries are skipped based on load address.
    int possible_object_files = 0;
    bfile_ct* possible_object_file = NULL;
    for(link_map* l = *dl_loaded_ptr; l; l = l->l_next)
    {
      bool already_loaded = false;
      BFD_ACQUIRE_READ_LOCK;
      for (object_files_ct::const_iterator iter = NEEDS_READ_LOCK_object_files().begin();
           iter != NEEDS_READ_LOCK_object_files().end(); ++iter)
      {
        if (reinterpret_cast<void*>(l->l_addr) == (*iter)->get_lbase())
        {
          // The paths are very likely the same, but I don't want to take any risk.
          struct stat statbuf1;
          struct stat statbuf2;
          int res1 = stat(l->l_name, &statbuf1);
          int res2 = stat((*iter)->get_object_file()->filepath(), &statbuf2);
          if (res1 == 0 && res2 == 0 && statbuf1.st_ino == statbuf2.st_ino)
          {
#if CWDEBUG_DEBUG
            Dout(dc::debug, '"' << l->l_name << "\" (" <<
                (*iter)->get_object_file()->filepath() << " --> " << (*iter)->get_bfd()->filename_str.c_str() <<
                ") is already loaded at address " << (*iter)->get_lbase() <<
                ", range: " << (*iter)->get_start() << " - " <<
                (void*)((char*)((*iter)->get_start()) + (*iter)->size()));
#endif
            already_loaded = true;
            if ((*iter)->get_lbase() <= addr && (char*)(*iter)->get_start() + (*iter)->size() > addr)
            {
              ++possible_object_files;
              possible_object_file = *iter;
            }
            break;
          }
        }
      }
      BFD_RELEASE_READ_LOCK;
      if (!already_loaded && l->l_name && ((l->l_name[0] == '/') || (l->l_name[0] == '.')))
        load_object_file(l->l_name, reinterpret_cast<void*>(l->l_addr));
    }
    BFD_ACQUIRE_WRITE_LOCK;
    set_alloc_checking_off(LIBCWD_TSD);
    NEEDS_WRITE_LOCK_object_files().sort(object_file_greater());
    set_alloc_checking_on(LIBCWD_TSD);
    BFD_ACQUIRE_WRITE2READ_LOCK;
    object_file = NEEDS_READ_LOCK_find_object_file(addr);
    BFD_RELEASE_READ_LOCK;
    // This can happen when address is in a function that is not visible
    // as function from the ELF sections, and is outside the detected
    // range determined by functions that are. It means that there
    // are not debug symbols for this object file, otherwise it can't
    // happen. Anyway, it's better to make this guess than to print
    // <unknown object file>.
    if (!object_file && possible_object_files == 1)
      object_file = possible_object_file;
#endif
  }
#endif // HAVE__DL_LOADED
  LIBCWD_RESTORE_CANCEL;

  M_initialization_delayed = NULL;
  if (!object_file)
  {
    LIBCWD_Dout(dc::bfd, "No object file for address " << addr);
    M_object_file = NULL;
    M_func = unknown_function_c;
    M_unknown_pc = addr;
    return;
  }
  M_object_file = object_file->get_object_file();

  //FIXME: implement this
  assert(false);
#if 0
  symbol_ct const* symbol = pc_symbol(addr, object_file);
  if (symbol)
  {
    elfxx::asymbol_st const* p = symbol->get_symbol();
    elfxx::bfd_st* abfd = p->bfd_ptr;
    elfxx::asection_st const* sect = p->section;
    char const* file;
    LIBCWD_ASSERT( object_file->get_bfd() == abfd );
    set_alloc_checking_off(LIBCWD_TSD);
    abfd->find_nearest_line(p, (char*)addr - (char*)object_file->get_lbase(), &file, &M_func, &M_line LIBCWD_COMMA_TSD);
    set_alloc_checking_on(LIBCWD_TSD);
    LIBCWD_ASSERT( !(M_func && !p->name) );	// Please inform the author of libcwd if this assertion fails.
    M_func = p->name;

    if (file && M_line)			// When line is 0, it turns out that `file' is nonsense.
    {
      size_t len = strlen(file);
      set_alloc_checking_off(LIBCWD_TSD);
      M_filepath = lockable_auto_ptr<char, true>(new char [len + 1]);	// LEAK5
      set_alloc_checking_on(LIBCWD_TSD);
      strcpy(M_filepath.get(), file);
      M_known = true;
      M_filename = strrchr(M_filepath.get(), '/') + 1;
      if (M_filename == (char const*)1)
        M_filename = M_filepath.get();
    }

    // Sanity check
    if (!p->name || M_line == 0)
    {
      if (p->name)
      {
        static int const BSF_WARNING_PRINTED = 0x40000000;
        if (!M_object_file->has_no_debug_line_sections() && !(p->flags & BSF_WARNING_PRINTED))
        {
          const_cast<elfxx::asymbol_st*>(p)->flags |= BSF_WARNING_PRINTED;
          set_alloc_checking_off(LIBCWD_TSD);
          {
            _private_::internal_string demangled_name;		// Alloc checking must be turned off already for this string.
            _private_::demangle_symbol(p->name, demangled_name);
            _private_::internal_string::size_type ofn = abfd->filename_str.find_last_of('/');
            _private_::internal_string object_file_name = abfd->filename_str.substr(ofn + 1); // substr can alloc memory
            set_alloc_checking_on(LIBCWD_TSD);
            LIBCWD_Dout(dc::bfd, "Warning: Address " << addr << " in section " << sect->name <<
                " of object file \"" << object_file_name << '"');
            LIBCWD_Dout(dc::bfd|blank_label_cf|blank_marker_cf, "does not have a line number, perhaps the unit containing the function");
#ifdef __FreeBSD__
            LIBCWD_Dout(dc::bfd|blank_label_cf|blank_marker_cf, '`' << demangled_name << "' wasn't compiled with flag -ggdb?");
#else
            LIBCWD_Dout(dc::bfd|blank_label_cf|blank_marker_cf, '`' << demangled_name << "' wasn't compiled with flag -g?");
#endif
            set_alloc_checking_off(LIBCWD_TSD);
          }
          set_alloc_checking_on(LIBCWD_TSD);
        }
      }
      else
        LIBCWD_Dout(dc::bfd, "Warning: Address in section " << sect->name <<
            " of object file \"" << abfd->filename_str << "\" does not contain a function.");
    }
    else
      LIBCWD_Dout(dc::bfd, "address " << addr << " corresponds to " << *static_cast<bfd_location_ct*>(this));
    return;
  }

  if (symbol)
  {
    Debug( dc::bfd.off() );
    size_t len = strlen(symbol->get_symbol()->name);
    len += sizeof("<undefined symbol: >\0");
    char* func = new char [len];		// This leaks memory(!), but I don't think we ever come here.
    strcpy(func, "<undefined symbol: ");
    strcpy(func + 19, symbol->get_symbol()->name);
    strcat(func, ">");
    M_func = func;
    Debug( dc::bfd.on() );
  }
  else
    M_func = unknown_function_c;
#endif
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
  M_object_file = NULL;
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
    if (handle == NULL)
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
      // Don't call dwarf::load_object_file when dlopen() was called with NULL as argument.
      if (name && *name)
      {
	object_file = dwarf::load_object_file(name, dwarf::unknown_l_addr, dwarf::unknown_l_addr);
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
