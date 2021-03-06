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

/** \file libcwd/pc_mangled_function_name.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PC_MANGLED_FUNCTION_NAME_H
#define LIBCWD_PC_MANGLED_FUNCTION_NAME_H

namespace libcwd {

extern char const* pc_mangled_function_name(void const* addr);

} // namespace libcwd

#endif // LIBCWD_PC_MANGLED_FUNCTION_NAME_H
