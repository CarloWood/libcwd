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

    } // namespace _private_
  } // namespace debug
} // namespace libcw

