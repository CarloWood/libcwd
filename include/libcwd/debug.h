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

/** \file libcwd/debug.h
 *
 * \brief This is the main header file of libcwd.
 *
 * Don't include this header file directly.&nbsp;
 * Instead use a \link chapter_custom_debug_h custom debug.h \endlink header file that includes this file,
 * that will allow others to compile your application without having libcwd installed.
 */

#ifndef LIBCWD_DEBUG_H
#define LIBCWD_DEBUG_H

#ifndef LIBCWD_SYS_H
#error "You need to #include "sys.h" at the top of every source file (which in turn should #include <libcwd/sys.h>)."
#endif

#ifndef CWDEBUG

// If you run into this error then you included <libcwd/debug.h> (or any other libcwd header file)
// while the macro CWDEBUG was not defined.  Doing so would cause the compilation of your
// application to fail on machines that do not have libcwd installed.  Instead you should use:
// #include "debug.h"
// and add a file debug.h to your applications distribution.  Please see the the example-project
// that comes with the source code of libcwd (or is included in the documentation that comes with
// the rpm (ie: /usr/doc/libcwd-1.0/example-project) for a description of the content of "debug.h".
// Note1: CWDEBUG should be defined on the compiler commandline, for example: g++ -DCWDEBUG ...
#error "You are including <libcwd/debug.h> while CWDEBUG is not defined.  See the comments in this header file for more information."

#else // CWDEBUG (normal usage of this file):

#include <libcwd/config.h>

//===================================================================================================
// The global debug channels used by libcwd.
//

#include <libcwd/class_channel.h>
#include <libcwd/class_fatal_channel.h>
#include <libcwd/class_continued_channel.h>
#include <libcwd/class_always_channel.h>

#ifndef DEBUGCHANNELS
/**
 * \brief The namespace containing the current %debug %channels (dc) namespace.
 *
 * This macro is internally used by libcwd macros to include the chosen set of %debug %channels.&nbsp;
 * For details please read section \ref chapter_custom_debug_h.
 */
#define DEBUGCHANNELS ::libcwd::channels
#endif

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

// For use in library header files: do not redefine these!
// Developers of libraries are recommended to define their own macro names.
#define __Debug(x) \
    LibcwDebug(::libcwd::channels, x)
#define __Dout(cntrl, data) \
    LibcwDout(::libcwd::channels, ::libcwd::libcw_do, cntrl, data)
#define __DoutFatal(cntrl, data) \
    LibcwDoutFatal(::libcwd::channels, ::libcwd::libcw_do, cntrl, data)

// For use in applications
/** \def Debug
 *
 * \brief Encapsulation macro for general debugging code.
 *
 * The parameter of this macro can be arbitrary code that will be eliminated
 * from the application when the macro CWDEBUG is \em not defined.
 *
 * It uses the namespaces \ref DEBUGCHANNELS and libcwd, making it unnecessary to
 * use the the full scopes for debug channels and utility functions provided by
 * libcwd.
 *
 * <b>Examples:</b>
 *
 * \code
 * Debug( check_configuration() );			// Configuration consistency check.
 * Debug( dc::notice.on() );				// Switch debug channel NOTICE on.
 * Debug( libcw_do.on() );				// Turn all debugging temporally off.
 * Debug( list_channels_on(libcw_do) );			// List all debug channels.
 * Debug( make_all_allocations_invisible_except(NULL) );	// Hide all allocations so far.
 * Debug( list_allocations_on(libcw_do) );		// List all allocations.
 * Debug( libcw_do.set_ostream(&std::cout) );		// Use std::cout as debug output stream.
 * Debug( libcw_do.set_ostream(&std::cout, &mutex) );	// use `mutex' as lock for std::cout.
 * Debug( libcw_do.inc_indent(4) );			// Increment indentation by 4 spaces.
 * Debug( libcw_do.get_ostream()->flush() );		// Flush the current debug output stream.
 * \endcode
 */
#define Debug(x) \
    LibcwDebug(DEBUGCHANNELS, x)

/** \def Dout
 *
 * \brief Macro for writing %debug output.
 *
 * This macro is used for writing %debug output to the default %debug object
 * \link libcwd::libcw_do libcw_do \endlink.&nbsp;
 * No code is generated when the macro CWDEBUG is not defined, in that case the macro
 * Dout is replaced by white space.
 *
 * The macro \ref Dout uses libcwds debug object \link libcwd::libcw_do libcw_do \endlink.&nbsp;
 * You will have to define your own macro when you want to use a second debug object.&nbsp;
 * Read chapter \ref page_why_macro for an explanation of why a macro was used instead of an inline function.
 *
 * \sa \ref group_control_flags
 *  \n \ref group_default_dc
 *  \n \link chapter_custom_debug_h Defining your own debug channels \endlink
 *  \n \link chapter_custom_do Defining your own debug objects \endlink
 *  \n \ref chapter_nesting "Nested debug calls"
 *
 * <b>Examples:</b>
 *
 * \code
 * Dout(dc::notice, "Hello World");
 * Dout(dc::malloc|dc::warning, "Out of memory in function " << func_name);
 * Dout(dc::notice|blank_label_cf, "The content of the object is: " << std::hex << obj);
 * \endcode
 */
#define Dout(cntrl, data) \
    LibcwDout(DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, data)

/**
 * \def DoutFatal
 * \ingroup group_fatal_output
 *
 * \brief Macro for writing fatal %debug output to the default %debug object
 * \link libcwd::libcw_do libcw_do \endlink.
 */
#define DoutFatal(cntrl, data) \
    LibcwDoutFatal(DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, data)

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
#ifndef HIDE_FROM_DOXYGEN
namespace libcwd_inserters {
  using libcwd::operator<<;
} // namespace libcwd_inserters
using namespace libcwd_inserters;
#endif

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
#include <libcwd/class_debug_string.inl>		// Public member of debug_ct.
#include <libcwd/class_channel_set.inl>		// Used in macro Dout et al.

// Include optional features.
#if CWDEBUG_LOCATION				// --enable-location
#include <libcwd/bfd.h>
#endif
#include <libcwd/debugmalloc.h>			// --enable-alloc

// Finally, in order for Dout() to be usable, we need this.
#ifndef LIBCW_IOSTREAM
#define LIBCW_IOSTREAM
#include <iostream>
#endif

#endif // CWDEBUG
#endif // LIBCWD_DEBUG_H
