// $Header$
//
// Copyright (C) 2000 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcwd/private_set_alloc_checking.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_SET_ALLOC_CHECKING_H
#define LIBCWD_PRIVATE_SET_ALLOC_CHECKING_H

#ifndef LIBCWD_CONFIG_H
#include "libcwd/config.h"
#endif
#ifndef LIBCWD_PRIVATE_STRUCT_TSD_H
#include "private_struct_TSD.h"
#endif

namespace libcwd {
  namespace _private_ {

#if CWDEBUG_ALLOC
#if CWDEBUG_DEBUGM
  extern void set_alloc_checking_off(LIBCWD_TSD_PARAM);
  extern void set_alloc_checking_on(LIBCWD_TSD_PARAM);
  extern int set_library_call_on(LIBCWD_TSD_PARAM);
  extern void set_library_call_off(int saved_internal LIBCWD_COMMA_TSD_PARAM);
#else
  inline void set_alloc_checking_off(LIBCWD_TSD_PARAM) { ++__libcwd_tsd.internal; }
  inline void set_alloc_checking_on(LIBCWD_TSD_PARAM) { --__libcwd_tsd.internal; }
  inline int set_library_call_on(LIBCWD_TSD_PARAM)
      {
	int internal_saved = __libcwd_tsd.internal;
	__libcwd_tsd.internal = 0;
	++__libcwd_tsd.library_call;
	return internal_saved;
      }
  inline void set_library_call_off(int saved_internal LIBCWD_COMMA_TSD_PARAM)
      {
	__libcwd_tsd.internal = saved_internal;
	--__libcwd_tsd.library_call;
      }
#endif
  inline void set_invisible_on(LIBCWD_TSD_PARAM) { ++__libcwd_tsd.invisible; }
  inline void set_invisible_off(LIBCWD_TSD_PARAM) { --__libcwd_tsd.invisible; }
#else // !CWDEBUG_ALLOC
  inline void set_alloc_checking_off(LIBCWD_TSD_PARAM_UNUSED) { }
  inline void set_alloc_checking_on(LIBCWD_TSD_PARAM_UNUSED) { }
  inline int set_library_call_on(LIBCWD_TSD_PARAM_UNUSED) { return 0; }
  inline void set_library_call_off(int LIBCWD_COMMA_TSD_PARAM_UNUSED) { }
  inline void set_invisible_on(LIBCWD_TSD_PARAM_UNUSED) { }
  inline void set_invisible_off(LIBCWD_TSD_PARAM_UNUSED) { }
#endif // !CWDEBUG_ALLOC

  } // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_SET_ALLOC_CHECKING_H
