// $Header$
//
// Copyright (C) 2001, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// Copyright (C) 2001, by Eric Lesage.
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include "cwd_debug.h"
#include <libcw/private_threading.h>

namespace libcw {
  namespace debug {
    namespace _private_ {

#ifdef LIBCWD_THREAD_SAFE
#ifdef DEBUGDEBUG
bool WST_multi_threaded = false;
#endif
#ifdef DEBUGDEBUG
int instance_locked[instance_locked_size];
#endif

void initialize_global_mutexes(void) throw()
{
#if !LIBCWD_USE_LINUXTHREADS || defined(DEBUGDEBUGTHREADS)
  mutex_tct<mutex_initialization_instance>::initialize();
  rwlock_tct<object_files_instance>::initialize();
  mutex_tct<dlopen_map_instance>::initialize();
#ifdef DEBUGMALLOC
  mutex_tct<alloc_tag_desc_instance>::initialize();
  rwlock_tct<memblk_map_instance>::initialize();
#endif
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
  mutex_tct<type_info_of_instance>::initialize();
#endif
#endif // !LIBCWD_USE_LINUXTHREADS || defined(DEBUGDEBUGTHREADS)
}

#ifdef LIBCWD_USE_LINUXTHREADS
// Specialization.
template <>
  pthread_mutex_t mutex_tct<tsd_initialization_instance>::S_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

#else // !LIBCWD_THREAD_SAFE
TSD_st __libcwd_tsd;
#endif // !LIBCWD_THREAD_SAFE

void fatal_cancellation(void* arg) throw()
{
  char* text = static_cast<char*>(arg);
  Dout(dc::core, "Cancelling a thread " << text << ".  This is not supported by libcwd, sorry.");
}

//===================================================================================================
// Implementation of Thread Specific Data.

TSD_st __libcwd_tsd_array[PTHREAD_THREADS_MAX];

#if LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS
extern void debug_tsd_init(LIBCWD_TSD_PARAM);

void TSD_st::S_initialize(void) throw()
{
  int oldtype;
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
  mutex_tct<tsd_initialization_instance>::initialize();
  mutex_tct<tsd_initialization_instance>::lock();
  debug_tsd_st* old_array[LIBCWD_DO_MAX];
  std::memcpy(old_array, do_array, sizeof(old_array));
  std::memset(this, 0, sizeof(struct TSD_st));	// This structure might be reused and therefore already contain data.
  tid = pthread_self();
  initialize_global_mutexes();			// This is a good moment to initialize all pthread mutexes.
  mutex_tct<tsd_initialization_instance>::unlock();
  if (WST_multi_threaded)			// Is this a second (or later) thread?
  {
    set_alloc_checking_off(*this);
    for (int i = 0; i < LIBCWD_DO_MAX; ++i)
      if (old_array[i])
	delete old_array[i];			// Free old objects when this structure is being reused.
    set_alloc_checking_on(*this);
    debug_tsd_init(*this);			// Initialize the TSD of existing debug objects.
  }
  pthread_setcanceltype(oldtype, NULL);
}

#endif // LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS

// End of Thread Specific Data
//===================================================================================================

    } // namespace _private_
  } // namespace debug
} // namespace libcw

