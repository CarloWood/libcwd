// $Header$
//
// Copyright (C) 2000 - 2001, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file class_channel_set.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_CLASS_CHANNEL_SET_H
#define LIBCW_CLASS_CHANNEL_SET_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_CONTROL_FLAG_H
#include <libcw/control_flag.h>
#endif

namespace libcw {
  namespace debug {

class debug_ct;
class channel_ct;
class fatal_channel_ct;

//===================================================================================================
// struct channel_set_data_st
//
// The attributes of channel_set_st and continued_channel_set_st
//

struct channel_set_data_st {
public:
  char const* label;
    // The label of the most left channel that is turned on.

  control_flag_t mask;
    // The bit-wise OR mask of all control flags and special channels.

  bool on;
    // Set if at least one of the provided channels is turned on.

  debug_ct* debug_object;
    // The owner of this object.
};

//===================================================================================================
// struct continued_channel_set_st
//
// The debug output target; a combination of channels and control bits.
//

struct continued_channel_set_st : public channel_set_data_st {
  // Warning: This struct may not have attributes of its own!
public:
  continued_channel_set_st& operator|(control_flag_t cf);
};

//===================================================================================================
// struct channel_set_st
//
// The debug output target; a combination of channels and control bits.
//

struct channel_set_st : public channel_set_data_st {
  // Warning: This struct may not have attributes of its own!
public:
  channel_set_st& operator|(control_flag_t cf);
  channel_set_st& operator|(channel_ct const& dc);
  channel_set_st& operator|(fatal_channel_ct const& fdc);
  continued_channel_set_st& operator|(continued_cf_nt);
};

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_CHANNEL_SET_H

