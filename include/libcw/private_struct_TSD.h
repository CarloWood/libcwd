// $Header$
//
// Copyright (C) 2001, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
// 

/** \file libcw/private_struct_TSD.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_STRUCT_TSD_H
#define LIBCW_STRUCT_TSD_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_CSTRING
#define LIBCW_CSTRING
#include <cstring>	// Needed for std::memset.
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {
      struct TSD_st;
    }
  }
}

// When LIBCWD_THREAD_SAFE is defined then `__libcwd_tsd' is a local variable
// (see LIBCWD_TSD_DECLARATION) or function parameter (LIBCWD_TSD_PARAM and LIBCWD_COMMA_TSD_PARAM).
// This approach means that many function signatures are different because with thread support a
// `__libcwd_tsd' reference needs to be passed.  We use several helper macros for this:
#ifdef LIBCWD_THREAD_SAFE

#define LIBCWD_TSD __libcwd_tsd		// Optional `__libcwd_tsd' parameter
					//   (foo() or foo(__libcwd_tsd)).
#define LIBCWD_COMMA_TSD , LIBCWD_TSD	// Idem, but as second or higher parameter.
#define LIBCWD_TSD_PARAM ::libcw::debug::_private_::TSD_st& __libcwd_tsd
					// Optional function parameter
					//   (foo(void) or foo(TSD_st& __libcwd_tsd)).
#define LIBCWD_COMMA_TSD_PARAM , LIBCWD_TSD_PARAM
					// Idem, but as second or higher parameter.
#else // !LIBCWD_THREAD_SAFE

#define LIBCWD_TSD
#define LIBCWD_COMMA_TSD
#define LIBCWD_TSD_PARAM void
#define LIBCWD_COMMA_TSD_PARAM

#endif // !LIBCWD_THREAD_SAFE

// This include uses the above macros.
#ifndef LIBCW_STRUCT_DEBUG_TSD
#include <libcw/struct_debug_tsd.h>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

struct TSD_st {
#ifdef DEBUGMALLOC
  int internal;
  int library_call;
#ifdef DEBUGDEBUGMALLOC
  int marker;
  int recursive;        	// Used for sanity double checks in debugmalloc.cc.
#endif
#endif // DEBUGMALLOC
  bool recursive_fatal;		// Detect loop involving dc::fatal or dc::core.
#ifdef DEBUGDEBUG
  bool recursive_assert;	// Detect loop involving LIBCWD_ASSERT.
#endif
#ifdef LIBCWD_THREAD_SAFE	// Directly contained in debug_ct when not threaded.
  debug_tsd_st do_array[16];	// Thread Specific Data of Debug Objects.
#ifdef DEBUGDEBUGTHREADS
  int cancel_explicitely_deferred;
  int cancel_explicitely_disabled;
  int inside_critical_area;
  int cleanup_handler_installed;
  int internal_debugging_code;
#endif
#endif
  int off_cnt_array[256];
  TSD_st(void) { std::memset(this, 0, sizeof(struct TSD_st)); }
};

// Thread Specific Data (TSD) is stored in a structure TSD_st
// and is accessed through a reference to `__libcwd_tsd'.
#ifndef LIBCWD_THREAD_SAFE
// When LIBCWD_THREAD_SAFE is not defined then `__libcwd_tsd' is simply a global object in namespace _private_:
extern TSD_st __libcwd_tsd;
#endif
extern int WST_initializing_TSD;

    } // namespace _private_
  } // namespace debug
} // namespace libcw
 
#ifndef LIBCWD_THREAD_SAFE
// Put __libcwd_tsd in global namespace because anywhere we always refer to it
// as `__libcwd_tsd' because when LIBCWD_THREAD_SAFE is defined it is local variable.
using ::libcw::debug::_private_::__libcwd_tsd;
#endif

#endif // LIBCW_STRUCT_TSD_H
