// SPDX-FileCopyrightText: 2000-2005, 2013, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_CHANNEL_H
#define LIBCWD_CLASS_CHANNEL_H

#include "control_flag.h"
#include "max_label_len.h"
#include "private/ThreadSpecificData.h"
#include "libcwd/config.h"

#include <atomic>

namespace libcwd {

namespace _private_ {
struct DebugChannels;
struct ChannelSetsWat;
} // namespace _private_

/**
 * @class Channel Channel.h libcwd/debug.h
 * @ingroup controlling-the-output-level-debug-channels
 *
 * @brief This object represents a debug channel, it has a fixed label.
 * A debug channel can be viewed upon as a single bit: on or off.
 *
 * Whenever %debug output is written, one or more %debug %channels must be specified (as first
 * parameter of the @ref Dout macro).
 * The %debug output is then written to the @link setting-the-output-destination ostream @endlink of the
 * @link DebugObject debug object @endlink unless the %debug object
 * is turned off or when all specified %debug %channels are off.
 * Each %debug channel can be turned on and off independently.
 *
 * Multiple %debug %channels can be given by using `operator|` between the channel names.
  * This shouldn't be read as \`or' but merely be seen as the bit-wise OR operation on the bit-masks
 * that these %channels actually represent.
 *
 * **Example:**
 *
 * ```cpp
 * Dout( dc::notice, "Libcwd is a great library" );
 * ```
 *
 * gives as result
 *
 * \exampleoutput
 * NOTICE: Libcwd is a great library
 * \endexampleoutput
 *
 * and
 *
 * ```cpp
 * Dout( dc::hello, "Hello World!" );
 * Dout( dc::kernel|dc::io, "This is written when either the kernel"
 *     "or io channel is turned on." );
 * ```
 *
 * gives as result
 *
 * \exampleoutput
 * HELLO : Hello World!
 * KERNEL: This is written when either the kernel or io channel is turned on.
 * \endexampleoutput
 */

class Channel
{
 private:
  int index_;
  // Assigned during initialization before this channel is made visible to other threads.
  // A unique id that is used as index into the TSD array `off_cnt_array`.

  char label_[max_label_len + 1]; // +1 for zero termination.
                                  // Initialized before this channel is made visible to other threads and read-only
                                  // afterward. A reference name for the represented debug channel This label will be
                                  // printed in front of each output written to this debug channel.

  bool initialized_;
  // Written during initialization before this channel is made visible to other threads.
  // Set to true when initialized.

  static Channel const off_channel;
  // Channel that is always off.

 public:
  //---------------------------------------------------------------------------
  // Constructor
  //

  // MT: All channel objects must be global so that `initialized_` is false
  //     at the start of the program and initialization occurs before other threads
  //     share the object.
  explicit Channel(char const* label, bool add_to_channel_list = true);

  // MT: May only be called from the constructors of global objects (or single threaded functions).
  void NS_initialize(char const* label, LIBCWD_TSD_PARAM, bool add_to_channel_list);
  // Force initialization in case the constructor of this global object
  // wasn't called yet. Does nothing when the object was already initialized.

 private:
  // MT: Take advantage of the public debug-channel registry write lock to prevent
  // simultaneous access to ChannelSets::next_index_ in the case of simultaneously
  // dlopen-loaded libraries and to create a happens-before edge between the write-once
  // and potential reads of this class members by other threads.
  friend struct _private_::DebugChannels;
  void initialize(_private_::ChannelSetsWat wat, char const* label, size_t label_len);

 public:
  //---------------------------------------------------------------------------
  // Manipulators
  //

  void off();
  void on();

  struct OnOffState
  {
    int off_cnt;
  };

  void force_on(OnOffState& state, char const* label);
  void restore(OnOffState const& state);

  Channel const& operator()(bool cond) const { return cond ? *this : off_channel; }

 public:
  //---------------------------------------------------------------------------
  // Accessors
  //

  int index() const { return index_; }
  char const* get_label() const;
  bool is_on() const;
  bool is_on(LIBCWD_TSD_PARAM) const;
};

} // namespace libcwd

#endif // LIBCWD_CLASS_CHANNEL_H
