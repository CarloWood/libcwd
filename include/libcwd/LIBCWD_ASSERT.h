// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_ASSERT_H
#define LIBCWD_PRIVATE_ASSERT_H

#include "libcwd/config.h"

#include <cassert>

namespace libcwd {
namespace _private_ {

void assert_fail(char const* expr, char const* file, int line, char const* function);

} // namespace _private_
} // namespace libcwd

// Solaris8 doesn't define __STRING().
#define LIBCWD_STRING(x) #x

#define LIBCWD_ASSERT(expr) \
  (static_cast<void>(       \
      (expr) ? 0            \
             : (::libcwd::_private_::assert_fail(LIBCWD_STRING(expr), __FILE__, __LINE__, __PRETTY_FUNCTION__), 0)))

#endif // LIBCWD_PRIVATE_ASSERT_H
