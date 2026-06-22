// SPDX-FileCopyrightText: 2000-2005, 2018, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_STRERRNO_H
#define LIBCWD_STRERRNO_H

namespace libcwd {

extern char const* strerrno(unsigned int err);

} // namespace libcwd

#endif // LIBCWD_STRERRNO_H
