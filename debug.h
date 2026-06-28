// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

// This file is included by threadsafe.

#include <libcwd/debug.h>

// Required for threadsafe.
#define ASSERT(x) LIBCWD_ASSERT(x)
#define DEBUG_ONLY(x) x
