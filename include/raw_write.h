// $Header$
//
// Copyright (C) 2001 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_PRIVATE_THREADING_H
#include <libcwd/private_threading.h>
#endif

#ifndef RAW_WRITE_H
#define RAW_WRITE_H

#include <libcwd/config.h>

#if CWDEBUG_DEBUG
#if CWDEBUG_DEBUGT
#define WRITE __libc_write
#else
#define WRITE write
#endif
extern "C" ssize_t write(int fd, const void *buf, size_t count);

#if LIBCWD_THREAD_SAFE
namespace libcwd {
  namespace _private_ {
    extern pthread_mutex_t raw_write_mutex;
  }
}
#define LIBCWD_CANCELSTATE_DISABLE int __libcwd_oldstate; pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &__libcwd_oldstate)
#define LIBCWD_CANCELSTATE_RESTORE pthread_setcancelstate(__libcwd_oldstate, NULL)
#else
#define LIBCWD_CANCELSTATE_DISABLE do { } while(0)
#define LIBCWD_CANCELSTATE_RESTORE do { } while(0)
#endif

#if CWDEBUG_ALLOC
#define LIBCWD_LIBRARY_CALL_INDENTATION \
	/*  __libcwd_lcwc means library_call write counter.  Used to avoid the 'scope of for changed' warning. */ \
	for (int __libcwd_lcwc = 0; __libcwd_lcwc < __libcwd_tsd.library_call; ++__libcwd_lcwc)	\
	  ::write(2, "    ", 4)
#else
#define LIBCWD_LIBRARY_CALL_INDENTATION do { } while(0)
#endif

// The difference between DEBUGDEBUG_CERR and FATALDEBUGDEBUG_CERR is that the latter is not suppressed
// when --disable-debug-output is used because a fatal error occured anyway, so this can't
// disturb the testsuite.
#define FATALDEBUGDEBUG_CERR(x)									\
    do {											\
      if (1/*::libcwd::_private_::WST_ios_base_initialized FIXME: uncomment again*/) {	\
	LIBCWD_CANCELSTATE_DISABLE;								\
	LIBCWD_TSD_DECLARATION;									\
	LibcwDebugThreads( ++__libcwd_tsd.internal_debugging_code );				\
	LibcwDebugThreads( pthread_mutex_lock(&::libcwd::_private_::raw_write_mutex) );	\
	::write(2, "CWDEBUG_DEBUG: ", 15);							\
	LIBCWD_LIBRARY_CALL_INDENTATION;							\
	LibcwDebugThreads( ::libcwd::_private_::raw_write << pthread_self() << ": ");	\
	::libcwd::_private_::raw_write << x << '\n';					\
	LibcwDebugThreads( pthread_mutex_unlock(&::libcwd::_private_::raw_write_mutex) );	\
	LibcwDebugThreads( --__libcwd_tsd.internal_debugging_code );				\
	LIBCWD_CANCELSTATE_RESTORE;								\
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

#if !defined(LIBCWD_NO_INTERNAL_STRING) && !defined(LIBCWD_RAW_WRITE_INTERNAL_STRING)
#define LIBCWD_RAW_WRITE_INTERNAL_STRING
#ifndef LIBCWD_NO_INTERNAL_STRING
#include <libcwd/private_internal_string.h>
#endif
#if CWDEBUG_DEBUG
namespace libcwd {

_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, libcwd::_private_::internal_string const& data);

} // namespace libcwd
#endif // CWDEBUG_DEBUG
#endif // !LIBCWD_NO_INTERNAL_STRING

