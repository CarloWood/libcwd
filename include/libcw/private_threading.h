// $Header$
//
// Copyright (C) 2001 - 2002, by
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

#define LIBCWD_DEBUGDEBUGRWLOCK 0

#if LIBCWD_DEBUGDEBUGRWLOCK
#define LIBCWD_NO_INTERNAL_STRING
#include <raw_write.h>
#undef LIBCWD_NO_INTERNAL_STRING
#define LIBCWD_DEBUGDEBUGRWLOCK_CERR(x) FATALDEBUGDEBUG_CERR(x)
#else
#define LIBCWD_DEBUGDEBUGRWLOCK_CERR(x)
#endif // LIBCWD_DEBUGDEBUGRWLOCK

#ifndef LIBCW_PRIVATE_SET_ALLOC_CHECKING_H
#include <libcw/private_set_alloc_checking.h>
#endif
#ifndef LIBCW_PRIVATE_STRUCT_TSD_H
#include <libcw/private_struct_TSD.h>
#endif
#ifndef LIBCW_CSTRING
#define LIBCW_CSTRING
#include <cstring>			// Needed for std::memset and std::memcpy.
#endif

#ifdef LIBCWD_HAVE_PTHREAD
#include <pthread.h>
#include <semaphore.h>
#if defined(PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP) && defined(PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP)
#define LIBCWD_USE_LINUXTHREADS 1
#else
#define LIBCWD_USE_POSIX_THREADS 1
#endif
#endif // LIBCWD_HAVE_PTHREAD

#ifndef LIBCWD_USE_LINUXTHREADS
#define LIBCWD_USE_LINUXTHREADS 0
#endif
#ifndef LIBCWD_USE_POSIX_THREADS
#define LIBCWD_USE_POSIX_THREADS 0
#endif

#ifdef DEBUGDEBUGTHREADS
#define LibcwDebugThreads(x) x
#else
#define LibcwDebugThreads(x)
#endif

#if defined(DEBUGDEBUGTHREADS) || defined(DEBUGDEBUG)
#ifndef LIBCW_PRIVATE_ASSERT_H
#include <libcw/private_assert.h>
#endif
#endif

#ifdef LIBCWD_THREAD_SAFE

namespace libcw {
  namespace debug {
    namespace _private_ {

extern void initialize_global_mutexes(void) throw();
#ifdef DEBUGDEBUG
extern bool WST_multi_threaded;
#endif

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

// The different instance-ids used in libcwd.
enum mutex_instance_nt {
  // Recursive mutexes.
  tsd_initialization_instance,
  object_files_instance,	// rwlock
  end_recursive_types,
  // Fast mutexes.
  memblk_map_instance,		// rwlock
  mutex_initialization_instance,
  ids_singleton_tct_S_ids_instance,
  alloc_tag_desc_instance,
  type_info_of_instance,
  dlopen_map_instance,
  write_max_len_instance,
  debug_objects_instance,	// rwlock
  debug_channels_instance,	// rwlock
  // Values reserved for read/write locks.
  reserved_instance_low,
  reserved_instance_high = 3 * reserved_instance_low,
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

//========================================================================================================================================17"
// class mutex_tct

#if LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS
// We have to use macros because pthread_cleanup_push and pthread_cleanup_pop
// are macros with an unmatched '{' and '}' respectively.
#define LIBCWD_DISABLE_CANCEL \
    { \
      LIBCWD_DISABLE_CANCEL_NO_BRACE
#define LIBCWD_DISABLE_CANCEL_NO_BRACE \
      int __libcwd_oldstate; \
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &__libcwd_oldstate); \
      LibcwDebugThreads( \
	  { \
	    LIBCWD_TSD_DECLARATION; \
	    ++__libcwd_tsd.cancel_explicitely_disabled; \
	  } )
#define LIBCWD_ENABLE_CANCEL_NO_BRACE \
      LibcwDebugThreads(\
	  { \
	    LIBCWD_TSD_DECLARATION; \
	    LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_disabled > 0 ); \
	    --__libcwd_tsd.cancel_explicitely_disabled; \
	    /* pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) can call      */ \
	    /* __pthread_do_exit() when the thread is cancelled in the meantime. */ \
	    /* This might free allocations that are allocated in userspace.      */ \
	    LIBCWD_ASSERT( !__libcwd_tsd.internal || __libcwd_tsd.cancel_explicitely_disabled || __libcwd_tsd.cancel_explicitely_deferred ); \
	  } ) \
      pthread_setcancelstate(__libcwd_oldstate, NULL);
#define LIBCWD_ENABLE_CANCEL \
      LIBCWD_ENABLE_CANCEL_NO_BRACE \
    }

#define LIBCWD_DEFER_CANCEL \
    { \
      LIBCWD_DEFER_CANCEL_NO_BRACE
#define LIBCWD_DEFER_CANCEL_NO_BRACE \
      int __libcwd_oldtype; \
      pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &__libcwd_oldtype); \
      LibcwDebugThreads( \
	  { \
	    LIBCWD_TSD_DECLARATION; \
	    ++__libcwd_tsd.cancel_explicitely_deferred; \
	  } )
#define LIBCWD_RESTORE_CANCEL_NO_BRACE \
      LibcwDebugThreads(\
	  { \
	    LIBCWD_TSD_DECLARATION; \
	    LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred > 0 ); \
	    --__libcwd_tsd.cancel_explicitely_deferred; \
	    /* pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) will calls */ \
	    /* __pthread_do_exit() when the thread is cancelled in the meantime.   */ \
	    /* This might free allocations that are allocated in userspace.        */ \
	    LIBCWD_ASSERT( !__libcwd_tsd.internal || __libcwd_tsd.cancel_explicitely_disabled || __libcwd_tsd.cancel_explicitely_deferred ); \
	  } ) \
      pthread_setcanceltype(__libcwd_oldtype, NULL);
#define LIBCWD_RESTORE_CANCEL \
      LIBCWD_RESTORE_CANCEL_NO_BRACE \
    }

