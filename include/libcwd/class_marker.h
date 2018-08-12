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

/** \file class_marker.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_MARKER_H
#define LIBCWD_CLASS_MARKER_H

#ifndef LIBCWD_CLASS_ALLOC_FILTER_H
#include <libcwd/class_alloc_filter.h>
#endif

namespace libcwd {

extern alloc_filter_ct const default_ooam_filter;

/**
 * \brief A memory allocation marker.
 * \ingroup group_markers
 */
class marker_ct {
private:
  void register_marker(char const* label);
  alloc_filter_ct const& M_filter;
  bool M_make_invisible;
public:
  /** \brief Construct a marker with label \p label.
   *
   * The \a filter must exist until the marker is deleted; it is used by the destructor
   * of the marker.  Allocations that are hidden as a result of the filter will be
   * placed outside the marker.  Allocations done after the creation of the marker
   * which are not deleted when the marker gets destructed and which are not hidden
   * by the filter will be listed.
   *
   * If \a make_invisible is set true then all filtered allocations will be made
   * \ref group_invisible "invisible" at the destruction of the marker.
   */
  marker_ct(char const* label, alloc_filter_ct const& filter, bool make_invisible = false) :
      M_filter(filter), M_make_invisible(make_invisible) { register_marker(label); }

  /** \brief Construct a marker with label \p label. */
  marker_ct(char const* label) : M_filter(default_ooam_filter), M_make_invisible(false) { register_marker(label); }

  /** \brief Construct a marker with label "An allocation marker".
   *
   * The \a filter must exist until the marker is deleted; it is used by the destructor
   * of the marker.  Allocations that are hidden as a result of the filter will be
   * placed outside the marker.  Allocations done after the creation of the marker
   * which are not deleted when the marker gets destructed and which are not hidden
   * by the filter will be listed.
   *
   * If \a make_invisible is set true then all filtered allocations will be made
   * \ref group_invisible "invisible" at the destruction of the marker.
   */
  marker_ct(alloc_filter_ct const& filter, bool make_invisible = false) :
      M_filter(filter), M_make_invisible(make_invisible) { register_marker("An allocation marker"); }

  /** \brief Construct a marker with label "An allocation marker". */
  marker_ct() : M_filter(default_ooam_filter), M_make_invisible(false) { register_marker("An allocation marker"); }

  ~marker_ct();
};

} // namespace libcwd

#endif // LIBCWD_CLASS_MARKER_H

