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

/** \file libcwd/private_internal_stringbuf.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_INTERNAL_STRINGBUF_H
#define LIBCWD_PRIVATE_INTERNAL_STRINGBUF_H

#ifndef LIBCWD_CONFIG_H
#include "config.h"
#endif
#ifndef LIBCWD_PRIVATE_ALLOCATOR_H
#include "private_allocator.h"
#endif
#if CWDEBUG_DEBUGM && !defined(LIBCWD_PRIVATE_STRUCT_TSD_H)
#include "private_struct_TSD.h"
#endif
#ifndef LIBCW_SSTREAM
#define LIBCW_SSTREAM
#include <sstream>
#endif

namespace libcwd {
  namespace _private_ {

#if CWDEBUG_ALLOC
typedef ::std::basic_stringbuf<char, ::std::char_traits<char>, ::libcwd::_private_::auto_internal_allocator> auto_internal_stringbuf;
#else
typedef ::std::stringbuf auto_internal_stringbuf;
#endif

  } // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_INTERNAL_STRINGBUF_H
