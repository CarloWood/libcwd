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

/** \file libcwd/debugmalloc.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_DEBUGMALLOC_H
#define LIBCWD_DEBUGMALLOC_H

#ifndef LIBCWD_LIBRARIES_DEBUG_H
#error "Don't include <libcwd/debugmalloc.h> directly, include the appropriate \"debug.h\" instead."
#endif

#ifndef LIBCWD_CONFIG_H
#include "libcwd/config.h"
#endif
#ifndef LIBCWD_MACRO_ALLOCTAG_H
#include "macro_AllocTag.h"
#endif

namespace libcwd {

/**
 * \brief Compatibility no-op for removed allocation-debugging visibility control.
 *
 * Allocation tracking was removed, so this function accepts any pointer and
 * has no side effects.
 */
inline void make_invisible(void const*) { }
/**
 * \brief Compatibility no-op for removed allocation-debugging visibility control.
 *
 * Allocation tracking was removed, so this function accepts any pointer and
 * has no side effects.
 */
inline void make_all_allocations_invisible_except(void const*) { }
/**
 * \brief Compatibility no-op for removed allocation-debugging visibility control.
 */
inline void set_invisible_on() { }
/**
 * \brief Compatibility no-op for removed allocation-debugging visibility control.
 */
inline void set_invisible_off() { }

} // namespace libcwd

#if CWDEBUG_DEBUGM
#define LIBCWD_DEBUGM_CERR(x) DEBUGDEBUG_CERR(x)
#define LIBCWD_DEBUGM_ASSERT(expr) do { if (!(expr)) { FATALDEBUGDEBUG_CERR("CWDEBUG_DEBUGM: " __FILE__ ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << ": Assertion`" << LIBCWD_STRING(expr) << "' failed."); core_dump(); } } while(0)
#else
#define LIBCWD_DEBUGM_CERR(x) do { } while(0)
#define LIBCWD_DEBUGM_ASSERT(x) do { } while(0)
#endif

namespace libcwd {

/**
 * \brief Compatibility no-op for removed allocation overview support.
 *
 * The debug object argument is accepted for source compatibility and is not
 * inspected. No output is produced because allocation tracking no longer
 * exists.
 */
inline void list_allocations_on(debug_ct&) { }

} // namespace libcwd

#endif // LIBCWD_DEBUGMALLOC_H
