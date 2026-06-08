// SPDX-FileCopyrightText: 2000-2007, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file class_channel_set.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_CHANNEL_SET_H
#define LIBCWD_CLASS_CHANNEL_SET_H

#include "libcwd/config.h"
#include "control_flag.h"
#include "private_struct_TSD.h"

namespace libcwd {

class DebugObject;
struct DebugObject_ThreadSpecificData;
class Channel;
class FatalChannel;

//===================================================================================================
// struct ChannelSetData
//
// The attributes of ChannelSetBootstrap, ChannelSet and ContinuedChannelSet
//

struct ChannelSetData {
public:
  char const* label;
    // The label of the most left channel that is turned on.

  control_flag_t mask;
    // The bit-wise OR mask of all control flags and special channels.

  bool on;
    // Set if at least one of the provided channels is turned on.

  DebugObject_ThreadSpecificData* do_tsd_ptr;
    // Thread specific data of current debug object.

#if CWDEBUG_DEBUG
  ChannelSetData() : do_tsd_ptr(nullptr) { }
#endif
};

//===================================================================================================
// struct ChannelSetBootstrap
//
// This is the left-most type of channel 'control' series
// existing of <ChannelSetBootstrap>|<one or more channels>|<optional control flags>.
// It is used in macro LibcwDoutScopeBegin.
//
// LibcwdDoutFatalScopeBegin uses FatalChannelSetBootstrap,
// to make sure it was used with dc::fatal or dc::core.
//
// The return type is a cast of this object to
// either a ChannelSet (the normal case) or a
// ContinuedChannelSet in the case that the
// special debug channel dc::continued was used.
//

class Channel;
class FatalChannel;
class ContinuedChannel;
class AlwaysChannel;
struct ChannelSet;
struct ContinuedChannelSet;

struct ChannelSetBootstrap : public ChannelSetData {
  // Warning: This struct may not have attributes of its own!
public:
  ChannelSetBootstrap(DebugObject_ThreadSpecificData& do_tsd LIBCWD_COMMA_TSD_PARAM_UNUSED) { do_tsd_ptr = &do_tsd; }

  //-------------------------------------------------------------------------------------------------
  // Operators that combine channels/control bits.
  //

  ChannelSet& operator|(Channel const& dc);
  ChannelSet& operator|(AlwaysChannel const& adc);
  ContinuedChannelSet& operator|(ContinuedChannel const& cdc);
};

struct FatalChannelSetBootstrap : public ChannelSetData {
  // Warning: This struct may not have attributes of its own!
public:
  FatalChannelSetBootstrap(DebugObject_ThreadSpecificData& do_tsd LIBCWD_COMMA_TSD_PARAM_UNUSED) { do_tsd_ptr = &do_tsd; }

  //-------------------------------------------------------------------------------------------------
  // Operators that combine channels/control bits.
  //
  ChannelSet& operator|(FatalChannel const& fdc);
};

//===================================================================================================
// struct ChannelSet
//
// The debug output target; a combination of channels and control bits.
// The final result of a series of <channel>|<control flag>|...
// is passed to struct_debug_tsd_st::start().
//

struct ChannelSet : public ChannelSetData {
  // Warning: This struct may not have attributes of its own!
public:
  ChannelSet& operator|(control_flag_t cf);
  ChannelSet& operator|(Channel const& dc);
  ChannelSet& operator|(FatalChannel const& fdc);
  ContinuedChannelSet& operator|(continued_cf_nt);
};

//===================================================================================================
// struct ContinuedChannelSet
//
// The channel set type used for a series that starts with dc::continued.
//

struct ContinuedChannelSet : public ChannelSetData {
  // Warning: This struct may not have attributes of its own!
public:
  ContinuedChannelSet& operator|(control_flag_t cf);
};

} // namespace libcwd

#endif // LIBCWD_CLASS_CHANNEL_SET_H
