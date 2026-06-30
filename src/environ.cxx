// SPDX-FileCopyrightText: 2000-2004, 2018, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"

#include <cstdlib>

namespace libcwd::_private_ {

bool always_print_loading = false;
bool suppress_startup_msgs = false;

void process_environment_variables()
{
  always_print_loading = (getenv("LIBCWD_PRINT_LOADING") != NULL);
  suppress_startup_msgs = (getenv("LIBCWD_NO_STARTUP_MSGS") != NULL);
}

} // namespace libcwd::_private_
