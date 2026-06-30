// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT
//
// Public header of the libexample library.
//
// This header demonstrates the documentation's remark that library headers
// should use LibexampleDout (not Dout).  The inline function exercises the
// macro so that the test verifies the full chain: the application includes its
// own "debug.h" first (which defines Debug), then this header (which includes
// <libexample/debug.h>), and the LibexampleDout call compiles and runs.

#pragma once

#include <libexample/debug.h>

namespace libexample {

// Announce that the warp engine is being calibrated.
//
// This inline function deliberately lives in the header so it can only compile
// when the LibexampleDout macro is available, which requires that the calling
// translation unit included its own debug.h before this header.
inline void warp_engine_announce()
{
  LibexampleDout(dc::warp, "Warp engine announcing from header.");
}

// Engage the warp engine.  Implemented in warp_engine.cc.
bool warp_engine_engage();

} // namespace libexample
