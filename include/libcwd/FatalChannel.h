// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_FATAL_CHANNEL_H
#define LIBCWD_CLASS_FATAL_CHANNEL_H

#include "control_flag.h"
#include "max_label_len.h"
#include "libcwd/config.h"

namespace libcwd {

namespace _private_ {
struct DebugChannels;
} // namespace _private_

//===================================================================================================
// class FatalChannel
//
// A debug channel with a special characteristic: It terminates the application.
//

class FatalChannel
{
  friend struct _private_::DebugChannels;

 private:
  char label_[max_label_len + 1]; // +1 for zero termination.
                                  // Initialized before this channel is made visible to other threads and read-only
                                  // afterward. A reference name for the represented debug channel This label will be
                                  // printed in front of each output written to this debug channel.

  control_flag_t maskbit_;
  // Written during initialization before this channel is made visible to other threads.
  // The mask that contains the control bit.

 public:
  //-------------------------------------------------------------------------------------------------
  // Constructor
  //

  // MT: All channel objects must be global so that `maskbit_' is zero
  //     at the start of the program and initialization occurs before other
  //     threads share the object.
  explicit FatalChannel(char const* lbl, control_flag_t maskbit);
  // Construct a special debug channel with label `lbl' and control bit `cb'.

  // MT: May only be called from the constructors of global objects (or single threaded functions).
  void NS_initialize(char const* lbl, control_flag_t maskbit);
  // Force initialization in case the constructor of this global object
  // wasn't called yet.  Does nothing when the object was already initialized.

 public:
  control_flag_t get_maskbit() const;
  char const* get_label() const;
};

} // namespace libcwd

#endif // LIBCWD_CLASS_FATAL_CHANNEL_H
