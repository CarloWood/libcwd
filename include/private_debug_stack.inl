// $Header$
//
// Copyright (C) 2000 - 2004, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef PRIVATE_DEBUG_STACK_INL
#define PRIVATE_DEBUG_STACK_INL

#ifndef LIBCWD_PRIVATE_DEBUG_STACK_H
#include <libcwd/private_debug_stack.h>
#endif
#ifndef LIBCWD_PRIVATE_ASSERT_H
#include <libcwd/private_assert.h>
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif

namespace libcwd {
  namespace _private_ {

// Stack implementation that doesn't have a constructor.
// The size of 64 should be MORE then enough.

template<typename T>
  inline
  void
  debug_stack_tst<T>::init()
  {
    p = st - 1;
    end = st + 63;
  }

template<typename T>
  inline
  void
  debug_stack_tst<T>::push(T ptr)
  {
#if CWDEBUG_DEBUG
    LIBCWD_ASSERT( end != NULL );
#endif
    if (p == end)
      core_dump();	// This is really not normal, if you core here you probably did something wrong.
			// Doing a back trace in gdb should reveal an `infinite' debug output re-entrance loop.
			// This means that while printing debug output you call a function that makes
			// your program return to the same line, starting to print out that debug output
			// again. Try to break this loop somehow.
    *++p = ptr;
  }

extern void print_pop_error();

template<typename T>
  inline
  void
  debug_stack_tst<T>::pop()
  {
#if CWDEBUG_DEBUG
    LIBCWD_ASSERT( end != NULL );
#endif
    if (p == st - 1)
      print_pop_error();
    --p;
  }

template<typename T>
  inline
  T
  debug_stack_tst<T>::top() const
  {
#if CWDEBUG_DEBUG
    LIBCWD_ASSERT( end != NULL );
#endif
    return *p;
  }

template<typename T>
  inline
  size_t
  debug_stack_tst<T>::size() const
  {
#if CWDEBUG_DEBUG
    LIBCWD_ASSERT( end != NULL );
#endif
    return p - (st - 1);
  }

  } // namespace _private_
}  // namespace libcwd

#endif // PRIVATE_DEBUG_STACK_INL
