// $Header$
//
// Copyright (C) 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

//
// Bug work around.
// See http://sources.redhat.com/ml/binutils/2002-01/msg00???.html
//

#include "sys.h"
#include <libcw/semwrapper.h>

namespace libcw {
  namespace debug {
    namespace _private_ {

int __libcwd_sem_post_wrapper(sem_t* sem)
{
  return sem_post(sem);
}

int __libcwd_sem_init_wrapper(sem_t* sem, int pshared, unsigned int value)
{
  return sem_init(sem, pshared, value);
}

int __libcwd_sem_wait_wrapper(sem_t* sem)
{
  return sem_wait(sem);
}

int __libcwd_sem_trywait_wrapper(sem_t* sem)
{
  return sem_trywait(sem);
}

int __libcwd_sem_getvalue_wrapper(sem_t* __restrict sem, int* __restrict sval)
{
  return sem_getvalue(sem, sval);
}

int __libcwd_sem_destroy_wrapper(sem_t* sem)
{
  return sem_destroy(sem); 
}

    } // namespace __private_
  } // namespace debug
} // namespace libcw

