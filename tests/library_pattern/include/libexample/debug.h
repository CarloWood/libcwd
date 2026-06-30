// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT
//
// Installed library debug header — this is the <libexample/debug.h> from the
// "Libraries" subsection of doc/reference-manual/custom-debug.h.md.
//
// The body is a verbatim copy of the code block shown there, so that this test
// pins down exactly what the documentation tells library authors to write.

#ifndef LIBEXAMPLE_DEBUG_H
#define LIBEXAMPLE_DEBUG_H

#ifdef CWDEBUG
#include <libcwd/libraries_debug.h>

namespace libexample::channels::dc {
using namespace libcwd::channels::dc;
extern libcwd::Channel warp;
// Add new channels here...
} // namespace libexample::channels::dc

// Define private debug output macros for use in header files of the library,
// there is no reason to do this for normal applications.
// We use a literal libexample::channels here and not LIBCWD_DEBUGCHANNELS!
#define LibexampleDebug(...) LibcwDebug(libexample::channels, __VA_ARGS__)
#define LibexampleDout(cntrl, ...) LibcwDout(libexample::channels, libcwd::libcw_do, cntrl, __VA_ARGS__)
#define LibexampleDoutFatal(cntrl, ...) LibcwDoutFatal(libexample::channels, libcwd::libcw_do, cntrl, __VA_ARGS__)
#define LibexampleForAllDebugChannels(...) LibcwdForAllDebugChannels(libexample::channels, __VA_ARGS__)
#define LibexampleForAllDebugObjects(...) LibcwdForAllDebugObjects(libexample::channels, __VA_ARGS__)

// All other macros might be used in header files of libexample, but need to be
// defined by the debug.h of the application that uses it.
// LIBEXAMPLE_INTERNAL is defined when the library itself is being compiled (see below).
#if !defined(Debug) && !defined(LIBEXAMPLE_INTERNAL)
#error The application source file (e.g. .cpp) must use '#include "debug.h"' _before_ including the header file that it includes now, that led to this error.
#endif

#else

#define LibexampleDebug(...) do { } while(0)
#define LibexampleDout(cntrl, ...) do { } while(0)
#define LibexampleDoutFatal(cntrl, ...) do { ::std::cerr << __VA_ARGS__ << ::std::endl; ::std::exit(EXIT_FAILURE); } while(1)
#define LibexampleForAllDebugChannels(...) do { } while(0)
#define LibexampleForAllDebugObjects(...) do { } while(0)

#endif // CWDEBUG

#endif // LIBEXAMPLE_DEBUG_H
