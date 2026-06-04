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
#if CWDEBUG_DEBUGT
  pthread_lock_interface_instance,	// Dummy instance that is used to store who locked the ostream.
  instance_rdlocked_size,		// Must come after last rwlock and pthread_lock_interface_instance.
#endif
  ids_singleton_tct_S_ids_instance,
  backtrace_instance,
  write_max_len_instance,
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

} // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_MUTEX_INSTANCES_H
