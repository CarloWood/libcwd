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

/** \file libcwd/debug.h
 *
 * \brief This is the main header file of libcwd.
 *
 * Don't include this header file directly.&nbsp;
 * Instead use a \link chapter_custom_debug_h custom debug.h \endlink header file that includes this file,
 * that will allow others to compile your application without having libcwd installed.
 */

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

#ifndef LIBCWD_SYS_H
#error "You need to #include "sys.h" at the top of every source file (which in turn should #include "sys.h")."
#endif

#if defined(LIBCWD_DEFAULT_DEBUGCHANNELS) && defined(DEBUGCHANNELS)
// If you run into this error then you included <libcwd/debug.h> (or any other libcwd header file)
// without defining DEBUGCHANNELS, and later (the moment of this error) you included it with
// DEBUGCHANNELS defined.
//
// The most likely reason for this is that you include <libcwd/debug.h> in one of your headers
// instead of using #include "debug.h", and then included that header file before including
// "debug.h" in the .cpp file.
// End-applications should #include "debug.h" everywhere.  See the the example-project that comes
// with the source code of libcwd (or is included in the documentation that comes with the rpm
// (ie: /usr/doc/libcwd-1.0/example-project) for a description of the content of "debug.h".
// More information for end-application users can be found on
// http://carlowood.github.io/libcwd/reference-manual/preparation.html
//
// Third-party libraries should never include <libcwd/debug.h> but also not "debug.h".  They
// should include <libcwd/libraries_debug.h> (and not use Dout et al in their headers).  If you
// are using a library that did include <libcwd/debug.h> then please report this bug the author
// of that library.  You can workaround it for now by including "debug.h" before including the
// header of that library.
// More information for library authors that use libcwd can be found on
// http://carlowood.github.io/libcwd/reference-manual/group__chapter__custom__debug__h.html
#error "DEBUGCHANNELS is defined while previously it was not defined.  See the comments in this header file for more information."
#endif

#endif // CWDEBUG

#ifndef LIBCWD_DEBUG_H
#define LIBCWD_DEBUG_H

#ifdef CWDEBUG

// The following header is also needed for end-applications, despite its name.
#include "libraries_debug.h"

#ifndef LIBCWD_DOXYGEN

// The real code
#ifdef DEBUGCHANNELS
#define LIBCWD_DEBUGCHANNELS DEBUGCHANNELS
#else
#define LIBCWD_DEBUGCHANNELS libcwd::channels
#define LIBCWD_DEFAULT_DEBUGCHANNELS
#endif

#else // LIBCWD_DOXYGEN

// This is only here for the documentation.  The user will define DEBUGCHANNELS, not LIBCWD_DEBUGCHANNELS.
/**
 * \brief The namespace containing the current %debug %channels (dc) namespace.
 *
 * This macro is internally used by libcwd macros to include the chosen set of %debug %channels.&nbsp;
 * For details please read section \ref chapter_custom_debug_h.
 */
#define DEBUGCHANNELS
#endif // LIBCWD_DOXYGEN

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
#define Debug(STATEMENTS...) \
    LibcwDebug(LIBCWD_DEBUGCHANNELS, STATEMENTS)

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
    LibcwDout(LIBCWD_DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, data)

/**
 * \def DoutFatal
 * \ingroup group_fatal_output
 *
 * \brief Macro for writing fatal %debug output to the default %debug object
 * \link libcwd::libcw_do libcw_do \endlink.
 */
#define DoutFatal(cntrl, data) \
    LibcwDoutFatal(LIBCWD_DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, data)

/**
 * \def ForAllDebugChannels
 * \ingroup group_special
 *
 * \brief Looping over all debug channels.
 *
 * The macro \c ForAllDebugChannels allows you to run over all %debug %channels.
 *
 * For example,
 *
 * \code
 * ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
 * \endcode
 *
 * which turns all %channels on.&nbsp;
 * And
 *
 * \code
 * ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() );
 * \endcode
 *
 * which turns all channels off.
 */
#define ForAllDebugChannels(STATEMENT...) \
    LibcwdForAllDebugChannels(LIBCWD_DEBUGCHANNELS, STATEMENT)

/**
 * \def ForAllDebugObjects
 * \ingroup group_special
 *
 * \brief Looping over all debug objects.
 *
 * The macro \c ForAllDebugObjects allows you to run over all %debug objects.
 *
 * For example,
 *
 * \code
 * ForAllDebugObjects( debugObject.set_ostream(&std::cerr, &cerr_mutex) );
 * \endcode
 *
 * would set the output stream of all %debug objects to <CODE>std::cerr</CODE>.
 */
#define ForAllDebugObjects(STATEMENT...) \
    LibcwdForAllDebugObjects(LIBCWD_DEBUGCHANNELS, STATEMENT)

// Finally, in order for Dout() to be usable, we need this.
#ifndef LIBCW_IOSTREAM
#define LIBCW_IOSTREAM
#include <iostream>
#endif

#endif // CWDEBUG
#endif // LIBCWD_DEBUG_H
