// $Header$
//
// Copyright (C) 2000 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcwd/macro_ForAllDebugChannels.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_MACRO_FORALLDEBUGCHANNELS_H
#define LIBCWD_MACRO_FORALLDEBUGCHANNELS_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif
#ifndef LIBCWD_PRIVATE_ASSERT_H
#include <libcwd/private_assert.h>
#endif
#ifndef LIBCWD_PRIVATE_INTERNAL_VECTOR_H
#include <libcwd/private_internal_vector.h>
#endif

//===================================================================================================
// Macro ForAllDebugChannels
//

namespace libcwd {

  class channel_ct;

  namespace _private_ {

class debug_channels_ct {
public:
  typedef internal_vector<channel_ct*> container_type;
  container_type* WNS_debug_channels;
public:
  void init(LIBCWD_TSD_PARAM);
#if LIBCWD_THREAD_SAFE
  void init_and_rdlock(void);
#endif
  container_type& write_locked(void);
  container_type const& read_locked(void) const;
};

__inline__
debug_channels_ct::container_type&
debug_channels_ct::write_locked(void)
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT( WNS_debug_channels );
#endif
  return *WNS_debug_channels;
}

__inline__
debug_channels_ct::container_type const&
debug_channels_ct::read_locked(void) const
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT( WNS_debug_channels );
#endif
  return *WNS_debug_channels;
}

extern debug_channels_ct debug_channels;

  } // namespace _private_
} // namespace libcwd

#if LIBCWD_THREAD_SAFE
#if CWDEBUG_DEBUGT
#define LIBCWD_ForAllDebugChannels_LOCK_TSD_DECLARATION LIBCWD_TSD_DECLARATION
#else
#define LIBCWD_ForAllDebugChannels_LOCK_TSD_DECLARATION
#endif
#define LIBCWD_ForAllDebugChannels_LOCK \
    LIBCWD_ForAllDebugChannels_LOCK_TSD_DECLARATION; \
    LIBCWD_DEFER_CLEANUP_PUSH(&::libcwd::_private_::rwlock_tct< ::libcwd::_private_::debug_channels_instance>::cleanup, NULL); \
    ::libcwd::_private_::debug_channels.init_and_rdlock()
#define LIBCWD_ForAllDebugChannels_UNLOCK \
    ::libcwd::_private_::rwlock_tct< ::libcwd::_private_::debug_channels_instance>::rdunlock(); \
    LIBCWD_CLEANUP_POP_RESTORE(false);
#else // !LIBCWD_THREAD_SAFE
#define LIBCWD_ForAllDebugChannels_LOCK ::libcwd::_private_::debug_channels.init(LIBCWD_TSD)
#define LIBCWD_ForAllDebugChannels_UNLOCK
#endif // !LIBCWD_THREAD_SAFE

/**
 * \def ForAllDebugChannels
 * \ingroup group_special
 *
 * \brief Looping over all debug channels.
 *
 * The macro \c ForAllDebugChannels allows you to run over all %debug %channels.
 *
 * For example,
 *
 * \code
 * ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
 * \endcode
 *
 * which turns all %channels on.&nbsp;
 * And
 *
 * \code
 * ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() );
 * \endcode
 *
 * which turns all channels off.
 */
#define ForAllDebugChannels(STATEMENT) \
       do { \
	 LIBCWD_ForAllDebugChannels_LOCK; \
	 for( ::libcwd::_private_::debug_channels_ct::container_type::\
	     const_iterator __libcwd_i(::libcwd::_private_::debug_channels.read_locked().begin());\
	     __libcwd_i != ::libcwd::_private_::debug_channels.read_locked().end(); ++__libcwd_i) \
	 { \
	   using namespace ::libcwd; \
	   using namespace DEBUGCHANNELS; \
	   ::libcwd::channel_ct& debugChannel(*(*__libcwd_i)); \
	   { STATEMENT; } \
	 } \
	 LIBCWD_ForAllDebugChannels_UNLOCK \
       } \
       while(0)

#endif // LIBCWD_MACRO_FORALLDEBUGCHANNELS_H
