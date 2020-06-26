// $Header$
//
// Copyright (C) 2002 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcwd/class_alloc_filter.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_ALLOC_FILTER_H
#define LIBCWD_CLASS_ALLOC_FILTER_H

#ifndef LIBCWD_LIBRARIES_DEBUG_H
#error "Don't include <libcwd/class_alloc_filter.h> directly, include the appropriate \"debug.h\" instead."
#endif

#include "config.h"

#if CWDEBUG_LOCATION
#include "class_location.h"
#endif

#if CWDEBUG_ALLOC

#if CWDEBUG_LOCATION
#include "private_allocator.h"
#endif
#ifndef LIBCW_VECTOR
#define LIBCW_VECTOR
#include <vector>
#endif
#include <sys/time.h>

#endif // CWDEBUG_ALLOC

namespace libcwd {

/** \addtogroup group_alloc_format */
/** \{ */

/** \brief The type used for the formatting flags of an alloc_filter_ct object. */
typedef unsigned short int alloc_format_t;

#if CWDEBUG_LOCATION
alloc_format_t const show_path = 1;			//!< Show the full path of the locations where the allocation was made.
alloc_format_t const show_objectfile = 2;		//!< Show the name of the shared library that is responsible for the allocation.
alloc_format_t const show_function = 4;			//!< Show the mangled name of the function that allocated the memory.
#endif
#if CWDEBUG_ALLOC
alloc_format_t const show_time = 8;			//!< Show the time at which the allocation was made.
alloc_format_t const show_allthreads = 16;		//!< Show the allocations of all threads, not just the current thread.
#if CWDEBUG_LOCATION
alloc_format_t const format_mask = (show_time|show_path|show_objectfile|show_function|show_allthreads);
#else
alloc_format_t const format_mask = (show_time|show_allthreads);
#endif
#endif // CWDEBUG_ALLOC

/** \} */

#if CWDEBUG_ALLOC

unsigned int const hide_untagged = 32;			// Call hide_untagged_allocations() to set this flag.
unsigned int const hide_unknown_loc = 64;		// Call hide_unknown_locations() to set this flag.

class dm_alloc_base_ct;
class dm_alloc_copy_ct;
#if CWDEBUG_MARKER
class marker_ct;
#endif

/** \class alloc_filter_ct debugmalloc.h libcwd/debug.h
 * \brief An allocated-memory filter class.
 * \ingroup group_alloc_format
 *
 * An object of this type can be passed to \ref list_allocations_on containing formatting information
 * for the \link group_overview Overview Of Allocated Memory \endlink.
 * It can also be passed as argument to the constructor of a \ref marker_ct object in order to specify
 * which allocations should not be capatured by the marker (and optionally made invisible).
 */
class alloc_filter_ct {
private:
#if CWDEBUG_LOCATION		// No synchronization needed when not defined.
  static int S_next_id;		// MT: protected by list_allocations_instance
  static int S_id;		// MT: protected by list_allocations_instance
  int M_id;
#endif
  friend class ::libcwd::dm_alloc_base_ct;
  friend class ::libcwd::dm_alloc_copy_ct;
  alloc_format_t M_flags;
  struct timeval M_start;
  struct timeval M_end;
#if CWDEBUG_LOCATION
  typedef std::basic_string<char, std::char_traits<char>, _private_::auto_internal_allocator> string_type;
  typedef std::vector<string_type, _private_::auto_internal_allocator::rebind<string_type>::other> vector_type;
  vector_type M_objectfile_masks;
  vector_type M_sourcefile_masks;
  typedef std::vector<std::pair<string_type, string_type>,
      _private_::auto_internal_allocator::rebind<std::pair<string_type, string_type> >::other> vector_pair_type;
  vector_pair_type M_function_masks;
#endif
public:
  /** The timeval used when there is no actual limit set, either start or end. */
  static struct timeval const no_time_limit;
  /** \brief Construct a formatting object */
  alloc_filter_ct(alloc_format_t flags = 0);
  /** \brief Set the general formatting flags. */
  void set_flags(alloc_format_t flags);
  /** \brief Returns the flags as set with set_flags */
  alloc_format_t get_flags() const;
  /** \brief Returns the start time as passed with set_time_interval. */
  struct timeval get_time_start() const;
  /** \brief Returns the end time as passed with set_time_interval. */
  struct timeval get_time_end() const;
#if CWDEBUG_LOCATION
  /** \brief Returns a copy of the list of object file masks.
   *
   * Don't use this function in a loop.
   */
  std::vector<std::string> get_objectfile_list() const;

