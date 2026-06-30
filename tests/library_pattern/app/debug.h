// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT
//
// Application-side debug.h for the library_pattern test driver.
//
// This file is NOT part of the "Libraries" documentation snippet — the
// application's own debug.h is described in the "Applications" subsection.
// It is kept minimal here so the test focuses on the library code.
//
// The only contract that <libexample/debug.h> relies on is that the macro
// Debug is defined before the library header is included (checked by the
// #error guard inside <libexample/debug.h>).

#pragma once

#define NAMESPACE_DEBUG app::debug

#ifdef CWDEBUG

#include <libcwd/debug.h>

// The root debug.h of the libcwd tree provides these for threadsafe consumers;
// replicate them here so that this translation unit is self-contained even if
// the root debug.h is shadowed by this file in the include search order.
#define ASSERT(...) LIBCWD_ASSERT(__VA_ARGS__)
#define DEBUG_ONLY(x) x

#else // !CWDEBUG

#include <iostream>
#include <cstdlib>

#define Debug(...) do { } while(0)
#define Dout(a, ...) do { } while(0)
#define DoutFatal(a, ...) do { ::std::cerr << __VA_ARGS__ << ::std::endl; ::std::exit(EXIT_FAILURE); } while(1)
#define ForAllDebugChannels(...)
#define ForAllDebugObjects(...)
#define LibcwDebug(dc_namespace, ...)
#define LibcwDout(a, b, c, ...)
#define LibcwDoutFatal(a, b, c, ...) do { ::std::cerr << __VA_ARGS__ << ::std::endl; ::std::exit(EXIT_FAILURE); } while(1)
#define LibcwdForAllDebugChannels(dc_namespace, ...)
#define LibcwdForAllDebugObjects(dc_namespace, ...)
#define CWDEBUG_LOCATION 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGT 0

#endif // CWDEBUG
