// SPDX-FileCopyrightText: 2000-2004, 2018, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef IOS_BASE_INIT_H
#define IOS_BASE_INIT_H

namespace libcwd::_private_ {

// Assume that ios_base is always initialized by the time we try to initialize libcwd.
constexpr bool WST_ios_base_initialized = true;

} // namespace libcwd::_private_

#endif // IOS_BASE_INIT__H
