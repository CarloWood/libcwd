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

/** \file libcw/private_assert.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_ASSERT_H
#define LIBCW_PRIVATE_ASSERT_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_CASSERT
#define LIBCW_CASSERT
#include <cassert>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

void assert_fail(char const* expr, char const* file, int line, char const* function);

    }
  }
}

// Solaris8 doesn't define __STRING().
#define LIBCWD_STRING(x) #x

#define LIBCWD_ASSERT(expr) \
	(static_cast<void>((expr) ? 0 \
			          : (::libcw::debug::_private_::\
			assert_fail(LIBCWD_STRING(expr), __FILE__, __LINE__, __PRETTY_FUNCTION__), 0)))

#endif // LIBCW_PRIVATE_ASSERT_H

