// $Header$
//
// Copyright (C) 2001 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
// 

/** \file libcwd/private_struct_TSD.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_PRIVATE_STRUCT_TSD_H
#define LIBCWD_PRIVATE_STRUCT_TSD_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif
#ifndef LIBCWD_PRIVATE_ASSERT_H
#include <libcwd/private_assert.h>
#endif
#ifndef LIBCWD_PRIVATE_MUTEX_INSTANCES_H
#include <libcwd/private_mutex_instances.h>
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
#include <libcwd/private_mutex.h>
#ifdef LIBCWD_HAVE_PTHREAD
#ifndef LIBCW_PTHREAD_H
#define LIBCW_PTHREAD_H
#include <pthread.h>
#endif
#endif
#endif

namespace libcwd {
  namespace _private_ {
    struct TSD_st;
  } // namespace _private_
} // namespace libcwd

// When LIBCWD_THREAD_SAFE is set then `__libcwd_tsd' is a local variable
// (see LIBCWD_TSD_DECLARATION) or function parameter (LIBCWD_TSD_PARAM and LIBCWD_COMMA_TSD_PARAM).
// This approach means that many function signatures are different because with thread support a
// `__libcwd_tsd' reference needs to be passed.  We use several helper macros for this:
#if LIBCWD_THREAD_SAFE

#define LIBCWD_TSD __libcwd_tsd				// Optional `__libcwd_tsd' parameter (foo() or foo(__libcwd_tsd)).
#define LIBCWD_COMMA_TSD , LIBCWD_TSD			// Idem, but as second or higher parameter.
#define LIBCWD_TSD_PARAM ::libcwd::_private_::TSD_st& __libcwd_tsd
							// Optional function parameter (foo(void) or foo(TSD_st& __libcwd_tsd)).
#define LIBCWD_COMMA_TSD_PARAM , LIBCWD_TSD_PARAM	// Idem, but as second or higher parameter.
#define LIBCWD_TSD_INSTANCE ::libcwd::_private_::TSD_st::instance()
							// For directly passing the `__libcwd_tsd' instance to a function (foo(TSD::instance())).
#define LIBCWD_COMMA_TSD_INSTANCE , LIBCWD_TSD_INSTANCE	// Idem, but as second or higher parameter.
#define LIBCWD_TSD_DECLARATION ::libcwd::_private_::TSD_st& __libcwd_tsd(::libcwd::_private_::TSD_st::instance())
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

// These includes use the above macros.
#ifndef LIBCWD_STRUCT_DEBUG_TSD_H
#include <libcwd/struct_debug_tsd.h>
#endif
#ifndef LIBCWD_PRIVATE_THREAD_H
#include <libcwd/private_thread.h>
#endif

namespace libcwd {

#if CWDEBUG_LOCATION
/** \addtogroup group_locations */
/** \{ */

/** \brief The type of the argument of location_format
 *
 * This type is the same as alloc_format_t but should
 * only be used together with the bit masks \ref show_objectfile, \ref show_function and \ref show_path.
 */
typedef unsigned short int location_format_t;

/** \} */ // End of group 'group_locations'
#endif

  namespace _private_ {

extern int WST_initializing_TSD;
class thread_ct;

struct TSD_st {
public:
#if CWDEBUG_ALLOC
  int internal;				// libsysrecord.so relies on this being the first element.
  int library_call;			// libsysrecord.so relies on this being the second element.
  int inside_malloc_or_free;		// Set when entering a (de)allocation routine non-internal.
  int invisible;			// When set, allocation done must be invisible.
#endif // CWDEBUG_ALLOC
#if CWDEBUG_LOCATION
  location_format_t format;		// Determines how to print location_ct to an ostream.
#endif
#if LIBCWD_THREAD_SAFE
  threadlist_t::iterator thread_iter;	// Persistant thread specific data (might even stay after this object is destructed).
  bool thread_iter_valid;
  thread_ct* target_thread;
  int terminating;
  bool pthread_lock_interface_is_locked;// Set while writing debugout to the final ostream.
  bool list_allocations_on_show_allthreads;
  int inside_free;			// Set when entering free().
#endif
#if CWDEBUG_DEBUGM
  int marker;
#if CWDEBUG_MAGIC
  int annotation;
#endif
#endif
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
  mutex_ct* waiting_for_mutex;		// mutex_ct that this thread is waiting for.
  int waiting_for_lock;			// The instance of the lock that this thread is waiting for.
  int waiting_for_rdlock;		// The instance of the rdlock that this thread is waiting for.
  int instance_rdlocked[instance_rdlocked_size];
  pthread_t rdlocked_by1[instance_rdlocked_size];
  pthread_t rdlocked_by2[instance_rdlocked_size];
  void const* rdlocked_from1[instance_rdlocked_size];
  void const* rdlocked_from2[instance_rdlocked_size];
#endif
#if LIBCWD_THREAD_SAFE
  pthread_t tid;			// Thread ID.
  pid_t pid;				// Process ID.
  int do_off_array[LIBCWD_DO_MAX];	// Thread Specific on/off counter for Debug Objects.
  debug_tsd_st* do_array[LIBCWD_DO_MAX];// Thread Specific Data of Debug Objects or NULL when no debug object.
  void cleanup_routine(void);
  int off_cnt_array[LIBCWD_DC_MAX];	// Thread Specific Data of Debug Channels.
private:
  int tsd_destructor_count;
#endif

public:
  void thread_destructed(void);

#if LIBCWD_THREAD_SAFE
//-------------------------------------------------------
// Static data and methods.
private:
  static TSD_st& S_create(int from_free);
  static pthread_key_t S_tsd_key;
  static pthread_once_t S_tsd_key_once;
  static void S_tsd_key_alloc(void);
  static void S_cleanup_routine(void* arg);

public:
  static TSD_st& instance(void);
  static TSD_st& instance_free(void);
  static void free_instance(TSD_st&);
#endif // LIBCWD_THREAD_SAFE
};

// Thread Specific Data (TSD) is stored in a structure TSD_st
// and is accessed through a reference to `__libcwd_tsd'.

#if !LIBCWD_THREAD_SAFE
// When LIBCWD_THREAD_SAFE is set then `__libcwd_tsd' is a local variable that references
// the Thread Specific Data as returned by TSD_st::instance(), otherwise it is simply a
// global object in namespace _private_:
extern TSD_st __libcwd_tsd;
#else
extern bool WST_tsd_key_created;

inline
TSD_st& TSD_st::instance(void)
{
  TSD_st* instance;
  if (!WST_tsd_key_created || !(instance = (TSD_st*)pthread_getspecific(S_tsd_key)))
    return S_create(0);
  return *instance;
}

// This function is called at the start of free().
inline
TSD_st& TSD_st::instance_free(void)
{
  TSD_st* instance;
  if (!WST_tsd_key_created || !(instance = (TSD_st*)pthread_getspecific(S_tsd_key)))
    return S_create(1);
  else
    instance->inside_free++;
  return *instance;
}
#endif

  } // namespace _private_
} // namespace libcwd
 
#if !LIBCWD_THREAD_SAFE
// Put __libcwd_tsd in global namespace because anywhere we always refer to it
// as `__libcwd_tsd' because when LIBCWD_THREAD_SAFE is set it is local variable.
using ::libcwd::_private_::__libcwd_tsd;
#endif

#endif // LIBCWD_PRIVATE_STRUCT_TSD_H
