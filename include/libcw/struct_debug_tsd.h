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

#ifndef LIBCW_STRUCT_DEBUG_TSD_H
#define LIBCW_STRUCT_DEBUG_TSD_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_CLASS_CHANNEL_SET_H
#include <libcw/class_channel_set.h>
#endif
#ifndef LIBCW_PRIVATE_DEBUG_STACK_H
#include <libcw/private_debug_stack.h>
#endif
#ifndef LIBCW_CLASS_DEBUG_STRING_H
#include <libcw/class_debug_string.h>
#endif
#ifndef LIBCW_PRIVATE_STRUCT_TSD_H
#include <libcw/private_struct_TSD.h>
#endif
#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>
#endif

namespace libcw {
  namespace debug {

#ifdef DEBUGUSEBFD
    namespace cwbfd { bool ST_init(void); }
#endif

class debug_ct;
class channel_ct;
class fatal_channel_ct;
class laf_ct;

//===================================================================================================
// struct debug_tsd_st
//
// Structure with Thread Specific Data of a debug object.
//

struct debug_tsd_st {
  friend class debug_ct;

  int _off;
    // Debug output is turned on when this variable is -1, otherwise it is off.

  bool tsd_initialized;
    // Set after initialization is completed.

#ifdef DEBUGDEBUGOUTPUT
  // Since with DEBUGDEBUG defined we start with _off is -1 instead of 0,
  // we need to ignore the call to on() the first time it is called.
  bool first_time;
#endif

  laf_ct* current;
    // Current laf.

  std::ostream* current_oss;
    // The stringstream of the current laf.  This should *always* be equal to current->oss.
    // The reason for keeping this copy is to avoid including <sstream> in debug.h.

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
  void start(debug_ct&, channel_set_data_st& LIBCWD_COMMA_TSD_PARAM);
  void finish(debug_ct&, channel_set_data_st& LIBCWD_COMMA_TSD_PARAM);
  void fatal_finish(debug_ct&, channel_set_data_st& LIBCWD_COMMA_TSD_PARAM) __attribute__ ((__noreturn__));

  // Initialization and de-initialization.
  void init(void);
  debug_tsd_st(void);
  ~debug_tsd_st();
};

  }  // namespace debug
}  // namespace libcw

#endif // LIBCW_STRUCT_DEBUG_TSD_H

