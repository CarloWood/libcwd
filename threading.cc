// $Header$
//
// Copyright (C) 2001 - 2002, by
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

bool WST_multi_threaded = false;
#if CWDEBUG_DEBUG
int instance_locked[instance_locked_size];
#endif

void initialize_global_mutexes(void) throw()
{
#if !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
  mutex_tct<mutex_initialization_instance>::initialize();
  rwlock_tct<object_files_instance>::initialize();
  mutex_tct<dlopen_map_instance>::initialize();
  mutex_tct<set_ostream_instance>::initialize();
#if CWDEBUG_ALLOC
  mutex_tct<alloc_tag_desc_instance>::initialize();
  mutex_tct<memblk_map_instance>::initialize();
#endif
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
  mutex_tct<type_info_of_instance>::initialize();
#endif
#endif // !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
}

#ifdef LIBCWD_USE_LINUXTHREADS
// Specialization.
template <>
  pthread_mutex_t mutex_tct<tsd_initialization_instance>::S_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

void fatal_cancellation(void* arg) throw()
{
  char* text = static_cast<char*>(arg);
  Dout(dc::core, "Cancelling a thread " << text << ".  This is not supported by libcwd, sorry.");
}

TSD_st __libcwd_tsd_array[PTHREAD_THREADS_MAX];

#if LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS
pthread_key_t TSD_st::S_exit_key;
pthread_once_t TSD_st::S_exit_key_once = PTHREAD_ONCE_INIT;

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
  if (thread_index(pthread_self()) != 0)	// Is this a second (or later) thread?
  {
    WST_multi_threaded = true;
    set_alloc_checking_off(*this);
    for (int i = 0; i < LIBCWD_DO_MAX; ++i)
      if (old_array[i])
	delete old_array[i];			// Free old objects when this structure is being reused.
    set_alloc_checking_on(*this);
    debug_tsd_init(*this);			// Initialize the TSD of existing debug objects.
  }
  pthread_once(&S_exit_key_once, &TSD_st::S_exit_key_alloc);
  pthread_setspecific(S_exit_key, (void*)this);
  pthread_setcanceltype(oldtype, NULL);
}

void TSD_st::S_exit_key_alloc(void) throw()
{
  pthread_key_create(&S_exit_key, &TSD_st::S_cleanup_routine);
}

void TSD_st::cleanup_routine(void) throw()
{
  set_alloc_checking_off(*this);
  for (int i = 0; i < LIBCWD_DO_MAX; ++i)
    if (do_array[i])
    {
      debug_tsd_st* ptr = do_array[i];
      if (ptr->tsd_keep)
	continue;
      do_off_array[i] = 0;			// Turn all debugging off!  Now, hopefully, we won't use do_array[i] anymore.
      do_array[i] = NULL;			// So we won't free it again.
      delete ptr;				// Free debug object TSD.
    }
  set_alloc_checking_on(*this);
}

void TSD_st::S_cleanup_routine(void* arg) throw()
{
  TSD_st* obj = reinterpret_cast<TSD_st*>(arg);
  obj->cleanup_routine();
}

#endif // LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS

// End of Thread Specific Data
//===================================================================================================

    } // namespace _private_
  } // namespace debug
} // namespace libcw
