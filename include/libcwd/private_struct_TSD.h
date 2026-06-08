// SPDX-FileCopyrightText: 2001-2006, 2017-2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_struct_TSD.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_STRUCT_TSD_H
#define LIBCWD_PRIVATE_STRUCT_TSD_H

#include "libcwd/config.h"
#include "private_assert.h"
#include <thread>

namespace libcwd::_private_ {
struct TSD_st;
} // namespace libcwd::_private_

// The active thread's TSD is passed as a local variable (see LIBCWD_TSD_DECLARATION)
// or function parameter (LIBCWD_TSD_PARAM and LIBCWD_COMMA_TSD_PARAM).
// Several helper macros keep those signatures compact:

#define LIBCWD_TSD __libcwd_tsd				// Optional `__libcwd_tsd' parameter (foo() or foo(__libcwd_tsd)).
#define LIBCWD_COMMA_TSD , LIBCWD_TSD			// Idem, but as second or higher parameter.
#define LIBCWD_TSD_PARAM ::libcwd::_private_::TSD_st& __libcwd_tsd
							// Optional function parameter (foo() or foo(TSD_st& __libcwd_tsd)).
#define LIBCWD_TSD_PARAM_UNUSED ::libcwd::_private_::TSD_st&
							// Same without parameter.
#define LIBCWD_COMMA_TSD_PARAM , LIBCWD_TSD_PARAM	// Idem, but as second or higher parameter.
#define LIBCWD_COMMA_TSD_PARAM_UNUSED , LIBCWD_TSD_PARAM_UNUSED
							// Idem, without parameter.
#define LIBCWD_TSD_INSTANCE ::libcwd::_private_::TSD_st::instance()
							// For directly passing the `__libcwd_tsd' instance to a function (foo(TSD::instance())).
#define LIBCWD_COMMA_TSD_INSTANCE , LIBCWD_TSD_INSTANCE	// Idem, but as second or higher parameter.
#define LIBCWD_TSD_DECLARATION ::libcwd::_private_::TSD_st& __libcwd_tsd(::libcwd::_private_::TSD_st::instance())
							// Declaration of local `__libcwd_tsd' structure reference.
#define LIBCWD_DO_TSD(debug_object) (*__libcwd_tsd.do_array[(debug_object).WNS_index])
							// For use inside class debug_ct to access member `m'.
#define LIBCWD_TSD_MEMBER_OFF (__libcwd_tsd.do_off_array[WNS_index])
							// For use inside class debug_ct to access member `_off'.
#define LIBCWD_DO_TSD_MEMBER_OFF(debug_object) (__libcwd_tsd.do_off_array[(debug_object).WNS_index])
							// To access member _off of debug object.


#define LIBCWD_DO_TSD_MEMBER(debug_object, m) (LIBCWD_DO_TSD(debug_object).m)
#define LIBCWD_TSD_MEMBER(m) LIBCWD_DO_TSD_MEMBER(*this, m)

// These includes use the above macros.
#include "struct_debug_tsd.h"

namespace libcwd {

#if CWDEBUG_LOCATION
/** \addtogroup group_locations */
/** \{ */

/** \brief The type of the argument of location_format
 *
 * This type is used together with the bit masks \ref show_objectfile,
 * \ref show_function and \ref show_path.
 */
typedef unsigned short int location_format_t;

location_format_t const show_path = 1;        //!< Show the full source path when printing a location.
location_format_t const show_objectfile = 2;  //!< Show the shared library or executable that owns the location.
location_format_t const show_function = 4;    //!< Show the mangled function name for the location.

/** \} */ // End of group 'group_locations'
#endif

namespace _private_ {

extern int WST_initializing_TSD;

struct TSD_st {
public:
#if CWDEBUG_LOCATION
  location_format_t format{};		// Determines how to print location_ct to an ostream.
#endif
  int terminating = 0;
  bool lock_interface_is_locked = false; // Set while writing debugout to the final ostream if ostream_state_ct::mutex_ was locked.
  bool recursive_fatal = false;			// Detect loop involving dc::fatal or dc::core.
#if CWDEBUG_DEBUG
  bool recursive_assert = false;	// Detect loop involving LIBCWD_ASSERT.
#endif
  int do_off_array[LIBCWD_DO_MAX]{};	// Thread Specific on/off counter for Debug Objects.
  debug_tsd_st* do_array[LIBCWD_DO_MAX]{};// Thread Specific Data of Debug Objects or NULL when no debug object.

  // Release per-thread debug-object data owned by this TSD and mark the thread-list entry as terminating.
  //
  // TSD_st::instance() continues to return this object while cleanup runs so diagnostics emitted during
  // cleanup see the same per-thread state. A second cleanup attempt is ignored.
  void cleanup_routine();
  int off_cnt_array[LIBCWD_DC_MAX]{};	// Thread Specific Data of Debug Channels.

  // Destroy this TSD object.
  //
  // Worker-thread TSD objects are deleted by a thread_local cleanup guard at thread exit. The main-thread
  // object is retained until process termination so global destructors can still use libcwd.
  ~TSD_st();
private:
  bool initialized = false;
  bool cleaning_up = false;

public:
  void thread_destructed();

//-------------------------------------------------------
// Static data and methods.
private:
  // Initialize tsd as the active TSD for the current thread.
  //
  // The initialized flag is set before subsystem initialization so recursive instance() calls reuse the
  // partially initialized object instead of starting a second initialization path.
  static TSD_st& S_create(TSD_st& tsd);

public:
  // Return the TSD object for the calling thread, creating it on first use.
  static TSD_st& instance();
};

// Thread Specific Data (TSD) is stored in a structure TSD_st
// and is accessed through a reference to `__libcwd_tsd'.

} // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_STRUCT_TSD_H
