// SPDX-FileCopyrightText: 2000-2004, 2017-2020, 2025-2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

// Libcwd-2 defines things that were originally in cwds.
#include "cwds_debug.h"

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
// application to fail on machines that do not have libcwd installed. Instead you should use:
// #include "debug.h"
// and add a file debug.h to your applications source distribution. Please see the the example-project
// that comes with the source code of libcwd (and is included in the documentation that comes with
// a libcwd package (ie: /usr/share/doc/libcwd/example-project) for a description of the content of "debug.h".
// Note: CWDEBUG should be defined on the compiler commandline, for example: g++ -DCWDEBUG ...
// which normally happens automatically, simply by linking with libcwd (when using cmake).
#error You are including <libcwd/debug.h> while CWDEBUG is not defined.  See the comments in this header file for more information.

#endif // CWDEBUG

#ifndef LIBCWD_DEBUG_H
#define LIBCWD_DEBUG_H

#ifdef CWDEBUG

// The following header is also needed for end-applications, despite its name.
#include "libraries_debug.h"

// For use in applications
// clang-format off
/** @def Debug
 *
 * @brief Encapsulation macro for general debugging code.
 *
 * The parameter of this macro can be arbitrary code that will be eliminated
 * from the application when the macro CWDEBUG is *not* defined.
 *
 * It uses the namespaces @ref LIBCWD_DEBUG_CHANNELS and `libcwd`, making it unnecessary to
 * use the the full scopes for debug channels and utility functions provided by
 * libcwd.
 *
 * **Examples:**
 *
 * ```cpp
 * Debug(main_reached());                           // Must be called at the top of main. Does configuration consistency check.
 *                                                  // Note that normally you should call instead:
 * Debug(NAMESPACE_DEBUG::init());                  // This already calls main_reached(), as well as read_rcfile(), etc.
 * Debug(dc::notice.on());                          // Switch debug channel NOTICE on.
 * Debug(libcw_do.off());                           // Turn all debugging temporarily off.
 * Debug(list_channels_on(libcw_do));               // List all debug channels (already done by init()).
 * Debug(libcw_do.set_ostream(&std::cout, &mutex)); // use ‛mutex' as lock for std::cout.
 * Debug(libcw_do.set_ostream(&std::cout));         // Use std::cout as debug output stream, leaving the previously configured mutex.
 * Debug(libcw_do.inc_indent(4));                   // Increment indentation by 4 spaces. Or use the RAI object libcwd::Indent.
 * Debug(libcw_do.get_ostream()->flush());          // Flush the current debug output stream.
 * ```
 */
#define Debug(/*STATEMENTS*/...) LibcwDebug(LIBCWD_DEBUG_CHANNELS, __VA_ARGS__)

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
#define Dout(cntrl, ...) LibcwDout(LIBCWD_DEBUG_CHANNELS, ::libcwd::libcw_do, cntrl, __VA_ARGS__)

/**
 * Debugging macro.
 *
 * Print "Entering " << @a data to channel @a cntrl and increment
 * debugging output indentation until the end of the current scope.
 */
#define DoutEntering(cntrl, ...)                                                \
  int __cwds_debug_indentation = 2;                                             \
  LibcwDoutScopeBegin(LIBCWD_DEBUG_CHANNELS, ::libcwd::libcw_do, cntrl)          \
  LibcwDoutStream << "Entering " << __VA_ARGS__;                                \
  LibcwDoutScopeEnd;                                                            \
  ::libcwd::Indent __cwds_debug_indent(__cwds_debug_indentation);

/**
 * @def DoutFatal
 * @ingroup fatal-debug-output
 *
 * @brief Macro for writing fatal %debug output to the default %debug object
 * @link libcwd::libcw_do libcw_do @endlink.
 */
#define DoutFatal(cntrl, ...) LibcwDoutFatal(LIBCWD_DEBUG_CHANNELS, ::libcwd::libcw_do, cntrl, __VA_ARGS__)

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
#define ForAllDebugChannels(/*STATEMENT*/...) LibcwdForAllDebugChannels(LIBCWD_DEBUG_CHANNELS, __VA_ARGS__)

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
#define ForAllDebugObjects(/*STATEMENT*/...) LibcwdForAllDebugObjects(LIBCWD_DEBUG_CHANNELS, __VA_ARGS__)

namespace libcwd {

/**
 * Interface for marking scopes with indented debug output.
 *
 * Creation of the object increments the debug indentation. Destruction
 * of the object automatically decrements the indentation again.
 */
struct Indent
{
  /// The extra number of spaces that were added to the indentation.
  int M_indent;

  /// Construct an Indent object.
  explicit Indent(int indent) : M_indent(indent) { if (M_indent > 0) libcwd::libcw_do.inc_indent(M_indent); }

  /// Destructor.
  ~Indent() { if (M_indent > 0) libcwd::libcw_do.dec_indent(M_indent); }
};

/**
 * Interface for marking scopes with a marker character.
 *
 * Creation of the object appends the character and a space to
 * the current marker after first adding the current indentation
 * to it as spaces, and sets the indentation to zero. Destruction
 * restores things again.
 */
struct Mark
{
  /// The old indentation.
  int M_indent;

  /// Construct a Mark object.
  explicit Mark(char m = '|') : M_indent(libcwd::libcw_do.get_indent())
  {
    libcwd::libcw_do.push_marker();
    libcwd::libcw_do.marker().append(std::string(M_indent, ' ') + m + ' ');
    // This is basically a decrement of M_indent.
    libcwd::libcw_do.set_indent(0);
  }
  explicit Mark(char const* utf8_m) : M_indent(libcwd::libcw_do.get_indent())
  {
    libcwd::libcw_do.push_marker();
    libcwd::libcw_do.marker().append(std::string(M_indent, ' ') + utf8_m + ' ');
    // This is basically a decrement of M_indent.
    libcwd::libcw_do.set_indent(0);
  }
#ifdef __cpp_char8_t
  explicit Mark(char8_t const* utf8_m) : Mark(reinterpret_cast<char const*>(utf8_m)) { }
#endif

  /// Destructor.
  ~Mark() { end(); }

  void end()
  {
    if (M_indent != -1)
    {
      libcwd::libcw_do.pop_marker();
      // Restore indentation relative to possible other indentation increments that happened in the mean time.
      libcwd::libcw_do.inc_indent(M_indent);
      // Mark that end() was already called.
      M_indent = -1;
    }
  }
};

} // namespace libcwd

#endif // CWDEBUG

// clang-format on
#include <libcwd/init_functions.h>

/// Define this macro as 1 when either CWDEBUG is defined or NDEBUG is not defined, otherwise as 0.
#if defined(CWDEBUG) || !defined(NDEBUG)
#define CW_DEBUG 1
#else
#define CW_DEBUG 0
#endif

#ifdef CWDEBUG

NAMESPACE_DEBUG_START
using namespace libcwd::init_functions;
NAMESPACE_DEBUG_END

// Build the LIBCWD_DEBUG_CHANNELS::dc facade.
// Users should insert their debug channels in here.
NAMESPACE_DEBUG_CHANNELS_START
using namespace libcwd::channels::dc;
using libcwd::Channel;
NAMESPACE_DEBUG_CHANNELS_END

#endif // CWDEBUG
#endif // LIBCWD_DEBUG_H
