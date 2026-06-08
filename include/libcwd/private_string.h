// SPDX-FileCopyrightText: 2001-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_string.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_STRING_H
#define LIBCWD_PRIVATE_STRING_H

#include "libcwd/config.h"
#include <string>

namespace libcwd::_private_ {

// String type used by private libcwd implementation code.
using string = ::std::string;

} // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_INTERNAL_STRING_H
