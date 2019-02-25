// $Header$
//
// Copyright (C) 2002 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_PRIVATE_MUTEX_INL
#define LIBCWD_PRIVATE_MUTEX_INL
#if LIBCWD_THREAD_SAFE

namespace libcwd {
  namespace _private_ {

inline
void mutex_ct::initialize()
{
  if (M_initialized)	// Check if `M_mutex' already has been initialized.
    return;		// No need to lock: `M_initialized' is only set after it is
			// really initialized.
  M_initialize();
}

inline
#if CWDEBUG_DEBUGT
bool mutex_ct::try_lock(LIBCWD_TSD_PARAM)
#else
bool mutex_ct::try_lock()
#endif
{
  LibcwDebugThreads( LIBCWD_ASSERT( M_initialized ) );
  LibcwDebugThreads( LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred || __libcwd_tsd.cancel_explicitely_disabled ) );
  bool success = (pthread_mutex_trylock(&M_mutex) == 0);
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
  if (success)
  {
#if CWDEBUG_DEBUGT
    _private_::test_for_deadlock(this, __libcwd_tsd, __builtin_return_address(0));
#endif
    M_instance_locked += 1;
#if CWDEBUG_DEBUGT
    M_locked_by = pthread_self();
    M_locked_from = __builtin_return_address(0);
#endif
  }
#endif
  LibcwDebugThreads( if (success) ++__libcwd_tsd.inside_critical_area; );
  return success;
}

#if CWDEBUG_DEBUGT
inline
bool mutex_ct::try_lock()
{
  LIBCWD_TSD_DECLARATION;
  return try_lock(LIBCWD_TSD);
}
#endif


inline
#if CWDEBUG_DEBUGT
void mutex_ct::lock(LIBCWD_TSD_PARAM)
#else
void mutex_ct::lock()
#endif
{
  LibcwDebugThreads( LIBCWD_ASSERT( M_initialized ) );
  LibcwDebugThreads( LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred || __libcwd_tsd.cancel_explicitely_disabled ) );
  LibcwDebugThreads( ++__libcwd_tsd.inside_critical_area; );
#if LIBCWD_DEBUGDEBUGRWLOCK
  LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": locking mutex " << (void*)this);
#endif
#if CWDEBUG_DEBUGT
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
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
#if CWDEBUG_DEBUGT
  _private_::test_for_deadlock(this, __libcwd_tsd, __builtin_return_address(0));
#endif
  M_instance_locked += 1;
#if CWDEBUG_DEBUGT
  M_locked_by = pthread_self();
  M_locked_from = __builtin_return_address(0);
#endif
#endif
}

#if CWDEBUG_DEBUGT
inline
void mutex_ct::lock()
{
  LIBCWD_TSD_DECLARATION;
  lock(LIBCWD_TSD);
}
#endif

inline
#if CWDEBUG_DEBUGT
void mutex_ct::unlock(LIBCWD_TSD_PARAM)
#else
void mutex_ct::unlock()
#endif
{
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
  M_instance_locked -= 1;
#if CWDEBUG_DEBUGT
  M_locked_by = 0;
#endif
#endif
  LibcwDebugThreads( LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred || __libcwd_tsd.cancel_explicitely_disabled ) );
#if LIBCWD_DEBUGDEBUGRWLOCK
  LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": unlocking mutex " << (void*)this);
#endif
  pthread_mutex_unlock(&M_mutex);
#if LIBCWD_DEBUGDEBUGRWLOCK
  LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": mutex " << (void*)this << " unlocked");
#endif
  LibcwDebugThreads( --__libcwd_tsd.inside_critical_area; );
}

#if CWDEBUG_DEBUGT
inline
void mutex_ct::unlock()
{
  LIBCWD_TSD_DECLARATION;
  unlock(LIBCWD_TSD);
}
#endif

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
inline
bool mutex_ct::is_locked()
{
  return M_instance_locked > 0;
}
#endif

  } // namespace _private_
} // namespace libcwd

#endif // LIBCWD_THREAD_SAFE
#endif // LIBCWD_PRIVATE_MUTEX_INL

