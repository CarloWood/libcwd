// $Header$
//
// Copyright (C) 2000 - 2004, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

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

#include <libcwd/config.h>

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

#include <libcwd/class_channel.h>
#include <libcwd/class_fatal_channel.h>
#include <libcwd/class_continued_channel.h>
#include <libcwd/class_always_channel.h>

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
    extern channel_ct debug;
    extern channel_ct notice;
    extern channel_ct system;
    extern channel_ct warning;
#if CWDEBUG_ALLOC
#ifdef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
    extern channel_ct malloc;
#else
    extern channel_ct __libcwd_malloc;
#endif
#else // !CWDEBUG_ALLOC
    extern channel_ct malloc;
#endif
#if CWDEBUG_LOCATION
    extern channel_ct bfd;
#endif
    extern fatal_channel_ct fatal;
    extern fatal_channel_ct core;
    extern continued_channel_ct continued;
    extern continued_channel_ct finish;
    extern always_channel_ct always;

  } // namespace dc
} // namespace channels

} // namespace libcwd


//===================================================================================================
// The global debug object
//

#include <libcwd/class_debug.h>

namespace libcwd {

extern debug_ct libcw_do;

} // namespace libcwd

//===================================================================================================
// Macros
//

#include <libcwd/macro_Libcwd_macros.h>

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

extern channel_ct* find_channel(char const* label);
extern void list_channels_on(debug_ct& debug_object);

// Make the inserter functions of std accessible in libcwd.
using std::operator<<;

} // namespace libcwd

// Make the inserter functions of libcwd accessible in global namespace.
namespace libcwd_inserters {
  using libcwd::operator<<;
} // namespace libcwd_inserters
using namespace libcwd_inserters;

#include <libcwd/macro_ForAllDebugChannels.h>
#include <libcwd/macro_ForAllDebugObjects.h>
#include <libcwd/private_environ.h>
#include <libcwd/class_rcfile.h>
#include <libcwd/attach_gdb.h>
#include <libcwd/demangle.h>

// Include the inline functions.
#include <libcwd/private_allocator.inl>		// Implementation of allocator_adaptor template.
#include <libcwd/class_channel.inl>		// Debug channels.
#include <libcwd/class_fatal_channel.inl>
#include <libcwd/class_continued_channel.inl>
#include <libcwd/class_always_channel.inl>
#include <libcwd/class_debug.inl>		// Debug objects (debug_ct).
#include <libcwd/class_debug_string.inl>	// Public member of debug_ct.
#include <libcwd/class_channel_set.inl>		// Used in macro Dout et al.
#include <libcwd/class_location.inl>

// Include optional features.
#if CWDEBUG_LOCATION				// --enable-location
#include <libcwd/bfd.h>
#endif
#include <libcwd/debugmalloc.h>			// --enable-alloc

#endif // CWDEBUG
#endif // LIBCWD_LIBRARIES_DEBUG_H

