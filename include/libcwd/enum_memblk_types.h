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

/** \file libcwd/enum_memblk_types.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_ENUM_MEMBLK_TYPES_H
#define LIBCWD_ENUM_MEMBLK_TYPES_H

#ifndef LIBCWD_CONFIG_H
#include "libcwd/config.h"
#endif
#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>		// Needed for ostream.
#endif

namespace libcwd {

//===================================================================================================
// Flags used to mark the type of `memblk':
//

/**
 * \brief A flag indicating the type of allocation.
 *
 * This is returned by alloc_ct::memblk_type.
 * The flags <CODE>memblk_type_marker</CODE> and <CODE>memblk_type_deleted_marker</CODE>
 * only exist when libcwd was configured with \ref enable_marker.
 *
 * \sa alloc_ct
 */
// If you change this, then also edit `expected_from' in debugmalloc.cc!
enum memblk_types_nt {
  memblk_type_new,              ///< Allocated with <code>operator new</code>
  memblk_type_deleted,          ///< Deleted with <code>operator delete</code>
  memblk_type_new_array,        ///< Allocated with <code>operator new []</code>
  memblk_type_deleted_array,    ///< Deleted with <code>operator delete []</code>
  memblk_type_malloc,           ///< Allocated with <code>%calloc()</code> or <code>%malloc()</code>
  memblk_type_realloc,          ///< Reallocated with <code>%realloc()</code>
  memblk_type_freed,            ///< Freed with <code>%free()</code>
#if CWDEBUG_MARKER
  memblk_type_marker,           ///< A memory allocation marker
  memblk_type_deleted_marker,   ///< A deleted memory allocation marker
#endif
  memblk_type_external,		///< Externally allocated with <code>%malloc()</code> (no magic numbers!)
  // These four must be last (we use if (f >= memblk_type_posix_memalign) in debugmalloc.cc).
  memblk_type_posix_memalign,	///< Allocated with <code>posix_memalign()</code>
  memblk_type_aligned_alloc,	///< Allocated with <code>aligned_alloc()</code>
  memblk_type_memalign,		///< Allocated with <code>memalign()</code>
  memblk_type_valloc		///< Allocated with <code>valloc()</code>
};
extern	// new-line needed for buggy doxygen
    ::std::ostream& operator<<(std::ostream& os, memblk_types_nt);

} // namespace libcwd

#endif // LIBCWD_ENUM_MEMBLK_TYPES_H

