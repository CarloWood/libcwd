// $Header$
//
// Copyright (C) 2000, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCW_SYS_H
  #ifdef __GNUC__
    #pragma interface
  #endif
#define LIBCW_SYS_H

//
// Type of optval
//

#if defined(__linux) || defined(__FreeBSD__)
  typedef void* optval_t;
#endif

//
// malloc overhead
//

#if defined(__linux) || defined(__FreeBSD__)		// All OS's that use gnu malloc
  unsigned int const malloc_overhead_c = 0;
#endif

//
// Need word alignment or not
//

#ifdef __i386__
  #undef NEED_WORD_ALIGNMENT
#else
  #error "Unknown architecture"
#endif

//
// Define blocking method
//

#if defined(__linux) || defined(__FreeBSD__)
  #define NBLOCK_POSIX
#endif

#if !defined(NBLOCK_POSIX) && !defined(NBLOCK_BSD) && !defined(NBLOCK_SYSV)
  // You must determine what is the correct non-blocking type for your
  // operating system. Please adjust the lines above to fix this and
  // mail the author (libcw@alinoe.com).
  #error "Undefined NBLOCK_... macro"
#endif

// Return type of signal handler

#define VOIDSIG void

// Bug work arounds

#ifdef __linux
// Including sys/resource.h fails with a warning:
// /usr/include/bits/resource.h:109: warning: `RLIM_INFINITY' redefined
// Work around:
#include <asm/resource.h>
#undef RLIM_INFINITY
#endif

// GNU CC improvements: We can only use this if we have a gcc/g++ compiler
#ifdef __GNUC__

  #define OSTREAM_HAS_VFORM

  typedef int sa_handler_param_type;

  #if (__GNUC__ < 2) || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
    #define NO_ATTRIBUTE
  #endif

  #if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 7)
    #define NEED_OPERATOR_DEFINES
  #endif

#else // !__GNUC__

  // No attributes if we don't have gcc-2.7 or higher
  #define NO_ATTRIBUTE

  typedef int streamsize;
  typedef int sa_handler_param_type;

#endif // !__GNUC__

#ifdef __GNUG__
  #ifdef __linux

    #if defined(__FDSET_INTS)
      typedef unsigned int fd_mask;
    #elif defined(__FDSET_LONGS)
      typedef long unsigned int fd_mask;
    #endif

  #endif // __linux
#endif // __GNUG__

#ifdef NO_ATTRIBUTE
  #define __attribute__(x)
  #define NEED_FAKE_RETURN(x) return x;
#endif

#ifndef NEED_FAKE_RETURN
  #define NEED_FAKE_RETURN(x)
#endif

#define RCSTAG_CC(string) static char const* const rcs_ident __attribute__ ((__unused__)) = string;
#define RCSTAG_H(name, string) static char const* const rcs_ident_##name##_h __attribute__ ((__unused__)) = string;
#define RCSTAG_INL(name, string) static char const* const rcs_ident##name##_inl __attribute__ ((__unused__)) = string;

#define UNUSED(x)
#define USE(var) static void* use_##var = (use_##var = (void*)&use_##var, (void*)&var)

RCSTAG_H(sys_sys, "$Id$")

#endif /* LIBCW_SYS_H */
