// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT
//
// Negative compile test for the #error guard in <libexample/debug.h>.
//
// This file deliberately includes <libexample/debug.h> WITHOUT first including
// "debug.h" (the application's debug header that defines the Debug macro).
// Under -DCWDEBUG, <libexample/debug.h> checks:
//
//   #if !defined(Debug) && !defined(LIBEXAMPLE_INTERNAL)
//   #error ...
//   #endif
//
// Since neither Debug nor LIBEXAMPLE_INTERNAL is defined here, compilation must
// fail with that #error.  The library_pattern_error cmake script verifies this.

#include <libexample/debug.h>

int main() { return 0; }
