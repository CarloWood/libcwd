// $Header$
//
// Copyright (C) 2000, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#define ALWAYS_PRINT_LOADING	// Define to temporally turn on dc::bfd in order to print the "Loading debug info from ..." lines.
#undef DEBUGDEBUGBFD		// Define to add debug code for this file.

#include <libcw/debug_config.h>
#ifdef DEBUGUSEBFD

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
#include "config.h"
#ifdef HAVE_DLOPEN
#include <map>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#endif
#ifdef HAVE_LINK_H
#include <link.h>
extern link_map* _dl_loaded;
#endif
#include <cstdio>		// Needed for vsnprintf.
#include <algorithm>
#include <libcw/debug.h>
#include <libcw/bfd.h>
#ifdef CWDEBUG_DLOPEN_DEFINED
#undef dlopen
#undef dlclose
#endif
#include <libcw/exec_prog.h>
#include <libcw/cwprint.h>
#include <libcw/demangle.h>
#ifdef DEBUGUSEGNULIBBFD
#if defined(BFD64) && !BFD_HOST_64BIT_LONG && defined(__GLIBCPP__) && !defined(_GLIBCPP_USE_LONG_LONG)
// libbfd is compiled with 64bit support on a 32bit host, but libstdc++ is not compiled with support
// for `long long'.  If you run into this error (and you insist on use libbfd) then either recompile
// libstdc++ with support for `long long' or recompile libbfd without 64bit support.
#error "Incompatible libbfd and libstdc++ (see comments in source code)."
#endif
#else // !DEBUGUSEGNULIBBFD
#include <libcw/elf32.h>
#endif // !DEBUGUSEGNULIBBFD

#ifdef DEBUGDEBUGBFD
#include <iomanip>
#endif

RCSTAG_CC("$Id$")

using namespace std;

extern char** environ;

namespace libcw {
  namespace debug {

    // New debug channel
    namespace channels {
      namespace dc {
	channel_ct const bfd("BFD");
      }
    }

    // Local stuff
    namespace cwbfd {

#ifndef DEBUGUSEGNULIBBFD

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
inline bool bfd_is_abs_section(asection const* sect) { return (sect == elf32::absolute_section); }
inline bool bfd_is_com_section(asection const* sect) { return false; }
inline bool bfd_is_ind_section(asection const* sect) { return false; }
inline bool bfd_is_und_section(asection const* sect) { return false; }

//----------------------------------------------------------------------------------------

#endif // !DEBUGUSEGNULIBBFD

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
	  set_alloc_checking_off();
	  char* bufp = new char[len + 1];
	  set_alloc_checking_on();
	  vsnprintf(bufp, sizeof(buf), format, vl);
	  Dout(dc::bfd, buf);
	  set_alloc_checking_off();
	  delete [] bufp;
	  set_alloc_checking_on();
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
      typedef set<symbol_ct, symbol_key_greater> function_symbols_ct;

      // cwbfd::
      class object_file_ct {
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

      // Global object (but libcwd must stay independent of stuff in libcw/kernel, so we don't use Global<>)
      // cwbfd::
      typedef list<object_file_ct*> object_files_ct;
      // cwbfd::
      static char object_files_instance_[sizeof(object_files_ct)] __attribute__((__aligned__));
      // cwbfd::
      object_files_ct& object_files(void) { return *reinterpret_cast<object_files_ct*>(object_files_instance_); }

      // cwbfd::
      object_file_ct* find_object_file(void const* addr)
      {
	object_files_ct::iterator i(object_files().begin());
	for(; i != object_files().end(); ++i)
	  if ((*i)->get_lbase() < addr && (char*)(*i)->get_lbase() + (*i)->size() > addr)
	    break;
	return (i != object_files().end()) ? (*i) : NULL;
      }

