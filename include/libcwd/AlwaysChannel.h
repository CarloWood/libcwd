// SPDX-FileCopyrightText: 2001-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref the-custom-debug-h-file "debug.h".
 */

#ifndef LIBCWD_CLASS_ALWAYS_CHANNEL_H
#define LIBCWD_CLASS_ALWAYS_CHANNEL_H

#include "control_flag.h"
#include "max_label_len.h"
#include "libcwd/config.h"

namespace libcwd {

//===================================================================================================
// class AlwaysChannel
//
// A debug channel with a special characteristic: It is always turned
// and cannot be turned off.
//

class AlwaysChannel
{
 public:
  static constexpr char label[max_label_len + 1] = {'>', '>', '>', '>', '>', '>', '>', '>', '>',
                                                    '>', '>', '>', '>', '>', '>', '>', 0};
};

} // namespace libcwd

#endif // LIBCWD_CLASS_ALWAYS_CHANNEL_H
