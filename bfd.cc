// $Header$
//
// Copyright (C) 2000 - 2002, by
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

#include <libcw/debug_config.h>
#if CWDEBUG_LOCATION

#include "sys.h"
#include <unistd.h>
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
#include <map>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#endif
#ifdef HAVE__DL_LOADED
#include <link.h>
extern link_map* _dl_loaded;
#endif
#include <cstdio>		// Needed for vsnprintf.
#include <algorithm>
#ifdef LIBCWD_DEBUGBFD
#include <iomanip>
#endif
#include "cwd_debug.h"
#include "ios_base_Init.h"
#ifdef LIBCWD_DLOPEN_DEFINED
#undef dlopen
#undef dlclose
#endif
#include "exec_prog.h"
#if CWDEBUG_LIBBFD
#if defined(BFD64) && !BFD_HOST_64BIT_LONG && defined(__GLIBCPP__) && !defined(_GLIBCPP_USE_LONG_LONG)
// libbfd is compiled with 64bit support on a 32bit host, but libstdc++ is not compiled with support
// for `long long'.  If you run into this error (and you insist on use libbfd) then either recompile
// libstdc++ with support for `long long' or recompile libbfd without 64bit support.
#error "Incompatible libbfd and libstdc++ (see comments in source code)."
#endif
#else // !CWDEBUG_LIBBFD
#include "elf32.h"
#endif // !CWDEBUG_LIBBFD

#ifdef LIBCWD_THREAD_SAFE
using libcw::debug::_private_::rwlock_tct;
using libcw::debug::_private_::mutex_tct;
using libcw::debug::_private_::object_files_instance;
using libcw::debug::_private_::dlopen_map_instance;
#define BFD_INITIALIZE_LOCK		rwlock_tct<object_files_instance>::initialize();
#define BFD_ACQUIRE_WRITE_LOCK	        rwlock_tct<object_files_instance>::wrlock();
#define BFD_RELEASE_WRITE_LOCK	        rwlock_tct<object_files_instance>::wrunlock();
#define BFD_ACQUIRE_READ_LOCK	        rwlock_tct<object_files_instance>::rdlock();
#define BFD_RELEASE_READ_LOCK	        rwlock_tct<object_files_instance>::rdunlock();
#define BFD_ACQUIRE_READ2WRITE_LOCK	rwlock_tct<object_files_instance>::rd2wrlock();
#define BFD_ACQUIRE_WRITE2READ_LOCK     rwlock_tct<object_files_instance>::wr2rdlock();
#define DLOPEN_MAP_ACQUIRE_LOCK	        mutex_tct<dlopen_map_instance>::lock();
#define DLOPEN_MAP_RELEASE_LOCK	        mutex_tct<dlopen_map_instance>::unlock();
#else // !LIBCWD_THREAD_SAFE
#define BFD_INITIALIZE_LOCK
#define BFD_ACQUIRE_WRITE_LOCK
#define BFD_RELEASE_WRITE_LOCK
#define BFD_ACQUIRE_READ_LOCK
#define BFD_RELEASE_READ_LOCK
#define BFD_ACQUIRE_READ2WRITE_LOCK
#define BFD_ACQUIRE_WRITE2READ_LOCK
#define DLOPEN_MAP_ACQUIRE_LOCK
#define DLOPEN_MAP_RELEASE_LOCK
#endif // !LIBCWD_THREAD_SAFE

extern char** environ;

namespace libcw {
  namespace debug {

