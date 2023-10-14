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

/** \file libcwd/macro_ForAllDebugObjects.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_MACRO_FORALLDEBUGOBJECTS_H
#define LIBCWD_MACRO_FORALLDEBUGOBJECTS_H

#ifndef LIBCWD_CONFIG_H
#include "libcwd/config.h"
#endif
#ifndef LIBCWD_PRIVATE_ASSERT_H
#include "private_assert.h"
#endif
#ifndef LIBCWD_PRIVATE_INTERNAL_VECTOR_H
#include "private_internal_vector.h"
#endif

//===================================================================================================
// Macro ForAllDebugObjects
//

namespace libcwd {

  class debug_ct;

  namespace _private_ {

class debug_objects_ct {
public:
  typedef internal_vector<debug_ct*> container_type;
private:
  container_type* WNS_debug_objects;
public:
  void init(LIBCWD_TSD_PARAM);
#if LIBCWD_THREAD_SAFE
  void init_and_rdlock();
#endif
  void ST_uninit();
  container_type& write_locked();
  container_type const& read_locked() const;

  // debug_objects is a global object. If it is destructed then in principle
  // all other global objects could already have been destructed, including
  // debug_ct objects. Therefore, destructing the container with with pointers
  // to the debug objects isn't wrong: accessing it (by using the macro
  // ForAllDebugObjects) is wrong. Call ST_uninit to delete WNS_debug_objects.
  ~debug_objects_ct() { ST_uninit(); }
};

inline
debug_objects_ct::container_type&
debug_objects_ct::write_locked()
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT( WNS_debug_objects );
#endif
  return *WNS_debug_objects;
}

inline
debug_objects_ct::container_type const&
debug_objects_ct::read_locked() const
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT( WNS_debug_objects );
#endif
  return *WNS_debug_objects;
}

extern debug_objects_ct debug_objects;

  } // namespace _private_
} // namespace libcwd

#if LIBCWD_THREAD_SAFE
#if CWDEBUG_DEBUGT
#define LIBCWD_ForAllDebugObjects_LOCK_TSD_DECLARATION LIBCWD_TSD_DECLARATION
#else
#define LIBCWD_ForAllDebugObjects_LOCK_TSD_DECLARATION
#endif
#define LIBCWD_ForAllDebugObjects_LOCK \
    LIBCWD_ForAllDebugObjects_LOCK_TSD_DECLARATION; \
    LIBCWD_DEFER_CLEANUP_PUSH(&::libcwd::_private_::rwlock_tct< ::libcwd::_private_::debug_objects_instance>::cleanup, NULL); \
    ::libcwd::_private_::debug_objects.init_and_rdlock()
#define LIBCWD_ForAllDebugObjects_UNLOCK \
    ::libcwd::_private_::rwlock_tct< ::libcwd::_private_::debug_objects_instance>::rdunlock(); \
    LIBCWD_CLEANUP_POP_RESTORE(false);
#else // !LIBCWD_THREAD_SAFE
#define LIBCWD_ForAllDebugObjects_LOCK ::libcwd::_private_::debug_objects.init(LIBCWD_TSD)
#define LIBCWD_ForAllDebugObjects_UNLOCK
#endif // !LIBCWD_THREAD_SAFE

#define LibcwdForAllDebugObjects(dc_namespace, STATEMENT...) \
       do { \
         LIBCWD_ForAllDebugObjects_LOCK; \
	 for( ::libcwd::_private_::debug_objects_ct::container_type::\
	     const_iterator __libcwd_i(::libcwd::_private_::debug_objects.read_locked().begin());\
	     __libcwd_i != ::libcwd::_private_::debug_objects.read_locked().end(); ++__libcwd_i) \
	 { \
	   using namespace ::libcwd; \
	   using namespace dc_namespace; \
	   ::libcwd::debug_ct& debugObject(*(*__libcwd_i)); \
	   { STATEMENT; } \
	 } \
	 LIBCWD_ForAllDebugObjects_UNLOCK \
       } \
       while(0)

#endif // LIBCWD_MACRO_FORALLDEBUGOBJECTS_H
