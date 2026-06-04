// SPDX-FileCopyrightText: 2001-2004, 2006-2010, 2013-2014, 2018-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include "ios_base_Init.h"
#include "macros.h"
#include "libcwd/debug.h"
#include <libcwd/core_dump.h>
#include <libcwd/private_mutex.inl>
#include <libcwd/private_threading.h>
#include "threadsafe/AIReadWriteMutex.h"
#include "threadsafe/threadsafe.h"

#include <unistd.h>
#include "cwd_debug.h"

#if LIBCWD_DEBUGDEBUGRWLOCK
pthread_mutex_t LIBCWD_DEBUGDEBUGLOCK_CERR_mutex;
unsigned int LIBCWD_DEBUGDEBUGLOCK_CERR_count;
#endif

namespace libcwd {
namespace _private_ {

bool WST_multi_threaded = false;
bool WST_first_thread_initialized = false;

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
int instance_locked[instance_locked_size];
#endif
#if CWDEBUG_DEBUGT
std::mutex raw_write_mutex;
#endif

void initialize_global_mutexes()
{
  mutex_tct<mutex_initialization_instance>::initialize();
  mutex_tct<set_ostream_instance>::initialize();
}

void mutex_ct::M_initialize()
{
  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
#if CWDEBUG_DEBUGT
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
  pthread_mutex_init(&M_mutex, &mutex_attr);
  pthread_mutexattr_destroy(&mutex_attr);
  M_initialized = true;
}

void fatal_cancellation(void* arg)
{
  char* text = static_cast<char*>(arg);
  DoutFatal(dc::core, "Cancelling a thread " << text << ".  This is not supported by libcwd, sorry.");
}

//===================================================================================================
// Thread Specific Data
//

pthread_key_t TSD_st::S_tsd_key;
pthread_once_t TSD_st::S_tsd_key_once = PTHREAD_ONCE_INIT;

extern void debug_tsd_init(LIBCWD_TSD_PARAM);
void threading_tsd_init(LIBCWD_TSD_PARAM);

// static
TSD_st& TSD_st::instance()
{
  TSD_st* instance;
  if (!WST_tsd_key_created || !(instance = (TSD_st*)pthread_getspecific(S_tsd_key)))
    return S_create();
  return *instance;
}

TSD_st* main_thread_tsd;

// Create, install, and initialize the TSD for the current thread.
//static
TSD_st& TSD_st::S_create()
{
  int oldtype;
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

  TSD_st* real_tsd = new TSD_st;
  // clang-format off
  PRAGMA_DIAGNOSTIC_PUSH_IGNORE_class_memaccess
  std::memset(real_tsd, 0, sizeof(struct TSD_st));
  PRAGMA_DIAGNOSTIC_POP
  // clang-format on
  real_tsd->tid = pthread_self();
  real_tsd->pid = getpid();

  // Install the pthread key before initializing libcwd subsystems. If initialization re-enters
  // TSD_st::instance(), it must see and reuse this partially initialized TSD instead of recursing.
  pthread_once(&S_tsd_key_once, &S_tsd_key_alloc);
  pthread_setspecific(S_tsd_key, (void*)real_tsd);

  if (!main_thread_tsd)
    main_thread_tsd = real_tsd;

  if (!WST_first_thread_initialized) // Is this the first thread?
  {
    WST_first_thread_initialized = true;
    initialize_global_mutexes();
    threading_tsd_init(*real_tsd); // Initialize the TSD of stuff that goes in threading.cc.
  }
  else
  {
    WST_multi_threaded = true;
    debug_tsd_init(*real_tsd); // Initialize the TSD of existing debug objects.
    threading_tsd_init(*real_tsd); // Initialize the TSD of stuff that goes in threading.cc.
  }

  pthread_setcanceltype(oldtype, NULL);

  return *real_tsd;
}

bool WST_tsd_key_created = false;

void TSD_st::S_tsd_key_alloc()
{
  pthread_key_create(&S_tsd_key, &TSD_st::S_cleanup_routine); // Leaks memory, we never destruct this key again.
  WST_tsd_key_created = true;
}

#define VALGRIND 0

void TSD_st::cleanup_routine()
{
#if !VALGRIND
  if (++tsd_destructor_count < PTHREAD_DESTRUCTOR_ITERATIONS)
#else
  if (1) // Valgrind doesn't iterate the key destruction routines.
#endif
  {
    // Add the key back a number of times in order to schedule our
    // deinitialization as far as possible after other key destruction
    // routines.
    pthread_setspecific(S_tsd_key, (void*)this);
#if !VALGRIND
    if (tsd_destructor_count < PTHREAD_DESTRUCTOR_ITERATIONS - 1)
      return;
#endif
    for (int i = 0; i < LIBCWD_DO_MAX; ++i)
      if (do_array[i])
      {
        debug_tsd_st* ptr = do_array[i];
        do_off_array[i] = 0; // Turn all debugging off!  Now, hopefully, we won't use do_array[i] anymore.
        do_array[i] = NULL; // So we won't free it again.
        ptr->tsd_initialized = false;
        delete ptr; // Free debug object TSD.
      }

    int oldtype;
    pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, &oldtype);
    thread_iter->terminating();
    pthread_setcanceltype(oldtype, NULL);

    pthread_setspecific(S_tsd_key, (void*)0);
    // Then we can safely delete the current TSD.
    delete this;
  }
}

void TSD_st::S_cleanup_routine(void* arg)
{
  TSD_st* obj = reinterpret_cast<TSD_st*>(arg);
  obj->cleanup_routine();
}

// End of Thread Specific Data
//===================================================================================================

int pthread_lock_interface_ct::try_lock()
{
#if CWDEBUG_DEBUGT
  bool success = pthread_mutex_trylock(ptr);
  if (success)
  {
    LIBCWD_TSD_DECLARATION;
    __libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] += 1;
    if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 1)
    {
      __libcwd_tsd.rdlocked_by1[pthread_lock_interface_instance] = pthread_self();
      __libcwd_tsd.rdlocked_from1[pthread_lock_interface_instance] = __builtin_return_address(0);
    }
    else if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 2)
    {
      __libcwd_tsd.rdlocked_by2[pthread_lock_interface_instance] = pthread_self();
      __libcwd_tsd.rdlocked_from2[pthread_lock_interface_instance] = __builtin_return_address(0);
    }
    else
      core_dump();
  }
  return success;
