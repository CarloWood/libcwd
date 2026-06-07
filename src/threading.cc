// SPDX-FileCopyrightText: 2001-2004, 2006-2010, 2013-2014, 2018-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include "libcwd/debug.h"
#include <libcwd/core_dump.h>
#include <libcwd/private_threading.h>
#include "threadsafe/AIReadWriteMutex.h"
#include "threadsafe/threadsafe.h"

#include <thread>
#include <unistd.h>
#include "cwd_debug.h"

namespace libcwd {
namespace _private_ {

bool WST_multi_threaded = false;

#if CWDEBUG_DEBUG
std::mutex raw_write_mutex;
#endif

//===================================================================================================
// Thread Specific Data
//

extern void debug_tsd_init(LIBCWD_TSD_PARAM);
void threading_tsd_init(LIBCWD_TSD_PARAM);

TSD_st* main_thread_tsd;

namespace {

thread_local TSD_st* current_tsd;

// Clean up the heap-allocated TSD that belongs to the current non-main thread.
//
// The guard has no inputs and no direct output. Its destructor runs during C++ thread_local teardown, releases
// per-thread libcwd state for ordinary worker threads, and deliberately leaves the main-thread TSD alive so
// global and shared-object destructors can continue to emit diagnostics after main() returns.
struct TSD_cleanup_guard
{
  ~TSD_cleanup_guard()
  {
    TSD_st* tsd = current_tsd;
    if (!tsd || tsd == main_thread_tsd)
      return;

    delete tsd;
    current_tsd = nullptr;
  }
};

thread_local TSD_cleanup_guard current_tsd_cleanup_guard;

} // namespace

// Return the TSD for the current thread, allocating and initializing it on first use.
//
// A C++ thread_local pointer gives each thread a fast lookup slot, while a thread_local cleanup guard releases
// worker-thread storage at thread exit. The main-thread object is heap allocated and intentionally retained so
// global destructors can continue to use libcwd after C++ has started tearing down thread-local objects. This
// relies on the first TSD allocation happening on the main thread; later first-use calls construct the cleanup
// guard before allocating worker-thread TSD.
//static
TSD_st& TSD_st::instance()
{
  if (current_tsd)
    return *current_tsd;

  // We don't necessarily need ~TSD_cleanup_guard() to be called for the main thread.
  if (main_thread_tsd)                  // This is true if this is NOT the main thread; otherwise it is false.
    (void)current_tsd_cleanup_guard;

  current_tsd = new TSD_st;
  return S_create(*current_tsd);
}

// Initialize the TSD object for the current thread.
//
// The object is marked initialized before libcwd subsystems are initialized because those paths can call
// TSD_st::instance() again. Returning the same object during that recursion keeps partially initialized
// state visible and avoids creating a second TSD for the same thread.
//static
TSD_st& TSD_st::S_create(TSD_st& real_tsd)
{
  real_tsd.initialized = true;
  real_tsd.tid = std::this_thread::get_id();

  if (!main_thread_tsd)
  {
    main_thread_tsd = &real_tsd;
    threading_tsd_init(real_tsd); // Initialize the TSD of stuff that goes in threading.cc.
  }
  else
  {
    WST_multi_threaded = true;
    debug_tsd_init(real_tsd); // Initialize the TSD of existing debug objects.
    threading_tsd_init(real_tsd); // Initialize the TSD of stuff that goes in threading.cc.
  }

  return real_tsd;
}

// Destroy this TSD and release any initialized per-thread libcwd state.
//
// Worker-thread objects reach this destructor from TSD_cleanup_guard at thread exit. The cleanup routine is
// idempotent, so explicit or recursive cleanup attempts are harmless.
TSD_st::~TSD_st()
{
  if (initialized)
    cleanup_routine();
}

// Release the debug-object TSD owned by this thread and mark the thread as terminating.
//
// Unlike the old pthread-key implementation, portable C++ does not allow delaying this destructor through
// repeated native key-destructor iterations. Therefore this cleanup runs when the C++ thread_local cleanup
// guard is destroyed; while it runs, recursive instance() calls still return this same TSD object.
void TSD_st::cleanup_routine()
{
  if (cleaning_up)
    return;
  cleaning_up = true;

  for (int i = 0; i < LIBCWD_DO_MAX; ++i)
    if (do_array[i])
    {
      debug_tsd_st* ptr = do_array[i];
      do_off_array[i] = 0; // Turn all debugging off!  Now, hopefully, we won't use do_array[i] anymore.
      do_array[i] = NULL; // So we won't free it again.
      ptr->tsd_initialized = false;
      delete ptr; // Free debug object TSD.
    }

  if (thread_iter_valid)
  {
    thread_iter->terminating();
    thread_iter_valid = false;
  }

  initialized = false;
}

// End of Thread Specific Data
//===================================================================================================

//---------------------------------------------------------------------------------------------------
// Below is the implementation of a list with thread specific objects
// that are kept even after the destruction of a thread, and even
// after that thread's TSD_st has been cleaned up.
//
// thread_ct stores per-thread state that must remain address-stable while
// libcwd observes thread lifetime and handles fatal debug-output paths.
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

// Called when a new thread is detected.
// Adds a new thread_ct to threadlist and initializes it.
void threading_tsd_init(LIBCWD_TSD_PARAM)
{
  ThreadList::instance().add_current_thread(LIBCWD_TSD);
}

// The default constructor of a thread_ct object.
// No real initialization is done yet, for that thread_ct::initialize
// needs to be called after this object is added to threadlist.
// The thread list write access needs to be held before insertion of the thread_ct takes place and not be
// released until initialization of the object has finished.
void thread_ct::initialize(LIBCWD_TSD_PARAM)
{
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
