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

/** \file control_flag.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_CONTROL_FLAG_H
#define LIBCWD_CONTROL_FLAG_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif

namespace libcwd {

/** \addtogroup group_control_flags */
/** \{ */

/** The type that is used for control flags and control flag mask. */
typedef unsigned int control_flag_t;

// The control bits:
//! Omit the default new line at the end
control_flag_t const nonewline_cf      		= 0x0001;

//! Omit margin, label, marker and indentation
control_flag_t const noprefix_cf       		= 0x0002;

//! Omit label, marker and indentation
control_flag_t const nolabel_cf        		= 0x0004;

//! Replace margin by white space
control_flag_t const blank_margin_cf       	= 0x0008;

//! Replace label by white space
control_flag_t const blank_label_cf       	= 0x0010;

//! Replace marker by white space
control_flag_t const blank_marker_cf       	= 0x0020;

//! Force output to be written to cerr
control_flag_t const cerr_cf           		= 0x0040;

//! Flush ostream after writing this output
control_flag_t const flush_cf          		= 0x0080;

//! If interactive, wait till return is pressed
control_flag_t const wait_cf           		= 0x0100;

//! Append error string according to errno
control_flag_t const error_cf	   		= 0x0200;

// Special mask bits.
control_flag_t const continued_cf_maskbit	= 0x0400;
control_flag_t const continued_expected_maskbit	= 0x0800;

// Mask bits of all special channels:
control_flag_t const fatal_maskbit     		= 0x1000;
control_flag_t const coredump_maskbit  		= 0x2000;
control_flag_t const continued_maskbit 		= 0x4000;
control_flag_t const finish_maskbit    		= 0x8000;

//! \a continued_cf has its own type for overloading purposes.
enum continued_cf_nt {
  continued_cf	//!< Start a \ref using_continued "continued" %debug output.
};

//! Returns nonewline_cf when \a cond is true.
inline control_flag_t const cond_nonewline_cf(bool cond) { return cond ? nonewline_cf : 0; } 
//! Returns noprefix_cf when \a cond is true.
inline control_flag_t const cond_noprefix_cf(bool cond) { return cond ? noprefix_cf : 0; }
//! Returns nolabel_cf when \a cond is true.
inline control_flag_t const cond_nolabel_cf(bool cond) { return cond ? nolabel_cf : 0; }
//! Returns error_cf when \a cond is true.
inline control_flag_t const cond_error_cf(bool err) { return err ? error_cf : 0; }

/** \} */

} // namespace libcwd

#endif // LIBCWD_CONTROL_FLAG_H

