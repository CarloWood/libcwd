// $Header$
//
// Copyright (C) 2002 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcwd/private_mutex.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_MUTEX_H
#define LIBCWD_PRIVATE_MUTEX_H

#ifndef LIBCW_PTHREAD_H
#define LIBCW_PTHREAD_H
#include <pthread.h>
#endif

namespace libcwd {
  namespace _private_ {
#if CWDEBUG_DEBUGT
    struct TSD_st;
#endif

class mutex_ct {
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

  } // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_MUTEX_H

