// SPDX-FileCopyrightText: 2001-2004, 2007, 2013, 2018-2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef RAW_WRITE_H
#define RAW_WRITE_H

#include "libcwd/config.h"

#if CWDEBUG_DEBUG
#if CWDEBUG_DEBUGT
#define WRITE __libc_write
#else
#define WRITE write
#endif
extern "C" ssize_t write(int fd, void const* buf, size_t count);

#if CWDEBUG_DEBUGT
#include <mutex>
#include <thread>
namespace libcwd::_private_ {
extern std::mutex raw_write_mutex;
} // namespace libcwd::_private_
#endif

// The difference between DEBUGDEBUG_CERR and FATALDEBUGDEBUG_CERR is that the latter is not suppressed
// when --disable-debug-output is used because a fatal error occured anyway, so this can't
// disturb the testsuite.
#define FATALDEBUGDEBUG_CERR(x)                                                        \
  do                                                                                   \
  {                                                                                    \
    std::lock_guard<std::mutex> lock(::libcwd::_private_::raw_write_mutex);            \
    std::cerr << "CWDEBUG_DEBUG: " << std::this_thread::get_id() << ": " << x << '\n'; \
  } while (0)

#define LIBCWD_DEBUG_ASSERT(expr)                                                                              \
  do                                                                                                           \
  {                                                                                                            \
    if (!(expr))                                                                                               \
    {                                                                                                          \
      std::lock_guard<std::mutex> lock(::libcwd::_private_::raw_write_mutex);                                  \
      std::cerr << "CWDEBUG_DEBUG: " << std::this_thread::get_id() << ": " << __FILE__ ":" << __LINE__ << ": " \
                << __PRETTY_FUNCTION__ << ": Assertion`" << LIBCWD_STRING(expr) << "' failed.\n";              \
      core_dump();                                                                                             \
    }                                                                                                          \
  } while (0)

#if CWDEBUG_DEBUGT
#define LIBCWD_DEBUGT_ASSERT(expr)                                                                              \
  do                                                                                                            \
  {                                                                                                             \
    if (!(expr))                                                                                                \
    {                                                                                                           \
      std::lock_guard<std::mutex> lock(::libcwd::_private_::raw_write_mutex);                                   \
      std::cerr << "CWDEBUG_DEBUGT: " << std::this_thread::get_id() << ": " << __FILE__ ":" << __LINE__ << ": " \
                << __PRETTY_FUNCTION__ << ": Assertion`" << LIBCWD_STRING(expr) << "' failed.\n";               \
      core_dump();                                                                                              \
    }                                                                                                           \
  } while (0)
#else // CWDEBUG_DEBUGT
#define LIBCWD_DEBUGT_ASSERT(expr) do { } while (0)
#endif // CWDEBUG_DEBUGT

#else // !CWDEBUG_DEBUG

#define LIBCWD_DEBUG_ASSERT(expr) do { } while (0)
#define FATALDEBUGDEBUG_CERR(x) do { } while (0)
#define LIBCWD_DEBUGT_ASSERT(expr) do { } while (0)

#endif // !CWDEBUG_DEBUG

#if CWDEBUG_DEBUGOUTPUT
#define DEBUGDEBUG_CERR(x) FATALDEBUGDEBUG_CERR(x)
#else // !CWDEBUG_DEBUGOUTPUT
#define DEBUGDEBUG_CERR(x) do { } while (0)
#endif // !CWDEBUG_DEBUGOUTPUT

#endif // RAW_WRITE_H
