// $Header$
//
// Copyright (C) 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file class_always_channel.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_ALWAYS_CHANNEL_H
#define LIBCW_ALWAYS_CHANNEL_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_CONTROL_FLAG_H
#include <libcw/control_flag.h>
#endif

namespace libcw {
  namespace debug {

//===================================================================================================
// class always_channel_ct
//
// A debug channel with a special characteristic: It is always turned
// and cannot be turned off.
//

class always_channel_ct {
public:
  static char const label[];
};

  } // namespace debug
} // namespace libcw

#endif // LIBCW_ALWAYS_CHANNEL_H

