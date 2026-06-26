// SPDX-FileCopyrightText: 2000-2004, 2017-2020, 2025-2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 *
 * @brief This is the main header file of libcwd.
 *
 * Don't include this header file directly.
 * Instead use a @link the-custom-debug-h-file custom debug.h @endlink header file that includes this file,
 * that will allow others to compile your application without having libcwd installed.
 */

#ifndef CWDEBUG

// If you run into this error then you included <libcwd/debug.h> (or any other libcwd header file)
// while the macro CWDEBUG was not defined.  Doing so would cause the compilation of your
// application to fail on machines that do not have libcwd installed.  Instead you should use:
// #include "debug.h"
// and add a file debug.h to your applications distribution.  Please see the the example-project
// that comes with the source code of libcwd (or is included in the documentation that comes with
// the rpm (ie: /usr/doc/libcwd-1.0/example-project) for a description of the content of "debug.h".
// Note1: CWDEBUG should be defined on the compiler commandline, for example: g++ -DCWDEBUG ...
#error You are including <libcwd/debug.h> while CWDEBUG is not defined.  See the comments in this header file for more information.

#else // CWDEBUG (normal usage of this file):

#if defined(LIBCWD_DEFAULT_DEBUGCHANNELS) && defined(DEBUGCHANNELS)
// If you run into this error then you included <libcwd/debug.h> (or any other libcwd header file)
// without defining DEBUGCHANNELS, and later (the moment of this error) you included it with
// DEBUGCHANNELS defined.
//
// The most likely reason for this is that you include <libcwd/debug.h> in one of your headers
// instead of using #include "debug.h", and then included that header file before including
// "debug.h" in the .cpp file.
// End-applications should #include "debug.h" everywhere.  See the the example-project that comes
// with the source code of libcwd (or is included in the documentation that comes with the rpm
// (ie: /usr/doc/libcwd-1.0/example-project) for a description of the content of "debug.h".
// More information for end-application users can be found on
// https://carlowood.github.io/libcwd/group__preparation.html
//
// Third-party libraries should never include <libcwd/debug.h> but also not "debug.h".  They
// should include <libcwd/libraries_debug.h> (and not use Dout et al in their headers).  If you
// are using a library that did include <libcwd/debug.h> then please report this bug the author
// of that library.  You can workaround it for now by including "debug.h" before including the
// header of that library.
// More information for library authors that use libcwd can be found on
// https://carlowood.github.io/libcwd/group__chapter__custom__debug__h.html
#error DEBUGCHANNELS is defined while previously it was not defined.  See the comments in this header file for more information.
#endif

#endif // CWDEBUG

#ifndef LIBCWD_DEBUG_H
#define LIBCWD_DEBUG_H

#ifdef CWDEBUG

// The following header is also needed for end-applications, despite its name.
#include "libraries_debug.h"

#ifndef LIBCWD_DOXYGEN

// The real code
#ifdef DEBUGCHANNELS
#define LIBCWD_DEBUGCHANNELS DEBUGCHANNELS
#else
#define LIBCWD_DEBUGCHANNELS libcwd::channels
#define LIBCWD_DEFAULT_DEBUGCHANNELS
#endif

#else // LIBCWD_DOXYGEN

// This is only here for the documentation.  The user will define DEBUGCHANNELS, not LIBCWD_DEBUGCHANNELS.
/**
 * @brief The namespace containing the current %debug %channels (dc) namespace.
 *
 * This macro is internally used by libcwd macros to include the chosen set of %debug %channels.
 * For details please read section @ref the-custom-debug-h-file.
 */
#define DEBUGCHANNELS
#endif // LIBCWD_DOXYGEN

