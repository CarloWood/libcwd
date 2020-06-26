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

/** \file class_alloc.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_ALLOC_H
#define LIBCWD_CLASS_ALLOC_H

#ifndef LIBCWD_CONFIG_H
#include "config.h"
#endif

#ifndef LIBCWD_ENUM_MEMBLK_TYPES_H
#include "enum_memblk_types.h"		// Needed for memblk_types_nt.
#endif
#ifndef LIBCW_LOCKABLE_AUTO_PTR_H
#include "lockable_auto_ptr.h"		// Needed for lockable_auto_ptr<char, true>.
#endif
#if CWDEBUG_LOCATION && !defined(LIBCWD_CLASS_LOCATION_H)
#include "class_location.h"		// Needed for location_ct.
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>				// Needed for size_t.
#endif
#ifndef LIBCWD_SMART_PTR_H
#include "smart_ptr.h"
#endif
#ifndef LIBCW_SYS_TIME_H
#define LIBCW_SYS_TIME_H
#include <sys/time.h>				// Needed for struct timeval.
#endif

namespace libcwd {

// Forward declaration
class type_info_ct;

//===================================================================================================
//
// The class which describes allocated memory blocks.
//

/**
 * \brief An object of type alloc_ct contains information about one allocated memory block.
 * \ingroup group_finding
 */
class alloc_ct {
protected:
  void const* a_start;			//!< Duplicate of (original) memblk_key_ct.
  size_t a_size;			//!< Duplicate of (original) memblk_key_ct.
  memblk_types_nt a_memblk_type;		//!< A flag which indicates the type of allocation.
  type_info_ct const* type_info_ptr;		//!< Type info of related object.
  _private_::smart_ptr a_description;		//!< A label describing this memblk.
  struct timeval a_time;			//!< The time at which the memory was allocated.
#if CWDEBUG_LOCATION
  location_ct const* M_location;		//!< Pointer into the location cache, with the source file, function and line number from where the allocator was called from.
#endif

public:
  /**
   * \brief The allocated size in bytes.
   */
  size_t size() const { return a_size; }

  /**
   * \brief A pointer to the start of the allocated memory block.
   */
  void const* start() const { return a_start; }

  /**
   * \brief A flag indicating the type of allocation.
   */
  memblk_types_nt memblk_type() const { return a_memblk_type; }

  /**
   * \brief A reference to the type info of the pointer to the allocated memory block.
   *
   * \returns a reference to the static \ref type_info_ct object that is returned
   * by a call to \ref libcwd::type_info_of "type_info_of"(p1).&nbsp;
   * Where \p p1 is the first parameter that was passed to \ref AllocTag().
   */
  type_info_ct const& type_info() const { return *type_info_ptr; }

  /**
   * \brief A pointer to a description of the allocated memory block.
   *
   * This is a character string that is the result of writing the second parameter of \ref AllocTag() to an ostrstream.
   */
  char const* description() const { return a_description; }

  /**
   * \brief The time at which this allocation was made.
   *
   * \returns the time at which the memory was allocated, as returned by a call to gettimeofday.
   */
  struct timeval const& time() const { return a_time; }

#if CWDEBUG_LOCATION
  /**
   * \brief The source file location that the allocator was called from.
   *
   * \returns a const \ref location_ct reference corresponding to the place where the allocation was done.&nbsp;
   * Class \ref location_ct describes a source file and line number location and in which function that location resides.&nbsp;
   * \sa \ref chapter_alloc_locations
   */
  location_ct const& location() const { return *M_location; }
#endif

protected:
  /**
   * \brief Construct an \c alloc_ct object with attributes \a s, \a sz, \a type, \a ti, \a t and \a l.
   * \internal
   */
  alloc_ct(void const* s, size_t sz, memblk_types_nt type, type_info_ct const& ti, struct timeval const& t
#if CWDEBUG_LOCATION
      , location_ct const* l
#endif
      ) : a_start(s), a_size(sz), a_memblk_type(type), type_info_ptr(&ti), a_time(t)
#if CWDEBUG_LOCATION
      , M_location(l)
#endif
      , M_tagged(false)
      { }

  /**
   * \brief Destructor.
   * \internal
   *
   * The destructor is virtual because libcwd really creates dm_alloc_ct
   * objects, objects \em derived from alloc_ct, using <code>operator new</code>.
   */
  virtual ~alloc_ct() { }

  // For internal use:
private:
  bool M_tagged;				// Set when AllocTag et al was called.
public:
  bool is_tagged() const { return M_tagged; }
  void alloctag_called() { M_tagged = true; }
  void reset_type_info() { type_info_ptr = &unknown_type_info_c; }
};

} // namespace libcwd

#endif // LIBCWD_CLASS_ALLOC_H
