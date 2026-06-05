// SPDX-FileCopyrightText: 2001-2005, 2008, 2010, 2017-2021, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_threading.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_THREADING_H
#define LIBCWD_PRIVATE_THREADING_H

#define LIBCWD_DEBUGDEBUGRWLOCK 0

#if LIBCWD_DEBUGDEBUGRWLOCK
#include "raw_write.h"
extern pthread_mutex_t LIBCWD_DEBUGDEBUGLOCK_CERR_mutex;
extern unsigned int LIBCWD_DEBUGDEBUGLOCK_CERR_count;
#define LIBCWD_DEBUGDEBUGRWLOCK_CERR(x) \
        do { \
	  pthread_mutex_lock(&LIBCWD_DEBUGDEBUGLOCK_CERR_mutex); \
	  FATALDEBUGDEBUG_CERR(x); \
	  pthread_mutex_unlock(&LIBCWD_DEBUGDEBUGLOCK_CERR_mutex); \
	} while(0)
#define LIBCWD_DEBUGDEBUGLOCK_CERR(x) \
	do { \
	  pthread_mutex_lock(&LIBCWD_DEBUGDEBUGLOCK_CERR_mutex); \
	  ++LIBCWD_DEBUGDEBUGLOCK_CERR_count; \
          FATALDEBUGDEBUG_CERR("[" << LIBCWD_DEBUGDEBUGLOCK_CERR_count << "] " << pthread_self() << ": " << x); \
	  pthread_mutex_unlock(&LIBCWD_DEBUGDEBUGLOCK_CERR_mutex); \
	} while(0)
#else // !LIBCWD_DEBUGDEBUGRWLOCK
#define LIBCWD_DEBUGDEBUGRWLOCK_CERR(x) do { } while(0)
#define LIBCWD_DEBUGDEBUGLOCK_CERR(x) do { } while(0)
#endif // !LIBCWD_DEBUGDEBUGRWLOCK

#include "private_struct_TSD.h"
#include "private_mutex_instances.h"
#include "core_dump.h"

#ifdef LIBCWD_HAVE_PTHREAD
#include <pthread.h>
#else
#error Fatal error: thread support was not detected during configuration of libcwd.
#endif // LIBCWD_HAVE_PTHREAD

#if CWDEBUG_DEBUGT
#define LibcwDebugThreads(x) do { x; } while(0)
#else
#define LibcwDebugThreads(x) do { } while(0)
#endif

#if CWDEBUG_DEBUGT || CWDEBUG_DEBUG
#include "private_assert.h"
#endif

namespace libcwd {

#if LIBCWD_DEBUGDEBUGRWLOCK
inline
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, pthread_mutex_t const& mutex)
{
  raw_write << "(pthread_mutex_t&)" << (void*)&mutex <<
    " = { __data = "
         "{ __lock = " << mutex.__data.__lock << ", "
           "__count = " << mutex.__data.__count << ", "
           "__owner = " << mutex.__data.__owner << ", "
           "__nusers = " << mutex.__data.__nusers << ", "
           "__kind = " << mutex.__data.__kind << "} }";
  return raw_write;
}
#endif

namespace _private_ {

// We have to use macros because pthread_cleanup_push and pthread_cleanup_pop
// are macros with an unmatched '{' and '}' respectively.
#define LIBCWD_DISABLE_CANCEL \
    { \
      LIBCWD_DISABLE_CANCEL_NO_BRACE
#define LIBCWD_DISABLE_CANCEL_NO_BRACE \
      int __libcwd_oldstate; \
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &__libcwd_oldstate); \
      LibcwDebugThreads( ++__libcwd_tsd.cancel_explicitely_disabled )
#define LIBCWD_ASSERT_USERSPACE_OR_DEFERED_BEFORE_SETCANCELSTATE
#define LIBCWD_ENABLE_CANCEL_NO_BRACE \
      LibcwDebugThreads(\
	LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_disabled > 0 ); \
	--__libcwd_tsd.cancel_explicitely_disabled; \
	LIBCWD_ASSERT_USERSPACE_OR_DEFERED_BEFORE_SETCANCELSTATE; \
      ); \
      pthread_setcancelstate(__libcwd_oldstate, NULL)
#define LIBCWD_ENABLE_CANCEL \
      LIBCWD_ENABLE_CANCEL_NO_BRACE; \
    }

#define LIBCWD_DEFER_CANCEL \
    { \
      LIBCWD_DEFER_CANCEL_NO_BRACE
#define LIBCWD_DEFER_CANCEL_NO_BRACE \
      int __libcwd_oldtype; \
      pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &__libcwd_oldtype); \
      LIBCWD_TSD_DECLARATION; \
      LibcwDebugThreads( ++__libcwd_tsd.cancel_explicitely_deferred )
#define LIBCWD_RESTORE_CANCEL_NO_BRACE \
      LibcwDebugThreads(\
	LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred > 0 ); \
	--__libcwd_tsd.cancel_explicitely_deferred; \
	LIBCWD_ASSERT_USERSPACE_OR_DEFERED_BEFORE_SETCANCELSTATE; \
      ); \
      pthread_setcanceltype(__libcwd_oldtype, NULL)
#define LIBCWD_RESTORE_CANCEL \
      LIBCWD_RESTORE_CANCEL_NO_BRACE; \
    }

#define LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED \
    LibcwDebugThreads( \
	/* When entering a critical area, make sure that we have explictely deferred cancellation of this */ \
	/* thread (or disabled that) because when cancellation would happen in the middle of the critical */ \
	/* area then the lock would stay locked.                                                          */ \
	LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred || __libcwd_tsd.cancel_explicitely_disabled ); \
    )

} // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_THREADING_H
