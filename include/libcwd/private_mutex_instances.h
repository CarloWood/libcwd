// $Header$
//
// Copyright (C) 2001 - 2003, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
// 

/** \file libcwd/private_mutex_instances.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_PRIVATE_MUTEX_INSTANCES_H
#define LIBCWD_PRIVATE_MUTEX_INSTANCES_H

#if LIBCWD_THREAD_SAFE

#if CWDEBUG_DEBUGT
#ifndef LIBCW_PTHREAD_H
#define LIBCW_PTHREAD_H
#include <pthread.h>
#endif
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

// The different instance-ids used in libcwd.
enum mutex_instance_nt {
  // Recursive mutexes.
  tsd_initialization_instance,
  object_files_instance,		// rwlock
  end_recursive_types,
  // Fast mutexes.
#if CWDEBUG_ALLOC
  memblk_map_instance,
  location_cache_instance,		// rwlock
#endif
  threadlist_instance,			// rwlock
  debug_objects_instance,		// rwlock
  debug_channels_instance,		// rwlock
#if CWDEBUG_DEBUGT
  keypair_map_instance,
  pthread_lock_interface_instance,	// Dummy instance that is used to store who locked the ostream.
  instance_rdlocked_size,		// Must come after last rwlock and pthread_lock_interface_instance.
#endif
  mutex_initialization_instance,
  ids_singleton_tct_S_ids_instance,
#if CWDEBUG_ALLOC
  alloc_tag_desc_instance,
  list_allocations_instance,
#endif
  dlopen_map_instance,
  dlclose_instance,
  write_max_len_instance,
  set_ostream_instance,
  kill_threads_instance,
  // Values reserved for read/write locks.
  reserved_instance_low,
  reserved_instance_high = 3 * reserved_instance_low,
  // Values reserved for test executables.
  test_instance0 = reserved_instance_high,
  test_instance1,
  test_instance2,
  test_instance3,
  instance_locked_size			// Must be last in list
};

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
extern int instance_locked[instance_locked_size];	// MT: Each element is locked by the
__inline__ bool is_locked(int instance) { return instance_locked[instance] > 0; }
#endif
#if CWDEBUG_DEBUGT
extern pthread_t locked_by[instance_locked_size];	// The id of the thread that last locked it, or 0 when that thread unlocked it.
extern void const* locked_from[instance_locked_size];	// and where is was locked.
int const read_lock_offset = instance_locked_size;
int const high_priority_read_lock_offset = 2 * instance_locked_size;
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCWD_THREAD_SAFE
#endif // LIBCWD_PRIVATE_MUTEX_INSTANCES_H