      // cwbfd::
      object_file_ct* find_object_file(bfd const* abfd)
      {
	object_files_ct::iterator i(object_files().begin());
	for(; i != object_files().end(); ++i)
	  if ((*i)->get_bfd() == abfd)
	    break;
	return (i != object_files().end()) ? (*i) : NULL;
      }

      // cwbfd::
      struct symbol_less {
	bool operator()(asymbol const* a, asymbol const* b) const
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
      };

      // cwbfd::
      void* const unknown_l_addr = (void*)-1;

      // cwbfd::
      object_file_ct::object_file_ct(char const* filename, void* base) : lbase(base)
      {
#ifdef DEBUGDEBUGMALLOC
	ASSERT( libcw::debug::_internal_::internal );
#endif

	abfd = bfd_openr(filename, NULL);
#ifdef DEBUGUSEGNULIBBFD
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
#ifdef DEBUGUSEGNULIBBFD
	  DoutFatal(dc::bfd, filename << ": can not get addresses from archive: " << bfd_errmsg(bfd_get_error()));
#else
	  DoutFatal(dc::bfd, filename << ": can not get addresses from archive.");
#endif
	}
#ifdef DEBUGUSEGNULIBBFD
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
#ifdef DEBUGUSEGNULIBBFD
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
	AllocTag_dynamic_description(symbol_table, "symbols of " << filename);

	number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
#ifdef DEBUGUSEGNULIBBFD
	if (number_of_symbols < 0)
	  DoutFatal(dc::bfd, "bfd_canonicalize_symtab: " << bfd_errmsg(bfd_get_error()));
#endif

