// SPDX-FileCopyrightText: 2001-2005, 2018-2019, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_mutex.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_MUTEX_H
#define LIBCWD_PRIVATE_MUTEX_H

#include <pthread.h>

namespace libcwd::_private_ {
#if CWDEBUG_DEBUGT
struct TSD_st;
#endif

class mutex_ct
{
 private:
  pthread_mutex_t M_mutex;
 public:
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
  int M_instance_locked;
#endif
#if CWDEBUG_DEBUGT
  pthread_t M_locked_by;
  void const* M_locked_from;
#endif
 protected:
  bool M_initialized;
  void M_initialize();
 public:
  void initialize();
 public:
  bool try_lock();
  void lock();
  void unlock();
#if CWDEBUG_DEBUGT
  bool try_lock(TSD_st&);
  void lock(TSD_st&);
  void unlock(TSD_st&);
#endif
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
  bool is_locked();
#endif
};

} // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_MUTEX_H