// For use in applications
// clang-format off
/** @def Debug
 *
 * @brief Encapsulation macro for general debugging code.
 *
 * The parameter of this macro can be arbitrary code that will be eliminated
 * from the application when the macro CWDEBUG is *not* defined.
 *
 * It uses the namespaces @ref DEBUGCHANNELS and libcwd, making it unnecessary to
 * use the the full scopes for debug channels and utility functions provided by
 * libcwd.
 *
 * **Examples:**
 *
 * ```cpp
 * Debug(main_reached());                           // Must be called at the top of main. Does configuration consistency check.
 * Debug(dc::notice.on());                          // Switch debug channel NOTICE on.
 * Debug(libcw_do.on());                            // Turn all debugging temporarily off.
 * Debug(list_channels_on(libcw_do));               // List all debug channels.
 * Debug(libcw_do.set_ostream(&std::cout));         // Use std::cout as debug output stream.
 * Debug(libcw_do.set_ostream(&std::cout, &mutex)); // use ‛mutex' as lock for std::cout.
 * Debug(libcw_do.inc_indent(4));                   // Increment indentation by 4 spaces.
 * Debug(libcw_do.get_ostream()->flush());          // Flush the current debug output stream.
 * ```
 */
#define Debug(/*STATEMENTS*/...) LibcwDebug(LIBCWD_DEBUGCHANNELS, __VA_ARGS__)

/** @def Dout
 *
 * @brief Macro for writing %debug output.
 *
 * This macro is used for writing %debug output to the default %debug object
 * @link libcwd::libcw_do libcw_do @endlink.
 * No code is generated when the macro CWDEBUG is not defined, in that case the macro
 * Dout is replaced by white space.
 *
 * The macro @ref Dout uses libcwds debug object @link libcwd::libcw_do libcw_do @endlink.
 * You will have to define your own macro when you want to use a second debug object.
 *
 * @sa @ref control-flags
 *  @n @ref predefined-debug-channels
 *  @n @link the-custom-debug-h-file Defining your own debug channels @endlink
 *  @n @link custom-debug-objects Defining your own debug objects @endlink
 *  @n @ref nesting-debug-output "Nested debug calls"
 *
 * **Examples:**
 *
 * ```cpp
 * Dout(dc::notice, "Hello World");
 * Dout(dc::warning, "Out of memory in function " << func_name);
 * Dout(dc::notice|blank_label_cf, "The content of the object is: " << std::hex << obj);
 * ```
 */
#define Dout(cntrl, ...) LibcwDout(LIBCWD_DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, __VA_ARGS__)

/**
 * @def DoutFatal
 * @ingroup fatal-debug-output
 *
 * @brief Macro for writing fatal %debug output to the default %debug object
 * @link libcwd::libcw_do libcw_do @endlink.
 */
#define DoutFatal(cntrl, ...) LibcwDoutFatal(LIBCWD_DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, __VA_ARGS__)

/**
 * @def ForAllDebugChannels
 * @ingroup special-functions-and-utilities
 *
 * @brief Looping over all debug channels.
 *
 * The macro `ForAllDebugChannels` allows you to run over all %debug %channels.
 *
 * For example,
 *
 * ```cpp
 * ForAllDebugChannels(while (!debugChannel.is_on()) debugChannel.on());
 * ```
 *
 * which turns all %channels on.
 * And
 *
 * ```cpp
 * ForAllDebugChannels(if (debugChannel.is_on()) debugChannel.off());
 * ```
 *
 * which turns all channels off.
 */
#define ForAllDebugChannels(/*STATEMENT*/...) LibcwdForAllDebugChannels(LIBCWD_DEBUGCHANNELS, __VA_ARGS__)

/**
 * @def ForAllDebugObjects
 * @ingroup special-functions-and-utilities
 *
 * @brief Looping over all debug objects.
 *
 * The macro `ForAllDebugObjects` allows you to run over all %debug objects.
 *
 * For example,
 *
 * ```cpp
 * ForAllDebugObjects(debugObject.set_ostream(&std::cerr, &cerr_mutex));
 * ```
 *
 * would set the output stream of all %debug objects to `std::cerr`.
 */
#define ForAllDebugObjects(/*STATEMENT*/...) LibcwdForAllDebugObjects(LIBCWD_DEBUGCHANNELS, __VA_ARGS__)

// clang-format on
// Finally, in order for Dout() to be usable, we need this.
#include <iostream>

#endif // CWDEBUG
#endif // LIBCWD_DEBUG_H
