// $Header$
//
// Copyright (C) 2000 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcw/macro_Libcwd_macros.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_LIBCWD_MACROS_H
#define LIBCW_LIBCWD_MACROS_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif

//===================================================================================================
// Macro LibcwDebug
//

/**
 * \brief General debug macro.
 *
 * This macro allows one to implement a customized &quot;\ref Debug&quot;,
 * using a custom %debug channel namespace.
 *
 * \sa \ref chapter_custom_debug_h
 */
#define LibcwDebug(dc_namespace, x)		\
	do {					\
	  using namespace ::libcw::debug;	\
	  using namespace dc_namespace;		\
	  {					\
	    x;					\
	  }					\
	} while(0)

//===================================================================================================
// Macro LibcwDout
//

// This is debugging libcwd itself.
#ifndef DEBUGDEBUGLIBCWDOUTMARKER
#ifdef DEBUGDEBUGOUTPUT
extern "C" ssize_t write(int fd, const void *buf, size_t count) throw();
#define LIBCWD_STR1(x) #x
#define LIBCWD_STR2(x) LIBCWD_STR1(x)
#define LIBCWD_STR3 "LibcwDout at " __FILE__ ":" LIBCWD_STR2(__LINE__) "\n"
#define DEBUGDEBUGLIBCWDOUTMARKER ::write(2, LIBCWD_STR3, sizeof(LIBCWD_STR3) - 1);
#else // !DEBUGDEBUGOUTPUT
#define DEBUGDEBUGLIBCWDOUTMARKER
#endif // !DEBUGDEBUGOUTPUT
#endif // !DEBUGDEBUGLIBCWDOUTMARKER

/**
 * \brief General debug output macro.
 *
 * This macro allows one to implement a customized &quot;\ref Dout&quot;, using
 * a custom \ref group_debug_object "debug object" and/or channel namespace.
 *
 * \sa \ref chapter_custom_debug_h
 */
#define LibcwDout( dc_namespace, debug_obj, cntrl, data )	\
    LibcwDoutScopeBegin(dc_namespace, debug_obj, cntrl)		\
    LibcwDoutStream << data;					\
    LibcwDoutScopeEnd

#define LibcwDoutScopeBegin( dc_namespace, debug_obj, cntrl )									\
  do																\
  {																\
    DEBUGDEBUGLIBCWDOUTMARKER													\
    LIBCWD_TSD_DECLARATION													\
    if (LIBCWD_DO_TSD_MEMBER(debug_obj, _off) < 0)										\
    {																\
      using namespace ::libcw::debug;												\
      ::libcw::debug::channel_set_bootstrap_st __libcwd_channel_set(LIBCWD_DO_TSD(debug_obj) LIBCWD_COMMA_TSD);			\
      bool on;															\
      {																\
        using namespace dc_namespace;												\
	on = (__libcwd_channel_set|cntrl).on;											\
      }																\
      if (on)															\
      {																\
	::libcw::debug::debug_ct& __libcwd_debug_object(debug_obj);								\
	LIBCWD_DO_TSD(__libcwd_debug_object).start(__libcwd_debug_object, __libcwd_channel_set LIBCWD_COMMA_TSD);

// Note: LibcwDoutStream is *not* equal to the ostream that was set with set_ostream.  It is a temporary stringstream.
#define LibcwDoutStream														\
        (*LIBCWD_DO_TSD_MEMBER(__libcwd_debug_object, current_oss))

#define LibcwDoutScopeEnd													\
	LIBCWD_DO_TSD(__libcwd_debug_object).finish(__libcwd_debug_object, __libcwd_channel_set LIBCWD_COMMA_TSD);		\
      }																\
    }																\
  } while(0)

//===================================================================================================
// Macro LibcwDoutFatal
//

// This is debugging libcwd itself.
#ifndef DEBUGDEBUGLIBCWDOUTFATALMARKER
#ifdef DEBUGDEBUGOUTPUT
#define LIBCWD_STR4 "LibcwDoutFatal at " __FILE__ ":" LIBCWD_STR2(__LINE__) "\n"
#define DEBUGDEBUGLIBCWDOUTFATALMARKER ::write(2, LIBCWD_STR4, sizeof(LIBCWD_STR4) - 1);
#else
#define DEBUGDEBUGLIBCWDOUTFATALMARKER
#endif
#endif // !DEBUGDEBUGLIBCWDOUTFATALMARKER

/**
 * \brief General fatal debug output macro.
 *
 * This macro allows one to implement a customized &quot;\ref DoutFatal&quot;, using
 * a custom \ref group_debug_object "debug object" and/or channel namespace.
 *
 * \sa \ref chapter_custom_debug_h
 */
#define LibcwDoutFatal( dc_namespace, debug_obj, cntrl, data )	\
    LibcwDoutFatalScopeBegin(dc_namespace, debug_obj, cntrl)	\
    LibcwDoutFatalStream << data;				\
    LibcwDoutFatalScopeEnd

#define LibcwDoutFatalScopeBegin( dc_namespace, debug_obj, cntrl )								\
  do																\
  {																\
    DEBUGDEBUGLIBCWDOUTFATALMARKER												\
    LIBCWD_TSD_DECLARATION													\
    using namespace ::libcw::debug;												\
    ::libcw::debug::channel_set_bootstrap_st __libcwd_channel_set(LIBCWD_DO_TSD(debug_obj) LIBCWD_COMMA_TSD);			\
    {																\
      using namespace dc_namespace;												\
      __libcwd_channel_set&cntrl;												\
    }																\
    ::libcw::debug::debug_ct& __libcwd_debug_object(debug_obj);									\
    LIBCWD_DO_TSD(__libcwd_debug_object).start(__libcwd_debug_object, __libcwd_channel_set LIBCWD_COMMA_TSD);

#define LibcwDoutFatalStream LibcwDoutStream

#define LibcwDoutFatalScopeEnd													\
    LIBCWD_DO_TSD(__libcwd_debug_object).fatal_finish(__libcwd_debug_object, __libcwd_channel_set LIBCWD_COMMA_TSD);		\
  } while(0)

#endif // LIBCW_LIBCWD_MACROS_H
