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

/** \file class_fatal_channel.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_CLASS_FATAL_CHANNEL_H
#define LIBCWD_CLASS_FATAL_CHANNEL_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif
#ifndef LIBCWD_MAX_LABEL_LEN_H
#include <libcwd/max_label_len.h>
#endif
#ifndef LIBCWD_CONTROL_FLAG_H
#include <libcwd/control_flag.h>
#endif

namespace libcw {
  namespace debug {

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
  void NS_initialize(char const* lbl, control_flag_t maskbit);
    // Force initialization in case the constructor of this global object
    // wasn't called yet.  Does nothing when the object was already initialized.

public:
  control_flag_t get_maskbit(void) const;
  char const* get_label(void) const;
};

  } // namespace debug
} // namespace libcw

#endif // LIBCWD_CLASS_FATAL_CHANNEL_H

