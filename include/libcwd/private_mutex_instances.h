// SPDX-FileCopyrightText: 2001-2005, 2008, 2018, 2021, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_mutex_instances.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_MUTEX_INSTANCES_H
#define LIBCWD_PRIVATE_MUTEX_INSTANCES_H

#if CWDEBUG_DEBUGT
#include <pthread.h>
#endif

namespace libcwd::_private_ {

// The different instance-ids used in libcwd.
enum mutex_instance_nt {
  // Recursive mutexes.
  static_tsd_instance,
  dlclose_instance,
  end_recursive_types,
#if CWDEBUG_DEBUGT
  keypair_map_instance,
  pthread_lock_interface_instance,	// Dummy instance that is used to store who locked the ostream.
  instance_rdlocked_size,		// Must come after last rwlock and pthread_lock_interface_instance.
#endif
  mutex_initialization_instance,
  ids_singleton_tct_S_ids_instance,
  dlopen_map_instance,
  backtrace_instance,
  write_max_len_instance,
  set_ostream_instance,
  kill_threads_instance,
  function_instance,
  // Values reserved for read/write locks.
  reserved_instance_low,
  reserved_instance_high = 3 * reserved_instance_low,
  // Values reserved for test executables.
  test_instance0 = reserved_instance_high,      // When using rw locks, use rwtest_instance0 etc.
  test_instance1,
  test_instance2,
  test_instance3,
  instance_locked_size			// Must be last in list
};

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
extern int instance_locked[instance_locked_size];	// MT: Each element is locked by the
inline bool is_locked(int instance) { return instance_locked[instance] > 0; }
#endif
#if CWDEBUG_DEBUGT
extern pthread_t locked_by[instance_locked_size];	// The id of the thread that last locked it, or 0 when that thread unlocked it.
extern void const* locked_from[instance_locked_size];	// and where is was locked.
size_t const read_lock_offset = instance_locked_size;
size_t const high_priority_read_lock_offset = 2 * instance_locked_size;
#endif

} // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_MUTEX_INSTANCES_H
