// SPDX-FileCopyrightText: 2000-2006, 2018, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref the-custom-debug-h-file "debug.h".
 */

#ifndef LIBCWD_PC_MANGLED_FUNCTION_NAME_H
#define LIBCWD_PC_MANGLED_FUNCTION_NAME_H

namespace libcwd {

extern char const* pc_mangled_function_name(void const* addr);

} // namespace libcwd

#endif // LIBCWD_PC_MANGLED_FUNCTION_NAME_H
