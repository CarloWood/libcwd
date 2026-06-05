// SPDX-FileCopyrightText: 2001-2004, 2007, 2013, 2018-2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "libcwd/private_threading.h"

#ifndef RAW_WRITE_H
#define RAW_WRITE_H

#include "libcwd/config.h"

#if CWDEBUG_DEBUG
#if CWDEBUG_DEBUGT
#define WRITE __libc_write
#else
#define WRITE write
#endif
extern "C" ssize_t write(int fd, const void *buf, size_t count);

#if CWDEBUG_DEBUGT
#include <mutex>
namespace libcwd::_private_ {
extern std::mutex raw_write_mutex;
} // namespace libcwd::_private_
#endif

// The difference between DEBUGDEBUG_CERR and FATALDEBUGDEBUG_CERR is that the latter is not suppressed
// when --disable-debug-output is used because a fatal error occured anyway, so this can't
// disturb the testsuite.
#define FATALDEBUGDEBUG_CERR(x)									\
    do {											\
      if (::libcwd::_private_::WST_ios_base_initialized) {	                                \
        int __libcwd_oldstate; pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &__libcwd_oldstate); \
	LIBCWD_TSD_DECLARATION;									\
	LibcwDebugThreads( ::libcwd::_private_::raw_write_mutex.lock() );	                \
	do { size_t __attribute__((unused)) __libcwd_len = ::write(2, "CWDEBUG_DEBUG: ", 15); } while(0); \
	LibcwDebugThreads( ::libcwd::_private_::raw_write << pthread_self() << ": ");	        \
	::libcwd::_private_::raw_write << x << '\n';					        \
	LibcwDebugThreads( ::libcwd::_private_::raw_write_mutex.unlock() );	                \
	pthread_setcancelstate(__libcwd_oldstate, NULL);                                        \
      }												\
    } while(0)

#define LIBCWD_DEBUG_ASSERT(expr) do { if (!(expr)) { FATALDEBUGDEBUG_CERR("CWDEBUG_DEBUG: " __FILE__ ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << ": Assertion`" << LIBCWD_STRING(expr) << "' failed."); core_dump(); } } while(0)
#if CWDEBUG_DEBUGT
#define LIBCWD_DEBUGT_ASSERT(expr) do { if (!(expr)) { FATALDEBUGDEBUG_CERR("CWDEBUG_DEBUGT: " __FILE__ ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << ": Assertion`" << LIBCWD_STRING(expr) << "' failed."); core_dump(); } } while(0)
#else
#define LIBCWD_DEBUGT_ASSERT(expr) do { } while(0)
#endif
#else // !CWDEBUG_DEBUG
#define LIBCWD_DEBUG_ASSERT(expr) do { } while(0)
#define FATALDEBUGDEBUG_CERR(x) do { } while(0)
#define LIBCWD_DEBUGT_ASSERT(expr) do { } while(0)
#endif // !CWDEBUG_DEBUG

#if CWDEBUG_DEBUGOUTPUT
#define DEBUGDEBUG_CERR(x) FATALDEBUGDEBUG_CERR(x)
#else // !CWDEBUG_DEBUGOUTPUT
#define DEBUGDEBUG_CERR(x) do { } while(0)
#endif // !CWDEBUG_DEBUGOUTPUT

#if CWDEBUG_DEBUG
namespace libcwd {

namespace _private_ {
  // Dummy type used as fake 'ostream' to write to write(2).
  enum raw_write_nt { raw_write };
} // namespace _private_

_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, char const* data);
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, void const* data);
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, bool data);
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, char data);
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, unsigned long data);
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, long data);
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, int data);
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, unsigned int data);

} // namespace libcwd
#endif // CWDEBUG_DEBUG

#endif // RAW_WRITE_H

#if CWDEBUG_DEBUG
#include <string>
namespace libcwd {

_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, std::string const& data);

} // namespace libcwd
#endif // CWDEBUG_DEBUG
