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

/** \file class_marker.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_CLASS_MARKER_H
#define LIBCW_CLASS_MARKER_H

namespace libcw {
  namespace debug {

/**
 * \brief A memory allocation marker.
 * \ingroup group_markers
 */
class marker_ct {
private:
  void register_marker(char const* label);
public:
  /** \brief Construct a marker with label \p label. */
  marker_ct(char const* label) { register_marker(label); }
  /** \brief Construct a marker with label "An allocation marker". */
  marker_ct(void) { register_marker("An allocation marker"); }
  ~marker_ct(void);
};

  } //namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_MARKER_H