#else
  return pthread_mutex_trylock(ptr);
#endif
}

void pthread_lock_interface_ct::lock()
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
  __libcwd_tsd.waiting_for_rdlock = pthread_lock_interface_instance;
#endif
  pthread_mutex_lock(ptr);
#if CWDEBUG_DEBUGT
  __libcwd_tsd.waiting_for_rdlock = 0;
  __libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] += 1;
  if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 1)
  {
    __libcwd_tsd.rdlocked_by1[pthread_lock_interface_instance] = pthread_self();
    __libcwd_tsd.rdlocked_from1[pthread_lock_interface_instance] = __builtin_return_address(0);
  }
  else if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 2)
  {
    __libcwd_tsd.rdlocked_by2[pthread_lock_interface_instance] = pthread_self();
    __libcwd_tsd.rdlocked_from2[pthread_lock_interface_instance] = __builtin_return_address(0);
  }
  else
    core_dump();
#endif
}

void pthread_lock_interface_ct::unlock()
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
  if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 2)
    __libcwd_tsd.rdlocked_by2[pthread_lock_interface_instance] = 0;
  else
    __libcwd_tsd.rdlocked_by1[pthread_lock_interface_instance] = 0;
  __libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] -= 1;
#endif
  pthread_mutex_unlock(ptr);
}

