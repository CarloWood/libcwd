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

/** \file libcw/private_internal_vector.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_INTERNAL_VECTOR_H
#define LIBCW_PRIVATE_INTERNAL_VECTOR_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_PRIVATE_ALLOCATOR_H
#include <libcw/private_allocator.h>
#endif
#ifndef LIBCW_VECTOR
#define LIBCW_VECTOR
#include <vector>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

// This is the vector type that we use in Multi Threaded internal functions.
template <typename T>
  struct internal_vector : public ::std::vector<T, typename internal_allocator::rebind<T>::other>
  { };

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCW_PRIVATE_INTERNAL_VECTOR_H

