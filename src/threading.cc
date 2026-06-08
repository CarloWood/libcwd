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

  initialized = false;
}

// End of Thread Specific Data
//===================================================================================================

} // namespace _private_
} // namespace libcwd
