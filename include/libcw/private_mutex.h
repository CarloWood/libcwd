// $Header$
//
// Copyright (C) 2002 - 2003, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
// 

/** \file libcw/private_mutex.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_MUTEX_H
#define LIBCW_PRIVATE_MUTEX_H

#ifndef LIBCW_PTHREAD_H
#define LIBCW_PTHREAD_H
#include <pthread.h>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

class mutex_ct {
private:
  pthread_mutex_t M_mutex;
public:
#if CWDEBUG_DEBUG
  int M_instance_locked;
#if CWDEBUG_DEBUGT
  pthread_t M_locked_by;
  void const* M_locked_from;
#endif
#endif
protected:
  bool M_initialized;
  void M_initialize(void);
public:
  void initialize(void);
public:
  bool trylock(void);
  void lock(void);
  void unlock(void);
#if CWDEBUG_DEBUG
  bool is_locked(void);
#endif
};

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCW_PRIVATE_MUTEX_H

