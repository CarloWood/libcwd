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

#ifndef LIBCW_SEMWRAPPER_H
#define LIBCW_SEMWRAPPER_H

#include <semaphore.h>

namespace libcw {
  namespace debug {
    namespace _private_ {

// This bug results in __old_sem_* to be used (in libpthread)
// which can not cause problems as long as we declare enough
// space.
//
// With typedef struct { long int sem_status; int sem_spinlock; } old_sem_t;
// it is garanteed however that sizeof(old_sem_t) <= sizeof(sem_t) and
// thus creating a sem_t variable and passing that to __old_sem_*
// functions can not cause problems.
//
// This wrapper is to make sure that __old_sem_* and __new_sem_* calls
// aren't mixed.

extern int __libcwd_sem_post_wrapper(sem_t* sem);
extern int __libcwd_sem_init_wrapper(sem_t* sem, int pshared, unsigned int value);
extern int __libcwd_sem_wait_wrapper(sem_t* sem);
extern int __libcwd_sem_trywait_wrapper(sem_t* sem);
extern int __libcwd_sem_getvalue_wrapper(sem_t* __restrict sem, int* __restrict sval);
extern int __libcwd_sem_destroy_wrapper(sem_t* sem);

    } // namespace __private_
  } // namespace debug
} // namespace libcw

#define LIBCWD_SEM(x) __libcwd_sem_##x##_wrapper

#endif // LIBCW_SEMWRAPPER_H
