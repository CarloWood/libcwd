// $Header$
//
// Copyright (C) 2001 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcwd/private_internal_vector.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_PRIVATE_INTERNAL_VECTOR_H
#define LIBCWD_PRIVATE_INTERNAL_VECTOR_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif
#ifndef LIBCWD_PRIVATE_ALLOCATOR_H
#include <libcwd/private_allocator.h>
#endif
#ifndef LIBCW_VECTOR
#define LIBCW_VECTOR
#include <vector>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

// This is the vector type that we use in Multi Threaded internal functions.
#if CWDEBUG_ALLOC
template <typename T>
  struct internal_vector : public ::std::vector<T, typename internal_allocator::rebind<T>::other>
  { };
#else
template <typename T>
  struct internal_vector : public ::std::vector<T>
  { };
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCWD_PRIVATE_INTERNAL_VECTOR_H

