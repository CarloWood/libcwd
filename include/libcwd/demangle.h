// SPDX-FileCopyrightText: 2000-2005, 2018, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/demangle.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_DEMANGLE_H
#define LIBCWD_DEMANGLE_H

#include <string>

namespace libcwd {

/** \addtogroup group_demangle */
/** \{ */

extern void demangle_type(char const* input, std::string& output);
extern void demangle_symbol(char const* input, std::string& output);

/** \} */

} // namespace libcwd

#endif // LIBCWD_DEMANGLE_H
