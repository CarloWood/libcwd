// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#define CW_DEBUG 1

#include "libcwd/private_struct_TSD.h"
#include "libcwd/LibcwdForAllDebugObjects.h"

// Required for threadsafe.
#define ASSERT(x) LIBCWD_ASSERT(x)
#define DEBUG_ONLY(x) x
