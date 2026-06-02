// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/macro_ForAllDebugChannels.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_MACRO_FORALLDEBUGCHANNELS_H
#define LIBCWD_MACRO_FORALLDEBUGCHANNELS_H

#include "libcwd/config.h"
#include "control_flag.h"
#include "private_assert.h"
#include <type_traits>

//===================================================================================================
// Macro ForAllDebugChannels
//

namespace libcwd {

class channel_ct;
class fatal_channel_ct;

namespace _private_ {

struct debug_channels_ct
{
  using callback_type = void (*)(channel_ct&, void*);

  class impl_ct;
  impl_ct* impl_;       // Deliberately leaked.

  static debug_channels_ct const& instance();

  // Return the channel whose label starts with label and is lexicographically largest among matches.
  //
  // The registry is read-locked while scanning. A null pointer is returned when no registered channel
  // matches the requested prefix.
  channel_ct* find(char const* label) const;

  // Initialize channel and optionally add it to the public ForAllDebugChannels registry.
  //
  // The registry write lock is held while WST_max_len, channel labels, and the sorted registry are
  // updated so concurrent global-channel initialization cannot observe a partially updated label width.
  void initialize_channel(channel_ct& channel, char const* label LIBCWD_COMMA_TSD_PARAM, bool add_to_channel_list) const;

  // Initialize a built-in fatal channel without adding it to the public channel registry.
  //
  // The function updates the shared label width while holding the same write lock used for normal
  // channel registration. The fatal channel itself remains write-once/read-mostly state.
  void initialize_fatal_channel(fatal_channel_ct& channel, char const* label,
                                control_flag_t maskbit LIBCWD_COMMA_TSD_PARAM) const;

  // Invoke func for each public debug channel currently registered in the registry.
  //
  // A read lock is held until all callbacks have returned. The callback receives debugChannel as a
  // channel_ct reference; it must not try to register another debug channel while iterating.
  template<typename Func>
  void for_each(Func&& func) const
  {
    using func_type = std::remove_reference_t<Func>;
    for_each_impl([](channel_ct& debugChannel, void* data) { (*static_cast<func_type*>(data))(debugChannel); }, &func);
  }

  void for_each_impl(callback_type callback, void* data) const;
};

static_assert(std::is_trivial_v<debug_channels_ct>, "debug_channels_ct must be trivial to survive static destruction");

} // namespace _private_
} // namespace libcwd

#define LibcwdForAllDebugChannels(dc_namespace, STATEMENT...) \
       do { \
	 ::libcwd::_private_::debug_channels_ct::instance().for_each([&](::libcwd::channel_ct& debugChannel) { \
	   using namespace ::libcwd; \
	   using namespace dc_namespace; \
	   { STATEMENT; } \
	 }); \
       } \
       while(0)

#endif // LIBCWD_MACRO_FORALLDEBUGCHANNELS_H
