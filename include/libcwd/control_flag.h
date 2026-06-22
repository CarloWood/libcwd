// SPDX-FileCopyrightText: 2000-2005, 2007, 2018, 2020, 2023, 2025-2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CONTROL_FLAG_H
#define LIBCWD_CONTROL_FLAG_H

#include "libcwd/config.h"

namespace libcwd {

/** @addtogroup group_control_flags */
/** \{ */

/** The type that is used for control flags and control flag mask. */
using control_flag_t = unsigned int;

// The control bits:
//! Omit the default new line at the end
control_flag_t const nonewline_cf = 0x0001;

//! Omit margin, label, marker and indentation
control_flag_t const noprefix_cf = 0x0002;

//! Omit label, marker and indentation
control_flag_t const nolabel_cf = 0x0004;

//! Replace margin by white space
control_flag_t const blank_margin_cf = 0x0008;

//! Replace label by white space
control_flag_t const blank_label_cf = 0x0010;

//! Replace marker by white space
control_flag_t const blank_marker_cf = 0x0020;

//! Force output to be written to cerr
control_flag_t const cerr_cf = 0x0040;

//! Flush ostream after writing this output
control_flag_t const flush_cf = 0x0080;

//! If interactive, wait till return is pressed
control_flag_t const wait_cf = 0x0100;

//! Append error string according to errno
control_flag_t const error_cf = 0x0200;

// Special mask bits.
control_flag_t const continued_cf_maskbit = 0x0400;
control_flag_t const continued_expected_maskbit = 0x0800;

// Mask bits of all special channels:
control_flag_t const fatal_maskbit = 0x1000;
control_flag_t const coredump_maskbit = 0x2000;
control_flag_t const continued_maskbit = 0x4000;
control_flag_t const finish_maskbit = 0x8000;

//! @p continued_cf has its own type for overloading purposes.
enum continued_cf_nt
{
  continued_cf //!< Start a @ref using_continued "continued" %debug output.
};

//! Returns nonewline_cf if @p cond is true.
inline control_flag_t cond_nonewline_cf(bool cond)
{
  return cond ? nonewline_cf : 0;
}
//! Returns noprefix_cf if @p cond is true.
inline control_flag_t cond_noprefix_cf(bool cond)
{
  return cond ? noprefix_cf : 0;
}
//! Returns nolabel_cf if @p cond is true.
inline control_flag_t cond_nolabel_cf(bool cond)
{
  return cond ? nolabel_cf : 0;
}
//! Returns blank_margin_cf if @p cond is true.
inline control_flag_t conf_blank_margin_cf(bool cond)
{
  return cond ? blank_margin_cf : 0;
}
//! Returns blank_label_cf if @p cond is true.
inline control_flag_t conf_blank_label_cf(bool cond)
{
  return cond ? blank_label_cf : 0;
}
//! Returns blank_marker_cf if @p cond is true.
inline control_flag_t conf_blank_marker_cf(bool cond)
{
  return cond ? blank_marker_cf : 0;
}
//! Returns cerr_cf if @p cond is true.
inline control_flag_t conf_cerr_cf(bool cond)
{
  return cond ? cerr_cf : 0;
}
//! Returns flush_cf if @p cond is true.
inline control_flag_t conf_flush_cf(bool cond)
{
  return cond ? flush_cf : 0;
}
//! Returns wait_cf if @p cond is true.
inline control_flag_t conf_wait_cf(bool cond)
{
  return cond ? wait_cf : 0;
}
//! Returns error_cf if @p cond is true.
inline control_flag_t cond_error_cf(bool cond)
{
  return cond ? error_cf : 0;
}

/** \} */

} // namespace libcwd

#endif // LIBCWD_CONTROL_FLAG_H
