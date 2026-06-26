// SPDX-FileCopyrightText: 2000-2005, 2007, 2017-2020, 2023-2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_MACRO_LIBCWD_MACROS_H
#define LIBCWD_MACRO_LIBCWD_MACROS_H

#include "libcwd/config.h"

#include <cstddef> // Needed for size_t

#define LIBCWD_ASSERT_NOT_INTERNAL

//===================================================================================================
// Macro LibcwDebug
//

/**
 * @brief General debug macro.
 *
 * This macro allows one to implement a customized "@ref Debug",
 * using a custom %debug channel namespace.
 *
 * @sa @ref the-custom-debug-h-file
 */
#define LibcwDebug(dc_namespace, ...)           \
  do                                            \
  {                                             \
    using namespace ::libcwd;                   \
    using namespace dc_namespace;               \
    {                                           \
      __VA_ARGS__;                              \
    }                                           \
  } while (0)

//===================================================================================================
// Macro LibcwDout
//

// This is debugging libcwd itself.
#ifndef LIBCWD_LibcwDoutScopeBegin_MARKER
#if CWDEBUG_DEBUGOUTPUT
#include <sys/types.h>
extern "C" ssize_t write(int fd, void const* buf, size_t count);
#define LIBCWD_STR1(x) #x
#define LIBCWD_STR2(x) LIBCWD_STR1(x)
#define LIBCWD_STR3 "LibcwDout at " __FILE__ ":" LIBCWD_STR2(__LINE__) "\n"
#define LIBCWD_LibcwDoutScopeBegin_MARKER                                                    \
  do                                                                                         \
  {                                                                                          \
    [[maybe_unused]] size_t __libcwd_len = ::write(2, LIBCWD_STR3, sizeof(LIBCWD_STR3) - 1); \
  } while (0)
#else // !CWDEBUG_DEBUGOUTPUT
#define LIBCWD_LibcwDoutScopeBegin_MARKER
#endif // !CWDEBUG_DEBUGOUTPUT
#endif // !LIBCWD_LibcwDoutScopeBegin_MARKER

// If this macro is defined, it is prepended immediately before any `os << ...stuff...` code that writes debug output.
// This can be used to add operator<<'s that are exclusively for debugging (most notably those that print things
// defined in namespace std).
#ifndef LIBCWD_USING_OSTREAM_PRELUDE
#define LIBCWD_USING_OSTREAM_PRELUDE
#endif

/**
 * @brief General debug output macro.
 *
 * This macro allows one to implement a customized "@ref Dout", using
 * a custom @ref the-output-device-debug-object "debug object" and/or channel namespace.
 *
 * @sa @ref the-custom-debug-h-file
 */
#define LibcwDout(dc_namespace, debug_obj, cntrl, ...)                                \
  LibcwDoutScopeBegin(dc_namespace, debug_obj, cntrl) LibcwDoutStream << __VA_ARGS__; \
  LibcwDoutScopeEnd

#define LibcwDoutScopeBegin(dc_namespace, debug_obj, cntrl)                                                       \
  do                                                                                                              \
  {                                                                                                               \
    LIBCWD_TSD_DECLARATION;                                                                                       \
    LIBCWD_ASSERT_NOT_INTERNAL;                                                                                   \
    LIBCWD_LibcwDoutScopeBegin_MARKER;                                                                            \
    if (LIBCWD_DO_TSD_MEMBER_OFF(debug_obj) < 0)                                                                  \
    {                                                                                                             \
      using namespace ::libcwd;                                                                                   \
      ::libcwd::ChannelSetBootstrap __libcwd_channel_set(LIBCWD_DO_TSD(debug_obj), LIBCWD_TSD);                   \
      bool on;                                                                                                    \
      {                                                                                                           \
        using namespace dc_namespace;                                                                             \
        on = (__libcwd_channel_set | cntrl).on;                                                                   \
      }                                                                                                           \
      if (on)                                                                                                     \
      {                                                                                                           \
        ::libcwd::DebugObject& __libcwd_debug_object(debug_obj);                                                  \
        LIBCWD_DO_TSD(__libcwd_debug_object).start(__libcwd_debug_object, __libcwd_channel_set, LIBCWD_TSD);      \
        LIBCWD_USING_OSTREAM_PRELUDE

// Note: LibcwDoutStream is *not* equal to the ostream that was set with set_ostream.  It is a temporary stringstream.
#define LibcwDoutStream (*LIBCWD_DO_TSD_MEMBER(__libcwd_debug_object, current_bufferstream))

#define LibcwDoutScopeEnd                                                                                    \
  LIBCWD_DO_TSD(__libcwd_debug_object).finish(__libcwd_debug_object, __libcwd_channel_set, LIBCWD_TSD);      \
  }                                                                                                          \
  }                                                                                                          \
  }                                                                                                          \
  while (0)

//===================================================================================================
// Macro LibcwDoutFatal
//

// This is debugging libcwd itself.
#ifndef LIBCWD_LibcwDoutFatalScopeBegin_MARKER
#if CWDEBUG_DEBUGOUTPUT
#define LIBCWD_STR4 "LibcwDoutFatal at " __FILE__ ":" LIBCWD_STR2(__LINE__) "\n"
#define LIBCWD_LibcwDoutFatalScopeBegin_MARKER                                               \
  do                                                                                         \
  {                                                                                          \
    [[maybe_unused]] size_t __libcwd_len = ::write(2, LIBCWD_STR4, sizeof(LIBCWD_STR4) - 1); \
  } while (0)
#else
#define LIBCWD_LibcwDoutFatalScopeBegin_MARKER
#endif
#endif // !LIBCWD_LibcwDoutFatalScopeBegin_MARKER

/**
 * @brief General fatal debug output macro.
 *
 * This macro allows one to implement a customized "@ref DoutFatal", using
 * a custom @ref the-output-device-debug-object "debug object" and/or channel namespace.
 *
 * @sa @ref the-custom-debug-h-file
 */
#define LibcwDoutFatal(dc_namespace, debug_obj, cntrl, ...)                                     \
  LibcwDoutFatalScopeBegin(dc_namespace, debug_obj, cntrl) LibcwDoutFatalStream << __VA_ARGS__; \
  LibcwDoutFatalScopeEnd

#define LibcwDoutFatalScopeBegin(dc_namespace, debug_obj, cntrl)                                              \
  do                                                                                                          \
  {                                                                                                           \
    LIBCWD_TSD_DECLARATION;                                                                                   \
    LIBCWD_LibcwDoutFatalScopeBegin_MARKER;                                                                   \
    using namespace ::libcwd;                                                                                 \
    ::libcwd::FatalChannelSetBootstrap __libcwd_channel_set(LIBCWD_DO_TSD(debug_obj), LIBCWD_TSD);            \
    {                                                                                                         \
      using namespace dc_namespace;                                                                           \
      __libcwd_channel_set | cntrl;                                                                           \
    }                                                                                                         \
    ::libcwd::DebugObject& __libcwd_debug_object(debug_obj);                                                  \
    LIBCWD_DO_TSD(__libcwd_debug_object).start(__libcwd_debug_object, __libcwd_channel_set, LIBCWD_TSD);      \
  LIBCWD_USING_OSTREAM_PRELUDE

#define LibcwDoutFatalStream LibcwDoutStream

#define LibcwDoutFatalScopeEnd                                                                                     \
  LIBCWD_DO_TSD(__libcwd_debug_object).fatal_finish(__libcwd_debug_object, __libcwd_channel_set, LIBCWD_TSD);      \
  }                                                                                                                \
  while (0)

#endif // LIBCWD_MACRO_LIBCWD_MACROS_H
