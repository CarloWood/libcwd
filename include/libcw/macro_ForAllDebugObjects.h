// $Header$
//
// Copyright (C) 2000 - 2001, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcw/macro_ForAllDebugObjects.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_MACRO_FORALLDEBUGOBJECTS_H
#define LIBCW_MACRO_FORALLDEBUGOBJECTS_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_PRIVATE_ASSERT_H
#include <libcw/private_assert.h>
#endif
#ifndef LIBCW_VECTOR
#define LIBCW_VECTOR
#include <vector>
#endif

//===================================================================================================
// Macro ForAllDebugObjects
//

namespace libcw {
  namespace debug {

    class debug_ct;

    namespace _private_ {

class debug_objects_ct {
public:
  typedef std::vector<debug_ct*> container_type;
private:
  container_type* WNS_debug_objects;
public:
  void init(LIBCWD_TSD_PARAM);
#ifdef LIBCWD_THREAD_SAFE
  void init_and_rdlock(void);
#endif
  void ST_uninit(void);
  container_type& write_locked(void);
  container_type const& read_locked(void) const;
};

__inline__
debug_objects_ct::container_type&
debug_objects_ct::write_locked(void)
{
#ifdef DEBUGDEBUG
  LIBCWD_ASSERT( WNS_debug_objects );
#endif
  return *WNS_debug_objects;
}

__inline__
debug_objects_ct::container_type const&
debug_objects_ct::read_locked(void) const
{
#ifdef DEBUGDEBUG
  LIBCWD_ASSERT( WNS_debug_objects );
#endif
  return *WNS_debug_objects;
}

extern debug_objects_ct debug_objects;

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#ifdef LIBCWD_THREAD_SAFE
#define LIBCWD_ForAllDebugObjects_LOCK \
    LIBCWD_DEFER_CLEANUP_PUSH(&::libcw::debug::_private_::rwlock_tct< ::libcw::debug::_private_::debug_objects_instance>::cleanup, NULL); \
    ::libcw::debug::_private_::debug_objects.init_and_rdlock();
#define LIBCWD_ForAllDebugObjects_UNLOCK \
    ::libcw::debug::_private_::rwlock_tct< ::libcw::debug::_private_::debug_objects_instance>::rdunlock(); \
    LIBCWD_CLEANUP_POP_RESTORE(false);
#else // !LIBCWD_THREAD_SAFE
#define LIBCWD_ForAllDebugObjects_LOCK ::libcw::debug::_private_::debug_objects.init(LIBCWD_TSD);
#define LIBCWD_ForAllDebugObjects_UNLOCK
#endif // !LIBCWD_THREAD_SAFE

/**
 * \def ForAllDebugObjects
 * \ingroup group_special
 *
 * \brief Looping over all debug objects.
 *
 * The macro \c ForAllDebugObjects allows you to run over all %debug objects.
 *
 * For example,
 *
 * \code
 * ForAllDebugObjects( debugObject.set_ostream(&std::cerr, &cerr_mutex) );
 * \endcode
 *
 * would set the output stream of all %debug objects to <CODE>std::cerr</CODE>.
 */
#define ForAllDebugObjects(STATEMENT) \
       do { \
         LIBCWD_ForAllDebugObjects_LOCK \
	 for( ::libcw::debug::_private_::debug_objects_ct::container_type::\
	     const_iterator __libcw_i(::libcw::debug::_private_::debug_objects.read_locked().begin());\
	     __libcw_i != ::libcw::debug::_private_::debug_objects.read_locked().end(); ++__libcw_i) \
	 { \
	   using namespace ::libcw::debug; \
	   using namespace DEBUGCHANNELS; \
	   ::libcw::debug::debug_ct& debugObject(*(*__libcw_i)); \
	   { STATEMENT; } \
	 } \
	 LIBCWD_ForAllDebugObjects_UNLOCK \
       } \
       while(0)

#endif // LIBCW_MACRO_FORALLDEBUGOBJECTS_H
