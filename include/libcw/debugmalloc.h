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

/** \file libcw/debugmalloc.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_DEBUGMALLOC_H
#define LIBCW_DEBUGMALLOC_H

#ifndef LIBCW_DEBUG_H
#error "Don't include <libcw/debugmalloc.h> directly, include the appropriate \"debug.h\" instead."
#endif

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif

#if CWDEBUG_ALLOC

#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t.
#endif
#ifndef LIBCW_CLASS_ALLOC_H
#include <libcw/class_alloc.h>
#endif
#ifndef LIBCW_LOCKABLE_AUTO_PTR_H
#include <libcw/lockable_auto_ptr.h>
#endif
#ifndef LIBCW_PRIVATE_SET_ALLOC_CHECKING_H
#include <libcw/private_set_alloc_checking.h>
#endif
#ifndef LIBCW_CLASS_MEMBLK_TYPES_H
#include <libcw/class_memblk_types.h>
#endif
#if CWDEBUG_MARKER && !defined(LIBCW_CLASS_MARKER_H)
#include <libcw/class_marker.h>
#endif
#ifndef LIBCW_MACRO_ALLOCTAG_H
#include <libcw/macro_AllocTag.h>
#endif
#ifndef LIBCWD_CLASS_OOAM_FILTER_H
#include <libcw/class_ooam_filter.h>
#endif
#ifndef LIBCW_SYS_TIME_H
#define LIBCW_SYS_TIME_H
#include <sys/time.h>		// Needed for struct timeval.
#endif

namespace libcw {
  namespace debug {

//! Type of malloc_report.
enum malloc_report_nt
{
  /**
   * \brief Writing the current number of allocated bytes and blocks to an ostream.
   *
   * \sa mem_size(void)
   *  \n mem_blocks(void)
   *
   * <b>Example:</b>
   *
   * \code
   * Dout(dc::malloc, malloc_report << '.');
   * \endcode
   *
   * will output something like
   *
   * \exampleoutput <PRE>
   * MALLOC: Allocated 4350 bytes in 7 blocks.</PRE>
   * \endexampleoutput
   */
  malloc_report
};
extern std::ostream& operator<<(std::ostream&, malloc_report_nt);

// Accessors:

extern size_t mem_size(void);
extern unsigned long mem_blocks(void);
extern alloc_ct const* find_alloc(void const* ptr);
extern bool test_delete(void const* ptr);

// Manipulators:
extern void make_invisible(void const* ptr);
extern void make_all_allocations_invisible_except(void const* ptr);
#if CWDEBUG_MARKER
extern void move_outside(marker_ct*, void const* ptr);
#endif

// Undocumented (libcw `internal' function)
extern void init_debugmalloc(void);

  } // namespace debug
} // namespace libcw

#else // !CWDEBUG_ALLOC

namespace libcw {
  namespace debug {

__inline__ void make_invisible(void const*) { } 
__inline__ void make_all_allocations_invisible_except(void const*) { }

  } // namespace debug
} // namespace libcw

#endif // !CWDEBUG_ALLOC

#if CWDEBUG_DEBUGM
#define LIBCWD_DEBUGM_CERR(x) DEBUGDEBUG_CERR(x)
#else
#define LIBCWD_DEBUGM_CERR(x)
#endif

namespace libcw {
  namespace debug {

#if CWDEBUG_ALLOC
extern void list_allocations_on(debug_ct& debug_object, ooam_filter_ct const& format);
extern void list_allocations_on(debug_ct& debug_object);
#else // !CWDEBUG_ALLOC
__inline__ void list_allocations_on(debug_ct&) { }
__inline__ void list_allocations_on(debug_ct&, ooam_filter_ct const&) { }
#endif // !CWDEBUG_ALLOC

  } // namespace debug
} // namespace libcw

#ifndef LIBCWD_DEBUGMALLOC_INTERNAL
#if CWDEBUG_ALLOC

#ifndef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
// Ugh, use kludge.
#include <cstdlib>	// Make sure the prototypes for malloc et al are declared
			// before defining the macros!
#define malloc __libcwd_malloc
#define calloc __libcwd_calloc
#define realloc __libcwd_realloc
#define free __libcwd_free
#endif

// Use external linkage to catch ALL calls to all malloc/calloc/realloc/free functions,
// also those that are done in libc, or any other shared library that might be linked.
// [ Note: if LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC wasn't defined, then these are the prototypes
// for __libcwd_malloc et al of course.  We still use external "C" linkage in that case
// in order to avoid a collision with possibily later included prototypes for malloc. ]
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
extern "C" void* malloc(size_t size) throw();
extern "C" void* calloc(size_t nmemb, size_t size) throw();
extern "C" void* realloc(void* ptr, size_t size) throw();
#else
extern "C" void* malloc(size_t size) throw() __attribute__((__malloc__));
extern "C" void* calloc(size_t nmemb, size_t size) throw() __attribute__((__malloc__));
extern "C" void* realloc(void* ptr, size_t size) throw() __attribute__((__malloc__));
#endif
extern "C" void  free(void* ptr) throw();

#ifndef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
// Use same kludge for other libc functions that return malloc-ed pointers.
#define strdup __libcwd_strdup
#ifdef HAVE_WMEMCPY
#define wcsdup __libcwd_wcsdup
#endif

__inline__
char*
__libcwd_strdup(char const* str)
{
  size_t size = strlen(str) + 1;
  char* p = (char*)malloc(size);
  if (p)
  {
    memcpy(p, str, size);
    AllocTag(p, "strdup()");
  }
  return p;
}

#ifdef HAVE_WMEMCPY
extern "C" {
  size_t wcslen(wchar_t const*);
  wchar_t* wmemcpy(wchar_t*, wchar_t const*, size_t);
}

__inline__
wchar_t*
__libcwd_wcsdup(wchar_t const* str)
{
  size_t size = wcslen(str) + 1;
  wchar_t* p = (wchar_t*)malloc(size * sizeof(wchar_t));
  if (p)
  {
    wmemcpy(p, str, size);
    AllocTag(p, "wcsdup()");
  }
  return p;
}
#endif // HAVE_WMEMCPY
#endif // !LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC

#endif // CWDEBUG_ALLOC
#endif // !DEBUG_INTERNAL

#endif // LIBCW_DEBUGMALLOC_H
