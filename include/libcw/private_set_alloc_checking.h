// $Header$
//
// Copyright (C) 2000 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcw/private_set_alloc_checking.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_SET_ALLOC_CHECKING_H
#define LIBCW_PRIVATE_SET_ALLOC_CHECKING_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_PRIVATE_STRUCT_TSD_H
#include <libcw/private_struct_TSD.h>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

#if CWDEBUG_ALLOC
#if CWDEBUG_DEBUGM
  extern void set_alloc_checking_off(LIBCWD_TSD_PARAM);
  extern void set_alloc_checking_on(LIBCWD_TSD_PARAM);
  extern int set_library_call_on(LIBCWD_TSD_PARAM);
  extern void set_library_call_off(int saved_internal LIBCWD_COMMA_TSD_PARAM);
#else
  __inline__ void set_alloc_checking_off(LIBCWD_TSD_PARAM) { ++__libcwd_tsd.internal; }
  __inline__ void set_alloc_checking_on(LIBCWD_TSD_PARAM) { --__libcwd_tsd.internal; }
  __inline__ int set_library_call_on(LIBCWD_TSD_PARAM)
      {
	int internal_saved = __libcwd_tsd.internal;
	__libcwd_tsd.internal = 0;
	++__libcwd_tsd.library_call;
	return internal_saved;
      }
  __inline__ void set_library_call_off(int saved_internal LIBCWD_COMMA_TSD_PARAM)
      {
	__libcwd_tsd.internal = saved_internal;
	--__libcwd_tsd.library_call;
      }
#endif
#else // !CWDEBUG_ALLOC
  __inline__ void set_alloc_checking_off(LIBCWD_TSD_PARAM) { }
  __inline__ void set_alloc_checking_on(LIBCWD_TSD_PARAM) { }
  __inline__ void set_library_call_on(LIBCWD_TSD_PARAM) { }
  __inline__ void set_library_call_off(LIBCWD_TSD_PARAM) { }
#endif // !CWDEBUG_ALLOC

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCW_PRIVATE_SET_ALLOC_CHECKING_H
