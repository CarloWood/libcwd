// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file class_continued_channel.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_CONTINUED_CHANNEL_H
#define LIBCWD_CLASS_CONTINUED_CHANNEL_H

#include "libcwd/config.h"
#include "control_flag.h"

namespace libcwd {

//===================================================================================================
// class continued_channel_ct
//
// A debug channel with a special characteristic: It uses the same label and
// flags as the previous Dout() call (no prefix is printed unless other
// debug output interrupted the start, indicated with `continued_cf').
//

class continued_channel_ct {
private:
  control_flag_t WNS_maskbit;
    // The mask that contains the control bit.

public:
  //-------------------------------------------------------------------------------------------------
  // Constructor
  //

  // MT: All channel objects must be global so that `WNS_maskbit' is zero
  //     at the start of the program and initialization occurs before other
  //     threads share the object.
  explicit continued_channel_ct(control_flag_t maskbit);
    // Construct a continued debug channel with extra control bit `cb'.

  // MT: May only be called from the constructors of global objects (or single threaded functions).
  void NS_initialize(control_flag_t maskbit);
    // Force initialization in case the constructor of this global object
    // wasn't called yet.  Does nothing when the object was already initialized.

public:
  control_flag_t get_maskbit() const;
};

} // namespace libcwd

#endif // LIBCWD_CLASS_CONTINUED_CHANNEL_H

