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

/** \file libcwd/private_debug_stack.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_DEBUG_STACK_H
#define LIBCWD_PRIVATE_DEBUG_STACK_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif

namespace libcwd {
  namespace _private_ {

// Stack implementation that doesn't have a constructor.
// The size of 64 should be MORE then enough.

template<typename T>		// T must be a builtin type.
  struct debug_stack_tst {
  private:
    T st[64];
    T* p;
    T* end;
  public:
    void init();
    void push(T ptr);
    void pop();
    T top() const;
    size_t size() const;
  };

  } // namespace _private_
}  // namespace libcwd

#endif // LIBCWD_PRIVATE_DEBUG_STACK_H
