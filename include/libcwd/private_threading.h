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

extern void initialize_global_mutexes();
extern bool WST_multi_threaded;

//===================================================================================================
//
// Mutex locking.
//
// template <int instance>	 This class may not use system calls (it may not call malloc(3)).
//   class mutex_tct;
//
// Usage.
//
// Global mutexes can be initialized once, before using the mutex.
// mutex_tct<instance_id_const>::initialize();
//
// Static mutexes in functions (or templates) that can not globally be initialized need to call
// `initialize()' prior to each use. With optimization this is at most a single initialized flag test.
//

//========================================================================================================================================17"
// class mutex_tct

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

template <int instance>
class mutex_tct {
  public:
    static pthread_mutex_t S_mutex;
  protected:
    static bool volatile S_initialized;
    static void S_initialize();
  public:
    static void initialize()
	{
	  if (S_initialized)	// Check if the static `S_mutex' already has been initialized.
	    return;		//   No need to lock: `S_initialized' is only set after it is
				//   really initialized.
	  S_initialize();
        }
  public:
    static bool try_lock()
    {
      LibcwDebugThreads( LIBCWD_ASSERT( S_initialized ) );
#if CWDEBUG_DEBUGT
      LIBCWD_TSD_DECLARATION;
#endif
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED;
      LIBCWD_DEBUGDEBUGLOCK_CERR("Trying to lock mutex " << instance << " (" << (void*)&S_mutex << ") from " << __builtin_return_address(0) << " from " << __builtin_return_address(1));
      LIBCWD_DEBUGDEBUGLOCK_CERR("pthread_mutex_trylock(" << S_mutex << ").");
      bool success = (pthread_mutex_trylock(&S_mutex) == 0);
      LIBCWD_DEBUGDEBUGLOCK_CERR("Result = " << success << ". Mutex now " << S_mutex << ".");
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
      if (success)
      {
	LIBCWD_DEBUGDEBUGLOCK_CERR("mutex_tct::try_lock(): instance_locked[" << instance << "] == " << instance_locked[instance] << "; incrementing it.");
	instance_locked[instance] += 1;
      }
#endif
      LibcwDebugThreads( if (success) { ++__libcwd_tsd.inside_critical_area; } );
      return success;
    }
    static void lock()
    {
      LibcwDebugThreads( LIBCWD_ASSERT( S_initialized ) );
#if CWDEBUG_DEBUGT
      LIBCWD_TSD_DECLARATION;
#endif
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED;
      LibcwDebugThreads( ++__libcwd_tsd.inside_critical_area );
      LIBCWD_DEBUGDEBUGLOCK_CERR("locking mutex " << instance << " (" << (void*)&S_mutex << ") from " << __builtin_return_address(0) << " from " << __builtin_return_address(1));
#if CWDEBUG_DEBUGT
      if constexpr (!(instance >= 2 * reserved_instance_low && instance < 3 * reserved_instance_low))
      {
	__libcwd_tsd.waiting_for_lock = instance;
	LIBCWD_DEBUGDEBUGLOCK_CERR("pthread_mutex_lock(" << S_mutex << ").");
        int res = pthread_mutex_lock(&S_mutex);
	LIBCWD_DEBUGDEBUGLOCK_CERR("Result = " << res << ". Mutex now " << S_mutex << ".");
#if LIBCWD_DEBUGDEBUGRWLOCK
	LIBCWD_ASSERT( res == 0 || res == EDEADLK );
#endif
        __libcwd_tsd.waiting_for_lock = 0;
	LIBCWD_ASSERT( res == 0 );
      }
      else
      {
	LIBCWD_DEBUGDEBUGLOCK_CERR("pthread_mutex_lock(" << S_mutex << ").");
	int res = pthread_mutex_lock(&S_mutex);
	LIBCWD_DEBUGDEBUGLOCK_CERR("Result = " << res << ". Mutex now " << S_mutex << ".");
	LIBCWD_ASSERT( res == 0 );
      }
#else // !CWDEBUG_DEBUGT
      pthread_mutex_lock(&S_mutex);
#endif // !CWDEBUG_DEBUGT
      LIBCWD_DEBUGDEBUGLOCK_CERR("Lock " << instance << " obtained (" << (void*)&S_mutex << ").");
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
      LIBCWD_DEBUGDEBUGLOCK_CERR("mutex_tct::lock(): instance_locked[" << instance << "] == " << instance_locked[instance] << "; incrementing it.");
      instance_locked[instance] += 1;
#endif
    }
    static void unlock()
    {
#if CWDEBUG_DEBUGT
      LIBCWD_TSD_DECLARATION;
#endif
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED;
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
      LIBCWD_DEBUGDEBUGLOCK_CERR("mutex_tct::unlock(): instance_locked[" << instance << "] == " << instance_locked[instance] << "; decrementing it.");
      LIBCWD_ASSERT( instance_locked[instance] > 0 );
      instance_locked[instance] -= 1;
#endif
      LIBCWD_DEBUGDEBUGLOCK_CERR("unlocking mutex " << instance << " (" << (void*)&S_mutex << ").");
      LIBCWD_DEBUGDEBUGLOCK_CERR("pthread_mutex_unlock(" << S_mutex << ").");
#if CWDEBUG_DEBUGT
      int res =
#endif
      pthread_mutex_unlock(&S_mutex);
#if CWDEBUG_DEBUGT
      LIBCWD_DEBUGDEBUGLOCK_CERR("Result = " << res << ". Mutex now " << S_mutex << ".");
      LIBCWD_ASSERT(res == 0);
#endif
      LIBCWD_DEBUGDEBUGLOCK_CERR("Lock " << instance << " released (" << (void*)&S_mutex << ").");
      LibcwDebugThreads( --__libcwd_tsd.inside_critical_area );
    }
};

template <int instance>
  bool volatile mutex_tct<instance>::S_initialized = false;

template <int instance>
  void mutex_tct<instance>::S_initialize()
  {
    if constexpr (instance == mutex_initialization_instance)	// Specialization.
    {
      pthread_mutexattr_t mutex_attr;
      pthread_mutexattr_init(&mutex_attr);
#if CWDEBUG_DEBUGT
      pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
      pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
      pthread_mutex_init(&S_mutex, &mutex_attr);
      pthread_mutexattr_destroy(&mutex_attr);
      S_initialized = true;
    }
    else						// General case.
    {
      mutex_tct<mutex_initialization_instance>::initialize();
      if (!S_initialized)					// Check again now that we are locked.
      {
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
#if CWDEBUG_DEBUGT
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
	pthread_mutex_init(&S_mutex, &mutex_attr);
	pthread_mutexattr_destroy(&mutex_attr);
	S_initialized = true;
      }
    }
  }

template <int instance>
  pthread_mutex_t mutex_tct<instance>::S_mutex;

extern void fatal_cancellation(void*);

} // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_THREADING_H