#if LIBCWD_USE_LINUXTHREADS
#define LIBCWD_DEFER_CLEANUP_PUSH(routine, arg) \
    pthread_cleanup_push_defer_np(reinterpret_cast<void(*)(void*)>(routine), reinterpret_cast<void*>(arg)); \
      LibcwDebugThreads( \
	  { \
	    LIBCWD_TSD_DECLARATION; \
	    ++__libcwd_tsd.cancel_explicitely_deferred; \
	    ++__libcwd_tsd.cleanup_handler_installed; \
	  } )
#define LIBCWD_CLEANUP_POP_RESTORE(execute) \
      LibcwDebugThreads(\
	  { \
	    LIBCWD_TSD_DECLARATION; \
	    --__libcwd_tsd.cleanup_handler_installed; \
	    LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred > 0 ); \
	    --__libcwd_tsd.cancel_explicitely_deferred; \
	    LIBCWD_ASSERT( !__libcwd_tsd.internal ); \
	  } ) \
    pthread_cleanup_pop_restore_np(static_cast<int>(execute))
#else // !LIBCWD_USE_LINUXTHREADS
#define LIBCWD_DEFER_CLEANUP_PUSH(routine, arg) \
      LIBCWD_DEFER_CANCEL \
      LibcwDebugThreads( \
	  { \
	    LIBCWD_TSD_DECLARATION; \
	    ++__libcwd_tsd.cleanup_handler_installed; \
	  } ) \
      pthread_cleanup_push(reinterpret_cast<void(*)(void*)>(routine), reinterpret_cast<void*>(arg))
#define LIBCWD_CLEANUP_POP_RESTORE(execute) \
      LibcwDebugThreads( \
	  { \
	    LIBCWD_TSD_DECLARATION; \
	    --__libcwd_tsd.cleanup_handler_installed; \
	  } ) \
      pthread_cleanup_pop(static_cast<int>(execute)); \
      LIBCWD_RESTORE_CANCEL
#endif // !LIBCWD_USE_LINUXTHREADS

#define LIBCWD_PUSH_DEFER_TRYLOCK_MUTEX(instance, unlock_routine) \
      LIBCWD_DEFER_CLEANUP_PUSH(static_cast<void (*)(void)>(unlock_routine), &::libcw::debug::_private_::mutex_tct<(instance)>::S_mutex); \
      bool __libcwd_lock_successful = ::libcw::debug::_private_::mutex_tct<(instance)>::trylock()
#define LIBCWD_DEFER_PUSH_LOCKMUTEX(instance, unlock_routine) \
      LIBCWD_DEFER_CLEANUP_PUSH(static_cast<void (*)(void)>(unlock_routine), &::libcw::debug::_private_::mutex_tct<(instance)>::S_mutex); \
      ::libcw::debug::_private_::mutex_tct<(instance)>::lock(); \
      bool const __libcwd_lock_successful = true