  /** \brief Returns a copy of the list of source file masks.
   *
   * Don't use this function in a loop.
   */
  std::vector<std::string> get_sourcefile_list() const;

  /** \brief Return a copy of the list of object-file/function pair masks.
   *
   * Don't use this function in a loop.
   */
  std::vector<std::pair<std::string, std::string> > get_function_list() const;
#endif

  /** \brief Select the time interval that should be shown.
   *
   * The time interval outside of which allocations will not be shown
   * in the allocated memory overview.  \a start is the earliest time shown
   * while \a end is the last time shown.
   *
   * \sa group_alloc_format
   */
  void set_time_interval(struct timeval const& start, struct timeval const& end);

#if CWDEBUG_LOCATION
  /** \brief Select which object files to hide in the Allocated Memory Overview.
   *
   * \a masks is a list of wildcard expressions ('*' matches anything) that are matched
   * against the name of the executable and all the shared library names (without their path).
   * Object files that match will be hidden from the \link group_overview overview of allocated memory \endlink.
   *
   * \sa group_alloc_format
   */
  void hide_objectfiles_matching(std::vector<std::string> const& masks);

  /** \brief Select which source files to hide in the Allocated Memory Overview.
   *
   * \a masks is a list of wildcard expressions ('*' matches anything) that are matched
   * against the source file, including path, of the location where the allocation took place.
   * Locations that match will be hidden from the \link group_overview overview of allocated memory \endlink.
   *
   * \sa group_alloc_format
   */
  void hide_sourcefiles_matching(std::vector<std::string> const& masks);

  /** \brief Select which object-file/function pairs to hide in the Allocator Memory Overview.
   *
   * \a masks is a list of pairs of masks.  The first mask is matched against the object-file name
   * of the location of the allocation, while the second mask is matched against the mangled
   * function name of that location.&nbsp;
   * An unknown object-file or \link libcwd::unknown_function_c function \endlink
   * does \em not cause a match.&nbsp;
   * If the object-file mask start with a '/' or with a '*' then it is matched against the
   * full path of the object file (including possible "<tt>../</tt>" parts, it is matched against what is
   * returned by <code>location_ct::objectfile()-&gt;filepath()</code>.&nbsp;
   * Otherwise it is matched against just the name of the loaded executable or shared library
   * (with path stripped off).
   *
   * \sa libcwd::location_ct::mangled_function_name,
   *     libcwd::location_ct::object_file,
   *     libcwd::object_file_ct
   */
  void hide_functions_matching(std::vector<std::pair<std::string, std::string> > const& masks);
#endif

  /** \brief Only show the allocations for which a AllocTag was added in the code.
   *
   * When \a hide is true, only allocations for which an \link group_annotation AllocTag \endlink
   * was seen are showed in the \link group_overview overview of allocated memory \endlink
   * otherwise all allocations are being showed.
   */
  void hide_untagged_allocations(bool hide = true) { if (hide) M_flags |= hide_untagged; else M_flags &= ~hide_untagged; }

  /** \brief Only show the allocations for which a source file and line number could be found.
   *
   * When \a hide is true, only allocations for which a location could be resolved
   * are shown.  This is the only way to get rid of allocations done in libraries
   * without debugging information like a stripped glibc.
   */
  void hide_unknown_locations(bool hide = true) { if (hide) M_flags |= hide_unknown_loc; else M_flags &= ~hide_unknown_loc; }

#if CWDEBUG_LOCATION
  // Return true if filepath matches one of the masks in M_source_masks.
  _private_::hidden_st check_hide(char const* filepath) const;

  // Return true if object_file/mangled_function_name matches one of the mask pairs in M_function_masks.
  _private_::hidden_st check_hide(object_file_ct const* object_file, char const* mangled_function_name) const;
#endif

private:
  friend unsigned long list_allocations_on(debug_ct&, alloc_filter_ct const&);
#if CWDEBUG_MARKER
  friend class marker_ct;
#endif
#if CWDEBUG_LOCATION
  void M_check_synchronization() const { if (M_id != S_id) M_synchronize(); }
  void M_synchronize() const;
  void M_synchronize_locations() const;
#endif
};

#endif // CWDEBUG_ALLOC

} // namespace libcwd

#endif // LIBCWD_CLASS_ALLOC_FILTER_H
