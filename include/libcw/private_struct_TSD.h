// $Header$
//
// Copyright (C) 2001 - 2002, by
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
#ifndef LIBCW_PRIVATE_ASSERT_H
#include <libcw/private_assert.h>
#endif
#ifndef LIBCW_CSTRING
#define LIBCW_CSTRING
#include <cstring>	// Needed for std::memset.
#endif
#ifndef LIBCW_LIMITS_H
#define LIBCW_LIMITS_H
#include <limits.h>	// For PTHREAD_THREADS_MAX
#endif
#if LIBCWD_THREAD_SAFE
#include <libcw/private_mutex.h>
#ifdef LIBCWD_HAVE_PTHREAD
#ifndef LIBCW_PTHREADS_H
#define LIBCW_PTHREADS_H
#include <pthread.h>
#endif
#endif
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {
      struct TSD_st;
    }
  }
}

// When LIBCWD_THREAD_SAFE is set then `__libcwd_tsd' is a local variable
// (see LIBCWD_TSD_DECLARATION) or function parameter (LIBCWD_TSD_PARAM and LIBCWD_COMMA_TSD_PARAM).
// This approach means that many function signatures are different because with thread support a
// `__libcwd_tsd' reference needs to be passed.  We use several helper macros for this:
#if LIBCWD_THREAD_SAFE

#define LIBCWD_TSD __libcwd_tsd				// Optional `__libcwd_tsd' parameter (foo() or foo(__libcwd_tsd)).
#define LIBCWD_COMMA_TSD , LIBCWD_TSD			// Idem, but as second or higher parameter.
#define LIBCWD_TSD_PARAM ::libcw::debug::_private_::TSD_st& __libcwd_tsd
							// Optional function parameter (foo(void) or foo(TSD_st& __libcwd_tsd)).
#define LIBCWD_COMMA_TSD_PARAM , LIBCWD_TSD_PARAM	// Idem, but as second or higher parameter.
#define LIBCWD_TSD_INSTANCE ::libcw::debug::_private_::TSD_st::instance()
							// For directly passing the `__libcwd_tsd' instance to a function (foo(TSD::instance())).
#define LIBCWD_COMMA_TSD_INSTANCE , LIBCWD_TSD_INSTANCE	// Idem, but as second or higher parameter.
#define LIBCWD_TSD_DECLARATION ::libcw::debug::_private_::TSD_st& __libcwd_tsd(::libcw::debug::_private_::TSD_st::instance());
							// Declaration of local `__libcwd_tsd' structure reference.
#define LIBCWD_DO_TSD(debug_object) (*__libcwd_tsd.do_array[(debug_object).WNS_index])
    							// For use inside class debug_ct to access member `m'.
#define LIBCWD_TSD_MEMBER_OFF (__libcwd_tsd.do_off_array[WNS_index])
							// For use inside class debug_ct to access member `_off'.
#define LIBCWD_DO_TSD_MEMBER_OFF(debug_object) (__libcwd_tsd.do_off_array[(debug_object).WNS_index])
							// To access member _off of debug object.

#else // !LIBCWD_THREAD_SAFE

#define LIBCWD_TSD
#define LIBCWD_COMMA_TSD
#define LIBCWD_TSD_PARAM void
#define LIBCWD_COMMA_TSD_PARAM
#define LIBCWD_TSD_INSTANCE
#define LIBCWD_COMMA_TSD_INSTANCE
#define LIBCWD_TSD_DECLARATION
#define LIBCWD_DO_TSD(debug_object) ((debug_object).tsd)
#define LIBCWD_TSD_MEMBER_OFF (tsd._off)
#define LIBCWD_DO_TSD_MEMBER_OFF(debug_object) ((debug_object).tsd._off)

#endif // !LIBCWD_THREAD_SAFE

#define LIBCWD_DO_TSD_MEMBER(debug_object, m) (LIBCWD_DO_TSD(debug_object).m)
#define LIBCWD_TSD_MEMBER(m) LIBCWD_DO_TSD_MEMBER(*this, m)

// This include uses the above macros.
#ifndef LIBCW_STRUCT_DEBUG_TSD
#include <libcw/struct_debug_tsd.h>
#endif
#ifndef LIBCW_PRIVATE_THREAD_H
#include <libcw/private_thread.h>
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

extern int WST_initializing_TSD;
class thread_ct;

#if LIBCWD_THREAD_SAFE
__inline__ TSD_st* get_tsd_instance(pthread_t tid);
#endif

struct TSD_st {
public:
#if CWDEBUG_ALLOC
  int internal;
  int library_call;
  int inside_malloc_or_free;		// Set when entering a (de)allocation routine non-internal.
#if LIBCWD_THREAD_SAFE
  threadlist_t::iterator thread_iter;	// Persistant thread specific data (might even stay after this object is destructed).
  bool thread_iter_valid;
#endif
#if CWDEBUG_DEBUGM
  int marker;
#endif
#endif // CWDEBUG_ALLOC
  bool recursive_fatal;			// Detect loop involving dc::fatal or dc::core.
#if CWDEBUG_DEBUG
  bool recursive_assert;		// Detect loop involving LIBCWD_ASSERT.
#endif
#if CWDEBUG_DEBUGT
  int cancel_explicitely_deferred;
  int cancel_explicitely_disabled;
  int inside_critical_area;
  int cleanup_handler_installed;
  int internal_debugging_code;
#endif
#if LIBCWD_THREAD_SAFE
  pthread_t tid;
  int do_off_array[LIBCWD_DO_MAX];	// Thread Specific on/off counter for Debug Objects.
  debug_tsd_st* do_array[LIBCWD_DO_MAX];// Thread Specific Data of Debug Objects or NULL when no debug object.
  void cleanup_routine(void) throw();
  int off_cnt_array[LIBCWD_DC_MAX];	// Thread Specific Data of Debug Channels.
#endif

  void S_initialize(void) throw();

#if LIBCWD_THREAD_SAFE
//-------------------------------------------------------
// Static data and methods.
private:
  static pthread_key_t S_exit_key;
  static pthread_once_t S_exit_key_once;
  static void S_exit_key_alloc(void) throw();
  static void S_cleanup_routine(void* arg) throw();

public:
  static TSD_st& instance(void) throw()
  {
    pthread_t tid = pthread_self();
    TSD_st* instance = get_tsd_instance(tid);
    if (!pthread_equal(tid, instance->tid))
      instance->S_initialize();
    return *instance;
  }
#endif // LIBCWD_THREAD_SAFE
};

// Thread Specific Data (TSD) is stored in a structure TSD_st
// and is accessed through a reference to `__libcwd_tsd'.

#if !LIBCWD_THREAD_SAFE
// When LIBCWD_THREAD_SAFE is not set then `__libcwd_tsd' is simply a global object in namespace _private_:
extern TSD_st __libcwd_tsd;
#else
// Otherwise, __libcwd_tsd is a local variable that references the Thread Specific Data that is stored in this global array.
extern TSD_st __libcwd_tsd_array[PTHREAD_THREADS_MAX];

// This function must return a TSD_st* to an instance that is unique per _running_ thread.
__inline__
TSD_st*
get_tsd_instance(pthread_t tid)
{
  return &__libcwd_tsd_array[tid % PTHREAD_THREADS_MAX];
}
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw
 
#if !LIBCWD_THREAD_SAFE
// Put __libcwd_tsd in global namespace because anywhere we always refer to it
// as `__libcwd_tsd' because when LIBCWD_THREAD_SAFE is set it is local variable.
using ::libcw::debug::_private_::__libcwd_tsd;
#endif

#endif // LIBCW_STRUCT_TSD_H