#define LIBCWD_UNLOCKMUTEX_POP_RESTORE(instance) \
      LIBCWD_CLEANUP_POP_RESTORE(__libcwd_lock_successful)

#define LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED \
    LibcwDebugThreads( \
	if (instance != tsd_initialization_instance) \
	{ \
	  LIBCWD_TSD_DECLARATION; \
	  /* When entering a critical area, make sure that we have explictely deferred cancellation of this */ \
	  /* thread (or disabled that) because when cancellation would happen in the middle of the critical */ \
	  /* area then the lock would stay locked.                                                          */ \
	  LIBCWD_ASSERT( __libcwd_tsd.cancel_explicitely_deferred || __libcwd_tsd.cancel_explicitely_disabled ); \
	} )

template <int instance>
  class mutex_tct {
  public:
    static pthread_mutex_t S_mutex;
#if !LIBCWD_USE_LINUXTHREADS || defined(DEBUGDEBUGTHREADS)
  protected:
    static bool S_initialized;
    static void S_initialize(void) throw();
#endif
  public:
    static void initialize(void) throw()
#if LIBCWD_USE_LINUXTHREADS && !defined(DEBUGDEBUGTHREADS)
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
      LibcwDebugThreads( LIBCWD_ASSERT( S_initialized ) );
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
      bool success = (pthread_mutex_trylock(&S_mutex) == 0);
#ifdef DEBUGDEBUG
      if (success)
	instance_locked[instance] += 1;
#endif
      LibcwDebugThreads( if (success) { LIBCWD_TSD_DECLARATION; ++__libcwd_tsd.inside_critical_area; } );
      return success;
    }
    static void lock(void) throw()
    {
      LibcwDebugThreads( LIBCWD_ASSERT( S_initialized ) );
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
      LibcwDebugThreads( if (instance != tsd_initialization_instance) { LIBCWD_TSD_DECLARATION ++__libcwd_tsd.inside_critical_area; } );
#if LIBCWD_DEBUGDEBUGRWLOCK
      if (instance != tsd_initialization_instance) LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": locking mutex " << instance);
#endif
#ifdef DEBUGDEBUGTHREADS
      int res =
#endif
      pthread_mutex_lock(&S_mutex);
      LibcwDebugThreads( LIBCWD_ASSERT( res == 0 ) );
#if LIBCWD_DEBUGDEBUGRWLOCK
      if (instance != tsd_initialization_instance) LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": mutex " << instance << " locked");
#endif
#ifdef DEBUGDEBUG
      instance_locked[instance] += 1;
#endif
    }
    static void unlock(void) throw()
    {
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
#ifdef DEBUGDEBUG
      instance_locked[instance] -= 1;
#endif
#if LIBCWD_DEBUGDEBUGRWLOCK
      if (instance != tsd_initialization_instance) LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": unlocking mutex " << instance);
#endif
      pthread_mutex_unlock(&S_mutex);
#if LIBCWD_DEBUGDEBUGRWLOCK
      if (instance != tsd_initialization_instance) LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": mutex " << instance << " unlocked");
#endif
      LibcwDebugThreads( if (instance != tsd_initialization_instance) { LIBCWD_TSD_DECLARATION --__libcwd_tsd.inside_critical_area; } );
    }
  };

#if !LIBCWD_USE_LINUXTHREADS || defined(DEBUGDEBUGTHREADS)
template <int instance>
  bool mutex_tct<instance>::S_initialized = false;

template <int instance>
  void mutex_tct<instance>::S_initialize(void) throw()
  {
    if (instance == mutex_initialization_instance)	// Specialization.
    {
#if !LIBCWD_USE_LINUXTHREADS
      pthread_mutexattr_t mutex_attr;
#ifdef DEBUGDEBUGTHREADS
      pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
      pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
      pthread_mutex_init(&S_mutex, &mutex_attr);
#endif // !LIBCWD_USE_LINUXTHREADS
      S_initialized = true;
    }
    else						// General case.
    {
      mutex_tct<mutex_initialization_instance>::initialize();
      /* LIBCWD_DEFER_PUSH_LOCKMUTEX(mutex_initialization_instance, mutex_tct<mutex_initialization_instance>::unlock); */
      if (!S_initialized)					// Check again now that we are locked.
      {
#if !LIBCWD_USE_LINUXTHREADS
	pthread_mutexattr_t mutex_attr;
	if (instance < end_recursive_types)
	  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	else
	{
#ifdef DEBUGDEBUGTHREADS
	  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
	  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
	}
	pthread_mutex_init(&S_mutex, &mutex_attr);
#endif // !LIBCWD_USE_LINUXTHREADS
	S_initialized = true;
      }
      /* LIBCWD_UNLOCKMUTEX_POP_RESTORE(mutex_initialization_instance); */
    }
  }
