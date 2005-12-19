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

/** \file class_continued_channel.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_CONTINUED_CHANNEL_H
#define LIBCWD_CLASS_CONTINUED_CHANNEL_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif
#ifndef LIBCWD_CONTROL_FLAG_H
#include <libcwd/control_flag.h>
#endif

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
  control_flag_t get_maskbit(void) const;
};

} // namespace libcwd

#endif // LIBCWD_CLASS_CONTINUED_CHANNEL_H

