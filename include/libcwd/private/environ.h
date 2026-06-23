// SPDX-FileCopyrightText: 2002-2004, 2018, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_PRIVATE_ENVIRON_H
#define LIBCWD_PRIVATE_ENVIRON_H

namespace libcwd::_private_ {

extern void process_environment_variables();

// Environment variable: LIBCWD_PRINT_LOADING
// Print the list with "ELFUTILS: new ObjectFile "/usr/lib/libc.so.6" with load base 0x7f490c400000" etc.
// at the start of the program *even* when this happens before main() is reached and libcw_do and dc::elfutils
// are still turned off.
extern bool always_print_loading;

// Environment variable: LIBCWD_NO_STARTUP_MSGS
// This will suppress all messages that normally could be printed
// before reaching main, including warning messages.
// This overrides LIBCWD_PRINT_LOADING.
extern bool suppress_startup_msgs;

} // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_ENVIRON_H
