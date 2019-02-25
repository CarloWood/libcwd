// $Header$
//
// Copyright (C) 2000 - 2007, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
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
#include <new>
#include <set>
#include <list>
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
#if defined(HAVE__DL_LOADED) || defined(HAVE__RTLD_GLOBAL)
#include <link.h>
static link_map** dl_loaded_ptr;
#ifndef HAVE__DL_LOADED
#define HAVE__DL_LOADED 1
#endif
#ifdef HAVE__RTLD_GLOBAL
struct rtld_global {
  link_map* _dl_loaded;
};
#endif
#endif // HAVE__DL_LOADED || HAVE__RTLD_GLOBAL
#include <cstdio>		// Needed for vsnprintf.
#include <algorithm>
#ifdef LIBCWD_DEBUGBFD
#include <iomanip>
#endif
#include "cwd_debug.h"
#include "ios_base_Init.h"
#include "exec_prog.h"
#include "match.h"
#include "elfxx.h"
#include "cwd_bfd.h"
#include <libcwd/class_object_file.h>
#include <libcwd/private_string.h>
#include <libcwd/core_dump.h>

extern char** environ;

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

    // New debug channel
    namespace channels {
      namespace dc {
	/** \addtogroup group_default_dc */
	/* \{ */

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

    // Local stuff
    namespace cwbfd {

#ifdef __PIC__
static bool const statically_linked = false;
#else
static bool const statically_linked = true;
#endif

      // cwbfd::
      void error_handler(char const* format, ...)
      {
	va_list vl;
	va_start(vl, format);
	int const buf_size = 256;
	char buf[buf_size];
	int len = vsnprintf(buf, sizeof(buf), format, vl);	// Needs glibc 2.1 (following the C99 standard).
	va_end(vl);
	if (len >= buf_size)
	{
	  LIBCWD_TSD_DECLARATION;
	  set_alloc_checking_off(LIBCWD_TSD);
	  char* bufp = new char[len + 1];
	  set_alloc_checking_on(LIBCWD_TSD);
	  vsnprintf(bufp, sizeof(buf), format, vl);
	  Dout(dc::bfd, buf);
	  set_alloc_checking_off(LIBCWD_TSD);
	  delete [] bufp;
	  set_alloc_checking_on(LIBCWD_TSD);
	}
	else
	  Dout(dc::bfd, buf);
      }

      //! \cond HIDE_FROM_DOXYGEN
      // cwbfd::
      bool symbol_key_greater::operator()(symbol_ct const& a, symbol_ct const& b) const
      {
	return symbol_start_addr(a.symbol) >= reinterpret_cast<char const*>(symbol_start_addr(b.symbol)) + symbol_size(b.symbol);
      }
      //! \endcond HIDE_FROM_DOXYGEN

      // cwbfd::
      char bfile_ct::ST_list_instance[sizeof(object_files_ct)] __attribute__((__aligned__));

      // cwbfd::
      bfile_ct* NEEDS_READ_LOCK_find_object_file(void const* addr)
      {
	object_files_ct::const_iterator i(NEEDS_READ_LOCK_object_files().begin());
	for(; i != NEEDS_READ_LOCK_object_files().end(); ++i)
	  if ((*i)->get_start() < addr && (char*)(*i)->get_start() + (*i)->size() > addr)
	    break;
	return (i != NEEDS_READ_LOCK_object_files().end()) ? (*i) : NULL;
      }

      // cwbfd::
      bfile_ct* NEEDS_READ_LOCK_find_object_file(elfxx::bfd_st const* abfd)
      {
	object_files_ct::const_iterator i(NEEDS_READ_LOCK_object_files().begin());
	for(; i != NEEDS_READ_LOCK_object_files().end(); ++i)
	  if ((*i)->get_bfd() == abfd)
	    break;
	return (i != NEEDS_READ_LOCK_object_files().end()) ? (*i) : NULL;
      }

      // cwbfd::
      struct symbol_less {
	bool operator()(elfxx::asymbol_st const* a, elfxx::asymbol_st const* b) const;
      };

      bool symbol_less::operator()(elfxx::asymbol_st const* a, elfxx::asymbol_st const* b) const
      {
	if (a == b)
	  return false;
	if (a->section->vma + a->value < b->section->vma + b->value)
	  return true;
	else if (a->section->vma + a->value > b->section->vma + b->value)
	  return false;
	else if (!(a->flags & BSF_FUNCTION) && (b->flags & BSF_FUNCTION))
	  return true;
	else if ((a->flags & BSF_FUNCTION) && !(b->flags & BSF_FUNCTION))
	  return false;
	else if (*a->name == '.')
	  return true;
	else if (*b->name == '.')
	  return false;
	else if (!strcmp(a->name, "gcc2_compiled."))
	  return true;
	else if (!strcmp(b->name, "gcc2_compiled."))
	  return false;
	else if (!strcmp(a->name, "force_to_data"))
	  return true;
	else if (!strcmp(b->name, "force_to_data"))
	  return false;
	else if (!(a->flags & BSF_GLOBAL) && (b->flags & BSF_GLOBAL))
	  return true;
	else if ((a->flags & BSF_GLOBAL) && !(b->flags & BSF_GLOBAL))
	  return false;
	else if (!(a->flags & BSF_LOCAL) && (b->flags & BSF_LOCAL))
	  return true;
	else if ((a->flags & BSF_LOCAL) && !(b->flags & BSF_LOCAL))
	  return false;
	else if (!(a->flags & BSF_OBJECT) && (b->flags & BSF_OBJECT))
	  return true;
	else if ((a->flags & BSF_OBJECT) && !(b->flags & BSF_OBJECT))
	  return false;
	// Lets hope that IF it matters, that a long name is more important ;)
	return (strlen(a->name) < strlen(b->name));
      }

      // cwbfd::
      void* const unknown_l_addr = (void*)-1;
      void* const executable_l_addr = (void*)-2;

      //! \cond HIDE_FROM_DOXYGEN
      // cwbfd::
      bfile_ct::bfile_ct(char const* filename, void* base) :
          M_abfd(NULL), M_lbase(base), M_start_last_symbol(0), M_object_file(filename)
      {
#if CWDEBUG_DEBUGM
	LIBCWD_TSD_DECLARATION;
	LIBCWD_ASSERT( __libcwd_tsd.internal );
#if CWDEBUG_DEBUGT
        LIBCWD_ASSERT( _private_::is_locked(object_files_instance) );
#endif
#endif
      }

#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
      bool bfile_ct::initialize(char const* filename LIBCWD_COMMA_ALLOC_OPT(bool is_libc), bool is_libstdcpp LIBCWD_COMMA_TSD_PARAM)
#else
      bool bfile_ct::initialize(char const* filename LIBCWD_COMMA_ALLOC_OPT(bool is_libc) LIBCWD_COMMA_TSD_PARAM)
#endif
      {
#if CWDEBUG_DEBUGM
	LIBCWD_ASSERT( __libcwd_tsd.internal == 1 );
#if CWDEBUG_DEBUGT
	LIBCWD_ASSERT( _private_::locked_by[object_files_instance] != __libcwd_tsd.tid );
	LIBCWD_ASSERT( __libcwd_tsd.rdlocked_by1[object_files_instance] != __libcwd_tsd.tid && __libcwd_tsd.rdlocked_by2[object_files_instance] != __libcwd_tsd.tid );
#endif
#elif !CWDEBUG_ALLOC
	(void)__libcwd_tsd;	// Suppress unused warning.
#endif
	LIBCWD_ASSERT(M_abfd == NULL);

	long storage_needed = 0;
        _private_::internal_string dbgfilename_str;
        _private_::internal_string realpath_str;
	char const* dbgfilename;

	for (int dbg = 0; dbg < 4; ++dbg)
	{
	  // This loop has dbgfilename_str run over the the values:
	  // 0) /path/libfoo.so.3
	  // 1) /path/.debug/libfoo.so.3
	  // 2) /usr/lib/debug/path/libfoo.so.3
	  // 3) /usr/lib/debug/libfoo.so.3
	  // where /path/libfoo.so.3 is the file that filename
	  // points to (if it's a symlink).

	  if (dbg == 0)
	  {
	    dbgfilename_str = filename;
	    struct stat buf;
	    for (;;)		// Resolve symbolic links.
	    {
#if CWDEBUG_ALLOC
	      __libcwd_tsd.internal = 0;
#endif
	      int res = lstat(dbgfilename_str.c_str(), &buf);
#if CWDEBUG_ALLOC
	      __libcwd_tsd.internal = 1;
#endif
              if (res == -1)
	      {
	        // Whatever, just use 'filename' and fail below.
		dbgfilename_str = filename;
		break;		// 'Done' resolving symlinks.
	      }
	      else if (!S_ISLNK(buf.st_mode))
		break;		// Done resolving symlinks.
	      else
	      {
	        ssize_t size = 0;
		ssize_t len;
		char* linkname = NULL;
		do
		{
		  if (linkname)
		    delete [] linkname;
		  size += 1024;
		  linkname = new char[size];
#if CWDEBUG_ALLOC
		  __libcwd_tsd.internal = 0;
#endif
		  len = readlink(dbgfilename_str.c_str(), linkname, size);
#if CWDEBUG_ALLOC
		  __libcwd_tsd.internal = 1;
#endif
                }
		while (len == size);
		if (size == 0)
		{
		  // A link pointing to nothing? Bail out and fail below.
		  dbgfilename_str = filename;
		  delete [] linkname;
		  break;	// 'Done' resolving symlinks.
		}
		else if (*linkname == '/')
		  dbgfilename_str.assign(linkname, len);
		else
		{
		  // Relative linkname. Append it to the directory component of the path.
		  _private_::internal_string::size_type slash = dbgfilename_str.find_last_of('/');
		  _private_::internal_string dir;
		  if (slash != _private_::internal_string::npos)
		    dir = dbgfilename_str.substr(0, slash + 1);
		  dbgfilename_str = dir;
		  dbgfilename_str.append(linkname, len);
		}
		delete [] linkname;
	      }
	    }		// Next symbolic link.
	    // Store the resolved real path, we need it for dbg == 1, dbg == 2 and dbg == 3.
	    realpath_str = dbgfilename_str;
	  }
	  else if (dbg == 1)
	  {
	    dbgfilename_str = realpath_str;
	    dbgfilename_str.insert(realpath_str.find_last_of('/') + 1, ".debug/");
	  }
	  else if (dbg == 2)
	  {
	    if (realpath_str[0] != '/')
	      continue;
	    dbgfilename_str = "/usr/lib/debug" + realpath_str;
	  }
	  else if (dbg == 3)
	    dbgfilename_str = "/usr/lib/debug/" + realpath_str.substr(realpath_str.find_last_of('/') + 1);

	  dbgfilename = dbgfilename_str.c_str();

	  if (dbg != 0)
	  {
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 0;
#endif
	    struct stat buf;
            int res = stat(dbgfilename, &buf);
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 1;
#endif
	    if (res == -1)
	      continue;		// This debug file doesn't exist, skip it.
	  }

	  // Store the previous values - if this try doesn't work out, we want to use the old values.
	  elfxx::bfd_st* old_M_abfd = M_abfd;
	  long old_storage_needed = storage_needed;

	  M_abfd = elfxx::bfd_st::openr(dbgfilename);

	  if (M_abfd->check_format())
	  {
	    M_abfd->close();
	    M_abfd = old_M_abfd;
	    if (!M_abfd)
	      DoutFatal(dc::fatal, dbgfilename << ": can not get addresses from archive.");
	    continue;		// Try next dbg value, and path.
	  }
	  if (!M_abfd->has_syms())
	  {
	    Dout(dc::warning, dbgfilename << " has no symbols, skipping.");
	    M_abfd->close();
	    M_abfd = old_M_abfd;
	    continue;		// Try next dbg value, and path.
	  }
	  storage_needed = M_abfd->get_symtab_upper_bound();
	  if (storage_needed == 0)
	  {
	    Dout(dc::warning, dbgfilename << " has no symbols, skipping.");
	    M_abfd->close();
	    M_abfd = old_M_abfd;
	    storage_needed = old_storage_needed;
	    continue;		// Try next dbg value, and path.
	  }
	  // If this file was stripped, keep the values - but continue looking.
	  if (M_abfd->is_stripped())
	  {
	    if (old_M_abfd)
	      old_M_abfd->close();	// Clean up old_M_abfd because we will overwrite it.
	    continue;		// Try next dbg value, and path.
	  }
	  // Found a suitable M_abfd with ELF headers and debug info.
	  break;
	}
	if (!M_abfd)		// Didn't find any suitable file to load?
	{
	  M_number_of_symbols = 0;
	  return false;		// Fail.
        }

	M_abfd->M_s_end_offset = 0;
	M_abfd->object_file = this;

	M_symbol_table = (elfxx::asymbol_st**) malloc(storage_needed);	// LEAK12
	M_number_of_symbols = M_abfd->canonicalize_symtab(M_symbol_table);

	if (M_number_of_symbols > 0)
	{
#if CWDEBUG_ALLOC
#if LIBCWD_THREAD_SAFE && __GNUC__ == 3 && __GNUC_MINOR__ == 4
	  elfxx::Elfxx_Off S_lock_value = 0;
#endif
	  elfxx::Elfxx_Off exit_funcs = 0;
#endif
	  size_t s_end_offset = M_abfd->M_s_end_offset;
	  if (!s_end_offset && M_number_of_symbols > 0)
	  {
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 0;
#endif
	    Dout(dc::warning, "Cannot find symbol _end");
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 1;
#endif
	  }

	  // Guess the start of the shared object when ldd didn't return it.
	  if (M_lbase == unknown_l_addr)
	  {
#ifdef HAVE_DLOPEN
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 0;
#endif
	    if (!real_dlopen.symptr)
	      real_dlopen.symptr = dlsym(RTLD_NEXT, "dlopen");
	    void* handle = real_dlopen.func(NULL, RTLD_LAZY);		// filename is already loaded.
	    if (!handle)
	    {
#if LIBCWD_THREAD_SAFE && CWDEBUG_DEBUG
	      LIBCWD_ASSERT( _private_::is_locked(object_files_instance) );
#endif
	      char const* dlerror_str;
	      dlerror_str = dlerror();
	      DoutFatal(dc::fatal, "::dlopen(NULL, RTLD_LAZY): " << dlerror_str);
	    }
#ifdef HAVE__DL_LOADED
	    if (!statically_linked)
            {
	      for(link_map* p = *dl_loaded_ptr; p; p = p->l_next)
	        if (!strcmp(p->l_name, filename))
		{
		  M_lbase = reinterpret_cast<void*>(p->l_addr);		// No idea why they use unsigned int for a load address.
		  break;
		}
	    }
#endif // HAVE__DL_LOADED
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 1;
#endif
	    if (M_lbase == unknown_l_addr)
	    {
#if CWDEBUG_ALLOC
	      __libcwd_tsd.internal = 1;
	      typedef std::map<void*, unsigned int, std::less<void*>,
	          _private_::internal_allocator::rebind<std::pair<void* const, unsigned int> >::other> start_values_map_ct;
#else
	      typedef std::map<void*, unsigned int, std::less<void*> > start_values_map_ct;
#endif
	      // The following code uses a heuristic approach to guess the start of an object file.
	      start_values_map_ct start_values;
	      unsigned int best_count = 0;
	      void* best_start = 0;
	      for (elfxx::asymbol_st** s = M_symbol_table; s <= &M_symbol_table[M_number_of_symbols - 1]; ++s)
	      {
		if ((*s)->name == 0 || ((*s)->flags & BSF_FUNCTION) == 0 || ((*s)->flags & (BSF_GLOBAL|BSF_WEAK)) == 0)
		  continue;
		elfxx::asection_st const* sect = (*s)->section;
		if (sect->name[1] == 't' && !strcmp(sect->name, ".text"))
		{
#if CWDEBUG_ALLOC
		  __libcwd_tsd.internal = 0;
#endif
		  void* val = dlsym(handle, (*s)->name);
		  if (dlerror() == NULL)
		  {
#if CWDEBUG_ALLOC
		    __libcwd_tsd.internal = 1;
#endif
		    void* start = reinterpret_cast<char*>(val) - (*s)->value - sect->vma;
		    // Shared object seem to be loaded at boundaries of 4096 bytes (a page size).
		    // It is possible that we looked up a symbol of the library we are looking
		    // for and got a symbol in a different library with the same name. The chance
		    // that that results in a load address that is a multiple is very small; make
		    // use of that fact and skip random looking start addresses.
		    if (((size_t)start & 0xfff) == 0)
		    {
		      std::pair<start_values_map_ct::iterator, bool> p =
			  start_values.insert(std::pair<void* const, unsigned int>(start, 0));
		      if (++(*(p.first)).second > best_count)
		      {
			best_start = start;
			if (++best_count == 10)	// Its unlikely that even more than 2 wrong values will have the same value.
			  break;			// So if we reach 10 then this is value we are looking for.
		      }
		    }
		  }
#if CWDEBUG_ALLOC
		  else
		    __libcwd_tsd.internal = 1;
#endif
		}
	      }
	      if (best_count < 3)
	      {
#if CWDEBUG_ALLOC
		__libcwd_tsd.internal = 0;
#endif
		Dout(dc::warning, "Unable to determine start of \"" << M_abfd->filename_str << "\", skipping.");
#if CWDEBUG_ALLOC
		__libcwd_tsd.internal = 1;
#endif
		free(M_symbol_table);
		M_symbol_table  = NULL;
		M_abfd->close();
		M_abfd = NULL;
		M_number_of_symbols = 0;
#if CWDEBUG_ALLOC
		__libcwd_tsd.internal = 0;
#endif
		if (!real_dlclose.symptr)
		  real_dlclose.symptr = dlsym(RTLD_NEXT, "dlclose");
		real_dlclose.func(handle);
#if CWDEBUG_ALLOC
		__libcwd_tsd.internal = 1;
#endif
		return false;
	      }
	      M_lbase = best_start;
	    }
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 0;
#endif
	    if (!real_dlclose.symptr)
	      real_dlclose.symptr = dlsym(RTLD_NEXT, "dlclose");
	    real_dlclose.func(handle);
	    Dout(dc::continued, '(' << M_lbase << ") ... ");
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 1;
#endif
#else // !HAVE_DLOPEN
	    DoutFatal(dc::fatal, "Can't determine start of shared library: you will need libdl to be detected by configure.");
#endif
	  }
	  else if (M_lbase == executable_l_addr)
	    M_lbase = 0;

	  void const* s_end_start_addr;

	  if (s_end_offset)
	    s_end_start_addr = (char*)M_lbase + s_end_offset;
	  else
	    s_end_start_addr = NULL;

	  // Sort the symbol table in order of start address.
	  std::sort(M_symbol_table, &M_symbol_table[M_number_of_symbols], symbol_less());

	  // Calculate sizes for every symbol
	  for (int i = 0; i < M_number_of_symbols - 1; ++i)
	    symbol_size(M_symbol_table[i]) =
	        (char*)symbol_start_addr(M_symbol_table[i + 1]) - (char*)symbol_start_addr(M_symbol_table[i]);

	  // Use reasonable size for last one.
	  // This should be "_end", or one beyond it, and will be thrown away in the next loop.
	  symbol_size(M_symbol_table[M_number_of_symbols - 1]) = 100001;

          BFD_ACQUIRE_WRITE_LOCK;	// Needed for M_function_symbols.

	  // Throw away all symbols that are not a global variable or function, store the rest in a vector.
	  elfxx::asymbol_st** se2 = &M_symbol_table[M_number_of_symbols - 1];
	  for (elfxx::asymbol_st** s = M_symbol_table; s <= se2;)
	  {
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
	    if (is_libstdcpp && strcmp((*s)->name,
#if __GNUC_PATCHLEVEL__ == 0
	    "_ZN9__gnu_cxx12__pool_allocILb1ELi0EE7_S_lockE"
#elif __GNUC_PATCHLEVEL__ == 1
	    "_ZN9__gnu_cxx11__pool_baseILb1EE7_S_lockE"
#else
	    "_ZN14__gnu_internal17palloc_init_mutexE"
#endif
	    ) == 0)
	      S_lock_value = (*s)->section->vma + (*s)->value;
#endif // LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
#if CWDEBUG_ALLOC
	    if (is_libc && strcmp((*s)->name, "__exit_funcs") == 0)
	      exit_funcs = (*s)->section->vma + (*s)->value;
#endif
	    if (((*s)->name[0] == '.' && (*s)->name[1] == 'L')
	        || ((*s)->flags & (BSF_SECTION_SYM|BSF_OLD_COMMON|BSF_NOT_AT_END|BSF_INDIRECT|BSF_DYNAMIC)) != 0
		|| (s_end_start_addr != NULL && symbol_start_addr(*s) >= s_end_start_addr)
		|| ((*s)->flags & BSF_FUNCTION) == 0)	// Only keep functions.
	    {
	      *s = *se2--;
	      --M_number_of_symbols;
	    }
	    else
	    {
	      M_function_symbols.insert(function_symbols_ct::key_type(*s));
	      ++s;
	    }
	  }

	  if (M_function_symbols.size() > 0)
	  {
	    // M_function_symbols is a set<> that sorts in reverse order (using symbol_key_greater)!
	    // So use begin() here in order to get the last symbol.
	    elfxx::asymbol_st const* last_symbol = (*M_function_symbols.begin()).get_symbol();
	    if (symbol_size(last_symbol) == 100001)
	    {
	      BFD_RELEASE_WRITE_LOCK;
#if CWDEBUG_ALLOC
	      __libcwd_tsd.internal = 0;
#endif
	      Dout(dc::warning, "Unknown size of symbol " << last_symbol->name);
#if CWDEBUG_ALLOC
	      __libcwd_tsd.internal = 1;
#endif
	      BFD_ACQUIRE_WRITE_LOCK;
	    }
	    elfxx::asymbol_st const* first_symbol = (*M_function_symbols.rbegin()).get_symbol();
	    M_start = symbol_start_addr(first_symbol); // Use the lowest start address of all symbols.
	    if (s_end_start_addr)
	      M_size = (char*)s_end_start_addr - (char*)M_start;
	    else
	    {
	      if (symbol_size(last_symbol) == 100001)			// In fact unknown?
	      {
		M_start_last_symbol = symbol_start_addr(last_symbol);	// Initialize M_start_last_symbol.
		M_size = (char*)last_symbol->section->vma + last_symbol->section->M_size - (char*)M_start;
		symbol_size(const_cast<elfxx::asymbol_st*>(last_symbol)) = M_size - ((char*)symbol_start_addr(last_symbol) - (char*)M_start);
	      }
	      else
		M_size = (char*)symbol_start_addr(last_symbol) + symbol_size(last_symbol) - (char*)M_start;
	    }
	  }
	  else
	  {
	    M_start = M_lbase;			// Initialize to arbitrary value.
	    M_size = 0;
	  }

#if 0
	  if (M_function_symbols.size() < 20000)
	  {
	    libcwd::debug_ct::OnOffState state;
	    Debug( libcw_do.force_on(state) );
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 0;
#endif
	    for(function_symbols_ct::iterator i(M_function_symbols.begin()); i != M_function_symbols.end(); ++i)
	    {
	      elfxx::asymbol_st const* s = i->get_symbol();
	      Dout(dc::always, s->name << " (" << s->section->name << ") = " << s->value);
	    }
#if CWDEBUG_ALLOC
	    __libcwd_tsd.internal = 1;
#endif
	    Debug( libcw_do.restore(state) );
	  }
#endif
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
	  if (is_libstdcpp && S_lock_value)
	    _private_::pool_allocator_lock_symbol_ptr = (char*)M_lbase + S_lock_value;
#endif
#if CWDEBUG_ALLOC
          if (is_libc && exit_funcs)
	    _private_::__exit_funcs_ptr = (_private_::exit_function_list**)((char*)M_lbase + exit_funcs);
#endif
	}

	bool already_exists = false;
	if (M_number_of_symbols > 0)
	{
#if CWDEBUG_ALLOC
	  __libcwd_tsd.internal = 0;
#endif
	  Dout(dc::bfd, "Adding \"" << get_object_file()->filepath() << "\", load address " <<
	      get_lbase() << ", start " << get_start() << " and end " << (void*)((size_t)get_start() + size()));
	  // Find out if there is any overlap.
	  for (object_files_ct::iterator iter = NEEDS_WRITE_LOCK_object_files().begin();
	       iter != NEEDS_WRITE_LOCK_object_files().end(); ++iter)
          {
	    bool shared_libraries_overlap = (size_t)(*iter)->get_start() < (size_t)get_start() + size() &&
	        (size_t)(*iter)->get_start() + (*iter)->size() > (size_t)get_start();
	    if (shared_libraries_overlap)
	    {
              if ((*iter)->get_start() == get_start())
              {
                LIBCWD_ASSERT((*iter)->size() == size());
                Dout(dc::bfd, "Already loaded as \"" << (*iter)->get_object_file()->filepath() << "\"");
                already_exists = true;
                break;
              }
	      Dout(dc::bfd, "It collides with \"" << (*iter)->get_object_file()->filepath() << "\", load address " <<
		  (*iter)->get_lbase() << ", start " << (*iter)->get_start() << " and end " <<
		  (void*)((size_t)(*iter)->get_start() + (*iter)->size()));

	      bfile_ct* lowbf;
	      bfile_ct* highbf;
	      // Real size:
	      // <----lowbf----><----highbf---->
	      // Since only the end can be too large, overlap means we have either:
	      // <----lowbf----------->
	      //                <----highbf---->
	      // or
	      // <----lowbf------------------------>
	      //                <----highbf---->
	      // Hence, it suffices to check the start.
	      if ((size_t)(*iter)->get_start() < (size_t)get_start())
	      {
	        lowbf = *iter;
	        highbf = this;
	      }
	      else
	      {
	        lowbf = this;
	        highbf = *iter;
	      }
	      if (!lowbf->M_start_last_symbol)			// Must be non-zero to be variable.
	      {
	        Dout(dc::warning, "\"" << lowbf->get_object_file()->filepath() << "\" overlaps with \"" <<
		    highbf->get_object_file()->filepath() << "\" but does not have a last symbol. "
		    "This probably means that it's sections are not contiguous. "
		    "Libcwd can't deal with that at the moment. The last sections will be disregarded which might cause "
		    "failures to lookup symbols in this library.");
	      }
	      LIBCWD_ASSERT(lowbf->M_start_last_symbol < highbf->get_start());	// At most last symbol size can be wrong.
	      lowbf->M_size = (char*)highbf->M_start - (char*)lowbf->M_start;
	      Dout(dc::bfd, "End of \"" << lowbf->get_object_file()->filepath() << "\" corrected to be " <<
	          (void*)((size_t)lowbf->get_start() + lowbf->size()));
	      break;
	    }
	  }
#if CWDEBUG_ALLOC
	  __libcwd_tsd.internal = 1;
#endif
          if (!already_exists)
	    NEEDS_WRITE_LOCK_object_files().push_back(this);
        }
	BFD_RELEASE_WRITE_LOCK;
        if (already_exists)
	{
	  free(M_symbol_table);
	  M_symbol_table  = NULL;
	  if (M_abfd)
	  {
	    M_abfd->close();
	    M_abfd = NULL;
	  }
	  M_number_of_symbols = 0;
	  return true;
	}
        return false;
      }

      void bfile_ct::deinitialize(LIBCWD_TSD_PARAM)
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
	BFD_ACQUIRE_WRITE_LOCK;
	set_alloc_checking_off(LIBCWD_TSD);
	M_function_symbols.erase(M_function_symbols.begin(), M_function_symbols.end());
	object_files_ct::iterator iter(find(NEEDS_WRITE_LOCK_object_files().begin(),
	                                    NEEDS_WRITE_LOCK_object_files().end(), this));
	if (iter != NEEDS_WRITE_LOCK_object_files().end())
	  NEEDS_WRITE_LOCK_object_files().erase(iter);
	set_alloc_checking_on(LIBCWD_TSD);
	BFD_RELEASE_WRITE_LOCK;
	LIBCWD_RESTORE_CANCEL;
	set_alloc_checking_off(LIBCWD_TSD);
	if (M_abfd)
	{
	  M_abfd->close();
	  M_abfd = NULL;
	}
	if (M_symbol_table)
	{
	  free(M_symbol_table);
	  M_symbol_table  = NULL;
	}
	set_alloc_checking_on(LIBCWD_TSD);
      }
      //! \endcond HIDE_FROM_DOXYGEN

      // cwbfd::
      struct object_file_greater {
	bool operator()(bfile_ct const* a, bfile_ct const* b) const { return a->get_lbase() > b->get_lbase(); }
      };

      // cwbfd::
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
#endif

	return false;
      }

      // cwbfd::
      _private_::string* ST_argv0_ptr;			// MT: Set in `ST_get_full_path_to_executable', used in `ST_decode_ps'.
      // cwbfd::
      _private_::string const* ST_pidstr_ptr;		// MT: Set in `ST_get_full_path_to_executable', used in `ST_decode_ps'.

      // cwbfd::
      int ST_decode_ps(char const* buf, size_t len)	// MT: Single Threaded function.
      {
#if CWDEBUG_DEBUGM
	LIBCWD_TSD_DECLARATION;
	LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
	static int pid_token = 0;
	static int command_token = 0;
	static size_t command_column;
	int current_token = 0;
	bool found_PID = false;
	bool eating_token = false;
	size_t current_column = 1;
	_private_::string token;

	for (char const* p = buf; p < &buf[len]; ++p, ++current_column)
	{
	  if (!eating_token)
	  {
	    if (*p != ' ' && *p != '\t' && *p != '\n')
	    {
	      ++current_token;
	      token = *p;
	      eating_token = true;
	    }
	    if (*p == '\n')
	    {
	      current_token = 0;
	      current_column = 0;
	    }
	  }
	  else
	  {
	    if (*p == ' ' || *p == '\t' || *p == '\n')
	    {
	      if (pid_token == current_token && token == *ST_pidstr_ptr)
		found_PID = true;
	      else if (found_PID && (command_token == current_token || current_column >= command_column))
	      {
		*ST_argv0_ptr = token + '\0';
		return 0;
	      }
	      else if (pid_token == 0 && token == "PID")
		pid_token = current_token;
	      else if ((command_token == 0 && (token == "COMMAND")) || (token == "CMD"))
	      {
		command_token = current_token;
		command_column = current_column;
	      }
	      if (*p == '\n')
	      {
		current_token = 0;
		current_column = 0;
	      }
	      eating_token = false;
	    }
	    token += *p;
	  }
	}
	return 0;
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
      // cwbfd::
      void ST_get_full_path_to_executable(_private_::internal_string& result LIBCWD_COMMA_TSD_PARAM)
      {
	_private_::string argv0;		// Like main()s argv[0], thus must be zero terminated.
	char buf[6];
	char* p = &buf[5];
	*p = 0;
	int pid = getpid();
	do { *--p = '0' + (pid % 10); } while ((pid /= 10) > 0);
	size_t const max_proc_path = sizeof("/proc/65535/cmdline\0");
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
	{
	  _private_::string pidstr;

	  size_t const max_pidstr = sizeof("65535\0");
	  char pidstr_buf[max_pidstr];
	  char* p = &pidstr_buf[max_pidstr - 1];
	  *p = 0;
	  int pid = getpid();
	  do { *--p = '0' + (pid % 10); } while ((pid /= 10) > 0);
	  pidstr = p;

	  // Path to `ps'
	  char const ps_prog[] = CW_PATH_PROG_PS;

	  char const* argv[4];
	  argv[0] = "ps";
	  argv[1] = PS_ARGUMENT;
	  argv[2] = p;
	  argv[3] = NULL;

	  ST_argv0_ptr = &argv0;	// Ugly way to pass these strings to ST_decode_ps:
	  ST_pidstr_ptr = &pidstr;	// pidstr is input, argv0 is output.

	  if (ST_exec_prog(ps_prog, argv, environ, ST_decode_ps) == -1 || argv0.empty())
	    DoutFatal(dc::fatal|error_cf, "Failed to execute \"" << ps_prog << "\"");
	}

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

      // cwbfd::
      static bool WST_initialized = false;			// MT: Set here to false, set to `true' once in `cwbfd::ST_init'.

      // cwbfd::
      struct my_link_map {
	void* l_addr;
	char l_name[MAXPATHLEN];
	my_link_map(char const* start, size_t len, void* addr) : l_addr(addr)
	{
	  if (len >= MAXPATHLEN)
	    len = MAXPATHLEN - 1;
	  strncpy(l_name, start, len);
	  l_name[len] = 0;
	}
      };

      // cwbfd::
      // Not really necessary to use the `object_files' allocator here, but it
      // is used inside that critical area anyway already, so why not.
#if CWDEBUG_ALLOC
      typedef std::vector<my_link_map, _private_::object_files_allocator::rebind<my_link_map>::other> ST_shared_libs_vector_ct;
#else
      typedef std::vector<my_link_map> ST_shared_libs_vector_ct;
#endif
      typedef char fake_shared_libs_type[sizeof(ST_shared_libs_vector_ct)];
      fake_shared_libs_type fake_ST_shared_libs;		// Written to only in `ST_decode_ldd' which is called from
								// `cwbfd::ST_init' and read from in a later part of
								// `cwbfd::ST_init'.  Initialization is done in ST_init too.
      ST_shared_libs_vector_ct* const ST_shared_libs_ptr = reinterpret_cast<ST_shared_libs_vector_ct*>(&fake_ST_shared_libs);
      ST_shared_libs_vector_ct& ST_shared_libs = *ST_shared_libs_ptr;

      // cwbfd::
      int ST_decode_ldd(char const* buf, size_t len)
      {
	LIBCWD_TSD_DECLARATION;
	for (char const* p = buf; p < &buf[len]; ++p)
	  if ((p[0] == '=' && p[1] == '>' && p[2] == ' ') || p[2] == '\t')
	  {
	    p += 2;
	    while (*p == ' ' || *p == '\t')
	      ++p;
	    if (*p != '/' && *p != '.')
	      break;
	    char const* q;
	    for (q = p; q < &buf[len] && *q > ' '; ++q) ;
	    if (*q == '\n')	// This ldd doesn't return an offset (ie, on solaris).
	    {
	      set_alloc_checking_off(LIBCWD_TSD);
	      ST_shared_libs.push_back(my_link_map(p, q - p, unknown_l_addr));
	      set_alloc_checking_on(LIBCWD_TSD);
	      break;
	    }
	    for (char const* r = q; r < &buf[len]; ++r)
	      if (r[0] == '(' && r[1] == '0' && r[2] == 'x')
	      {
		char* s;
		void* addr = reinterpret_cast<void*>(strtol(++r, &s, 0));
		set_alloc_checking_off(LIBCWD_TSD);
		ST_shared_libs.push_back(my_link_map(p, q - p, addr));
		set_alloc_checking_on(LIBCWD_TSD);
		break;
	      }
	    break;
	  }
	return 0;
      }

#ifdef LIBCWD_DEBUGBFD
      // cwbfd::
      void dump_object_file_symbols(bfile_ct const* object_file)
      {
	std::cout << std::setiosflags(std::ios::left) <<
	             std::setw(15) << "Start address" <<
	             std::setw(50) << "File name" <<
		     std::setw(20) << "Number of symbols" <<
		     std::resetiosflags(std::ios::left) << '\n';
	std::cout << "0x" << std::setfill('0') << std::setiosflags(std::ios::right) <<
	             std::setw(8) << std::hex << (unsigned long)object_file->get_lbase() <<
		     std::resetiosflags(std::ios::right) << "     ";
	std::cout << std::setfill(' ') << std::setiosflags(std::ios::left) <<
	             std::setw(50) << object_file->get_bfd()->filename <<
	             std::dec << object_file->get_number_of_symbols() <<
		     std::resetiosflags(std::ios::left) << '\n';
	std::cout << std::setiosflags(std::ios::left) <<
	             std::setw(12) << "Start" <<
		     std::resetiosflags(std::ios::left) <<
	             ' ' << std::setiosflags(std::ios::right) <<
		     std::setw(6) << "Size" << ' ' <<
		     std::resetiosflags(std::ios::right);
	std::cout << "Name value flags\n";
	elfxx::asymbol_st** symbol_table = object_file->get_symbol_table();
	for (long n = object_file->get_number_of_symbols() - 1; n >= 0; --n)
	{
	  std::cout << std::setiosflags(std::ios::left) <<
	               std::setw(12) << (void*)symbol_start_addr(symbol_table[n]) <<
		       std::resetiosflags(std::ios::left);
	  std::cout << ' ' << std::setiosflags(std::ios::right) <<
	               std::setw(6) << symbol_size(symbol_table[n]) << ' ' <<
	               symbol_table[n]->name <<
		       ' ' << (void*)symbol_table[n]->value <<
		       ' ' << std::oct << symbol_table[n]->flags << std::dec <<
		       std::resetiosflags(std::ios::right) << std::endl;
	}
      }
#endif

      // cwbfd::
      bfile_ct* load_object_file(char const* name, void* l_addr, bool initialized = false)
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
	bfile_ct* object_file;
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
	BFD_ACQUIRE_WRITE_LOCK;
	set_alloc_checking_off(LIBCWD_TSD);
	object_file = new bfile_ct(name, l_addr);		// LEAK6
	BFD_RELEASE_WRITE_LOCK;
	already_exists =
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC && __GNUC__ == 3 && __GNUC_MINOR__ == 4
	    object_file->initialize(name LIBCWD_COMMA_ALLOC_OPT(is_libc), is_libstdcpp LIBCWD_COMMA_TSD);
#else
	    object_file->initialize(name LIBCWD_COMMA_ALLOC_OPT(is_libc) LIBCWD_COMMA_TSD);
#endif
	set_alloc_checking_on(LIBCWD_TSD);
	LIBCWD_RESTORE_CANCEL;
	if (!already_exists && object_file->get_number_of_symbols() > 0)
	{
	  Dout(dc::finish, "done (" << std::dec << object_file->get_number_of_symbols() << " symbols)");
#ifdef LIBCWD_DEBUGBFD
	  dump_object_file_symbols(object_file);
#endif
	}
	else
	{
	  if (already_exists)
	    Dout(dc::finish, "Already loaded");
	  else
	  {
	    Dout(dc::finish, "No symbols found");
	    object_file->deinitialize(LIBCWD_TSD);
	  }
	  set_alloc_checking_off(LIBCWD_TSD);
	  delete object_file;
	  set_alloc_checking_on(LIBCWD_TSD);
	  return NULL;
        }
	return object_file;
      }

      // cwbfd::
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

#ifdef HAVE__DL_LOADED
	if (!statically_linked)
	{
	  // Initialize dl_loaded_ptr.
	  if (!real_dlopen.symptr)
	  {
	    real_dlopen.symptr = dlsym(RTLD_NEXT, "dlopen");
	    if (real_dlopen.symptr == NULL)
	    {
	      DoutFatal(dc::core, "libcwd:cwbfd::ST_init: dlsym(RTLD_NEXT, \"dlopen\") returns NULL; "
	          "please check that you didn't specify -ldl before (left of) -lcwd while linking.\n");
	    }
	  }
	  void* handle = real_dlopen.func(NULL, RTLD_LAZY);
#ifdef HAVE__RTLD_GLOBAL
	  rtld_global* rtld_global_ptr = (rtld_global*)::dlsym(handle, "_rtld_global");
	  if (!rtld_global_ptr)
	    DoutFatal(dc::core, "Configuration of libcwd detected _rtld_global, but I can't find it now?!");
          dl_loaded_ptr = &rtld_global_ptr->_dl_loaded;
#else
	  dl_loaded_ptr = (link_map**)::dlsym(handle, "_dl_loaded");
	  if (!dl_loaded_ptr)
	    DoutFatal(dc::core, "Configuration of libcwd detected _dl_loaded, but I can't find it now?!");
#endif
	  if (!real_dlclose.symptr)
	    real_dlclose.symptr = dlsym(RTLD_NEXT, "dlclose");
	  real_dlclose.func(handle);
	}
#endif // HAVE__DL_LOADED

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

        new (fake_ST_shared_libs) ST_shared_libs_vector_ct;

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
	BFD_ACQUIRE_WRITE_LOCK;
	new (&NEEDS_WRITE_LOCK_object_files()) object_files_ct;
	BFD_RELEASE_WRITE_LOCK;
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

	  // Load executable
	  BFD_INITIALIZE_LOCK;
	  load_object_file(fullpath.data(), executable_l_addr, true);

	  if (!statically_linked)
	  {

	    // Load all shared objects
  #ifndef HAVE__DL_LOADED
	    // Path to `ldd'
	    char const ldd_prog[] = "/usr/bin/ldd";

	    char const* argv[3];
	    argv[0] = "ldd";
	    argv[1] = fullpath.data();
	    argv[2] = NULL;
	    ST_exec_prog(ldd_prog, argv, environ, ST_decode_ldd);

	    for(ST_shared_libs_vector_ct::const_iterator iter = ST_shared_libs.begin(); iter != ST_shared_libs.end(); ++iter)
	    {
	      my_link_map const* l = &(*iter);
  #else
	    for(link_map const* l = *dl_loaded_ptr; l; l = l->l_next)
	    {
  #if 0
	    }
  #endif
  #endif
	      if (l->l_name && ((l->l_name[0] == '/') || (l->l_name[0] == '.')))
		load_object_file(l->l_name, reinterpret_cast<void*>(l->l_addr));
	    }

	    LIBCWD_DEFER_CANCEL;
	    BFD_ACQUIRE_WRITE_LOCK;
	    set_alloc_checking_off(LIBCWD_TSD);
	    NEEDS_WRITE_LOCK_object_files().sort(object_file_greater());
	    set_alloc_checking_on(LIBCWD_TSD);
	    BFD_RELEASE_WRITE_LOCK;
	    LIBCWD_RESTORE_CANCEL;
	  }

	  set_alloc_checking_off(LIBCWD_TSD);
	  ST_shared_libs.~ST_shared_libs_vector_ct();
	  set_alloc_checking_on(LIBCWD_TSD);

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

      // cwbfd::
      symbol_ct const* pc_symbol(void const* addr, bfile_ct* object_file)
      {
	if (object_file)
	{
	  elfxx::asymbol_st dummy_symbol;		// A dummy symbol with size 1 and start `addr',
	  elfxx::asection_st dummy_section;

	  // Make symbol_start_addr(&dummy_symbol) and symbol_size(&dummy_symbol) return the correct value:
	  dummy_symbol.bfd_ptr = object_file->get_bfd();
	  dummy_section.vma = 0;			// Use a vma of 0 and
	  dummy_symbol.section = &dummy_section;	// use dummy_symbol.value to store (value + offset):
	  dummy_symbol.value = reinterpret_cast<char const*>(addr) - reinterpret_cast<char const*>(object_file->get_lbase());
	  symbol_size(&dummy_symbol) = 1;
	  function_symbols_ct::iterator i(object_file->get_function_symbols().find(symbol_ct(&dummy_symbol)));
	  if (i != object_file->get_function_symbols().end())
	  {
	    elfxx::asymbol_st const* p = (*i).get_symbol();
	    if (addr < symbol_start_addr(p) + symbol_size(p))
	      return &(*i);
	  }
	  Dout(dc::bfd, "No symbol found: " << (void*)addr);
	}
	else
	  Dout(dc::bfd, "No source file found: " << (void*)addr);
	return NULL;
      }

    } // namespace cwbfd

    char const* const location_ct::S_uninitialized_location_ct_c = "<uninitialized location_ct>";
    char const* const location_ct::S_pre_ios_initialization_c = "<pre ios initialization>";
    char const* const location_ct::S_pre_libcwd_initialization_c = "<pre libcwd initialization>";
    char const* const location_ct::S_cleared_location_ct_c = "<cleared location ct>";

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
      using namespace cwbfd;

      if (!WST_initialized)	// `WST_initialized' is only false when we are still Single Threaded.
      {
	LIBCWD_TSD_DECLARATION;
	if (!ST_init(LIBCWD_TSD))
	  return unknown_function_c;
      }

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
typedef location_ct bfd_location_ct;
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

      using namespace cwbfd;

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

      bfile_ct* object_file;
      LIBCWD_DEFER_CANCEL;
      BFD_ACQUIRE_READ_LOCK;
      object_file = NEEDS_READ_LOCK_find_object_file(addr);
      BFD_RELEASE_READ_LOCK;
#ifdef HAVE__DL_LOADED
      if (!object_file && !statically_linked)
      {
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
      }
#endif
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
  cwbfd::bfile_ct* M_object_file;
  int M_flags;
  int M_refcount;
  dlloaded_st(cwbfd::bfile_ct* object_file, int flags) : M_object_file(object_file), M_flags(flags), M_refcount(1) { }
};

