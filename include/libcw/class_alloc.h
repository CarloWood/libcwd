// $Header$
//
// Copyright (C) 2000 - 2002, by
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
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_CLASS_ALLOC_H
#define LIBCW_CLASS_ALLOC_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif

#ifndef LIBCW_CLASS_MEMBLK_TYPES_H
#include <libcw/enum_memblk_types.h>		// Needed for memblk_types_nt.
#endif
#ifndef LIBCW_LOCKABLE_AUTO_PTR_H
#include <libcw/lockable_auto_ptr.h>		// Needed for lockable_auto_ptr<char, true>.
#endif
#if defined(DEBUGUSEBFD) && !defined(LIBCW_CLASS_LOCATION_H)
#include <libcw/class_location.h>		// Needed for location_ct.
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>				// Needed for size_t.
#endif

namespace libcw {
  namespace debug {

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
  void const* a_start;        			//!< Duplicate of (original) memblk_key_ct.
  size_t a_size;              			//!< Duplicate of (original) memblk_key_ct.
  memblk_types_nt a_memblk_type;		//!< A flag which indicates the type of allocation.
  type_info_ct const* type_info_ptr;		//!< Type info of related object.
  lockable_auto_ptr<char, true> a_description;	//!< A label describing this memblk.
#ifdef DEBUGUSEBFD
  location_ct M_location;			//!< Source file, function and line number from where the allocator was called from.
#endif

public:
  /**
   * \brief The allocated size in bytes.
   */
  size_t size(void) const { return a_size; }

  /**
   * \brief A pointer to the start of the allocated memory block.
   */
  void const* start(void) const { return a_start; }

  /**
   * \brief A flag indicating the type of allocation.
   */
  memblk_types_nt memblk_type(void) const { return a_memblk_type; }

  /**
   * \brief A reference to the type info of the pointer to the allocated memory block.
   *
   * \returns a reference to the static \ref type_info_ct object that is returned
   * by a call to \ref libcw::debug::type_info_of "type_info_of"(p1).&nbsp;
   * Where \p p1 is the first parameter that was passed to \ref AllocTag().
   */
  type_info_ct const& type_info(void) const { return *type_info_ptr; }

  /**
   * \brief A pointer to a description of the allocated memory block.
   *
   * This is a character string that is the result of writing the second parameter of \ref AllocTag() to an ostrstream.
   */
  char const* description(void) const { return a_description.get(); }

#ifdef DEBUGUSEBFD
  /**
   * \brief The source file location that the allocator was called from.
   *
   * \returns a non-const \ref location_ct reference corresponding to the place where the allocation was done.&nbsp;
   * Class \ref location_ct describes a source file and line number location and in which function that location resides.&nbsp;
   * \sa \ref chapter_alloc_locations
   */
  location_ct& location_reference(void) { return M_location; }

  /**
   * \brief The source file location that the allocator was called from.
   *
   * \returns a const \ref location_ct reference corresponding to the place where the allocation was done.&nbsp;
   * Class \ref location_ct describes a source file and line number location and in which function that location resides.&nbsp;
   * \sa \ref chapter_alloc_locations
   */
  location_ct const& location(void) const { return M_location; }
#endif

protected:
  /**
   * \brief Construct an \c alloc_ct object with attributes \a s, \a sz, \a type and \a ti.
   * \internal
   */
  alloc_ct(void const* s, size_t sz, memblk_types_nt type, type_info_ct const& ti) :
      a_start(s), a_size(sz), a_memblk_type(type), type_info_ptr(&ti) { }

  /**
   * \brief Destructor.
   * \internal
   *
   * The destructor is virtual because libcwd really creates dm_alloc_ct
   * objects, objects \em derived from alloc_ct, using <code>operator new</code>.
   */
  virtual ~alloc_ct() { }
};

  } //namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_ALLOC_H
