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
#include <libcw/private_mutex.inl>

namespace libcw {
  namespace debug {
    namespace _private_ {

bool WST_multi_threaded = false;
bool WST_first_thread_initialized = false;
#if CWDEBUG_DEBUG
int instance_locked[instance_locked_size];
#if CWDEBUG_DEBUGT
pthread_t locked_by[instance_locked_size];
void const* locked_from[instance_locked_size];
#endif
#endif

void initialize_global_mutexes(void) throw()
{
#if !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
  mutex_tct<mutex_initialization_instance>::initialize();
  rwlock_tct<object_files_instance>::initialize();
  mutex_tct<dlopen_map_instance>::initialize();
  mutex_tct<set_ostream_instance>::initialize();
  mutex_tct<kill_threads_instance>::initialize();
#if CWDEBUG_ALLOC
  mutex_tct<alloc_tag_desc_instance>::initialize();
  mutex_tct<memblk_map_instance>::initialize();
  rwlock_tct<location_cache_instance>::initialize();
  mutex_tct<list_allocations_instance>::initialize();
#endif
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
  mutex_tct<type_info_of_instance>::initialize();
#endif
#endif // !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
}

#if LIBCWD_USE_LINUXTHREADS
// Specialization.
template <>
  pthread_mutex_t mutex_tct<tsd_initialization_instance>::S_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

void mutex_ct::M_initialize(void) throw()
{
  pthread_mutexattr_t mutex_attr;
#if CWDEBUG_DEBUGT
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
  pthread_mutex_init(&M_mutex, &mutex_attr);
  M_initialized = true;
}

void fatal_cancellation(void* arg) throw()
{
  char* text = static_cast<char*>(arg);
  Dout(dc::core, "Cancelling a thread " << text << ".  This is not supported by libcwd, sorry.");
}

//===================================================================================================
// Thread Specific Data
//

TSD_st __libcwd_tsd_array[PTHREAD_THREADS_MAX];

#if LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS
pthread_key_t TSD_st::S_exit_key;
pthread_once_t TSD_st::S_exit_key_once = PTHREAD_ONCE_INIT;

extern void debug_tsd_init(LIBCWD_TSD_PARAM);
extern void* new_memblk_map(LIBCWD_TSD_PARAM);
extern void delete_memblk_map(void* LIBCWD_COMMA_TSD_PARAM);

void TSD_st::S_initialize(void) throw()
{
  int oldtype;
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
  mutex_tct<tsd_initialization_instance>::initialize();
  mutex_tct<tsd_initialization_instance>::lock();
  debug_tsd_st* old_array[LIBCWD_DO_MAX];
  std::memcpy(old_array, do_array, sizeof(old_array));
  void* old_memblk_map = memblk_map;
  std::memset(this, 0, sizeof(struct TSD_st));	// This structure might be reused and therefore already contain data.
  tid = pthread_self();
  mutex_tct<tsd_initialization_instance>::unlock();
  memblk_map_mutex.initialize();		// Initialize the mutex that will be used for memblk_map.
  // We assume that the main() thread will call malloc() at least
  // once before it reaches main() and thus before any other thread is created.
  // When it does we get here; and thus are still single threaded.
  if (!WST_first_thread_initialized)		// Is this the first thread?
  {
    WST_first_thread_initialized = true;
    initialize_global_mutexes();		// This is a good moment to initialize all pthread mutexes.
  }
  else
  {
    WST_multi_threaded = true;
    set_alloc_checking_off(*this);
    for (int i = 0; i < LIBCWD_DO_MAX; ++i)
      if (old_array[i])
	delete old_array[i];			// Free old objects when this structure is being reused.
    set_alloc_checking_on(*this);
    debug_tsd_init(*this);			// Initialize the TSD of existing debug objects.
    if (old_memblk_map)
      delete_memblk_map(old_memblk_map, *this);	// Free old memblk_map when this structure is being reused.
    memblk_map = new_memblk_map(*this);
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
      ptr->tsd_initialized = false;
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

class thread_ct { };

typedef std::vector<thread_ct, internal_allocator::rebind<thread_ct>::other> threadlist_t;

    } // namespace _private_
  } // namespace debug
} // namespace libcw
