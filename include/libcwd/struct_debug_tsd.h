// SPDX-FileCopyrightText: 2002-2004, 2006, 2018, 2020-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_STRUCT_DEBUG_TSD_H
#define LIBCWD_STRUCT_DEBUG_TSD_H

#include "libcwd/config.h"
#include "class_channel_set.h"
#include "private_debug_stack.h"
#include "class_debug_string.h"
#include "private_struct_TSD.h"
#include <iosfwd>

namespace libcwd {

#if CWDEBUG_LOCATION
namespace dwarf {
bool ensure_initialization(LIBCWD_TSD_PARAM);
} // namespace dwarf
#endif // CWDEBUG_LOCATION

class debug_ct;
class channel_ct;
class fatal_channel_ct;
class laf_ct;

//===================================================================================================
// struct debug_tsd_st
//
// Structure with Thread Specific Data of a debug object.
//

struct debug_tsd_st
{
  friend class debug_ct;

  bool tsd_initialized;
    // Set after initialization is completed.

#if CWDEBUG_DEBUGOUTPUT
  // Since we start with _off is -1 instead of 0 when CWDEBUG_DEBUG is set,
  // we need to ignore the call to on() the first time it is called.
  bool first_time;
#endif

  laf_ct* current_;
    // Current laf.

  std::ostream* current_bufferstream;
    // The stringstream of the current_ laf.
    // This should be set to current_->bufferstream at all times.

  _private_::debug_stack_tst<laf_ct*> laf_stack;
    // Store for nested debug calls.

  bool start_expected;
    // Set to true when start() is expected, otherwise we expect a call to finish().

  bool unfinished_expected;
    // Set to true when start() should cause a <unfinished>.

  int off_count;
    // Number of nested and switched off continued channels till first switched on continued channel.

  _private_::debug_stack_tst<int> continued_stack;
    // Stores the number of nested and switched off continued channels.

  debug_string_ct color_on;
    // Colorization code for debug output.

  debug_string_ct color_off;
    // Undo the effect of color_on.

  debug_string_ct margin;
    // The margin string.

  debug_string_ct marker;
    // The marker string.

  debug_string_stack_element_ct* M_margin_stack;
    // Pointer to list of pushed margins.

  debug_string_stack_element_ct* M_marker_stack;
    // Pointer to list of pushed markers.

  unsigned short indent;
    // Position at which debug message is printed.
    // A value of 0 means directly behind the marker.

  // Accessed from LibcwdDout.
  void start(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM);
  void finish(debug_ct& debug_object, channel_set_data_st& /*channel_set*/ LIBCWD_COMMA_TSD_PARAM);
  [[noreturn]] void fatal_finish(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM);

  // Initialization and de-initialization.
  void init();
  // In the non-threaded case, debug_ct contains a debug_tsd_st which
  // may already be initialized before.  Therefore don't initialize
  // these in the non-threaded case, but rely on tsd_initialized,
  // current_bufferstream and _off to be zeroed as a result of being
  // part of a global object.
  debug_tsd_st() : tsd_initialized(false), current_bufferstream(NULL) { }
  ~debug_tsd_st();
};

} // namespace libcwd

#endif // LIBCWD_STRUCT_DEBUG_TSD_H
