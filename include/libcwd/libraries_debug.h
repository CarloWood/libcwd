// SPDX-FileCopyrightText: 2000, 2004-2005, 2007, 2017-2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/libraries_debug.h
 *
 * \brief This is the header file that third-party library headers should include.
 *
 * Don't include this header file directly.&nbsp;
 * See \ref libraries "The Custom debug.h File - libraries" for more information.
 */

#ifndef LIBCWD_LIBRARIES_DEBUG_H
#define LIBCWD_LIBRARIES_DEBUG_H

#ifdef CWDEBUG

#include "libcwd/config.h"

// See http://gcc.gnu.org/onlinedocs/libstdc++/debug.html for more information on -D_GLIBCXX_DEBUG
#if defined(_GLIBCXX_DEBUG) && !CWDEBUG_GLIBCXX_DEBUG
#error Libcwd was not compiled with -D_GLIBCXX_DEBUG while your application is. Please reconfigure libcwd with --enable-glibcxx-debug.
#endif
#if !defined(_GLIBCXX_DEBUG) && CWDEBUG_GLIBCXX_DEBUG
#error Libcwd was compiled with -D_GLIBCXX_DEBUG but your application is not. Please reconfigure libcwd without --enable-glibcxx-debug or use -D_GLIBCXX_DEBUG.
#endif

//===================================================================================================
// The global debug channels used by libcwd.
//

#include "class_channel.h"
#include "class_fatal_channel.h"
#include "class_continued_channel.h"
#include "class_always_channel.h"

namespace libcwd {

namespace channels {

  /** \brief This namespace contains the standard %debug %channels of libcwd.
   *
   * Custom %debug %channels should be added in another namespace in order to
   * avoid the possibility of collisions with %channels defined in other libraries.
   *
   * \sa \ref chapter_custom_debug_h
   */
  namespace dc {
    extern Channel debug;
    extern Channel notice;
    extern Channel system;
    extern Channel warning;
#if CWDEBUG_LOCATION
    extern Channel elfutils;
#endif
    extern FatalChannel fatal;
    extern FatalChannel core;
    extern ContinuedChannel continued;
    extern ContinuedChannel finish;
    extern AlwaysChannel always;

  } // namespace dc
} // namespace channels

} // namespace libcwd


//===================================================================================================
// The global debug object
//

#include "class_debug.h"

namespace libcwd {

extern DebugObject libcw_do;

/** \brief Initialize libcwd global state before normal dynamic initialization reaches libcw_do.
 *
 * This entry point is intended for constructors of user global objects that
 * need libcwd to be usable before libcwd::libcw_do's own constructor has run.
 * The underlying initializer is idempotent: repeated calls are allowed and only
 * the first call constructs the global mutexes, default channels, debug object
 * state, and optional location-support caches.
 *
 * The function has process-wide side effects but does not enable libcw_do or
 * any debug channel besides dc::warning; callers that want output must still turn
 * on the desired debug object and channels.
 */
void initialize();

// Called from main_reached defined in include/libcwd/config.h.in. This does further initialization.
extern void internal_main_reached();
// Returns true once main_reached() was called.
extern bool test_main_reached();

} // namespace libcwd

//===================================================================================================
// Macros
//

#include "macro_Libcwd_macros.h"

// For use in (libcwd) library header files: do not redefine these!
// Developers of libraries are recommended to define their own macro names,
// see "Libraries" on reference-manual/group__chapter__custom__debug__h.html
#define __Debug(STATEMENTS...) \
    LibcwDebug(::libcwd::channels, STATEMENTS)
#define __Dout(cntrl, data) \
    LibcwDout(::libcwd::channels, ::libcwd::libcw_do, cntrl, data)
#define __DoutFatal(cntrl, data) \
    LibcwDoutFatal(::libcwd::channels, ::libcwd::libcw_do, cntrl, data)

//===================================================================================================
// Miscellaneous
//

namespace libcwd {

extern Channel* find_channel(char const* label);
extern void list_channels_on(DebugObject& debug_object);

// Make the inserter functions of std accessible in libcwd.
using std::operator<<;

} // namespace libcwd

// Make the inserter functions of libcwd accessible in global namespace.
namespace libcwd_inserters {
  using libcwd::operator<<;
} // namespace libcwd_inserters
using namespace libcwd_inserters;

#include "macro_ForAllDebugChannels.h"
#include "macro_ForAllDebugObjects.h"
#include "private_environ.h"
#include "class_rcfile.h"
#include "attach_gdb.h"
#include "demangle.h"

// Include the inline functions.
#include "Channel.inl.h"		// Debug channels.
#include "FatalChannel.inl.h"
#include "ContinuedChannel.inl.h"
#include "AlwaysChannel.inl.h"
#include "DebugObject.inl.h"		// Debug objects (DebugObject).
#include "DebugString.inl.h"            // Public member of DebugObject.
#include "ChannelSet.inl.h"	        // Used in macro Dout et al.
#include "Location.inl.h"

// Include optional features.
#if CWDEBUG_LOCATION			// --enable-location
#include "elfutils.h"
#endif

#endif // CWDEBUG
#endif // LIBCWD_LIBRARIES_DEBUG_H
