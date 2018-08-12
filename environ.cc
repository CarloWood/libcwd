// $Header$
//
// Copyright (C) 2002 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include <cstdlib>
#include "cwd_debug.h"

namespace libcwd {
  namespace _private_ {

bool always_print_loading = false;
bool suppress_startup_msgs = false;

void process_environment_variables()
{
  always_print_loading = (getenv("LIBCWD_PRINT_LOADING") != NULL);
  suppress_startup_msgs = (getenv("LIBCWD_NO_STARTUP_MSGS") != NULL);
}

  } // namespace _private_
} // namespace libcwd
