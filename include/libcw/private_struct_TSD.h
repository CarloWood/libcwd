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

namespace libcw {
  namespace debug {

#ifdef LIBCWD_THREAD_SAFE
// This function must return 0 for the initial thread and an integer smaller than PTHREAD_THREADS_MAX that
// is unique per _running_ thread.  It can be used as index into an array of size PTHREAD_THREADS_MAX.
__inline__
unsigned long
thread_index(pthread_t tid)
{
  return tid % PTHREAD_THREADS_MAX;
}
#endif

    namespace _private_ {

struct TSD_st;

extern int WST_initializing_TSD;
// Thread Specific Data (TSD) is stored in a structure TSD_st
// and is accessed through a reference to `__libcwd_tsd'.
#ifndef LIBCWD_THREAD_SAFE
// When LIBCWD_THREAD_SAFE is not defined then `__libcwd_tsd' is simply a global object in namespace _private_:
extern TSD_st __libcwd_tsd;
#else
// Otherwise, the Thread Specific Data is stored in this global area:
extern TSD_st __libcwd_tsd_array[PTHREAD_THREADS_MAX];
#endif

#ifdef LIBCWD_THREAD_SAFE
// Internal structure of a thread description structure.
struct pthread_descr_struct {
  union {
    struct {
      struct pthread_descr_struct* self;	// Pointer to this structure.
      void* __padding[12];			// Reserved for future use by pthreads.
      unsigned short libcwd_tsd_index;		// Abuse this space for libcwd.
    } data;
    void* __padding[16];
  } p_header;
  // ...
};
#endif

struct TSD_st {
public:
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
#ifdef DEBUGDEBUGTHREADS
  int cancel_explicitely_deferred;
  int cancel_explicitely_disabled;
  int inside_critical_area;
  int cleanup_handler_installed;
  int internal_debugging_code;
#endif
#ifdef LIBCWD_THREAD_SAFE
  pthread_t tid;
  int do_off_array[LIBCWD_DO_MAX];	// Thread Specific on/off counter for Debug Objects.
  debug_tsd_st* do_array[LIBCWD_DO_MAX];// Thread Specific Data of Debug Objects or NULL when no debug object.
  void cleanup_routine(void) throw();
  int off_cnt_array[LIBCWD_DC_MAX];	// Thread Specific Data of Debug Channels.
#endif

  void S_initialize(void) throw();

#ifdef LIBCWD_THREAD_SAFE
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
    TSD_st* instance = &__libcwd_tsd_array[thread_index(tid)];
    if (!pthread_equal(tid, instance->tid))
      instance->S_initialize();
    return *instance;
  }
#endif // LIBCWD_THREAD_SAFE
};

    } // namespace _private_
  } // namespace debug
} // namespace libcw
 
#ifndef LIBCWD_THREAD_SAFE
// Put __libcwd_tsd in global namespace because anywhere we always refer to it
// as `__libcwd_tsd' because when LIBCWD_THREAD_SAFE is defined it is local variable.
using ::libcw::debug::_private_::__libcwd_tsd;
#endif

#endif // LIBCW_STRUCT_TSD_H
