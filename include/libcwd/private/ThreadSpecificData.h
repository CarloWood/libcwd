// SPDX-FileCopyrightText: 2001-2006, 2017-2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include @ref the-custom-debug-h-file "debug.h".
 */

#ifndef LIBCWD_PRIVATE_STRUCT_TSD_H
#define LIBCWD_PRIVATE_STRUCT_TSD_H

#include "libcwd/LIBCWD_ASSERT.h"
#include "libcwd/config.h"

namespace libcwd::_private_ {
struct ThreadSpecificData;
} // namespace libcwd::_private_

// The active thread's TSD is passed as a local variable (see LIBCWD_TSD_DECLARATION)
// or function parameter (LIBCWD_TSD_PARAM).
// Several helper macros keep those signatures compact:

#define LIBCWD_TSD __libcwd_tsd // Optional `__libcwd_tsd' parameter (foo() or foo(__libcwd_tsd)).
#define LIBCWD_TSD_PARAM ::libcwd::_private_::ThreadSpecificData& __libcwd_tsd
  // Optional function parameter (foo() or foo(ThreadSpecificData& __libcwd_tsd)).
#define LIBCWD_TSD_PARAM_UNUSED ::libcwd::_private_::ThreadSpecificData&
  // Same without parameter.
#define LIBCWD_TSD_INSTANCE ::libcwd::_private_::ThreadSpecificData::instance()
  // For directly passing the `__libcwd_tsd' instance to a function (foo(TSD::instance())).
#define LIBCWD_TSD_DECLARATION \
  ::libcwd::_private_::ThreadSpecificData& __libcwd_tsd(::libcwd::_private_::ThreadSpecificData::instance())
  // Declaration of local `__libcwd_tsd' structure reference.
#define LIBCWD_DO_TSD(debug_object) (*__libcwd_tsd.debug_object_array[(debug_object).index_])
  // For use inside class DebugObject to access member `m'.
#define LIBCWD_TSD_MEMBER_OFF (__libcwd_tsd.debug_object_off_array[index_])
  // For use inside class DebugObject to access member `_off'.
#define LIBCWD_DO_TSD_MEMBER_OFF(debug_object) (__libcwd_tsd.debug_object_off_array[(debug_object).index_])
  // To access member _off of debug object.

#define LIBCWD_DO_TSD_MEMBER(debug_object, m) (LIBCWD_DO_TSD(debug_object).m)
#define LIBCWD_TSD_MEMBER(m) LIBCWD_DO_TSD_MEMBER(*this, m)

// These includes use the above macros.
#include "libcwd/DebugObject_ThreadSpecificData.h"

namespace libcwd {

#if CWDEBUG_LOCATION
/** @addtogroup source-file-line-number-information */
/** \{ */

/** @brief The type of the argument of location_format
 *
 * This type is used together with the bit masks @ref show_objectfile,
 * @ref show_function and @ref show_path.
 */
using location_format_t = unsigned short int;

location_format_t const show_path = 1; //!< Show the full source path when printing a location.
location_format_t const show_objectfile = 2; //!< Show the shared library or executable that owns the location.
location_format_t const show_function = 4; //!< Show the mangled function name for the location.

/** \} */ // End of group 'source-file-line-number-information'
#endif

#ifndef HIDE_FROM_DOXYGEN
namespace _private_ {

extern int WST_initializing_TSD;

struct ThreadSpecificData
{
 public:
#if CWDEBUG_LOCATION
  location_format_t format{}; // Determines how to print Location to an ostream.
#endif
  bool lock_interface_is_locked =
      false; // Set while writing debugout to the final ostream if OstreamState::mutex_ was locked.
  bool recursive_fatal = false; // Detect loop involving dc::fatal or dc::core.
#if CWDEBUG_DEBUG
  bool recursive_assert = false; // Detect loop involving LIBCWD_ASSERT.
#endif
  int debug_object_off_array[LIBCWD_DO_MAX]{}; // Thread Specific on/off counter for Debug Objects.
  DebugObject_ThreadSpecificData*
      debug_object_array[LIBCWD_DO_MAX]{}; // Thread Specific Data of Debug Objects or NULL when no debug object.

  // Release per-thread debug-object data owned by this TSD.
  //
  // ThreadSpecificData::instance() continues to return this object while cleanup runs so diagnostics emitted during
  // cleanup see the same per-thread state. A second cleanup attempt is ignored.
  void cleanup_routine();
  int off_cnt_array[LIBCWD_DC_MAX]{}; // Thread Specific Data of Debug Channels.

  // Destroy this TSD object.
  //
  // Worker-thread TSD objects are deleted by a thread_local cleanup guard at thread exit. The main-thread
  // object is retained until process termination so global destructors can still use libcwd.
  ~ThreadSpecificData();

 private:
  bool initialized_ = false;
  bool cleaning_up_ = false;

  //-------------------------------------------------------
  // Static data and methods.
 private:
  // Initialize tsd as the active TSD for the current thread.
  //
  // The initialized_ flag is set before subsystem initialization so recursive instance() calls reuse the
  // partially initialized object instead of starting a second initialization path.
  static ThreadSpecificData& S_create(ThreadSpecificData& tsd);

 public:
  // Return the TSD object for the calling thread, creating it on first use.
  static ThreadSpecificData& instance();
};

// Thread Specific Data (TSD) is stored in a structure ThreadSpecificData
// and is accessed through a reference to `__libcwd_tsd'.

} // namespace _private_
#endif // HIDE_FROM_DOXYGEN

} // namespace libcwd

#endif // LIBCWD_PRIVATE_STRUCT_TSD_H