#endif // !LIBCWD_USE_LINUXTHREADS || defined(DEBUGDEBUGTHREADS)

template <int instance>
  pthread_mutex_t mutex_tct<instance>::S_mutex
#if LIBCWD_USE_LINUXTHREADS
      =
#ifdef DEBUGDEBUGTHREADS
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

//========================================================================================================================================17"
// class cond_tct

template <int instance>
  class cond_tct : public mutex_tct<instance> {
  private:
    static pthread_cond_t S_condition;
#if defined(DEBUGDEBUGTHREADS) || !LIBCWD_USE_LINUXTHREADS
  private:
    static void S_initialize(void) throw();
#endif
  public:
    static void initialize(void) throw()
#if defined(DEBUGDEBUGTHREADS) || !LIBCWD_USE_LINUXTHREADS
	{
	  if (S_initialized)
	    return;
	  S_initialize();
        }
#else
	{ }
#endif
  public:
    void wait(void) {
#ifdef DEBUGDEBUG
      LIBCWD_ASSERT( is_locked(instance) );
#endif
      pthread_cond_wait(&S_condition, &S_mutex);
    }
    void signal(void) { pthread_cond_signal(&S_condition); }
    void broadcast(void) { pthread_cond_broadcast(&S_condition); }
  };

#if defined(DEBUGDEBUGTHREADS) || !LIBCWD_USE_LINUXTHREADS
template <int instance>
  void cond_tct<instance>::S_initialize(void) throw()
  {
#if !LIBCWD_USE_LINUXTHREADS
    mutex_tct<mutex_initialization_instance>::initialize();
    LIBCWD_DEFER_PUSH_LOCKMUTEX(mutex_initialization_instance, mutex_tct<mutex_initialization_instance>::unlock);
    if (!S_initialized)					// Check again now that we are locked.
    {
      pthread_cond_init(&S_condition, NULL);
    }
    LIBCWD_UNLOCKMUTEX_POP_RESTORE(mutex_initialization_instance);
#endif
    mutex_tct<instance>::S_initialize();
  }
#endif // !LIBCWD_USE_LINUXTHREADS

#if !LIBCWD_USE_LINUXTHREADS
template <int instance>
  bool cond_tct<instance>::S_initialized = false;
#endif

template <int instance>
  pthread_cond_t cond_tct<instance>::S_condition
#if LIBCWD_USE_LINUXTHREADS
      = PTHREAD_COND_INITIALIZER;
#else // !LIBCWD_USE_LINUXTHREADS
      ;
#endif // !LIBCWD_USE_LINUXTHREADS

#endif // LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS

//========================================================================================================================================17"
// class rwlock_tct

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
    static int const holders_instance = instance + 2 * reserved_instance_low;
    typedef cond_tct<holders_instance> cond_t;
    static cond_t S_no_holders_condition;
    static int S_holders_count;				// Number of readers or -1 if a writer locked this object.
    static bool S_writer_is_waiting;
    static pthread_t S_writer_id;
#if defined(DEBUGDEBUGTHREADS) || !LIBCWD_USE_LINUXTHREADS
    static bool S_initialized;				// Set when initialized.
