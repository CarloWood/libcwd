// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT
//
// CWDEBUG test for the "Libraries" subsection of custom-debug.h.md.
//
// This executable links against libexample (compiled with CWDEBUG) and verifies:
//
//   1. The application can include its own "debug.h" and then <libexample/debug.h>
//      without triggering the #error guard (snippet 2, remark about Debug).
//   2. The library's warp channel (snippet 1) is registered in
//      libexample::channels::dc, not in libcwd::channels::dc.
//   3. LibexampleDout writes through the warp channel and the output carries the
//      "WARP" label (snippet 2 macro definitions).
//   4. Dout in the library's source file works too (remark: "source files may
//      use Dout after including debug.h"), writing through libexample::channels::dc.
//   5. LibexampleForAllDebugChannels finds the warp channel (snippet 2 macro).
//   6. LibexampleDout from a library *header* (inline function) compiles and runs
//      (remark: "Don't use Dout etc. in header files of libraries").

// Include the application debug.h FIRST — this is the contract enforced by the
// #error guard in <libexample/debug.h>.
#include "debug.h"

#include <libexample/warp_engine.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

// Simple RAII redirect of std::cerr so we can capture the debug output that
// libcwd writes to its default ostream.  All diagnostics from the test itself
// are written to std::cout so they are never swallowed by the redirect.
class CerrRedirect {
  std::streambuf* saved_;
public:
  explicit CerrRedirect(std::ostream& target) : saved_(std::cerr.rdbuf(target.rdbuf())) { }
  ~CerrRedirect() { std::cerr.rdbuf(saved_); }
};

// Trim trailing spaces from a channel label.
//
// libcwd stores channel labels in a fixed-width buffer padded with spaces to
// WST_max_len, so get_label() returns e.g. "WARP    " rather than "WARP".
// This helper strips the padding so the caller can compare against the bare name.
std::string trim_label(char const* label)
{
  std::string s(label);
  s.resize(s.find_last_not_of(' ') + 1); // npos+1 == 0 → empty string if all spaces.
  return s;
}

int main()
{
  // Configuration consistency check (requires CWDEBUG + linked libcwd).
  Debug(main_reached());

  std::stringstream captured;
  int warp_count = 0;

  {
    CerrRedirect redir(captured);

    // Turn on the debug object, the library's warp channel, and the standard
    // notice channel (the library .cc writes through dc::notice via Dout).
    Debug(libcw_do.on());
    Debug(libexample::channels::dc::warp.on());
    Debug(dc::notice.on());

    // Exercise LibexampleDout from a library header (inline function).
    libexample::warp_engine_announce();

    // Exercise LibexampleDout and Dout from a library source file.
    libexample::warp_engine_engage();

    // Count how many registered channels carry the label "WARP".
    LibexampleForAllDebugChannels(
      if (trim_label(debugChannel.get_label()) == "WARP")
        ++warp_count;
    );
  }

  // --- All checks happen OUTSIDE the redirect so diagnostics reach the user. ---

  bool ok = true;

  // Check 5: LibexampleForAllDebugChannels must find exactly one WARP channel.
  if (warp_count != 1)
  {
    std::cout << "FAIL: LibexampleForAllDebugChannels found WARP " << warp_count
              << " time(s), expected exactly 1." << std::endl;
    ok = false;
  }

  // Inspect captured debug output for the expected messages.
  bool found_header_msg = false;
  bool found_engage_msg = false;
  bool found_notice_msg = false;

  std::string line;
  int expected_on_off = 0;
  while (std::getline(captured, line))
  {
    if (line.find("WARP") != std::string::npos &&
        line.find("announcing from header") != std::string::npos)
      found_header_msg = true;
    if (line.find("WARP") != std::string::npos &&
        line.find("Engaging warp engine") != std::string::npos)
      found_engage_msg = true;
    if (line.find("NOTICE") != std::string::npos)
    {
      if (line.find("Warp engine engage called") != std::string::npos)
        found_notice_msg = true;
      else if (line.find("ELFUTILS is OFF"))
        ++expected_on_off;
      else if (line.find("DEBUG    is OFF"))
        ++expected_on_off;
      else if (line.find("SYSTEM   is OFF"))
        ++expected_on_off;
      else if (line.find("NOTICE   is OFF"))
        ++expected_on_off;
      else if (line.find("WARNING  is OFF"))
        ++expected_on_off;
      else if (line.find("WARP     is OFF"))
        ++expected_on_off;
    }
  }

  if (!found_header_msg)
  {
    std::cout << "FAIL: did not find WARP output from warp_engine_announce (header)." << std::endl;
    ok = false;
  }
  if (!found_engage_msg)
  {
    std::cout << "FAIL: did not find WARP output from warp_engine_engage (LibexampleDout)." << std::endl;
    ok = false;
  }
  if (!found_notice_msg)
  {
    std::cout << "FAIL: did not find NOTICE output from warp_engine_engage (Dout)." << std::endl;
    ok = false;
  }
  if (expected_on_off != 6)
  {
    std::cout << "FAIL: did not find all six expected ON/OFF declarations (ForAllDebugChannels).)" << std::endl;
    ok = false;
  }
  if (ok)
    std::cout << "PASS: all library_pattern CWDEBUG checks succeeded." << std::endl;
  else
  {
    // Dump captured output to aid diagnosis.
    std::cout << "---- captured debug output ----" << std::endl;
    std::cout << captured.str() << std::endl;
    std::cout << "---- end captured output ----" << std::endl;
  }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
