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

/** \file libcwd/max_label_len.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_MAX_LABEL_LEN_H
#define LIBCWD_MAX_LABEL_LEN_H

namespace libcwd {

//! The maximum number of characters that are allowed in a %debug channel label.
unsigned short const max_label_len_c = 16;

} // namespace libcwd

#endif // LIBCWD_MAX_LABEL_LEN_H