	if (number_of_symbols > 0)
	{
#ifndef DEBUGUSEGNULIBBFD
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
#endif // DEBUGUSEGNULIBBFD

	  if (!s_end_vma && number_of_symbols > 0)
	    Dout(dc::warning, "Cannot find symbol _end");

	  // Guess the start of the shared object when ldd didn't return it.
	  if (lbase == unknown_l_addr)
	  {
#ifdef HAVE_DLOPEN
	    libcw::debug::_internal_::internal = false;
	    void* handle = ::dlopen(filename, RTLD_LAZY|RTLD_LOCAL|RTLD_NOLOAD);
	    if (!handle)
	    {
	      char const* dlerror_str = dlerror();
	      DoutFatal(dc::fatal, "::dlopen(" << filename << ", RTLD_LAZY|RTLD_LOCAL|RTLD_NOLOAD): " << dlerror_str);
	    }
	    char* val;
	    if (s_end_vma && (val = (char*)dlsym(handle, "_end")))	// dlsym will fail when _end is a local symbol.
	    {
	      libcw::debug::_internal_::internal = true;
	      lbase = val - s_end_vma;
	    }
	    else
#ifdef HAVE_LINK_H
            {
	      libcw::debug::_internal_::internal = true;
	      for(link_map* p = _dl_loaded; p; p = p->l_next)
	        if (!strcmp(p->l_name, filename))
		{
		  lbase = reinterpret_cast<void*>(p->l_addr);		// No idea why they use unsigned int for a load address.
		  break;
		}
	    }
#else // !HAVE_LINK_H
	    {
	      libcw::debug::_internal_::internal = true;
	      // Warning: the following code is black magic.
	      map<void*, unsigned int> start_values;
	      unsigned int best_count = 0;
	      void* best_start = 0;
	      for (asymbol** s = symbol_table; s <= &symbol_table[number_of_symbols - 1]; ++s)
	      {
		if ((*s)->name == 0 || ((*s)->flags & BSF_FUNCTION) == 0 || ((*s)->flags & (BSF_GLOBAL|BSF_WEAK)) == 0)
		  continue;
		asection const* sect = bfd_get_section(*s);
		if (sect->name[1] == 't' && !strcmp(sect->name, ".text"))
		{
		  libcw::debug::_internal_::internal = false;
		  void* val = dlsym(handle, (*s)->name);
		  if (dlerror() == NULL)
		  {
		    libcw::debug::_internal_::internal = true;
		    void* start = reinterpret_cast<char*>(val) - (*s)->value - sect->vma;
		    pair<map<void*, unsigned int>::iterator, bool> p = start_values.insert(pair<void* const, unsigned int>(start, 0));
		    if (++(*(p.first)).second > best_count)
		    {
		      best_start = start;
		      if (++best_count == 10)	// Its unlikely that even more than 2 wrong values will have the same value.
			break;			// So if we reach 10 then this is value we are looking for.
		    }
		  }
		  else
		    libcw::debug::_internal_::internal = true;
		}
	      }
	      if (best_count < 3)
	      {
		Dout(dc::warning, "Unable to determine start of \"" << filename << "\", skipping.");
		free(symbol_table);
		symbol_table  = NULL;
		bfd_close(abfd);
		number_of_symbols = 0;
		libcw::debug::_internal_::internal = false;
		::dlclose(handle);
		libcw::debug::_internal_::internal = true;
		return;
	      }
	      lbase = best_start;
	    }
#endif // !HAVE_LINK_H
	    libcw::debug::_internal_::internal = false;
	    ::dlclose(handle);
	    libcw::debug::_internal_::internal = true;
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
	  sort(symbol_table, &symbol_table[number_of_symbols], symbol_less());

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
	  object_files().push_back(this);
      }

      // cwbfd::
      object_file_ct::~object_file_ct()
      {
        list<object_file_ct*>::iterator iter(find(object_files().begin(), object_files().end(), this));
	if (iter != object_files().end())
	  object_files().erase(iter);
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
      string* argv0_ptr;
      // cwbfd::
      string const* pidstr_ptr;

      // cwbfd::
      int decode_ps(char const* buf, size_t len)
      {
	static int pid_token = 0;
	static int command_token = 0;
	static size_t command_column;
	int current_token = 0;
	bool found_PID = false;
	bool eating_token = false;
	size_t current_column = 1;
	string token;

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
	      if (pid_token == current_token && token == *pidstr_ptr)
		found_PID = true;
	      else if (found_PID && (command_token == current_token || current_column >= command_column))
	      {
		*argv0_ptr = token + '\0';
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
      void get_full_path_to_executable(string& result)
      {
	string argv0;		// Like main()s argv[0], thus must be zero terminated.
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
	ifstream proc_file(proc_path);

	if (proc_file)
	{
	  proc_file >> argv0;
	  proc_file.close();
	}
	else
	{
	  string pidstr;

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

	  argv0_ptr = &argv0;		// Ugly way to pass these strings to decode_ps:
	  pidstr_ptr = &pidstr;		// pidstr is input, argv0 is output.

	  if (exec_prog(ps_prog, argv, environ, decode_ps) == -1 || argv0.empty())
	    DoutFatal(dc::fatal|error_cf, "Failed to execute \"" << ps_prog << "\"");
	}

	if (argv0.find('/') == string::npos)
	{
	  string prog_name(argv0);
	  string path_list(getenv("PATH"));
	  string::size_type start_pos = 0, end_pos;
	  string path;
	  struct stat finfo;
	  prog_name += '\0';
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
	    if (end_pos == string::npos)
	      break;
	    start_pos = end_pos + 1;
	  }
	}

	char full_path_buf[MAXPATHLEN];
	char* full_path = realpath(argv0.data(), full_path_buf);

	if (!full_path)
	  DoutFatal(dc::fatal|error_cf, "realpath(\"" << argv0.data() << "\", full_path_buf)");

	Dout(dc::debug, "Full path to executable is \"" << full_path << "\".");
	result = full_path;
      }

      // cwbfd::
      static bool initialized = false;

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
      vector<my_link_map> shared_libs;

      // cwbfd::
      int decode(char const* buf, size_t len)
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
	      shared_libs.push_back(my_link_map(p, q - p, unknown_l_addr));
	      break;
	    }
	    for (char const* r = q; r < &buf[len]; ++r)
	      if (r[0] == '(' && r[1] == '0' && r[2] == 'x')
	      {
		char* s;
		void* addr = reinterpret_cast<void*>(strtol(++r, &s, 0));
		shared_libs.push_back(my_link_map(p, q - p, addr));
		break;
	      }
	    break;
	  }
	return 0;
      }

#ifdef DEBUGDEBUGBFD
      // cwbfd::
      void dump_object_file_symbols(object_file_ct const* object_file)
      {
	cout << setiosflags(ios_base::left) << setw(15) << "Start address" << setw(50) << "File name" << setw(20) << "Number of symbols" << endl;
	cout << "0x" << setfill('0') << setiosflags(ios_base::right) << setw(8) << hex << (unsigned long)object_file->get_lbase() << "     ";
	cout << setfill(' ') << setiosflags(ios_base::left) << setw(50) << object_file->get_bfd()->filename;
	cout << dec << setiosflags(ios_base::left);
	cout << object_file->get_number_of_symbols() << endl;

	cout << setiosflags(ios_base::left) << setw(12) << "Start";
	cout << ' ' << setiosflags(ios_base::right) << setw(6) << "Size" << ' ';
	cout << "Name value flags\n";
	asymbol** symbol_table = object_file->get_symbol_table();
	for (long n = object_file->get_number_of_symbols() - 1; n >= 0; --n)
	{
	  cout << setiosflags(ios_base::left) << setw(12) << (void*)symbol_start_addr(symbol_table[n]);
	  cout << ' ' << setiosflags(ios_base::right) << setw(6) << symbol_size(symbol_table[n]) << ' ';
	  cout << symbol_table[n]->name << ' ' << (void*)symbol_table[n]->value << ' ' << oct << symbol_table[n]->flags << endl;
	}
      }
#endif

