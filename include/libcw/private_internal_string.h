// $Header$
//
// Copyright (C) 2001 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcw/private_internal_string.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_INTERNAL_STRING_H
#define LIBCW_PRIVATE_INTERNAL_STRING_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_PRIVATE_ALLOCATOR_H
#include <libcw/private_allocator.h>
#endif
#ifndef LIBCW_STRING
#define LIBCW_STRING
#include <string>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

// This is the string type that we use in Single Threaded internal functions.
#if CWDEBUG_ALLOC
#if __GNUC__ < 3
typedef ::std::basic_string<char, ::string_char_traits<char>, ST_internal_allocator> ST_internal_string;
#else
typedef ::std::basic_string<char, ::std::char_traits<char>, ST_internal_allocator> ST_internal_string;
#endif
#else
typedef ::std::string ST_internal_string;
#endif

// This is the string type that we use in Multi Threaded internal functions.
#if CWDEBUG_ALLOC
#if __GNUC__ < 3
typedef ::std::basic_string<char, ::string_char_traits<char>, internal_allocator> internal_string;
#else
typedef ::std::basic_string<char, ::std::char_traits<char>, internal_allocator> internal_string;
#endif
#else
typedef ::std::string internal_string;
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCW_PRIVATE_INTERNAL_STRING_H

