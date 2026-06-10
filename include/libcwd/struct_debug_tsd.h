// SPDX-FileCopyrightText: 2002-2004, 2006, 2018, 2020-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_STRUCT_DEBUG_TSD_H
#define LIBCWD_STRUCT_DEBUG_TSD_H

#include "class_channel_set.h"
#include "class_debug_string.h"
#include "private_debug_stack.h"
#include "private_struct_TSD.h"
#include "libcwd/config.h"

#include <iosfwd>

namespace libcwd {

#if CWDEBUG_LOCATION
namespace dwarf {
bool ensure_initialization(LIBCWD_TSD_PARAM);
} // namespace dwarf
#endif // CWDEBUG_LOCATION

class DebugObject;
class Channel;
class FatalChannel;
class OutputState;

//===================================================================================================
// struct DebugObject_ThreadSpecificData
//
// Structure with Thread Specific Data of a debug object.
//

struct DebugObject_ThreadSpecificData
{
  friend class DebugObject;

  bool tsd_initialized;
  // Set after initialization is completed.

#if CWDEBUG_DEBUGOUTPUT
  // Since we start with _off is -1 instead of 0 when CWDEBUG_DEBUG is set,
  // we need to ignore the call to on() the first time it is called.
  bool first_time;
#endif

  OutputState* current;
  // Current output state.

  std::ostream* current_bufferstream;
  // The stringstream of the current output state.
  // This should be set to current->bufferstream at all times.

  _private_::DebugStack<OutputState*> output_state_stack;
  // Store for nested debug calls.

  bool start_expected;
  // Set to true when start() is expected, otherwise we expect a call to finish().

  bool unfinished_expected;
  // Set to true when start() should cause a <unfinished>.

  int off_count;
  // Number of nested and switched off continued channels till first switched on continued channel.

  _private_::DebugStack<int> continued_stack;
  // Stores the number of nested and switched off continued channels.

  DebugString color_on;
  // Colorization code for debug output.

  DebugString color_off;
  // Undo the effect of color_on.

  DebugString margin;
  // The margin string.

  DebugString marker;
  // The marker string.

  DebugStringStackElement* margin_stack;
  // Pointer to list of pushed margins.

  DebugStringStackElement* marker_stack;
  // Pointer to list of pushed markers.

  unsigned short indent;
  // Position at which debug message is printed.
  // A value of 0 means directly behind the marker.

  // Accessed from LibcwdDout.
  void start(DebugObject& debug_object, ChannelSetData& channel_set LIBCWD_COMMA_TSD_PARAM);
  void finish(DebugObject& debug_object, ChannelSetData& /*channel_set*/ LIBCWD_COMMA_TSD_PARAM);
  [[noreturn]] void fatal_finish(DebugObject& debug_object, ChannelSetData& channel_set LIBCWD_COMMA_TSD_PARAM);

  // Initialization and de-initialization.
  void init();
  // In the non-threaded case, DebugObject contains a DebugObject_ThreadSpecificData which
  // may already be initialized before.  Therefore don't initialize
  // these in the non-threaded case, but rely on tsd_initialized,
  // current_bufferstream and _off to be zeroed as a result of being
  // part of a global object.
  DebugObject_ThreadSpecificData() : tsd_initialized(false), current_bufferstream(NULL) { }
  ~DebugObject_ThreadSpecificData();
};

} // namespace libcwd

#endif // LIBCWD_STRUCT_DEBUG_TSD_H