      // cwbfd::
      object_file_ct* load_object_file(char const* name, void* l_addr)
      {
	ASSERT( libcw::debug::_internal_::internal );
        if (l_addr == unknown_l_addr)
	  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug info from " << name << ' ');
	else if (l_addr == 0)
	  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug info from " << name << "... ");
	else
	  Dout(dc::bfd|continued_cf|flush_cf, "Loading debug info from " << name << " (" << l_addr << ") ... ");
	object_file_ct* object_file = new object_file_ct(name, l_addr);
	if (object_file->get_number_of_symbols() > 0)
	{
	  Dout(dc::finish, "done (" << dec << object_file->get_number_of_symbols() << " symbols)");
#ifdef DEBUGDEBUGBFD
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
      int init(void)
      {
	static bool being_initialized = false;
	// This should catch it when we call new or malloc while 'internal'.
	if (being_initialized)
	{
#ifdef DEBUGMALLOC
	  libcw::debug::_internal_::internal = false;
#endif
	  DoutFatal(dc::core, "Bug in libcwd: libcw_bfd_init() called twice or recursively entering itself!  Please submit a full bug report to libcw@alinoe.com.");
	}
	being_initialized = true;

#if defined(DEBUGDEBUG) && defined(DEBUGMALLOC)
	// First time we get here, this string is intialized - this must be with `internal' off!
	static bool second_time = false;
	if (!second_time)
	{
	  second_time = true;
	  ASSERT( !libcw::debug::_internal_::internal );
	}
#endif

	// ****************************************************************************
	// Start INTERNAL!
	set_alloc_checking_off();

#ifdef DEBUGMALLOC
	// Initialize the malloc library if not done yet.
	init_debugmalloc();
#endif

#ifdef ALWAYS_PRINT_LOADING
	// We want debug output to BFD
	bool const libcwd_was_off =
#ifdef DEBUGDEBUG
	  false;
#else
	  true;
#endif
	if (libcwd_was_off)
	  Debug( libcw_do.on() );
	bool bfd_was_off;
	Debug( bfd_was_off = !dc::bfd.is_on() );
	if (bfd_was_off)
	  Debug( dc::bfd.on() );
#endif

	// Initialize object files list
	new (object_files_instance_) object_files_ct;

#ifdef DEBUGUSEGNULIBBFD
	bfd_init();
#endif

	// Get the full path and name of executable
	struct non_alloc_checking_string_st {
	  string* value;
          non_alloc_checking_string_st(void) { value = new string; }	// alloc checking already off.
	  ~non_alloc_checking_string_st() { set_alloc_checking_off(); delete value; set_alloc_checking_on(); }
        };
	static non_alloc_checking_string_st fullpath;	// Must be static because bfd keeps a pointer to its data()
	get_full_path_to_executable(*fullpath.value);
	*fullpath.value += '\0';			// Make string null terminated so we can use data().

#ifdef DEBUGUSEGNULIBBFD
	bfd_set_error_program_name(fullpath.value->data() + fullpath.value->find_last_of('/') + 1);
	bfd_set_error_handler(error_handler);
#endif

	// Load executable
	load_object_file(fullpath.value->data(), 0);

	// Load all shared objects
#ifndef HAVE_LINK_H
	// Path to `ldd'
	char const ldd_prog[] = "/usr/bin/ldd";

	char const* argv[3];
	argv[0] = "ldd";
	argv[1] = fullpath.value->data();
	argv[2] = NULL;
	exec_prog(ldd_prog, argv, environ, decode);

	for(vector<my_link_map>::iterator iter = shared_libs.begin(); iter != shared_libs.end(); ++iter)
	{
	  my_link_map* l = &(*iter);
#else
	for(link_map* l = _dl_loaded; l; l = l->l_next)
	{
#endif
	  if (l->l_addr)
	    load_object_file(l->l_name, reinterpret_cast<void*>(l->l_addr));
	}
	object_files().sort(object_file_greater());

#ifdef ALWAYS_PRINT_LOADING
	if (libcwd_was_off)
	  Debug( libcw_do.off() );
	if (bfd_was_off)
	  Debug( dc::bfd.off() );
#endif

	initialized = true;

	set_alloc_checking_on();
	// End INTERNAL!
	// ****************************************************************************

#ifdef DEBUGDEBUGBFD
	// Dump all symbols
	for (object_files_ct::reverse_iterator i(object_files().rbegin()); i != object_files().rend(); ++i)
          dump_object_file_symbols(*i);
#endif

	return 0;
      }

      // cwbfd::
      symbol_ct const* pc_symbol(bfd_vma addr, object_file_ct* object_file)
      {
	static asymbol dummy_symbol;
	static asection dummy_section;
	if (object_file)
	{
	  // Make symbol_start_addr(&dummy_symbol) and symbol_size(&dummy_symbol) return the correct value
	  bfd_asymbol_bfd(&dummy_symbol) = object_file->get_bfd();
	  dummy_symbol.section = &dummy_section;	// Has dummy_section.vma == 0.  Use dummy_symbol.value to store (value + vma):
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

    char const* const unknown_function_c = "<unknown function>";

    //
    // Find the mangled function name of the address `addr'.
    //
    char const* pc_mangled_function_name(void const* addr)
    {
      using namespace cwbfd;

      if (!initialized)
	init();

      symbol_ct const* symbol = pc_symbol((bfd_vma)(size_t)addr, find_object_file(addr));

      if (!symbol)
	return unknown_function_c;

      return symbol->get_symbol()->name;
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
    void location_ct::M_pc_location(void const* addr)
    {
      using namespace cwbfd;

      if (!initialized)
      {
#ifdef __GLIBCPP__	// Pre libstdc++ v3, there is no malloc done for initialization of cerr.
        if (!_internal_::ios_base_initialized && _internal_::inside_ios_base_Init_Init())
	{
	  M_filepath = NULL;
	  M_func = "<pre ios initialization>";
	  return;
	}
#endif
	init();
      }

      object_file_ct* object_file = find_object_file(addr);
#ifdef HAVE_LINK_H
      if (!object_file)
      {
	set_alloc_checking_off();
        // Try to load everything again... previous loaded libraries are skipped based on load address.
	for(link_map* l = _dl_loaded; l;)
	{
	  if (l->l_addr)
	  {
	    for (object_files_ct::iterator iter = object_files().begin(); iter != object_files().end(); ++iter)
	      if (reinterpret_cast<void*>(l->l_addr) == (*iter)->get_lbase())
		goto already_loaded;
	    load_object_file(l->l_name, reinterpret_cast<void*>(l->l_addr));
	  }
already_loaded:
	  l = l->l_next;
	}
	object_files().sort(object_file_greater());
	set_alloc_checking_on();
	object_file = find_object_file(addr);
      }
#endif
      if (!object_file)
      {
        Dout(dc::bfd, "No object file for address " << addr);
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
	ASSERT( object_file->get_bfd() == abfd );
	set_alloc_checking_off();
#ifdef DEBUGUSEGNULIBBFD
	bfd_find_nearest_line(abfd, const_cast<asection*>(sect), const_cast<asymbol**>(object_file->get_symbol_table()),
	    (char*)addr - (char*)object_file->get_lbase() - sect->vma, &file, &M_func, &M_line);
#else
        abfd->find_nearest_line(p, (char*)addr - (char*)object_file->get_lbase(), &file, &M_func, &M_line);
#endif
	set_alloc_checking_on();
	ASSERT( !(M_func && !p->name) );	// Please inform the author of libcwd if this assertion fails.
	M_func = p->name;

	if (file && M_line)			// When line is 0, it turns out that `file' is nonsense.
	{
	  size_t len = strlen(file);
	  // `file' is allocated by `bfd_find_nearest_line', however - it is also libbfd
	  // that will free `file' again a second call to `bfd_find_nearest_line' (for the
	  // same value of `abfd': the allocated pointer is stored in a structure
	  // that is kept for each bfd seperately).
	  // The call to `new char [len + 1]' below could cause this function (pc_location)
	  // to be called again (in order to store the file:line where the allocation
	  // is done) and thus a new call to `bfd_find_nearest_line', which then would
	  // free `file' before we copy it!
	  // Therefore we need to call `set_alloc_checking_off', to prevent this.
	  set_alloc_checking_off();
	  M_filepath = new char [len + 1];
	  set_alloc_checking_on();
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
	      set_alloc_checking_off();
	      {
		string demangled_name;
		demangle_symbol(p->name, demangled_name);
		set_alloc_checking_on();
		char const* ofn = strrchr(abfd->filename, '/');
		Dout(dc::bfd, "Warning: Address " << addr << " in section " << sect->name <<
		    " of object file \"" << (ofn ? ofn + 1 : abfd->filename) << '"');
		Dout(dc::bfd|blank_label_cf|blank_marker_cf, "does not have a line number, perhaps the unit containing the function");
  #ifdef __FreeBSD__
		Dout(dc::bfd|blank_label_cf|blank_marker_cf, '`' << demangled_name << "' wasn't compiled with flag -ggdb?");
  #else
		Dout(dc::bfd|blank_label_cf|blank_marker_cf, '`' << demangled_name << "' wasn't compiled with flag -g?");
  #endif
		set_alloc_checking_off();
	      }
	      set_alloc_checking_on();
	    }
	  }
	  else
	    Dout(dc::bfd, "Warning: Address in section " << sect->name <<
	        " of object file \"" << abfd->filename << "\" does not contain a function.");
	}
	else
	  Dout(dc::bfd, "address " << addr << dec << " corresponds to " << *this);
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

    void location_ct::clear(void)
    {
      if (M_filepath)
      {
	set_alloc_checking_off();
	delete [] M_filepath;
	M_filepath = NULL;
	set_alloc_checking_on();
      }
      M_func = "<cleared location_ct>";
    }

    location_ct::location_ct(location_ct const &prototype) : M_filepath(NULL)
    {
      if (prototype.M_filepath)
      {
	set_alloc_checking_off();
	M_filepath = new char [strlen(prototype.M_filepath) + 1];
	set_alloc_checking_on();
	strcpy(M_filepath, prototype.M_filepath);
	M_filename = M_filepath + (prototype.M_filename - prototype.M_filepath);
	M_line = prototype.M_line;
	M_func = prototype.M_func;
      }
    }

    void location_ct::move(location_ct& prototype)
    {
      ASSERT( !this->is_known() );
      M_filepath = prototype.M_filepath;
      M_filename = M_filepath + (prototype.M_filename - prototype.M_filepath);
      M_line = prototype.M_line;
      M_func = prototype.M_func;
      prototype.M_filepath = NULL;
      prototype.M_func = "<moved location_ct>";
    }

    location_ct& location_ct::operator=(location_ct const &prototype)
    {
      if (this != &prototype)
      {
	clear();
	if (prototype.M_filepath)
	{
	  set_alloc_checking_off();
	  M_filepath = new char [strlen(prototype.M_filepath) + 1];
	  set_alloc_checking_on();
	  strcpy(M_filepath, prototype.M_filepath);
	  M_filename = M_filepath + (prototype.M_filename - prototype.M_filepath);
	  M_line = prototype.M_line;
	  M_func = prototype.M_func;
	}
      }
      return *this;
    }

    ostream& operator<<(ostream& os, location_ct const& location)
    {
      if (location.M_filepath)
	os << location.M_filename << ':' << location.M_line;
      else
	os << "<unknown location>";
      return os;
    }

  } // namespace debug
} // namespace libcw

#ifdef CWDEBUG_DLOPEN_DEFINED
using namespace libcw::debug;

struct dlloaded_st {
  cwbfd::object_file_ct* M_object_file;
  int M_flags;
  dlloaded_st(cwbfd::object_file_ct* object_file, int flags) : M_object_file(object_file), M_flags(flags) { }
};

typedef map<void*, dlloaded_st> libcwd_dlopen_map_type;
static libcwd_dlopen_map_type libcwd_dlopen_map;

extern "C" {
  void* __libcwd_dlopen(char const* name, int flags)
  {
    ASSERT( !libcw::debug::_internal_::internal );
    void* handle = ::dlopen(name, flags);
    if ((flags & RTLD_NOLOAD))
      return handle;
    set_alloc_checking_off();
    cwbfd::object_file_ct* object_file = cwbfd::load_object_file(name, cwbfd::unknown_l_addr);
    set_alloc_checking_on();
    if (object_file)
    {
      cwbfd::object_files().sort(cwbfd::object_file_greater());
      libcwd_dlopen_map.insert(pair<void* const, dlloaded_st>(handle, dlloaded_st(object_file, flags)));
    }
    return handle;
  }

  int __libcwd_dlclose(void *handle)
  {
    ASSERT( !libcw::debug::_internal_::internal );
    int ret = ::dlclose(handle);
    libcwd_dlopen_map_type::iterator iter(libcwd_dlopen_map.find(handle));
    if (iter != libcwd_dlopen_map.end())
    {
      if (!((*iter).second.M_flags & RTLD_NODELETE))
      {
        set_alloc_checking_off();
	delete (*iter).second.M_object_file;
        set_alloc_checking_on();
      }
      libcwd_dlopen_map.erase(iter);
    }
    return ret;
  }
} // extern "C"

#endif // CWDEBUG_DLOPEN_DEFINED

#endif // !DEBUGUSEBFD