//---------------------------------------------------------------------------------------------------
// Below is the implementation of a list with thread specific objects
// that are kept even after the destruction of a thread, and even
// after the TSD_st structure is reused for another thread.
//
// thread_ct stores per-thread state that must remain address-stable while
// libcwd observes thread lifetime and, in fatal debug-output paths, cancels
// all known peer threads.
//

// Store the global list of thread_ct objects behind a typed read/write lock.
//
// The list is reached through ThreadList::instance() instead of a global object so that it can be
// constructed on first use, including during early TSD initialization before normal global construction
// has completed. The contained std::list keeps thread_iter values address-stable across later insertions.
struct ThreadList::impl_ct
{
  using threads_ts = threadsafe::Unlocked<ThreadList::list_type, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  threads_ts threads_;
};

// Return the process-wide list that tracks all threads known to libcwd.
//static
ThreadList const& ThreadList::instance()
{
  static ThreadList const threadlist_registry{new ThreadList::impl_ct};
  return threadlist_registry;
}

// Insert a thread_ct for __libcwd_tsd and initialize it while the list write lock is held.
//
// The resulting iterator is stored in __libcwd_tsd before initialization finishes so later TSD cleanup can
// mark this exact list entry as terminating. Holding the write access until after thread_ct::initialize
// ensures that concurrent traversals only observe fully initialized thread objects.
void ThreadList::add_current_thread(LIBCWD_TSD_PARAM) const
{
  impl_ct::threads_ts::wat threads_w(impl_->threads_);
  __libcwd_tsd.thread_iter = threads_w->insert(threads_w->end(), thread_ct());
  __libcwd_tsd.thread_iter_valid = true;
  __libcwd_tsd.thread_iter->initialize(LIBCWD_TSD);
}

struct ThreadsWat
{
  ThreadList::impl_ct::threads_ts::wat& ref_;
  ThreadsWat(ThreadList::impl_ct::threads_ts::wat& wat) : ref_(wat) { }
};

// Mark a known thread-list entry as terminated while serializing with list traversals and insertions.
//
// The current thread_iter implementation does not erase the entry, but this wrapper keeps the iterator
// operation under the same write access that would be required if terminated ever starts recycling nodes.
void ThreadList::mark_thread_terminated(ThreadList::list_type::iterator thread_iter LIBCWD_COMMA_TSD_PARAM) const
{
  impl_ct::threads_ts::wat threads_w(impl_->threads_);
  thread_iter->terminated(threads_w LIBCWD_COMMA_TSD);
}

// Cancel every known peer thread while a read access prevents concurrent modification of the list.
//
// current_tid is never cancelled.
void ThreadList::cancel_all_other_threads(pthread_t current_tid) const
{
  impl_ct::threads_ts::crat threads_r(impl_->threads_);
  for (thread_ct const& thread : *threads_r)
    if (!pthread_equal(thread.tid, current_tid))
      pthread_cancel(thread.tid);
}

// Called when a new thread is detected.
// Adds a new thread_ct to threadlist and initializes it.
void threading_tsd_init(LIBCWD_TSD_PARAM)
{
  LIBCWD_DEFER_CANCEL;
  ThreadList::instance().add_current_thread(LIBCWD_TSD);
  LIBCWD_RESTORE_CANCEL;
}

// The default constructor of a thread_ct object.
// No real initialization is done yet, for that thread_ct::initialize
// needs to be called after this object is added to threadlist.
// The thread list write access needs to be held before insertion of the thread_ct takes place and not be
// released until initialization of the object has finished.
void thread_ct::initialize(LIBCWD_TSD_PARAM)
{
  thread_mutex.initialize();
  tid = __libcwd_tsd.tid;
}

// This member function is called when the TSD_st structure
// is being reused, which is currently our only way to know
// for sure that the corresponding thread has terminated.
void thread_ct::terminated(ThreadsWat wat LIBCWD_COMMA_TSD_PARAM_UNUSED)
{
}

} // namespace _private_
} // namespace libcwd
