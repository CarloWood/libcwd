// SPDX-FileCopyrightText: 2001-2004, 2006-2010, 2013-2014, 2018-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include <libcwd/private_struct_TSD.h>

#include <atomic>
#include <mutex>

namespace libcwd {
namespace _private_ {

std::atomic_bool WST_multi_threaded = false;

#if CWDEBUG_DEBUG
std::mutex raw_write_mutex;
#endif

//===================================================================================================
// Thread Specific Data
//

extern void debug_tsd_init(LIBCWD_TSD_PARAM);

ThreadSpecificData* main_thread_tsd;

namespace {

thread_local ThreadSpecificData* current_tsd;

// Clean up the heap-allocated TSD that belongs to the current non-main thread.
//
// The guard has no inputs and no direct output. Its destructor runs during C++ thread_local teardown, releases
// per-thread libcwd state for ordinary worker threads, and deliberately leaves the main-thread TSD alive so
// global and shared-object destructors can continue to emit diagnostics after main() returns.
struct TSD_cleanup_guard
{
  ~TSD_cleanup_guard()
  {
    ThreadSpecificData* tsd = current_tsd;
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
ThreadSpecificData& ThreadSpecificData::instance()
{
  if (current_tsd)
    return *current_tsd;

  // Under the main-thread-first assumption, a first-use access after main_thread_tsd was set belongs to a
  // worker thread and therefore needs the cleanup guard to delete its TSD at thread exit.
  if (main_thread_tsd)
    (void)current_tsd_cleanup_guard;

  current_tsd = new ThreadSpecificData;
  return S_create(*current_tsd);
}

// Initialize the TSD object for the current thread.
//
// The object is marked initialized before libcwd subsystems are initialized because those paths can call
// ThreadSpecificData::instance() again. Returning the same object during that recursion keeps partially initialized
// state visible and avoids creating a second TSD for the same thread.
//static
ThreadSpecificData& ThreadSpecificData::S_create(ThreadSpecificData& real_tsd)
{
  real_tsd.initialized_ = true;

  if (!main_thread_tsd)
    main_thread_tsd = &real_tsd;
  else
  {
    WST_multi_threaded = true;
    debug_tsd_init(real_tsd); // Initialize the TSD of existing debug objects.
  }

  return real_tsd;
}

// Destroy this TSD and release any initialized per-thread libcwd state.
//
// Worker-thread objects reach this destructor from TSD_cleanup_guard at thread exit. The cleanup routine is
// idempotent, so explicit or recursive cleanup attempts are harmless.
ThreadSpecificData::~ThreadSpecificData()
{
  if (initialized_)
    cleanup_routine();
}

// Release the debug-object TSD owned by this thread.
//
// Unlike the old pthread-key implementation, portable C++ does not allow delaying this destructor through
// repeated native key-destructor iterations. Therefore this cleanup runs when the C++ thread_local cleanup
// guard is destroyed; while it runs, recursive instance() calls still return this same TSD object.
void ThreadSpecificData::cleanup_routine()
{
  if (cleaning_up_)
    return;
  cleaning_up_ = true;

  for (int i = 0; i < LIBCWD_DO_MAX; ++i)
    if (debug_object_array[i])
    {
      DebugObject_ThreadSpecificData* ptr = debug_object_array[i];
      debug_object_off_array[i] = 0; // Turn all debugging off!  Now, hopefully, we won't use debug_object_array[i] anymore.
      debug_object_array[i] = NULL; // So we won't free it again.
      ptr->tsd_initialized = false;
      delete ptr; // Free debug object TSD.
    }

  initialized_ = false;
}

// End of Thread Specific Data
//===================================================================================================

} // namespace _private_
} // namespace libcwd
