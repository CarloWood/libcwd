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

/** \file class_channel_set.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_CLASS_CHANNEL_SET_H
#define LIBCWD_CLASS_CHANNEL_SET_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif
#ifndef LIBCWD_CONTROL_FLAG_H
#include <libcwd/control_flag.h>
#endif
#ifndef LIBCWD_PRIVATE_STRUCT_TSD_H
#include <libcwd/private_struct_TSD.h>
#endif

namespace libcwd {

class debug_ct;
struct debug_tsd_st;
class channel_ct;
class fatal_channel_ct;

//===================================================================================================
// struct channel_set_data_st
//
// The attributes of channel_set_bootstrap_st, channel_set_st and continued_channel_set_st
//

struct channel_set_data_st {
public:
  char const* label;
    // The label of the most left channel that is turned on.

  control_flag_t mask;
    // The bit-wise OR mask of all control flags and special channels.

  bool on;
    // Set if at least one of the provided channels is turned on.

  debug_tsd_st* do_tsd_ptr;
    // Thread specific data of current debug object.

#if CWDEBUG_DEBUG
  channel_set_data_st(void) : do_tsd_ptr(NULL) { }
#endif
};

//===================================================================================================
// struct channel_set_bootstrap_st
//
// This is the left-most type of channel 'control' series
// existing of <channel_set_bootstrap_st>|<one or more channels>|<optional control flags>.
// It is used in macro LibcwDoutScopeBegin.
//
// LibcwDoutFatal uses operator& while LibcwDout uses operator|.
//
// The return type is a cast of this object to
// either a channel_set_st (the normal case) or a
// continued_channel_set_st in the case that the
// special debug channel dc::continued was used.
//

class channel_ct;
class fatal_channel_ct;
class continued_channel_ct;
class always_channel_ct;
struct channel_set_st;
struct continued_channel_set_st;

struct channel_set_bootstrap_st : public channel_set_data_st {
  // Warning: This struct may not have attributes of its own!
public:
  channel_set_bootstrap_st(debug_tsd_st& do_tsd LIBCWD_COMMA_TSD_PARAM) { do_tsd_ptr = &do_tsd; }

  //-------------------------------------------------------------------------------------------------
  // Operators that combine channels/control bits.
  //

  channel_set_st& operator|(channel_ct const& dc);
  channel_set_st& operator|(always_channel_ct const& adc);
  channel_set_st& operator&(fatal_channel_ct const& fdc);  // Using operator& just to detect that we indeed used LibcwDoutFatal!
  continued_channel_set_st& operator|(continued_channel_ct const& cdc);
  channel_set_st& operator|(fatal_channel_ct const&);
  channel_set_st& operator&(channel_ct const&);
};

//===================================================================================================
// struct channel_set_st
//
// The debug output target; a combination of channels and control bits.
// The final result of a series of <channel>|<control flag>|...
// is passed to struct_debug_tsd_st::start().
//

struct channel_set_st : public channel_set_data_st {
  // Warning: This struct may not have attributes of its own!
public:
  channel_set_st& operator|(control_flag_t cf);
  channel_set_st& operator|(channel_ct const& dc);
  channel_set_st& operator|(fatal_channel_ct const& fdc);
  continued_channel_set_st& operator|(continued_cf_nt);
};

//===================================================================================================
// struct continued_channel_set_st
//
// The channel set type used for a series that starts with dc::continued.
//

struct continued_channel_set_st : public channel_set_data_st {
  // Warning: This struct may not have attributes of its own!
public:
  continued_channel_set_st& operator|(control_flag_t cf);
};

} // namespace libcwd

#endif // LIBCWD_CLASS_CHANNEL_SET_H

