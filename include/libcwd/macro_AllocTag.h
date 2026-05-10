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

/** \file libcwd/macro_AllocTag.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_MACRO_ALLOCTAG_H
#define LIBCWD_MACRO_ALLOCTAG_H

#ifndef LIBCWD_LIBRARIES_DEBUG_H
#error "Don't include <libcwd/macro_AllocTag.h> directly, include the appropriate \"debug.h\" instead."
#endif

#ifndef LIBCWD_CONFIG_H
#include "libcwd/config.h"
#endif

/**
 * \addtogroup group_annotation Allocation annotation compatibility macros
 *
 * Allocation debugging has been removed. These macros are intentionally kept
 * as no-ops so existing code can still compile after the removal; arguments
 * are not evaluated, no allocation metadata is stored, and no side effects are
 * performed. NEW(x) remains a thin spelling of <code>new x</code> so code that
 * used it continues to create the requested object.
 *
 * \{ */

#define AllocTag(p, x)
#define AllocTag_dynamic_description(p, x)
#define AllocTag1(p)
#define AllocTag2(p, desc)
#define NEW(x) new x
#define RegisterExternalAlloc(p, s)

/** \} */ // End of group 'group_annotation'.

#endif // LIBCWD_MACRO_ALLOCTAG_H
