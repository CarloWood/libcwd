// $Header$
//
// Copyright (C) 2003 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include <libcwd/config.h>

#if CWDEBUG_LOCATION

#include "cwd_debug.h"
#include "cwd_bfd.h"
#ifndef LIBCWD_PRIVATE_THREADING_H
#include <libcwd/private_threading.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <regex.h>
#include <cstdlib>
#include <libcwd/class_function.h>

#include <iomanip>
extern "C" {
  pid_t getpid();
  size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
}

namespace libcwd {

void Function::M_init()
{
#if LIBCWD_THREAD_SAFE
  mutex_tct<_private_::function_instance>::lock();
  if (!M_initialized)
  {
#endif
    M_initialized = 1;

#if LIBCWD_THREAD_SAFE
  }
  mutex_tct<_private_::function_instance>::unlock();
#endif
}

void Function::M_init(Function& function)
{
#if LIBCWD_THREAD_SAFE
  mutex_tct<_private_::function_instance>::lock();
#endif
  M_initialized = 1;
  function.init();
  // FIXME std::copy(function.instances().begin(), function.instances().end(), std::back_inserter(M_vector));
#if LIBCWD_THREAD_SAFE
  mutex_tct<_private_::function_instance>::unlock();
#endif
}

void Function::M_init(char const* expr, unsigned int flags)
{
#if LIBCWD_THREAD_SAFE
  mutex_tct<_private_::function_instance>::lock();
  if (!M_initialized)
  {
#endif
    M_initialized = 1;

    using cwbfd::object_files_ct;
    using cwbfd::NEEDS_READ_LOCK_object_files;
    using cwbfd::function_symbols_ct;
    using cwbfd::symbol_ct;
    using cwbfd::BSF_FUNCTION;
    using cwbfd::symbol_start_addr;
    using cwbfd::symbol_size;
    using _private_::set_alloc_checking_off;
    using _private_::set_alloc_checking_on;

    LIBCWD_TSD_DECLARATION;

    regex_t re;

    struct timeval foo;
    gettimeofday(&foo, NULL);

    set_alloc_checking_off(LIBCWD_TSD);

    if ((flags & regexp))
    {
      int errcode = regcomp(&re, expr, REG_EXTENDED|REG_NOSUB);
      if (errcode)
      {
	int sz = regerror(errcode, &re, NULL, 0);
        char* errbuf = (char*)malloc(sz);			// Leaks memory.
	set_alloc_checking_on(LIBCWD_TSD);
	regerror(errcode, &re, errbuf, sz);
	location_ct loc0((char*)__builtin_return_address(0) + builtin_return_address_offset);
	location_ct loc1((char*)__builtin_return_address(1) + builtin_return_address_offset);
	location_ct loc2((char*)__builtin_return_address(2) + builtin_return_address_offset);
	Dout(dc::notice, "loc0 = " << loc0);
	Dout(dc::notice, "loc1 = " << loc1);
	Dout(dc::notice, "loc2 = " << loc2);
        DoutFatal(dc::core, "recomp() failed: " << errbuf);
      }
    }

    int count;

    LIBCWD_DEFER_CANCEL;
    BFD_ACQUIRE_READ_LOCK;
    count = 0;
    for (object_files_ct::const_reverse_iterator i(NEEDS_READ_LOCK_object_files().rbegin());
	 i != NEEDS_READ_LOCK_object_files().rend();
	 ++i)
    {
      function_symbols_ct const& function_symbols((*i)->get_function_symbols());
      for(function_symbols_ct::const_iterator i2(function_symbols.begin());
	  i2 != function_symbols.end(); ++i2)
      {
	static unsigned int const setflags = BSF_FUNCTION;
	symbol_ct const& symbol(*i2);
	if ((symbol.get_symbol()->flags & setflags) == setflags)
	{
	  bool matched = false;
	  char const* name = symbol.get_symbol()->name;
	  if (name[0] == '_' && name[1] == 'Z')
	  {
	    if ((flags & cpp_linkage))
	    {
	      if ((flags & mangled))
	      {
		if (!strcmp(name, expr))
		{
		  matched = true;
		  ++count;
		}
	      }
	      else if ((flags & regexp))
	      {
		std::string output;
		demangle_symbol(name, output);
		if ((flags & regexp))
		{
		  if (!regexec(&re, output.c_str(), 0, NULL, 0))
		  {
		    matched = true;
		    ++count;
		  }
		}
	      }
	      else
	      {
		if (!strcmp(name, expr))
		{
		  matched = true;
		  ++count;
		}
	      }
	    }
	  }
	  else if ((flags & c_linkage))
	  {
	    if ((flags & regexp))
	    {
	      if (!regexec(&re, name, 0, NULL, 0))
	      {
		matched = true;
		++count;
	      }
	    }
	    else
	    {
	      if (!strcmp(name, expr))
	      {
		matched = true;
		++count;
	      }
	    }
	  }

	  if (matched)
	  {
	    std::string output;
	    demangle_symbol(name, output);
	    std::cout << output << " ; object file: " << (*i)->get_object_file()->filename() <<
		" ; start: " << (void*)symbol_start_addr(symbol.get_symbol()) <<
		" ; size: " << symbol_size(symbol.get_symbol()) << '\n';
	  }
	}
      }
    }
    BFD_RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL;

    if ((flags & regexp))
      regfree(&re);
    set_alloc_checking_on(LIBCWD_TSD);

    if (!(flags & nofail) && count == 0)
      DoutFatal(dc::fatal, "Function initialization does not match any function.");

    struct timeval foo2;
    gettimeofday(&foo2, NULL);
    std::cout << "Time used: " << ((foo2.tv_sec * 1000 + foo2.tv_usec / 1000) - (foo.tv_sec * 1000 + foo.tv_usec / 1000)) / 1000.0 << " seconds.\n";
    std::cout << "Number of symbols: " << count << '\n';

#if LIBCWD_THREAD_SAFE
  }
  mutex_tct<_private_::function_instance>::unlock();
#endif
}

} // namespace libcwd

#endif // CWDEBUG_LOCATION
