// $Header$
//
// Copyright (C) 2000 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file core_dump.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_CORE_DUMP_H
#define LIBCWD_CORE_DUMP_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif

namespace libcw {
  namespace debug {

extern void core_dump(void) __attribute__ ((__noreturn__));

  } // namespace debug
} // namespace libcw

#endif // LIBCWD_CORE_DUMP_H

