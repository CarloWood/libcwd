// $Header$
//
// Copyright (C) 2002 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcwd/class_ooam_filter.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_CLASS_OOAM_FILTER_H
#define LIBCWD_CLASS_OOAM_FILTER_H

#ifndef LIBCWD_DEBUG_H
#error "Don't include <libcwd/class_ooam_filter.h> directly, include the appropriate \"debug.h\" instead."
#endif

#include <libcwd/config.h>

#if CWDEBUG_ALLOC
namespace libcw {
  namespace debug {

/** \addtogroup group_alloc_format */
/** \{ */

/** \brief The type used for the formatting flags of an ooam_filter_ct object */
typedef unsigned short int ooam_format_t;

ooam_format_t const show_time = 1;			//!< Show the time at which the allocation was made.
#if CWDEBUG_LOCATION
ooam_format_t const show_path = 2;			//!< Show the full path of the locations where the allocation was made.
ooam_format_t const show_objectfile = 4;		//!< Show the name of the shared library that is responsible for the allocation.
#endif
ooam_format_t const show_allthreads = 8;		//!< Show the allocations of all threads, not just the current thread.
#if CWDEBUG_LOCATION
ooam_format_t const format_mask = (show_time|show_path|show_objectfile|show_allthreads);
#else
ooam_format_t const format_mask = (show_time|show_allthreads);
#endif

/** \} */

unsigned int const hide_untagged = 16;			// Call hide_untagged_allocations() to set this flag.
unsigned int const hide_unknown_loc = 32;		// Call hide_unknown_locations() to set this flag.

class dm_alloc_base_ct;
class dm_alloc_copy_ct;
#if CWDEBUG_MARKER
class marker_ct;
#endif

/** \class ooam_filter_ct debugmalloc.h libcwd/debug.h
 * \ingroup group_alloc_format
 *
 * The object passed to \ref list_allocations_on containing formatting information
 * for the \link group_overview Overview Of Allocated Memory \endlink.
 */
class ooam_filter_ct {
private:
#if CWDEBUG_LOCATION		// No synchronization needed when not defined.
  static int S_next_id;		// MT: protected by list_allocations_instance
  static int S_id;		// MT: protected by list_allocations_instance
  int M_id;
#endif
  friend class ::libcw::debug::dm_alloc_base_ct;
  friend class ::libcw::debug::dm_alloc_copy_ct;
  ooam_format_t M_flags;
  struct timeval M_start;
  struct timeval M_end;
#if CWDEBUG_LOCATION
  std::vector<std::string> M_objectfile_masks;
  std::vector<std::string> M_sourcefile_masks;
#endif
public:
  /** The timeval used when there is no actual limit set, either start or end. */
  static struct timeval const no_time_limit;
  /** \brief Construct a formatting object */
  ooam_filter_ct(ooam_format_t flags = 0);
  /** \brief Set the general formatting flags. */
  void set_flags(ooam_format_t flags);
  /** \brief Returns the flags as set with set_flags */
  ooam_format_t get_flags(void) const;
  /** \brief Returns the start time as passed with set_time_interval. */
  struct timeval get_time_start(void) const;
  /** \brief Returns the end time as passed with set_time_interval. */
  struct timeval get_time_end(void) const;
#if CWDEBUG_LOCATION
  /** \brief Returns the list of object file masks. */
  std::vector<std::string> get_objectfile_list(void) const;
  /** \brief Returns the list of source file masks. */
  std::vector<std::string> get_sourcefile_list(void) const;
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
#endif

private:
  friend void list_allocations_on(debug_ct&, ooam_filter_ct const&);
#if CWDEBUG_MARKER
  friend class marker_ct;
#endif
#if CWDEBUG_LOCATION
  void M_check_synchronization(void) const { if (M_id != S_id) M_synchronize(); }
  void M_synchronize(void) const;
  void M_synchronize_locations(void) const;
#endif
};

  } // namespace debug
} // namespace libcw

#endif // CWDEBUG_ALLOC
#endif // LIBCWD_CLASS_OOAM_FILTER_H
