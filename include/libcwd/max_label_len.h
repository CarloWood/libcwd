// SPDX-FileCopyrightText: 2000-2005, 2018, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/max_label_len.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_MAX_LABEL_LEN_H
#define LIBCWD_MAX_LABEL_LEN_H

namespace libcwd {

//! The maximum number of characters that are allowed in a %debug channel label.
unsigned short const max_label_len_c = 16;

} // namespace libcwd

#endif // LIBCWD_MAX_LABEL_LEN_H