    extern void demangle_symbol(char const* in, _private_::internal_string& out);

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
      }
    }

    using _private_::set_alloc_checking_on;
    using _private_::set_alloc_checking_off;

    // Local stuff
    namespace cwbfd {

#if !CWDEBUG_LIBBFD

//----------------------------------------------------------------------------------------
// Interface Adaptor

typedef char* PTR;
typedef elf32::bfd_st bfd;
typedef elf32::Elf32_Addr bfd_vma;
typedef elf32::asection_st asection;
typedef elf32::asymbol_st asymbol;

int const bfd_archive = 0;
uint32_t const HAS_SYMS = 0xffffffff;

inline asection const* bfd_get_section(asymbol const* s) { return s->section; }
inline bfd*& bfd_asymbol_bfd(asymbol* s) { return s->bfd_ptr; }
inline bfd* bfd_asymbol_bfd(asymbol const* s) { return s->bfd_ptr; }
inline bfd* bfd_openr(char const* filename, void*) { return bfd::openr(filename); }
inline void bfd_close(bfd* abfd) { delete abfd; }
inline bool bfd_check_format(bfd const* abfd, int) { return abfd->check_format(); }
inline uint32_t bfd_get_file_flags(bfd const* abfd) { return abfd->has_syms() ? HAS_SYMS : 0; }
inline long bfd_get_symtab_upper_bound(bfd* abfd) { return abfd->get_symtab_upper_bound(); }
inline long bfd_canonicalize_symtab(bfd* abfd, asymbol** symbol_table) { return abfd->canonicalize_symtab(symbol_table); }
inline bool bfd_is_abs_section(asection const* sect) { return (sect == elf32::absolute_section_c); }
inline bool bfd_is_com_section(asection const* sect) { return false; }
inline bool bfd_is_ind_section(asection const* sect) { return false; }
inline bool bfd_is_und_section(asection const* sect) { return false; }

//----------------------------------------------------------------------------------------

#endif // !CWDEBUG_LIBBFD

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
	  LIBCWD_TSD_DECLARATION
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

      // cwbfd::
      class symbol_ct {
      private:
	asymbol* symbol;
	bool defined;
      public:
	symbol_ct(asymbol* p, bool def) : symbol(p), defined(def) { }
	asymbol const* get_symbol(void) const { return symbol; }
	bool is_defined(void) const { return defined; }
	bool operator==(symbol_ct const&) const { DoutFatal(dc::core, "Calling operator=="); }
	friend struct symbol_key_greater;
      };

      // cwbfd::
      struct symbol_key_greater {
	// Returns true when the start of a lays behond the end of b (ie, no overlap).
	bool operator()(symbol_ct const& a, symbol_ct const& b) const;
      };

      // cwbfd::
      typedef std::set<symbol_ct, symbol_key_greater, _private_::object_files_allocator::rebind<symbol_ct>::other> function_symbols_ct;

      // cwbfd::
      class object_file_ct;

      // cwbfd::
      typedef std::list<object_file_ct*, _private_::object_files_allocator> object_files_ct;

      // cwbfd::
      class object_file_ct {					// All allocations related to object_file_ct must be `internal'.
      private:
	bfd* abfd;
	void* lbase;
	size_t M_size;
	asymbol** symbol_table;
	long number_of_symbols;
	function_symbols_ct function_symbols;
      public:
	object_file_ct(void) : lbase(0) { }
	object_file_ct(char const* filename, void* base);
	~object_file_ct();

	bfd* get_bfd(void) const { return abfd; }
	void* const get_lbase(void) const { return lbase; }
	size_t size(void) const { return M_size; }
	asymbol** get_symbol_table(void) const { return symbol_table; }
	long get_number_of_symbols(void) const { return number_of_symbols; }
	function_symbols_ct& get_function_symbols(void) { return function_symbols; }
	function_symbols_ct const& get_function_symbols(void) const { return function_symbols; }
      private:
	friend object_files_ct const& NEEDS_READ_LOCK_object_files(void);	// Need access to `ST_list_instance'.
	friend object_files_ct& NEEDS_WRITE_LOCK_object_files(void);		// Need access to `ST_list_instance'.
	static char ST_list_instance[sizeof(object_files_ct)];
      };

      // cwbfd::
      typedef PTR addr_ptr_t;

      // cwbfd::
#ifdef PTR
      typedef const PTR addr_const_ptr_t;	// Warning: PTR is a macro, must put `const' in front of it
#else
      typedef char const* addr_const_ptr_t;
#endif

      // cwbfd::
      inline addr_const_ptr_t
      symbol_start_addr(asymbol const* s)
      {
	return s->value + bfd_get_section(s)->vma
	    + reinterpret_cast<char const*>(reinterpret_cast<object_file_ct const*>(bfd_asymbol_bfd(s)->usrdata)->get_lbase());
      }

      // cwbfd::
      inline size_t symbol_size(asymbol const* s)
      {
	return reinterpret_cast<size_t>(s->udata.p);
      }

      // cwbfd::
      inline size_t& symbol_size(asymbol* s)
      {
	return *reinterpret_cast<size_t*>(&s->udata.p);
      }

      // cwbfd::
      bool symbol_key_greater::operator()(symbol_ct const& a, symbol_ct const& b) const
      {
	return symbol_start_addr(a.symbol) >= reinterpret_cast<char const*>(symbol_start_addr(b.symbol)) + symbol_size(b.symbol);
      }

      // cwbfd::
      char object_file_ct::ST_list_instance[sizeof(object_files_ct)] __attribute__((__aligned__));

      // cwbfd::
      inline object_files_ct const& NEEDS_READ_LOCK_object_files(void)
      {
	return *reinterpret_cast<object_files_ct const*>(object_file_ct::ST_list_instance);
      }

      // cwbfd::
      inline object_files_ct& NEEDS_WRITE_LOCK_object_files(void)
      {
	return *reinterpret_cast<object_files_ct*>(object_file_ct::ST_list_instance);
      }

      // cwbfd::
      object_file_ct* NEEDS_READ_LOCK_find_object_file(void const* addr)
      {
	object_files_ct::const_iterator i(NEEDS_READ_LOCK_object_files().begin());
	for(; i != NEEDS_READ_LOCK_object_files().end(); ++i)
	  if ((*i)->get_lbase() < addr && (char*)(*i)->get_lbase() + (*i)->size() > addr)
	    break;
	return (i != NEEDS_READ_LOCK_object_files().end()) ? (*i) : NULL;
      }

      // cwbfd::
      object_file_ct* NEEDS_READ_LOCK_find_object_file(bfd const* abfd)
      {
	object_files_ct::const_iterator i(NEEDS_READ_LOCK_object_files().begin());
	for(; i != NEEDS_READ_LOCK_object_files().end(); ++i)
	  if ((*i)->get_bfd() == abfd)
	    break;
	return (i != NEEDS_READ_LOCK_object_files().end()) ? (*i) : NULL;
      }

      // cwbfd::
      struct symbol_less {
	bool operator()(asymbol const* a, asymbol const* b) const;
      };

      bool symbol_less::operator()(asymbol const* a, asymbol const* b) const
      {
	if (a == b)
	  return false;
	if (bfd_get_section(a)->vma + a->value < bfd_get_section(b)->vma + b->value)
	  return true;
	else if (bfd_get_section(a)->vma + a->value > bfd_get_section(b)->vma + b->value)
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

      // cwbfd::
      object_file_ct::object_file_ct(char const* filename, void* base) : lbase(base)
      {
#if CWDEBUG_DEBUGM
	{
	  LIBCWD_TSD_DECLARATION
	  LIBCWD_ASSERT( __libcwd_tsd.internal );
	}
#endif

	abfd = bfd_openr(filename, NULL);
#if CWDEBUG_LIBBFD
	if (!abfd)
	  DoutFatal(dc::bfd, "bfd_openr: " << bfd_errmsg(bfd_get_error()));
	abfd->cacheable = bfd_tttrue;
#else
	abfd->M_s_end_vma = 0;
#endif
	abfd->usrdata = (addr_ptr_t)this;

	if (bfd_check_format(abfd, bfd_archive))
	{
	  bfd_close(abfd);
#if CWDEBUG_LIBBFD
	  DoutFatal(dc::bfd, filename << ": can not get addresses from archive: " << bfd_errmsg(bfd_get_error()));
#else
	  DoutFatal(dc::bfd, filename << ": can not get addresses from archive.");
#endif
	}
#if CWDEBUG_LIBBFD
	char** matching;
	if (!bfd_check_format_matches(abfd, bfd_object, &matching))
	{
	  if (bfd_get_error() == bfd_error_file_ambiguously_recognized)
	  {
	    Dout(dc::warning, "bfd_check_format_matches: ambiguously object format, recognized formats: " <<
		cwprint(::libcw::debug::environment_ct(matching)));
	    free(matching);
	  }
	  DoutFatal(dc::fatal, filename << ": can not get addresses from object file: " << bfd_errmsg(bfd_get_error()));
	}
#endif

	if (!(bfd_get_file_flags(abfd) & HAS_SYMS))
	{
	  Dout(dc::warning, filename << " has no symbols, skipping.");
	  bfd_close(abfd);
	  number_of_symbols = 0;
	  return;
	}

	long storage_needed = bfd_get_symtab_upper_bound (abfd);
#if CWDEBUG_LIBBFD
	if (storage_needed < 0)
	  DoutFatal(dc::bfd, "bfd_get_symtab_upper_bound: " << bfd_errmsg(bfd_get_error()));
#else
	if (storage_needed == 0)
	{
	  Dout(dc::warning, filename << " has no symbols, skipping.");
	  bfd_close(abfd);
	  number_of_symbols = 0;
	  return;
	}
#endif

	symbol_table = (asymbol**) malloc(storage_needed);

	number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
#if CWDEBUG_LIBBFD
	if (number_of_symbols < 0)
	  DoutFatal(dc::bfd, "bfd_canonicalize_symtab: " << bfd_errmsg(bfd_get_error()));
#endif

	if (number_of_symbols > 0)
	{
#if !CWDEBUG_LIBBFD
	  size_t s_end_vma = abfd->M_s_end_vma;
#else
	  size_t s_end_vma = 0;
	  // Throw away symbols that can endanger determining the size of functions
	  // like: local symbols, debugging symbols.  Also throw away all symbols
	  // in uninteresting sections, so safe time with sorting.
	  // Also try to find symbol _end to determine the size of the bfd.
	  asymbol** se = &symbol_table[number_of_symbols - 1];
	  for (asymbol** s = symbol_table; s <= se;)
	  {
	    if ((*s)->name == 0)
	    {
	      *s = *se--;
	      --number_of_symbols;
	    }
	    // Find the start address of the last symbol: "_end".
	    else if ((*s)->name[0] == '_' && (*s)->name[1] == 'e' && (*s)->name[2] == 'n' && (*s)->name[3] == 'd' && (*s)->name[4] == 0)
	      s_end_vma = (*s++)->value;		// Relative to (yet unknown) lbase.
	    else if (((*s)->flags & (BSF_GLOBAL|BSF_FUNCTION|BSF_OBJECT)) == 0
		|| ((*s)->flags & (BSF_DEBUGGING|BSF_CONSTRUCTOR|BSF_WARNING|BSF_FILE)) != 0
		|| bfd_is_abs_section(bfd_get_section(*s))
		|| bfd_is_com_section(bfd_get_section(*s))
		|| bfd_is_ind_section(bfd_get_section(*s)))
	    {
	      *s = *se--;
	      --number_of_symbols;
	    }
	    else
	      ++s;
	  }
#endif // CWDEBUG_LIBBFD

	  if (!s_end_vma && number_of_symbols > 0)
	    Dout(dc::warning, "Cannot find symbol _end");

	  // Guess the start of the shared object when ldd didn't return it.
	  if (lbase == unknown_l_addr)
	  {
#ifdef HAVE_DLOPEN
	    LIBCWD_TSD_DECLARATION
	    int saved_internal = __libcwd_tsd.internal;
	    __libcwd_tsd.internal = 0;
	    void* handle = ::dlopen(filename, RTLD_LAZY);
	    if (!handle)
	    {
	      char const* dlerror_str = dlerror();
	      DoutFatal(dc::fatal, "::dlopen(" << filename << ", RTLD_LAZY): " << dlerror_str);
	    }
	    char* val;
	    if (s_end_vma && (val = (char*)dlsym(handle, "_end")))	// dlsym will fail when _end is a local symbol.
	    {
	      __libcwd_tsd.internal = saved_internal;
	      lbase = val - s_end_vma;
	    }
	    else
#ifdef HAVE__DL_LOADED
            {
	      __libcwd_tsd.internal = saved_internal;
	      for(link_map* p = _dl_loaded; p; p = p->l_next)
	        if (!strcmp(p->l_name, filename))
		{
		  lbase = reinterpret_cast<void*>(p->l_addr);		// No idea why they use unsigned int for a load address.
		  break;
		}
	    }
#else // !HAVE_LINK_H
	    {
	      __libcwd_tsd.internal = saved_internal;
	      // The following code uses a heuristic approach to guess the start of an object file.
	      typedef std::map<void*, unsigned int, std::less<void*>, _private_::internal_allocator::rebind<void*>::other> start_values_map_ct;
	      start_values_map_ct start_values;
	      unsigned int best_count = 0;
	      void* best_start = 0;
	      for (asymbol** s = symbol_table; s <= &symbol_table[number_of_symbols - 1]; ++s)
	      {
		if ((*s)->name == 0 || ((*s)->flags & BSF_FUNCTION) == 0 || ((*s)->flags & (BSF_GLOBAL|BSF_WEAK)) == 0)
		  continue;
		asection const* sect = bfd_get_section(*s);
		if (sect->name[1] == 't' && !strcmp(sect->name, ".text"))
		{
		  __libcwd_tsd.internal = 0;
		  void* val = dlsym(handle, (*s)->name);
		  if (dlerror() == NULL)
		  {
		    __libcwd_tsd.internal = saved_internal;
		    void* start = reinterpret_cast<char*>(val) - (*s)->value - sect->vma;
		    std::pair<start_values_map_ct::iterator, bool> p = start_values.insert(std::pair<void* const, unsigned int>(start, 0));
		    if (++(*(p.first)).second > best_count)
		    {
		      best_start = start;
		      if (++best_count == 10)	// Its unlikely that even more than 2 wrong values will have the same value.
			break;			// So if we reach 10 then this is value we are looking for.
		    }
		  }
		  else
		    __libcwd_tsd.internal = saved_internal;
		}
	      }
	      if (best_count < 3)
	      {
		Dout(dc::warning, "Unable to determine start of \"" << filename << "\", skipping.");
		free(symbol_table);
		symbol_table  = NULL;
		bfd_close(abfd);
		number_of_symbols = 0;
		__libcwd_tsd.internal = 0;
		::dlclose(handle);
		__libcwd_tsd.internal = saved_internal;
		return;
	      }
	      lbase = best_start;
	    }
#endif // !HAVE_LINK_H
	    __libcwd_tsd.internal = 0;
	    ::dlclose(handle);
	    __libcwd_tsd.internal = saved_internal;
	    Dout(dc::continued, '(' << lbase << ") ... ");
#else // !HAVE_DLOPEN
	    DoutFatal(dc::fatal, "Can't determine start of shared library: you will need libdl to be detected by configure.");
#endif
	  }

	  void const* s_end_start_addr;
	  
	  if (s_end_vma)
	  {
	    s_end_start_addr = (char*)lbase + s_end_vma;
	    M_size = s_end_vma;
          }
	  else
	  {
	    s_end_start_addr = NULL;
	    M_size = 0;			// We will determine this later.
	  }

	  // Sort the symbol table in order of start address.
	  std::sort(symbol_table, &symbol_table[number_of_symbols], symbol_less());

	  // Calculate sizes for every symbol
	  for (int i = 0; i < number_of_symbols - 1; ++i)
	    symbol_size(symbol_table[i]) = (char*)symbol_start_addr(symbol_table[i + 1]) - (char*)symbol_start_addr(symbol_table[i]);

	  // Use reasonable size for last one.
	  // This should be "_end", or one behond it, and will be thrown away in the next loop.
	  symbol_size(symbol_table[number_of_symbols - 1]) = 100000;

	  // Throw away all symbols that are not a global variable or function, store the rest in a vector.
	  asymbol** se2 = &symbol_table[number_of_symbols - 1];
	  for (asymbol** s = symbol_table; s <= se2;)
	  {
	    if (((*s)->flags & (BSF_SECTION_SYM|BSF_OLD_COMMON|BSF_NOT_AT_END|BSF_INDIRECT|BSF_DYNAMIC)) != 0
		|| (s_end_start_addr != NULL && symbol_start_addr(*s) >= s_end_start_addr)
		|| (*s)->flags & BSF_FUNCTION == 0)	// Only keep functions.
	    {
	      *s = *se2--;
	      --number_of_symbols;
	    }
	    else
	    {
	      function_symbols.insert(function_symbols_ct::key_type(*s, !bfd_is_und_section(bfd_get_section(*s))));
	      ++s;
	    }
	  }

	  if (function_symbols.size() > 0)
	  {
	    asymbol const* last_symbol = (*function_symbols.rbegin()).get_symbol();
	    if (symbol_size(last_symbol) == 100000)
	      Dout( dc::warning, "Unknown size of symbol " << last_symbol->name);
	    if (!M_size)
	      M_size = (char*)symbol_start_addr(last_symbol) + symbol_size(last_symbol) - (char*)lbase;
	  }
	}

	if (number_of_symbols > 0)
	  NEEDS_WRITE_LOCK_object_files().push_back(this);
      }

      // cwbfd::
      object_file_ct::~object_file_ct()
      {
#if defined(LIBCWD_THREAD_SAFE) && CWDEBUG_DEBUG
	LIBCWD_ASSERT( _private_::is_locked(object_files_instance) );
#endif
	object_files_ct::iterator iter(find(NEEDS_WRITE_LOCK_object_files().begin(), NEEDS_WRITE_LOCK_object_files().end(), this));
	if (iter != NEEDS_WRITE_LOCK_object_files().end())
	  NEEDS_WRITE_LOCK_object_files().erase(iter);
      }

      // cwbfd::
      struct object_file_greater {
	bool operator()(object_file_ct const* a, object_file_ct const* b) const { return a->get_lbase() > b->get_lbase(); }
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
      _private_::ST_internal_string* ST_argv0_ptr;		// MT: Set in `ST_get_full_path_to_executable', used in `ST_decode_ps'.
      // cwbfd::
      _private_::ST_internal_string const* ST_pidstr_ptr;	// MT: Set in `ST_get_full_path_to_executable', used in `ST_decode_ps'.

      // cwbfd::
      int ST_decode_ps(char const* buf, size_t len)	// MT: Single Threaded function.
      {
	static int pid_token = 0;
	static int command_token = 0;
	static size_t command_column;
	int current_token = 0;
	bool found_PID = false;
	bool eating_token = false;
	size_t current_column = 1;
	_private_::ST_internal_string token;

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
	      else if (command_token == 0 && (token == "COMMAND") || (token == "CMD"))
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
      void ST_get_full_path_to_executable(_private_::ST_internal_string& result)
      {
	_private_::ST_internal_string argv0;		// Like main()s argv[0], thus must be zero terminated.
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
	  _private_::ST_internal_string pidstr;

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

	  ST_argv0_ptr = &argv0;	// Ugly way to pass these ST_internal_strings to ST_decode_ps:
	  ST_pidstr_ptr = &pidstr;	// pidstr is input, argv0 is output.

	  if (ST_exec_prog(ps_prog, argv, environ, ST_decode_ps) == -1 || argv0.empty())
	    DoutFatal(dc::fatal|error_cf, "Failed to execute \"" << ps_prog << "\"");
	}

	argv0 += '\0';
	// argv0 now contains a zero terminated string optionally followed by the command line
	// arguments (which is why we can't use argv0.find('/') here), all seperated by the 0 character.
	if (!strchr(argv0.data(), '/'))
	{
	  _private_::ST_internal_string prog_name(argv0);
	  _private_::ST_internal_string path_list(getenv("PATH"));
	  _private_::ST_internal_string::size_type start_pos = 0, end_pos;
	  _private_::ST_internal_string path;
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
	    if (end_pos == _private_::ST_internal_string::npos)
	      break;
	    start_pos = end_pos + 1;
	  }
	}

	char full_path_buf[MAXPATHLEN];
	char* full_path = realpath(argv0.data(), full_path_buf);

	if (!full_path)
	  DoutFatal(dc::fatal|error_cf, "realpath(\"" << argv0.data() << "\", full_path_buf)");

	Dout(dc::debug, "Full path to executable is \"" << full_path << "\".");
	result.assign(full_path);
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
      typedef std::vector<my_link_map, _private_::object_files_allocator::rebind<my_link_map>::other> ST_shared_libs_vector_ct;
      ST_shared_libs_vector_ct ST_shared_libs;			// Written to only in `ST_decode_ldd' which is called from
      								// `cwbfd::ST_init' and read from in a later part of `cwbfd::ST_init'.

      // cwbfd::
      int ST_decode_ldd(char const* buf, size_t len)
      {
	for (char const* p = buf; p < &buf[len]; ++p)
	  if (p[0] == '=' && p[1] == '>' && p[2] == ' ' || p[2] == '\t')
	  {
	    p += 2;
	    while (*p == ' ' || *p == '\t')
	      ++p;
	    if (*p != '/' && *p != '.')
	      break;
	    char const* q;
	    for (q = p; q < &buf[len] && *q > ' '; ++q);
	    if (*q == '\n')	// This ldd doesn't return an offset (ie, on solaris).
	    {
	      ST_shared_libs.push_back(my_link_map(p, q - p, unknown_l_addr));
	      break;
	    }
	    for (char const* r = q; r < &buf[len]; ++r)
	      if (r[0] == '(' && r[1] == '0' && r[2] == 'x')
	      {
		char* s;
		void* addr = reinterpret_cast<void*>(strtol(++r, &s, 0));
		ST_shared_libs.push_back(my_link_map(p, q - p, addr));
		break;
	      }
	    break;
	  }
	return 0;
      }

#ifdef LIBCWD_DEBUGBFD
      // cwbfd::
      void dump_object_file_symbols(object_file_ct const* object_file)
      {
	std::cout << std::setiosflags(std::ios_base::left) << std::setw(15) << "Start address" << std::setw(50) << "File name" << std::setw(20) << "Number of symbols\n";
	std::cout << "0x" << std::setfill('0') << std::setiosflags(std::ios_base::right) << std::setw(8) << std::hex << (unsigned long)object_file->get_lbase() << "     ";
	std::cout << std::setfill(' ') << std::setiosflags(std::ios_base::left) << std::setw(50) << object_file->get_bfd()->filename;
	std::cout << std::dec << std::setiosflags(std::ios_base::left);
	std::cout << object_file->get_number_of_symbols() << std::endl;

	std::cout << std::setiosflags(std::ios_base::left) << std::setw(12) << "Start";
	std::cout << ' ' << std::setiosflags(std::ios_base::right) << std::setw(6) << "Size" << ' ';
	std::cout << "Name value flags\n";
	asymbol** symbol_table = object_file->get_symbol_table();
	for (long n = object_file->get_number_of_symbols() - 1; n >= 0; --n)
	{
	  std::cout << std::setiosflags(std::ios_base::left) << std::setw(12) << (void*)symbol_start_addr(symbol_table[n]);
	  std::cout << ' ' << std::setiosflags(std::ios_base::right) << std::setw(6) << symbol_size(symbol_table[n]) << ' ';
	  std::cout << symbol_table[n]->name << ' ' << (void*)symbol_table[n]->value << ' ' << std::oct << symbol_table[n]->flags << std::endl;
	}
      }
#endif

      // cwbfd::
      object_file_ct* load_object_file(char const* name, void* l_addr)
      {
	{
	  LIBCWD_TSD_DECLARATION
	  LIBCWD_ASSERT( __libcwd_tsd.internal );
	}
        if (l_addr == unknown_l_addr)
	  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug info from " << name << ' ');
	else if (l_addr == 0)
	  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug info from " << name << "... ");
	else
	  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug info from " << name << " (" << l_addr << ") ... ");
	object_file_ct* object_file = new object_file_ct(name, l_addr);
	if (object_file->get_number_of_symbols() > 0)
	{
	  Dout(dc::finish, "done (" << std::dec << object_file->get_number_of_symbols() << " symbols)");
#ifdef LIBCWD_DEBUGBFD
	  dump_object_file_symbols(object_file);
#endif
	}
	else
	{
	  Dout(dc::finish, "No symbols found");
	  delete object_file;
	  return NULL;
        }
	return object_file;
      }

      // cwbfd::
      bool ST_init(void)
      {
	// This must be called before calling ST_init().
	libcw_do.NS_init();

	static bool WST_being_initialized = false;
	// This should catch it when we call new or malloc while 'internal'.
	if (WST_being_initialized)
	  return false;
	WST_being_initialized = true;

        // MT: We assume this is called before reaching main().
	//     Therefore, no synchronisation is required.
#if CWDEBUG_DEBUG && defined(LIBCWD_THREAD_SAFE)
	if (_private_::WST_multi_threaded)
	  core_dump();
#endif

	LIBCWD_TSD_DECLARATION

#if CWDEBUG_DEBUG && CWDEBUG_ALLOC
	LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif

	// ****************************************************************************
	// Start INTERNAL!
	set_alloc_checking_off(LIBCWD_TSD);

#if CWDEBUG_ALLOC
	// Initialize the malloc library if not done yet.
	init_debugmalloc();
#endif

	libcw::debug::debug_ct::OnOffState state;
	libcw::debug::channel_ct::OnOffState state2;
	if (_private_::always_print_loading && !_private_::suppress_startup_msgs)
	{
	  // We want debug output to BFD
	  Debug( libcw_do.force_on(state) );
	  Debug( dc::bfd.force_on(state2, "BFD") );
	}

	// Initialize object files list, we don't really need the
	// write lock because this function is Single Threaded.
	new (&NEEDS_WRITE_LOCK_object_files()) object_files_ct;

#if CWDEBUG_LIBBFD
	bfd_init();
#endif

	// Get the full path and name of executable
	struct static_internal_string {
	  _private_::ST_internal_string* value;
          static_internal_string(void)
	      {
		value = new _private_::ST_internal_string;	// alloc checking already off.
	      }
	  ~static_internal_string()
	      {
		LIBCWD_TSD_DECLARATION
		set_alloc_checking_off(LIBCWD_TSD);
#if CWDEBUG_DEBUG && defined(LIBCWD_THREAD_SAFE)
		_private_::WST_multi_threaded = false;		// `fullpath' is static and will only be destroyed from exit().
#endif
		delete value;
#if CWDEBUG_DEBUG && defined(LIBCWD_THREAD_SAFE)
		_private_::WST_multi_threaded = true;		// Make sure we catch other global strings (in order to avoid a static destructor ordering fiasco).
#endif
		set_alloc_checking_on(LIBCWD_TSD);
	      }
        };
	static static_internal_string fullpath;			// Must be static because bfd keeps a pointer to its data()
	ST_get_full_path_to_executable(*fullpath.value);
	*fullpath.value += '\0';				// Make string null terminated so we can use data().

#if CWDEBUG_LIBBFD
	bfd_set_error_program_name(fullpath.value->data() + fullpath.value->find_last_of('/') + 1);
	bfd_set_error_handler(error_handler);
#endif

	// Load executable
	// No write lock is really needed because this is a Single Threaded function,
	// but the sanity checks inside the allocators used in load_object_file()
	// require the lock to be set.  Fortunately it therefore also doesn't hurt
	// that we keep the lock a long time (during the execution of ldd_prog).
	// Same thing for the LIBCWD_DEFER_CANCEL, needed for the sanity checks.
	BFD_INITIALIZE_LOCK
	LIBCWD_DEFER_CLEANUP_PUSH(_private_::fatal_cancellation, const_cast<char*>("while reading debug information from a shared library"))
        BFD_ACQUIRE_WRITE_LOCK
	load_object_file(fullpath.value->data(), 0);

	// Load all shared objects
#ifndef HAVE__DL_LOADED
	// Path to `ldd'
	char const ldd_prog[] = "/usr/bin/ldd";

	char const* argv[3];
	argv[0] = "ldd";
	argv[1] = fullpath.value->data();
	argv[2] = NULL;
	set_alloc_checking_on(LIBCWD_TSD);
	ST_exec_prog(ldd_prog, argv, environ, ST_decode_ldd);
	set_alloc_checking_off(LIBCWD_TSD);

	for(ST_shared_libs_vector_ct::const_iterator iter = ST_shared_libs.begin(); iter != ST_shared_libs.end(); ++iter)
	{
	  my_link_map const* l = &(*iter);
#else
	for(link_map const* l = _dl_loaded; l; l = l->l_next)
	{
#endif
	  if (l->l_addr)
	    load_object_file(l->l_name, reinterpret_cast<void*>(l->l_addr));
	}
	NEEDS_WRITE_LOCK_object_files().sort(object_file_greater());
	BFD_RELEASE_WRITE_LOCK

	set_alloc_checking_on(LIBCWD_TSD);
	// End INTERNAL!
	// ****************************************************************************

	// We put this outside the 'internal' region because it might call __pthread_do_exit.
	LIBCWD_CLEANUP_POP_RESTORE(false)

        if (_private_::always_print_loading)
	{
	  Debug( dc::bfd.restore(state2) );
	  Debug( libcw_do.restore(state) );
	}

	WST_initialized = true;			// MT: Safe, this function is Single Threaded.

#ifdef LIBCWD_DEBUGBFD
	LIBCWD_DEFER_CANCEL
	BFD_ACQUIRE_READ_LOCK
	// Dump all symbols
	for (object_files_ct::const_reverse_iterator i(NEEDS_READ_LOCK_object_files().rbegin()); i != NEEDS_READ_LOCK_object_files().rend(); ++i)
          dump_object_file_symbols(*i);
	BFD_RELEASE_READ_LOCK
	LIBCWD_RESTORE_CANCEL
#endif

	return true;
      }

      // cwbfd::
      symbol_ct const* pc_symbol(bfd_vma addr, object_file_ct* object_file)
      {
	if (object_file)
	{
	  asymbol dummy_symbol;				// A dummy symbol with size 1 and start `addr',
	  asection dummy_section;

	  // Make symbol_start_addr(&dummy_symbol) and symbol_size(&dummy_symbol) return the correct value:
	  bfd_asymbol_bfd(&dummy_symbol) = object_file->get_bfd();
	  dummy_section.vma = 0;			// Use a vma of 0 and
	  dummy_symbol.section = &dummy_section;	// use dummy_symbol.value to store (value + vma):
	  dummy_symbol.value = reinterpret_cast<char const*>(addr) - reinterpret_cast<char const*>(object_file->get_lbase());
	  symbol_size(&dummy_symbol) = 1;
	  function_symbols_ct::iterator i(object_file->get_function_symbols().find(symbol_ct(&dummy_symbol, true)));
	  if (i != object_file->get_function_symbols().end())
	  {
	    asymbol const* p = (*i).get_symbol();
	    if (addr < (bfd_vma)(size_t)symbol_start_addr(p) + symbol_size(p))
	      return &(*i);
	  }
	  Dout(dc::bfd, "No symbol found: " << (void*)addr);
	}
	else
	  Dout(dc::bfd, "No source file found: " << (void*)addr);
	return NULL;
      }

    } // namespace cwbfd

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

      if (!WST_initialized	// `WST_initialized' is only false when we are still Single Threaded.
	  && !ST_init())
	return unknown_function_c;

      symbol_ct const* symbol;
      LIBCWD_DEFER_CANCEL
      BFD_ACQUIRE_READ_LOCK;
      symbol = pc_symbol((bfd_vma)(size_t)addr, NEEDS_READ_LOCK_find_object_file(addr));
      BFD_RELEASE_READ_LOCK;
      LIBCWD_RESTORE_CANCEL

      if (!symbol)
	return unknown_function_c;

      return symbol->get_symbol()->name;
    }

    /** \} */	// End of group 'group_locations'.

    struct bfd_location_ct : public location_ct {
      friend _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, bfd_location_ct const& data);
    };

    _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, bfd_location_ct const& location)
    {
      if (location.M_filepath)
	os << location.M_filename << ':' << location.M_line;
      else
	os << "<unknown location>";
      return os;
    }

    //
    // location_ct::M_bfd_pc_location
    //
    // Find source file, (mangled) function name and line number of the address `addr'.
    //
    // Like `pc_function', this function looks up the symbol (function) that
    // belongs to the address `addr' and stores the pointer to the name of that symbol
    // in the member `M_func'.  When no symbol could be found then `M_func' is set to
    // `libcw::debug::unknown_function_c' and `M_filepath' is set to NULL.
    //
    // If a symbol is found then this function attempts to lookup source file and line number
    // nearest to the given address.  The result - if any - is put into `M_filepath' (source
    // file) and `M_line' (line number), and `M_filename' is set to point to the filename
    // part of `M_filepath'.  If a lookup fails then `M_filepath' is set to NULL.
    //
    void location_ct::M_pc_location(void const* addr LIBCWD_COMMA_TSD_PARAM)
    {
      using namespace cwbfd;

      if (!WST_initialized)
      {
	// MT: `WST_initialized' is only false when we're still Single Threaded.
	//     Therefore it is safe to call ST_* functions.

#ifdef __GLIBCPP__	// Pre libstdc++ v3, there is no malloc done for initialization of cerr.
        if (!_private_::WST_ios_base_initialized && _private_::inside_ios_base_Init_Init())
	{
	  M_filepath = NULL;
	  M_func = "<pre ios initialization>";
	  return;
	}
#endif
	if (!ST_init())	// Initialization of BFD code fails?
	{
	  M_filepath = NULL;
	  M_func = "<pre libcwd initialization>";
	  return;
	}
      }

      object_file_ct* object_file;
      LIBCWD_DEFER_CANCEL
      BFD_ACQUIRE_READ_LOCK;
      object_file = NEEDS_READ_LOCK_find_object_file(addr);
#ifdef HAVE__DL_LOADED
      if (!object_file)
      {
	set_alloc_checking_off(LIBCWD_TSD);
        // Try to load everything again... previous loaded libraries are skipped based on load address.
        BFD_ACQUIRE_READ2WRITE_LOCK;
	for(link_map* l = _dl_loaded; l;)
	{
	  if (l->l_addr)
	  {
	    for (object_files_ct::const_iterator iter = NEEDS_READ_LOCK_object_files().begin();
		 iter != NEEDS_READ_LOCK_object_files().end();
		 ++iter)
	      if (reinterpret_cast<void*>(l->l_addr) == (*iter)->get_lbase())
		goto already_loaded;
	    load_object_file(l->l_name, reinterpret_cast<void*>(l->l_addr));
	  }
already_loaded:
	  l = l->l_next;
	}
	NEEDS_WRITE_LOCK_object_files().sort(object_file_greater());
	set_alloc_checking_on(LIBCWD_TSD);
	object_file = NEEDS_READ_LOCK_find_object_file(addr);
        BFD_RELEASE_WRITE_LOCK;
      }
      else
      {
        BFD_RELEASE_READ_LOCK;
      }
#else
      BFD_RELEASE_READ_LOCK;
#endif
      LIBCWD_RESTORE_CANCEL
      if (!object_file)
      {
        LIBCWD_Dout(dc::bfd, "No object file for address " << addr);
	M_filepath = NULL;
	M_func = unknown_function_c;
	return;
      }

      symbol_ct const* symbol = pc_symbol((bfd_vma)(size_t)addr, object_file);
      if (symbol && symbol->is_defined())
      {
	asymbol const* p = symbol->get_symbol();
	bfd* abfd = bfd_asymbol_bfd(p);
	asection const* sect = bfd_get_section(p);
	char const* file;
	LIBCWD_ASSERT( object_file->get_bfd() == abfd );
	LIBCWD_DEFER_CANCEL
	set_alloc_checking_off(LIBCWD_TSD);
#if CWDEBUG_LIBBFD
	bfd_find_nearest_line(abfd, const_cast<asection*>(sect), const_cast<asymbol**>(object_file->get_symbol_table()),
	    (char*)addr - (char*)object_file->get_lbase() - sect->vma, &file, &M_func, &M_line);
#else
        abfd->find_nearest_line(p, (char*)addr - (char*)object_file->get_lbase(), &file, &M_func, &M_line);
#endif
	set_alloc_checking_on(LIBCWD_TSD);
	LIBCWD_RESTORE_CANCEL
	LIBCWD_ASSERT( !(M_func && !p->name) );	// Please inform the author of libcwd if this assertion fails.
	M_func = p->name;

	if (file && M_line)			// When line is 0, it turns out that `file' is nonsense.
	{
	  size_t len = strlen(file);
	  // `file' is allocated by `bfd_find_nearest_line', however - it is also libbfd
	  // that will free `file' again a second call to `bfd_find_nearest_line' (for the
	  // same value of `abfd': the allocated pointer is stored in a structure
	  // that is kept for each bfd seperately).
	  // The call to `new char [len + 1]' below could cause this function (M_pc_location)
	  // to be called again (in order to store the file:line where the allocation
	  // is done) and thus a new call to `bfd_find_nearest_line', which then would
	  // free `file' before we copy it!
	  // Therefore we need to call `set_alloc_checking_off', to prevent this.
	  set_alloc_checking_off(LIBCWD_TSD);
	  M_filepath = new char [len + 1];
	  set_alloc_checking_on(LIBCWD_TSD);
	  strcpy(M_filepath, file);
	  M_filename = strrchr(M_filepath, '/') + 1;
	  if (M_filename == (char const*)1)
	    M_filename = M_filepath;
	}
	else
	  M_filepath = NULL;

	// Sanity check
	if (!p->name || M_line == 0)
	{
	  if (p->name)
	  {
	    static int const BSF_WARNING_PRINTED = 0x40000000;
	    if (!(p->flags & BSF_WARNING_PRINTED))
	    {
	      const_cast<asymbol*>(p)->flags |= BSF_WARNING_PRINTED;
	      set_alloc_checking_off(LIBCWD_TSD);
	      {
		_private_::internal_string demangled_name;		// Alloc checking must be turned off already for this string.
		demangle_symbol(p->name, demangled_name);
		set_alloc_checking_on(LIBCWD_TSD);
		char const* ofn = strrchr(abfd->filename, '/');
		LIBCWD_Dout(dc::bfd, "Warning: Address " << addr << " in section " << sect->name <<
		    " of object file \"" << (ofn ? ofn + 1 : abfd->filename) << '"');
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
	        " of object file \"" << abfd->filename << "\" does not contain a function.");
	}
	else
	  LIBCWD_Dout(dc::bfd, "address " << addr << " corresponds to " << *static_cast<bfd_location_ct*>(this));
	return;
      }

      M_filepath = NULL;
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
    void location_ct::clear(void)
    {
      if (M_filepath)
      {
	LIBCWD_TSD_DECLARATION
	set_alloc_checking_off(LIBCWD_TSD);
	delete [] M_filepath;
	M_filepath = NULL;
	set_alloc_checking_on(LIBCWD_TSD);
      }
      M_func = "<cleared location_ct>";
    }

    // Undocumented: shouldn't be used I think.
    location_ct::location_ct(location_ct const &prototype) : M_filepath(NULL)
    {
      if (prototype.M_filepath)
      {
	LIBCWD_TSD_DECLARATION
	set_alloc_checking_off(LIBCWD_TSD);
	M_filepath = new char [strlen(prototype.M_filepath) + 1];
	set_alloc_checking_on(LIBCWD_TSD);
	strcpy(M_filepath, prototype.M_filepath);
	M_filename = M_filepath + (prototype.M_filename - prototype.M_filepath);
	M_line = prototype.M_line;
	M_func = prototype.M_func;
      }
    }

    /**
     * \brief Move \p prototype to this location object
     *
     * \p prototype must be \ref is_known "known" and the current object \ref is_known "unknown";
     * \p prototype is clear()-ed afterwards.
     */
    void location_ct::move(location_ct& prototype)
    {
      // MT: This method is used assuming that *only* attributes of the
      //     location_ct objects `this' and `prototype' are accessed.
      // (so no locking is needed when the shared objects are known to be unique for the current thread).
      LIBCWD_ASSERT( !this->is_known() );
      M_filepath = prototype.M_filepath;
      M_filename = M_filepath + (prototype.M_filename - prototype.M_filepath);
      M_line = prototype.M_line;
      M_func = prototype.M_func;
      prototype.M_filepath = NULL;
      prototype.M_func = "<moved location_ct>";
    }

    // Undocumented: shouldn't be used I think.
    location_ct& location_ct::operator=(location_ct const &prototype)
    {
      if (this != &prototype)
      {
	clear();
	if (prototype.M_filepath)
	{
	  LIBCWD_TSD_DECLARATION
	  set_alloc_checking_off(LIBCWD_TSD);
	  M_filepath = new char [strlen(prototype.M_filepath) + 1];
	  set_alloc_checking_on(LIBCWD_TSD);
	  strcpy(M_filepath, prototype.M_filepath);
	  M_filename = M_filepath + (prototype.M_filename - prototype.M_filepath);
	  M_line = prototype.M_line;
	  M_func = prototype.M_func;
	}
      }
      return *this;
    }

    /**
     * \brief Write \a location to ostream \a os.
     * \ingroup group_locations
     *
     * Write the contents of a location_ct object to an ostream in the form "source-%file:line-number",
     * or writes "<unknown location>" when the location is unknown.
     */
    std::ostream& operator<<(std::ostream& os, location_ct const& location)
    {
      if (location.M_filepath)
	os << location.M_filename << ':' << location.M_line;
      else
	os << "<unknown location>";
      return os;
    }

  } // namespace debug
} // namespace libcw

#ifdef LIBCWD_DLOPEN_DEFINED
using namespace libcw::debug;

struct dlloaded_st {
  cwbfd::object_file_ct* M_object_file;
  int M_flags;
  dlloaded_st(cwbfd::object_file_ct* object_file, int flags) : M_object_file(object_file), M_flags(flags) { }
};

namespace libcw {
  namespace debug {
    namespace _private_ {
      typedef std::map<void*, dlloaded_st, std::less<void*>, userspace_allocator::rebind<void*>::other> dlopen_map_ct;
      static dlopen_map_ct dlopen_map;

#ifdef LIBCWD_THREAD_SAFE
void dlopen_cleanup1(void* arg)
{
  TSD_st& __libcwd_tsd = (*static_cast<TSD_st*>(arg));
  set_alloc_checking_on(LIBCWD_TSD);
  BFD_RELEASE_WRITE_LOCK
}
#endif

    }
  }
}

extern "C" {
  void* __libcwd_dlopen(char const* name, int flags)
  {
    LIBCWD_TSD_DECLARATION
    LIBCWD_ASSERT( !__libcwd_tsd.internal );
    void* handle = ::dlopen(name, flags);
#ifdef RTLD_NOLOAD
    if ((flags & RTLD_NOLOAD))
      return handle;
#endif
    cwbfd::object_file_ct* object_file;
    LIBCWD_DEFER_CLEANUP_PUSH(libcw::debug::_private_::dlopen_cleanup1, &__libcwd_tsd)	// In case we are cancelled inside cwbfd::load_object_file().
    BFD_ACQUIRE_WRITE_LOCK;
    set_alloc_checking_off(LIBCWD_TSD);
    object_file = cwbfd::load_object_file(name, cwbfd::unknown_l_addr);
    if (object_file)
      cwbfd::NEEDS_WRITE_LOCK_object_files().sort(cwbfd::object_file_greater());
    set_alloc_checking_on(LIBCWD_TSD);
    BFD_RELEASE_WRITE_LOCK;
    LIBCWD_CLEANUP_POP_RESTORE(false)
    if (object_file)
    {
      // MT: dlopen_map uses a userspace_allocator and might therefore write to dc::malloc.
      // This means that the thread can be cancelled inside the insert() call, leaving
      // dlopen_map in an undefined state.  Therefore we disable cancellation.
      LIBCWD_DISABLE_CANCEL
      DLOPEN_MAP_ACQUIRE_LOCK
      libcw::debug::_private_::dlopen_map.insert(std::pair<void* const, dlloaded_st>(handle, dlloaded_st(object_file, flags)));
      DLOPEN_MAP_RELEASE_LOCK
      LIBCWD_ENABLE_CANCEL
    }
    return handle;
  }

  int __libcwd_dlclose(void *handle)
  {
    LIBCWD_TSD_DECLARATION
    LIBCWD_ASSERT( !__libcwd_tsd.internal );
    int ret = ::dlclose(handle);
    LIBCWD_DEFER_CANCEL
    DLOPEN_MAP_ACQUIRE_LOCK
    libcw::debug::_private_::dlopen_map_ct::iterator iter(libcw::debug::_private_::dlopen_map.find(handle));
    if (iter != libcw::debug::_private_::dlopen_map.end())
    {
#ifdef RTLD_NODELETE
      if (!((*iter).second.M_flags & RTLD_NODELETE))
      {
        set_alloc_checking_off(LIBCWD_TSD);
	delete (*iter).second.M_object_file;
        set_alloc_checking_on(LIBCWD_TSD);
      }
#endif
#ifdef LIBCWD_THREAD_SAFE
      // MT: dlopen_map uses a userspace_allocator and might therefore write to dc::malloc.
      // This means that the thread can be cancelled inside the erase() call, leaving
      // dlopen_map in an undefined state.  Therefore we disable cancellation.
      int oldstate;
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
#endif
      libcw::debug::_private_::dlopen_map.erase(iter);
#ifdef LIBCWD_THREAD_SAFE
      pthread_setcancelstate(oldstate, NULL);
#endif
    }
    DLOPEN_MAP_RELEASE_LOCK
    LIBCWD_RESTORE_CANCEL
    return ret;
  }
} // extern "C"

#endif // LIBCWD_DLOPEN_DEFINED

#endif // !CWDEBUG_LOCATION
