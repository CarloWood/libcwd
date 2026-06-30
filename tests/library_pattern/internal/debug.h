// SPDX-FileCopyrightText: 2026 Carlo Wood
// SPDX-License-Identifier: MIT
//
// Internal (non-installed) debug.h for the libexample library — this is the
// second "debug.h" from the "Libraries" subsection of
// doc/reference-manual/custom-debug.h.md.
//
// The documentation shows this file with a \verbinclude "nodebug.h" that
// inlines the content of libcwd/nodebug.h between the <cstdlib> include and the
// #endif.  We reproduce that verbatim here so the test exercises exactly the
// code a library author would copy from the manual.

#ifndef DEBUG_H
#define DEBUG_H

#ifndef CWDEBUG

#include <iostream>           // std::cerr
#include <cstdlib>            // std::exit, EXIT_FAILURE
#define Debug(/*STATEMENTS*/...)
#define Dout(cntrl, ...)
#define DoutFatal(cntrl, ...) LibcwDoutFatal(, , cntrl, __VA_ARGS__)
#define ForAllDebugChannels(/*STATEMENT*/...)
#define ForAllDebugObjects(/*STATEMENT*/...)
#define LibcwDebug(dc_namespace, /*STATEMENTS*/...)
#define LibcwDout(dc_namespace, d, cntrl, ...)
#define LibcwDoutFatal(dc_namespace, d, cntrl, ...) \
  do                                                \
  {                                                 \
    ::std::cerr << __VA_ARGS__ << ::std::endl;      \
    ::std::exit(EXIT_FAILURE);                      \
  } while (1)
#define LibcwdForAllDebugChannels(dc_namespace, /*STATEMENT*/...)
#define LibcwdForAllDebugObjects(dc_namespace, /*STATEMENT*/...)
#define CWDEBUG_LOCATION 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGT 0
#endif // CWDEBUG

#define LIBEXAMPLE_INTERNAL   // See above.
#include <libexample/debug.h> // The debug.h shown above.
#define DEBUGCHANNELS libexample::channels
#ifdef CWDEBUG
#include <libcwd/debug.h>
#endif

#endif // DEBUG_H
