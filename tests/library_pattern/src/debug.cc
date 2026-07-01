// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT
//
// Translation unit that defines the warp debug channel — this is the first code
// block from the "Libraries" subsection of doc/reference-manual/custom-debug.h.md.
//
// The top of the file is a verbatim copy of the documented snippet; the rest of
// the file adds a concrete library function so the test can exercise the
// channel at run time.

//--->>>---start verbatim copy of doc/group__the-custom-debug-h-file.html---
// This is some .cpp file of your library.
#define LIBCWD_DEBUG_CHANNELS libexample::channels
#include "debug.h"
// ...
#ifdef CWDEBUG
namespace LIBCWD_DEBUG_CHANNELS::dc {
libcwd::Channel warp("WARP");
// Add new channels here...
} // namespace LIBCWD_DEBUG_CHANNELS::dc
#endif
//---<<<---end verbatim copy of doc/group__the-custom-debug-h-file.html---

#include <libexample/warp_engine.h>
#include <iostream>

// Library implementation that writes through the warp channel.
//
// In the CWDEBUG build both LibexampleDout and Dout are available here because
// the internal debug.h included <libcwd/debug.h>, which sets LIBCWD_DEBUG_CHANNELS
// to libexample::channels.  The documentation's "Header files of libraries"
// subsection says source files may use Dout after including "debug.h".
bool libexample::warp_engine_engage()
{
  LibexampleDout(dc::warp, "Engaging warp engine via LibexampleDout.");
  Dout(dc::notice, "Warp engine engage called."); // Uses LIBCWD_DEBUG_CHANNELS = libexample::channels.

  ForAllDebugChannels(
    Dout(dc::notice, debugChannel.get_label() << " is " << (debugChannel.is_on() ? "ON" : "OFF"));
  );

  return true;
}
