// $Header$
//
// Copyright (C) 2001, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
// 

/** \file libcw/private_threading.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_THREADING_H
#define LIBCW_PRIVATE_THREADING_H

#ifndef LIBCW_PRIVATE_TSD_H
#include <libcw/private_TSD.h>
#endif
#ifndef LIBCW_PRIVATE_SET_ALLOC_CHECKING_H
#include <libcw/private_set_alloc_checking.h>
#endif

#ifdef LIBCWD_HAVE_PTHREAD
#include <pthread.h>
#if defined(PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP) && defined(PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP)
#define LIBCWD_USE_LINUXTHREADS
#else
#define LIBCWD_USE_POSIX_THREADS
#endif
#endif // LIBCWD_HAVE_PTHREAD

#ifndef DEBUGDEBUG
#define LIBCWD_DEBUGTHREADS 0		// Toggle this if necessary.
#else
#define LIBCWD_DEBUGTHREADS 1
#endif

#if LIBCWD_DEBUGTHREADS
#ifndef LIBCW_CASSERT
#define LIBCW_CASSERT
#include <cassert>
#endif
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

//===================================================================================================
// Thread Specific Data
//

#ifdef DEBUGDEBUG
extern bool WST_multi_threaded;
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw
 
#ifdef _REENTRANT
#define LIBCWD_TSD_INSTANCE ::libcw::debug::_private_::\
    thread_specific_data_tct< ::libcw::debug::_private_::TSD_st>::instance()
	// For directly passing the `__libcwd_tsd' instance to a function (foo(TSD::instance())).
#define LIBCWD_COMMA_TSD_INSTANCE , LIBCWD_TSD_INSTANCE
	// Idem, but as second or higher parameter.
#define LIBCWD_TSD_DECLARATION ::libcw::debug::_private_::\
    TSD_st& __libcwd_tsd(::libcw::debug::_private_::\
	                 thread_specific_data_tct< ::libcw::debug::_private_::TSD_st>::instance());
	// Declaration of local `__libcwd_tsd' structure reference.
#else // !_REENTRANT
#define LIBCWD_TSD_INSTANCE
#define LIBCWD_COMMA_TSD_INSTANCE
#define LIBCWD_TSD_DECLARATION
#endif // !_REENTRANT

// End of Thread Specific Data
//===================================================================================================

#ifdef _REENTRANT

namespace libcw {
  namespace debug {
    namespace _private_ {

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
// Static mutexes in functions (or templates) that can not globally
// be initialized need to call `initialize()' prior to *each* use
// (using -O2 this is at most a single test and nothing at all when
// Linuxthreads are being used.
//
// Examples.
//
// mutex_tct<instance_id_const>::initialize();
// if (mutex_tct<instance_id_const>::trylock()) ...
// mutex_tct<instance_id_const>::lock();
// mutex_tct<instance_id_const>::unlock();
//

// The different instance-ids used in libcwd.
enum mutex_instance_nt {
  // Recursive mutexes.
  tsd_initialization_instance,
  object_files_instance,	// rwlock
  end_recursive_types,
  // Fast mutexes.
  mutex_initialization_instance,
  ids_singleton_tct_S_ids_instance,
  alloc_tag_desc_instance,
  type_info_of_instance,
  memblk_map_instance,		// rwlock
  dlopen_map_instance,
  write_max_len_instance,
  debug_objects_instance,	// rwlock
  debug_channels_instance,	// rwlock
  // Values reserved for read/write locks.
  reserved_instance_low,
  reserved_instance_high = 4 * reserved_instance_low,
  // Values reserved for test executables.
  test_instance0 = reserved_instance_high,
  test_instance1,
  test_instance2,
  test_instance3,
  instance_locked_size		// Must be last in list
};

#ifdef DEBUGDEBUG
extern int instance_locked[instance_locked_size];	// MT: Each element is locked by the
							//     corresponding instance.
__inline__ bool is_locked(int instance) { return instance_locked[instance] > 0; }
#endif

#if defined(LIBCWD_USE_POSIX_THREADS) || defined(LIBCWD_USE_LINUXTHREADS)
template <int instance>
  class mutex_tct {
  private:
    static pthread_mutex_t S_mutex;
#ifndef LIBCWD_USE_LINUXTHREADS
    static bool S_initialized;
  private:
    static void S_initialize(void) throw();
#endif
  public:
    static void initialize(void) throw()
#ifdef LIBCWD_USE_LINUXTHREADS
	{ }
#else
	{
	  if (S_initialized)	// Check if the static `S_mutex' already has been initialized.
	    return;		//   No need to lock: `S_initialized' is only set after it is
				//   really initialized.
	  S_initialize();
        }
#endif
  public:
    static bool trylock(void) throw()
    {
#ifndef DEBUGDEBUG
      return (pthread_mutex_trylock(&S_mutex) == 0);
#else
      bool res = (pthread_mutex_trylock(&S_mutex) == 0);
      if (res)
	instance_locked[instance] += 1;
      return res;
#endif
    }
    static void lock(void) throw()
    {
      pthread_mutex_lock(&S_mutex);
#ifdef DEBUGDEBUG
      instance_locked[instance] += 1;
#endif
    }
    static void unlock(void) throw()
    {
#ifdef DEBUGDEBUG
      instance_locked[instance] -= 1;
#endif
      pthread_mutex_unlock(&S_mutex);
    }
  };

#ifndef LIBCWD_USE_LINUXTHREADS
template <int instance>
  void mutex_tct<instance>::S_initialize(void) throw()
  {
    if (instance != mutex_initialization_instance)	// The mutex_initialization_instance is
      mutex_tct<mutex_initialization_instance>::lock();	// initialized before threads are created
							// (from initialize_global_mutexes()).
    if (!S_initialized)					// Check again now that we are locked.
    {
      pthread_mutexattr_t mutex_attr;
      if (instance < end_recursive_types)
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
      else
      {
#if LIBCWD_DEBUGTHREADS
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
      }
      pthread_mutex_init(&S_mutex, &mutex_attr);
      S_initialized = true;
    }
    if (instance != mutex_initialization_instance)
      mutex_tct<mutex_initialization_instance>::unlock();
  }
#endif // LIBCWD_USE_LINUXTHREADS

#ifndef LIBCWD_USE_LINUXTHREADS
template <int instance>
  bool mutex_tct<instance>::S_initialized = false;
#endif

template <int instance>
  pthread_mutex_t mutex_tct<instance>::S_mutex
#ifdef LIBCWD_USE_LINUXTHREADS
      =
#if LIBCWD_DEBUGTHREADS
      	PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#else
      	PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
#endif

// Specialization.
template <>
  extern pthread_mutex_t mutex_tct<tsd_initialization_instance>::S_mutex;
      // = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#else // !LIBCWD_USE_LINUXTHREADS
      ;
#endif // !LIBCWD_USE_LINUXTHREADS

#endif // defined(LIBCWD_USE_POSIX_THREADS) || defined(LIBCWD_USE_LINUXTHREADS)

//
// template <int instance>	This class may not use system calls (it may not call malloc(3)).
//   class rwlock_tct;
//
// Read/write mutex lock implementation.  Readers can set arbitrary number of locks, only locking
// writers.  Writers lock readers and writers.
//
// Examples.
//
// rwlock_tct<instance_id_const>::initialize();
// if (rwlock_tct<instance_id_const>::tryrdlock()) ...
// if (rwlock_tct<instance_id_const>::trywrlock()) ...
// rwlock_tct<instance_id_const>::rdlock();		// Readers lock.
// rwlock_tct<instance_id_const>::rdunlock();
// rwlock_tct<instance_id_const>::wrlock();		// Writers lock.
// rwlock_tct<instance_id_const>::wrunlock();
// rwlock_tct<instance_id_const>::rd2wrlock();		// Convert read lock into write lock.
// rwlock_tct<instance_id_const>::wr2rdlock();		// Convert write lock into read lock.
//

template <int instance>
  class rwlock_tct {
  private:
    static int const readers_instance = instance + reserved_instance_low;
    static int const writers_instance = instance + 2 * reserved_instance_low;
    static int const readers_count_instance = instance + 3 * reserved_instance_low;
    static int S_readers_count;
    static bool S_writer_is_waiting;
    static pthread_t S_writer_id;
  public:
    static void initialize(void) throw()
    {
      mutex_tct<readers_instance>::initialize();
      mutex_tct<writers_instance>::initialize();
      mutex_tct<readers_count_instance>::initialize();
    }
    static bool tryrdlock(void) throw()
    {
      if (instance < end_recursive_types && pthread_equal(S_writer_id, pthread_self()))
        return true;					// No error checking is done.
      if (S_writer_is_waiting || !mutex_tct<readers_count_instance>::trylock())
        return false;
      bool success = (++S_readers_count != 1) || mutex_tct<writers_instance>::trylock();
      if (!success)
	S_readers_count = 0;
      mutex_tct<readers_count_instance>::unlock();
      return success;
    }
    static bool trywrlock(void) throw()
    {
#ifndef DEBUGDEBUG
      return mutex_tct<writers_instance>::trylock();
#else
      bool res = mutex_tct<writers_instance>::trylock();
      if (res)
	instance_locked[instance] += 1;
      return res;
#endif
    }
    static void rdlock(void) throw()
    {
      if (instance < end_recursive_types && pthread_equal(S_writer_id, pthread_self()))
	return;						// No error checking is done.
      if (S_writer_is_waiting)
      {
	mutex_tct<readers_instance>::lock();
        mutex_tct<readers_instance>::unlock();
      }
      mutex_tct<readers_count_instance>::lock();
      if (++S_readers_count == 1)
        mutex_tct<writers_instance>::lock();		// Warning: must be done while
      							// S_readers_count is still locked!
      mutex_tct<readers_count_instance>::unlock();
    }
    static void rdunlock(void) throw()
    {
      if (instance < end_recursive_types && pthread_equal(S_writer_id, pthread_self()))
	return;						// No error checking is done.
      mutex_tct<readers_count_instance>::lock();
      if (--S_readers_count == 0)
        mutex_tct<writers_instance>::unlock();
      mutex_tct<readers_count_instance>::unlock();
    }
    static void wrlock(void) throw()
    {
      mutex_tct<readers_instance>::lock();		// Block new readers,
      S_writer_is_waiting = true;
      mutex_tct<writers_instance>::lock();		// ... while waiting for current readers to
      							// be finished (if any).
      if (instance < end_recursive_types)
        S_writer_id = pthread_self();
      S_writer_is_waiting = false;
      mutex_tct<readers_instance>::unlock();
#ifdef DEBUGDEBUG
      instance_locked[instance] += 1;
#endif
    }
    static void wrunlock(void) throw()
    {
#ifdef DEBUGDEBUG
      instance_locked[instance] -= 1;
#endif
      if (instance < end_recursive_types)
        S_writer_id = 0;
      mutex_tct<writers_instance>::unlock();
    }
    static void rd2wrlock() throw()
    {
      mutex_tct<readers_count_instance>::lock();
      bool inherit_lock = (--S_readers_count == 0);
      mutex_tct<readers_count_instance>::unlock();
      if (!inherit_lock)
      {
	mutex_tct<readers_instance>::lock();		// Block new readers,
	S_writer_is_waiting = true;
	mutex_tct<writers_instance>::lock();		// ... while waiting for remaining readers to
							// be finished.
        if (instance < end_recursive_types)
          S_writer_id = pthread_self();
	S_writer_is_waiting = false;
	mutex_tct<readers_instance>::unlock();
      }
#ifdef DEBUGDEBUG
      instance_locked[instance] += 1;
#endif
    }
    static void wr2rdlock() throw()
    {
#ifdef DEBUGDEBUG
      instance_locked[instance] -= 1;
#endif
      mutex_tct<readers_count_instance>::lock();
      if (instance < end_recursive_types)
        S_writer_id = 0;
      ++S_readers_count;
      mutex_tct<readers_count_instance>::unlock();
    }
  };

template <int instance>
  int rwlock_tct<instance>::S_readers_count = 0;

template <int instance>
  bool rwlock_tct<instance>::S_writer_is_waiting = 0;

template <int instance>
  pthread_t rwlock_tct<instance>::S_writer_id = 0;

//===================================================================================================
//
// Thread Specific Data.
//

#if defined(LIBCWD_USE_POSIX_THREADS) || defined(LIBCWD_USE_LINUXTHREADS)
template<class TSD>
  class thread_specific_data_tct {
  private:
    static pthread_once_t S_key_once;
    static pthread_key_t S_key;
    static TSD* S_temporary_instance;
    static bool S_initializing;
    static bool S_WNS_initialized;
    static void S_alloc_key(void) throw();
    static TSD* S_initialize(void) throw();
    static void S_destroy(void* tsd_ptr) throw();
  public:
    static TSD& instance(void) throw()
    {
      TSD* instance = reinterpret_cast<TSD*>(pthread_getspecific(S_key));
      if (!instance)
	instance = S_initialize();
      return *instance;
    }
    static bool initialized(void) { return S_WNS_initialized; }
  };

template<class TSD>
  pthread_once_t thread_specific_data_tct<TSD>::S_key_once = PTHREAD_ONCE_INIT;

template<class TSD>
  pthread_key_t thread_specific_data_tct<TSD>::S_key;

template<class TSD>
  TSD* thread_specific_data_tct<TSD>::S_temporary_instance;

template<class TSD>
  bool thread_specific_data_tct<TSD>::S_initializing;

template<class TSD>
  bool thread_specific_data_tct<TSD>::S_WNS_initialized;

template<class TSD>
  void thread_specific_data_tct<TSD>::S_destroy(void* tsd_ptr) throw()
  {
    TSD* instance = reinterpret_cast<TSD*>(tsd_ptr);
    LIBCWD_TSD_DECLARATION
    set_alloc_checking_off(LIBCWD_TSD);
    delete instance;
#if LIBCWD_DEBUGTHREADS
    // Hopefully S_destroy doesn't get called in the thread itself!
    assert( instance != &thread_specific_data_tct<TSD>::instance() );
#endif
    set_alloc_checking_on(LIBCWD_TSD);
  }

template<class TSD>
  void thread_specific_data_tct<TSD>::S_alloc_key(void) throw()
  {
    pthread_key_create(&S_key, S_destroy);
  }

template<class TSD>
  TSD* thread_specific_data_tct<TSD>::S_initialize(void) throw()
  {
    pthread_once(&S_key_once, S_alloc_key);
    mutex_tct<tsd_initialization_instance>::lock();
    if (S_initializing)
    {
      mutex_tct<tsd_initialization_instance>::unlock();
      return S_temporary_instance;
    }
    TSD new_TSD;
    S_temporary_instance = &new_TSD;
    S_initializing = true;
    LIBCWD_TSD_DECLARATION
    set_alloc_checking_off(LIBCWD_TSD);
    TSD* instance = new TSD;
    set_alloc_checking_on(LIBCWD_TSD);
    pthread_setspecific(S_key, instance);
    S_initializing = false;
    S_WNS_initialized = true;
    mutex_tct<tsd_initialization_instance>::unlock();
    return instance;
  }
#endif // defined(LIBCWD_USE_POSIX_THREADS) || defined(LIBCWD_USE_LINUXTHREADS)

extern void initialize_global_mutexes(void) throw();

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // _REENTRANT
#endif // LIBCW_PRIVATE_THREADING_H

