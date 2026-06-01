// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/macro_ForAllDebugChannels.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_MACRO_FORALLDEBUGCHANNELS_H
#define LIBCWD_MACRO_FORALLDEBUGCHANNELS_H

#include "libcwd/config.h"
#include "private_assert.h"
#include <vector>

//===================================================================================================
// Macro ForAllDebugChannels
//

namespace libcwd {

  class channel_ct;

  namespace _private_ {

class debug_channels_ct {
public:
  typedef std::vector<channel_ct*> container_type;
  container_type* WNS_debug_channels;
public:
  void init(LIBCWD_TSD_PARAM);
  void init_and_rdlock();
  container_type& write_locked();
  container_type const& read_locked() const;
};

inline
debug_channels_ct::container_type&
debug_channels_ct::write_locked()
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT( WNS_debug_channels );
#endif
  return *WNS_debug_channels;
}

inline
debug_channels_ct::container_type const&
debug_channels_ct::read_locked() const
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT( WNS_debug_channels );
#endif
  return *WNS_debug_channels;
}

extern debug_channels_ct debug_channels;

  } // namespace _private_
} // namespace libcwd

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

#define LibcwdForAllDebugChannels(dc_namespace, STATEMENT...) \
       do { \
	 LIBCWD_ForAllDebugChannels_LOCK; \
	 for( ::libcwd::_private_::debug_channels_ct::container_type::\
	     const_iterator __libcwd_i(::libcwd::_private_::debug_channels.read_locked().begin());\
	     __libcwd_i != ::libcwd::_private_::debug_channels.read_locked().end(); ++__libcwd_i) \
	 { \
	   using namespace ::libcwd; \
	   using namespace dc_namespace; \
	   ::libcwd::channel_ct& debugChannel(*(*__libcwd_i)); \
	   { STATEMENT; } \
	 } \
	 LIBCWD_ForAllDebugChannels_UNLOCK \
       } \
       while(0)

#endif // LIBCWD_MACRO_FORALLDEBUGCHANNELS_H