#endif
  public:
    static void initialize(void) throw()
    {
#if defined(DEBUGDEBUGTHREADS) || !LIBCWD_USE_LINUXTHREADS
      if (S_initialized)
	return;
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling initialize() instance " << instance);
      mutex_tct<readers_instance>::initialize();
      S_no_holders_condition.initialize();
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving initialize() instance " << instance);
      S_initialized = true;
#endif
    }
    static bool tryrdlock(void) throw()
    {
      LibcwDebugThreads( LIBCWD_ASSERT( S_initialized ) );
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling rwlock_tct<" << instance << ">::tryrdlock()");
      if (instance < end_recursive_types && pthread_equal(S_writer_id, pthread_self()))
      {
	LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::tryrdlock() (skipped: thread has write lock)");
	return;						// No error checking is done.
      }
      // Give a writer a higher priority (kinda fuzzy).
      if (S_writer_is_waiting || !S_no_holders_condition.trylock())
        return false;
      bool success = (S_holders_count != -1);
      if (success)
	++S_holders_count;				// Add one reader.
      S_no_holders_condition.unlock();
      LibcwDebugThreads( if (success) { LIBCWD_TSD_DECLARATION; ++__libcwd_tsd.inside_critical_area; } );
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::tryrdlock()");
      return success;
    }
    static bool trywrlock(void) throw()
    {
      LibcwDebugThreads( LIBCWD_ASSERT( S_initialized ) );
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling rwlock_tct<" << instance << ">::trywrlock()");
      bool success = mutex_tct<readers_instance>::trylock();		// Block new readers,
      while (success)
      {
	S_writer_is_waiting = true;					// from this moment on.
	if (!S_no_holders_condition.trylock())
	{
	  S_writer_is_waiting = false;
	  mutex_tct<readers_instance>::unlock();
	  success = false;
	  break;
	}
	while (S_holders_count != 0)					// Other readers or writers have this lock?
	  S_no_holders_condition.wait();					// Wait until all current holders are done.
	S_writer_is_waiting = false;					// Stop checking the lock for new readers.
	mutex_tct<readers_instance>::unlock();				// Release blocked readers.
	S_holders_count = -1;						// Mark that we have a writer.
	S_no_holders_condition.unlock();
	if (instance < end_recursive_types)
	  S_writer_id = pthread_self();
      }
      LibcwDebugThreads( if (success) { LIBCWD_TSD_DECLARATION; ++__libcwd_tsd.inside_critical_area; } );
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::trywrlock()");
      return success;
    }
    static void rdlock(void) throw()
    {
      LibcwDebugThreads( LIBCWD_ASSERT( S_initialized ) );
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling rwlock_tct<" << instance << ">::rdlock()");
      if (instance < end_recursive_types && pthread_equal(S_writer_id, pthread_self()))
      {
	LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::rdlock() (skipped: thread has write lock)");
	return;						// No error checking is done.
      }
      // Give a writer a higher priority (kinda fuzzy).
      if (S_writer_is_waiting)						// If there is a writer interested,
      {
	mutex_tct<readers_instance>::lock();				// then give it precedence and wait here.
        mutex_tct<readers_instance>::unlock();
      }
      S_no_holders_condition.lock();
      while (S_holders_count == -1)			// Writer locked it?
	S_no_holders_condition.wait();			// Wait for writer to finish.
      ++S_holders_count;				// Add one reader.
      S_no_holders_condition.unlock();
      LibcwDebugThreads( LIBCWD_TSD_DECLARATION; ++__libcwd_tsd.inside_critical_area );
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::rdlock()");
    }
    static void rdunlock(void) throw()
    {
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling rwlock_tct<" << instance << ">::rdunlock()");
      if (instance < end_recursive_types && pthread_equal(S_writer_id, pthread_self()))
      {
	LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::rdunlock() (skipped: thread has write lock)");
	return;						// No error checking is done.
      }
      LibcwDebugThreads( LIBCWD_TSD_DECLARATION; --__libcwd_tsd.inside_critical_area );
      S_no_holders_condition.lock();
      if (--S_holders_count == 0)			// Was this the last reader?
        S_no_holders_condition.signal();		// Tell waiting threads.
      S_no_holders_condition.unlock();
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::rdunlock()");
    }
    static void wrlock(void) throw()
    {
      LibcwDebugThreads( LIBCWD_ASSERT( S_initialized ) );
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling rwlock_tct<" << instance << ">::wrlock()");
      mutex_tct<readers_instance>::lock();		// Block new readers,
      S_writer_is_waiting = true;			// from this moment on.
      S_no_holders_condition.lock();
      while (S_holders_count != 0)			// Other readers or writers have this lock?
	S_no_holders_condition.wait();			// Wait until all current holders are done.
      S_writer_is_waiting = false;			// Stop checking the lock for new readers.
      mutex_tct<readers_instance>::unlock();		// Release blocked readers.
      S_holders_count = -1;				// Mark that we have a writer.
      S_no_holders_condition.unlock();
      if (instance < end_recursive_types)
        S_writer_id = pthread_self();
      LibcwDebugThreads( LIBCWD_TSD_DECLARATION; ++__libcwd_tsd.inside_critical_area );
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::wrlock()");
#ifdef DEBUGDEBUG
      instance_locked[instance] += 1;
#endif
    }
    static void wrunlock(void) throw()
    {
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
#ifdef DEBUGDEBUG
      instance_locked[instance] -= 1;
#endif
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling rwlock_tct<" << instance << ">::wrunlock()");
      LibcwDebugThreads( LIBCWD_TSD_DECLARATION; --__libcwd_tsd.inside_critical_area) ;
      if (instance < end_recursive_types)
        S_writer_id = 0;
      S_no_holders_condition.lock();
      S_holders_count = 0;				// We have no writer anymore.
      S_no_holders_condition.signal();			// No readers and no writers left.
      S_no_holders_condition.unlock();
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::wrunlock()");
    }
    static void rd2wrlock(void) throw()
    {
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling rwlock_tct<" << instance << ">::rd2wrlock()");
      S_no_holders_condition.lock();
      if (--S_holders_count > 0)
      {
	mutex_tct<readers_instance>::lock();	// Block new readers.
	S_writer_is_waiting = true;
	while (S_holders_count != 0)
	  S_no_holders_condition.wait();
	S_writer_is_waiting = false;
	mutex_tct<readers_instance>::unlock();	// Release blocked readers.
      }
      S_holders_count = -1;			// We are a writer now.
      S_no_holders_condition.unlock();
      if (instance < end_recursive_types)
	S_writer_id = pthread_self();
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::rd2wrlock()");
#ifdef DEBUGDEBUG
      instance_locked[instance] += 1;
#endif
    }
    static void wr2rdlock(void) throw()
    {
      LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
#ifdef DEBUGDEBUG
      instance_locked[instance] -= 1;
#endif
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Calling rwlock_tct<" << instance << ">::wr2rdlock()");
      if (instance < end_recursive_types)
        S_writer_id = 0;
      S_holders_count = 1;				// Turn writer into a reader (atomic operation).
      LIBCWD_DEBUGDEBUGRWLOCK_CERR(pthread_self() << ": Leaving rwlock_tct<" << instance << ">::wr2rdlock()");
    }
    // This is used as cleanup handler with LIBCWD_DEFER_CLEANUP_PUSH.
    static void cleanup(void*);
  };

