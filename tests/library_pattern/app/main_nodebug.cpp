// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT
//
// Non-CWDEBUG test for the "Libraries" subsection of custom-debug.h.md.
//
// This executable is compiled WITHOUT -DCWDEBUG and does NOT link against
// libcwd at all.  It verifies that:
//
//   1. The three documented snippets compile when CWDEBUG is not defined.
//   2. <libexample/debug.h> falls back to no-op macros (the #else branch).
//   3. The internal debug.h's inlined nodebug.h content provides no-op Debug,
//      Dout, etc.  The library's .cc file compiles without referencing any
//      libcwd symbol.
//   4. LibexampleDout produces no output and the program exits cleanly.

#include "debug.h"

#include <libexample/warp_engine.h>

#include <cstdlib>
#include <iostream>
#include <sstream>

int main()
{
  // When CWDEBUG is not defined, no debug object or channel exists.
  // The library functions should still be callable and produce no debug output.

  std::stringstream captured;
  {
    // Save/restore std::cerr to detect any accidental output.
    std::streambuf* saved = std::cerr.rdbuf(captured.rdbuf());

    libexample::warp_engine_announce();
    libexample::warp_engine_engage();

    std::cerr.rdbuf(saved);
  }

  if (!captured.str().empty())
  {
    std::cerr << "FAIL: expected no output in non-CWDEBUG build, but got:\n"
              << captured.str() << std::endl;
    return EXIT_FAILURE;
  }

  std::cerr << "PASS: library_pattern non-CWDEBUG build produced no debug output." << std::endl;
  return EXIT_SUCCESS;
}
