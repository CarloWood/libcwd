// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file class_fatal_channel.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_FATAL_CHANNEL_H
#define LIBCWD_CLASS_FATAL_CHANNEL_H

#include "libcwd/config.h"
#include "max_label_len.h"
#include "control_flag.h"

namespace libcwd {

//===================================================================================================
// class fatal_channel_ct
//
// A debug channel with a special characteristic: It terminates the application.
//

class fatal_channel_ct {
private:
  char WNS_label[max_label_len_c + 1];				// +1 for zero termination.
    // A reference name for the represented debug channel
    // This label will be printed in front of each output written to
    // this debug channel.

  control_flag_t WNS_maskbit;
    // The mask that contains the control bit.

public:
  //-------------------------------------------------------------------------------------------------
  // Constructor
  //

  // MT: All channel objects must be global so that `WNS_maskbit' is zero
  //     at the start of the program and initialization occurs before other
  //     threads share the object.
  explicit fatal_channel_ct(char const* lbl, control_flag_t maskbit);
    // Construct a special debug channel with label `lbl' and control bit `cb'.

  // MT: May only be called from the constructors of global objects (or single threaded functions).
  void NS_initialize(char const* lbl, control_flag_t maskbit LIBCWD_COMMA_TSD_PARAM);
    // Force initialization in case the constructor of this global object
    // wasn't called yet.  Does nothing when the object was already initialized.

public:
  control_flag_t get_maskbit() const;
  char const* get_label() const;
};

} // namespace libcwd

#endif // LIBCWD_CLASS_FATAL_CHANNEL_H