template <int instance>
  int rwlock_tct<instance>::S_holders_count = 0;

template <int instance>
  bool rwlock_tct<instance>::S_writer_is_waiting = 0;

template <int instance>
  pthread_t rwlock_tct<instance>::S_writer_id = 0;

#if defined(DEBUGDEBUGTHREADS) || !LIBCWD_USE_LINUXTHREADS
template <int instance>
  bool rwlock_tct<instance>::S_initialized = 0;
#endif

template <int instance>
  typename  rwlock_tct<instance>::cond_t rwlock_tct<instance>::S_no_holders_condition;

template <int instance>
  void rwlock_tct<instance>::cleanup(void*)
  {
    if (S_holders_count == -1)
      wrunlock();
    else
      rdunlock();
  }

extern void fatal_cancellation(void*) throw();

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#else // !LIBCWD_THREAD_SAFE
#define LIBCWD_DISABLE_CANCEL
#define LIBCWD_DISABLE_CANCEL_NO_BRACE
#define LIBCWD_ENABLE_CANCEL_NO_BRACE
#define LIBCWD_ENABLE_CANCEL
#define LIBCWD_DEFER_CANCEL
#define LIBCWD_DEFER_CANCEL_NO_BRACE
#define LIBCWD_RESTORE_CANCEL_NO_BRACE
#define LIBCWD_RESTORE_CANCEL
#define LIBCWD_DEFER_CLEANUP_PUSH(routine, arg)
#define LIBCWD_CLEANUP_POP_RESTORE(execute)
#define LIBCWD_PUSH_DEFER_TRYLOCK_MUTEX(instance, unlock_routine)
#define LIBCWD_DEFER_PUSH_LOCKMUTEX(instance, unlock_routine)
#define LIBCWD_UNLOCKMUTEX_POP_RESTORE(instance)
#define LIBCWD_DEBUGDEBUG_ASSERT_CANCEL_DEFERRED
#endif // LIBCWD_THREAD_SAFE
#endif // LIBCW_PRIVATE_THREADING_H

