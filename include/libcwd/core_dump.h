// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file core_dump.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CORE_DUMP_H
#define LIBCWD_CORE_DUMP_H

#include "libcwd/config.h"

namespace libcwd {

[[noreturn]] void core_dump();

} // namespace libcwd

#endif // LIBCWD_CORE_DUMP_H