namespace libcwd {
  namespace _private_ {
#if CWDEBUG_ALLOC
typedef std::map<void*, dlloaded_st, std::less<void*>,
    internal_allocator::rebind<std::pair<void* const, dlloaded_st> >::other> dlopen_map_ct;
#else
typedef std::map<void*, dlloaded_st, std::less<void*> > dlopen_map_ct;
#endif
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

  } // namespace _private_

} // namespace libcwd

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
    if (libcwd::cwbfd::statically_linked)
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
      cwbfd::bfile_ct* object_file;
#ifdef HAVE__DL_LOADED
      if (name)
      {
	name = ((link_map*)handle)->l_name;	// This is dirty, but its the only reasonable way to get
						// the full path to the loaded library.
      }
#endif
      // Don't call cwbfd::load_object_file when dlopen() was called with NULL as argument.
      if (name && *name)
      {
	object_file = cwbfd::load_object_file(name, cwbfd::unknown_l_addr);
	if (object_file)
	{
	  LIBCWD_DEFER_CANCEL;
	  BFD_ACQUIRE_WRITE_LOCK;
	  set_alloc_checking_off(LIBCWD_TSD);
	  cwbfd::NEEDS_WRITE_LOCK_object_files().sort(cwbfd::object_file_greater());
	  set_alloc_checking_on(LIBCWD_TSD);
	  BFD_RELEASE_WRITE_LOCK;
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
    if (ret != 0 || libcwd::cwbfd::statically_linked)
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
