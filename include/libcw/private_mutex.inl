// $Header$
//
// Copyright (C) 2002, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
// 

#ifndef LIBCW_PRIVATE_MUTEX_INL
#define LIBCW_PRIVATE_MUTEX_INL
#if LIBCWD_THREAD_SAFE

namespace libcw {
  namespace debug {
    namespace _private_ {

__inline__
void mutex_ct::initialize(void) throw()
{
  if (M_initialized)	// Check if `M_mutex' already has been initialized.
    return;		// No need to lock: `M_initialized' is only set after it is
			// really initialized.
  M_initialize();
}

__inline__
bool mutex_ct::trylock(void) throw()
{
  LibcwDebugThreads( LIBCWD_ASSERT( M_initialized ) );
  LibcwDebugThreads( LIBCWD_TSD_DECLARATION; LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred || __libcwd_tsd.cancel_explicitely_disabled ) );
  bool success = (pthread_mutex_trylock(&M_mutex) == 0);
#if CWDEBUG_DEBUG
  if (success)
  {
    M_instance_locked += 1;
#if CWDEBUG_DEBUGT
    M_locked_by = pthread_self();
    M_locked_from = __builtin_return_address(0);
#endif
  }
#endif
  LibcwDebugThreads( if (success) { LIBCWD_TSD_DECLARATION; ++__libcwd_tsd.inside_critical_area; } );
  return success;
}

__inline__
void mutex_ct::lock(void) throw()
{
  LibcwDebugThreads( LIBCWD_ASSERT( M_initialized ) );
  LibcwDebugThreads( LIBCWD_TSD_DECLARATION; LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred || __libcwd_tsd.cancel_explicitely_disabled ) );
  LibcwDebugThreads( LIBCWD_TSD_DECLARATION ++__libcwd_tsd.inside_critical_area; );
#if LIBCWD_DEBUGDEBUGRWLOCK
  LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": locking mutex " << (void*)this);
#endif
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION
  __libcwd_tsd.waiting_for_mutex = this;
  int res =
#endif
  pthread_mutex_lock(&M_mutex);
#if CWDEBUG_DEBUGT
  __libcwd_tsd.waiting_for_mutex = 0;
#endif
  LibcwDebugThreads( LIBCWD_ASSERT( res == 0 ) );
#if LIBCWD_DEBUGDEBUGRWLOCK
  LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": mutex " << (void*)this << " locked");
#endif
#if CWDEBUG_DEBUG
  M_instance_locked += 1;
#if CWDEBUG_DEBUGT
  M_locked_by = pthread_self();
  M_locked_from = __builtin_return_address(0);
#endif
#endif
}

__inline__
void mutex_ct::unlock(void) throw()
{
#if CWDEBUG_DEBUG
  M_instance_locked -= 1;
#if CWDEBUG_DEBUGT
  M_locked_by = 0;
#endif
#endif
  LibcwDebugThreads( LIBCWD_TSD_DECLARATION; LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred || __libcwd_tsd.cancel_explicitely_disabled ) );
#if LIBCWD_DEBUGDEBUGRWLOCK
  LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": unlocking mutex " << (void*)this);
#endif
  pthread_mutex_unlock(&M_mutex);
#if LIBCWD_DEBUGDEBUGRWLOCK
  LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": mutex " << (void*)this << " unlocked");
#endif
  LibcwDebugThreads( LIBCWD_TSD_DECLARATION --__libcwd_tsd.inside_critical_area; );
}

#if CWDEBUG_DEBUG
__inline__
bool mutex_ct::is_locked(void) throw()
{
  return M_instance_locked > 0;
}
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCWD_THREAD_SAFE
#endif // LIBCW_PRIVATE_MUTEX_INL

