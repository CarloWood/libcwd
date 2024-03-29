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

/** \file libcwd/debugmalloc.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_DEBUGMALLOC_H
#define LIBCWD_DEBUGMALLOC_H

#ifndef LIBCWD_LIBRARIES_DEBUG_H
#error "Don't include <libcwd/debugmalloc.h> directly, include the appropriate \"debug.h\" instead."
#endif

#ifndef LIBCWD_CONFIG_H
#include "libcwd/config.h"
#endif
#ifndef LIBCWD_MACRO_ALLOCTAG_H
#include "macro_AllocTag.h"
#endif

#if CWDEBUG_ALLOC

#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t.
#endif
#ifndef LIBCWD_CLASS_ALLOC_H
#include "class_alloc.h"
#endif
#ifndef LIBCW_LOCKABLE_AUTO_PTR_H
#include "lockable_auto_ptr.h"
#endif
#ifndef LIBCWD_PRIVATE_SET_ALLOC_CHECKING_H
#include "private_set_alloc_checking.h"
#endif
#ifndef LIBCWD_ENUM_MEMBLK_TYPES_H
#include "enum_memblk_types.h"
#endif
#if CWDEBUG_MARKER && !defined(LIBCWD_CLASS_MARKER_H)
#include "class_marker.h"
#endif
#ifndef LIBCWD_CLASS_ALLOC_FILTER_H
#include "class_alloc_filter.h"
#endif
#ifndef LIBCW_SYS_TIME_H
#define LIBCW_SYS_TIME_H
#include <sys/time.h>		// Needed for struct timeval.
#endif

namespace libcwd {

//! Type of malloc_report.
enum malloc_report_nt
{
  /**
   * \brief Writing the current number of allocated bytes and blocks to an ostream.
   *
   * \sa mem_size()
   *  \n mem_blocks()
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

// Backtrace hook:
int const max_frames = 16;
extern void (*backtrace_hook)(void** buffer, int frames LIBCWD_COMMA_TSD_PARAM);

#ifndef LIBCWD_DOXYGEN
extern std::ostream& operator<<(std::ostream&, malloc_report_nt);
#endif

// Accessors:

extern size_t mem_size();
extern unsigned long mem_blocks();
extern alloc_ct const* find_alloc(void const* ptr);
extern bool test_delete(void const* ptr);

// Manipulators:
extern void make_invisible(void const* ptr);
extern void make_all_allocations_invisible_except(void const* ptr);
extern void make_exit_function_list_invisible();
/**
 * \brief Make all future allocations invisible.
 * \ingroup group_invisible
 *
 * All following allocations are made invisible; they won't show up
 * anymore in the \ref group_overview "overview of allocated memory".
 *
 * \sa \ref group_invisible
 * \sa \ref group_overview
 *
 * <b>Example:</b>
 *
 * \code
 * Debug(set_invisible_on());
 * gtk_init(&argc, &argv);
 * Debug(set_invisible_off());
 * \endcode
 *
 * Note: In the threaded case, this isn't blazing fast (it is in the non-threaded case).
 *       You shouldn't use it inside tight loops when using libcwd_r.
 */
inline void set_invisible_on() { LIBCWD_TSD_DECLARATION; _private_::set_invisible_on(LIBCWD_TSD); }
/**
 * \brief Cancel a call to set_invisible_on.
 * \ingroup group_invisible
 *
 * See \ref set_invisible_on
 */
inline void set_invisible_off() { LIBCWD_TSD_DECLARATION; _private_::set_invisible_off(LIBCWD_TSD); }
#if CWDEBUG_MARKER
extern void move_outside(marker_ct*, void const* ptr);
#endif

// Undocumented (libcwd `internal' function)
extern void init_debugmalloc();

} // namespace libcwd

#else // !CWDEBUG_ALLOC

namespace libcwd {

inline void make_invisible(void const*) { }
inline void make_all_allocations_invisible_except(void const*) { }
inline void set_invisible_on() { }
inline void set_invisible_off() { }

} // namespace libcwd

#endif // !CWDEBUG_ALLOC

#if CWDEBUG_DEBUGM
#define LIBCWD_DEBUGM_CERR(x) DEBUGDEBUG_CERR(x)
#define LIBCWD_DEBUGM_ASSERT(expr) do { if (!(expr)) { FATALDEBUGDEBUG_CERR("CWDEBUG_DEBUGM: " __FILE__ ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << ": Assertion`" << LIBCWD_STRING(expr) << "' failed."); core_dump(); } } while(0)
#else
#define LIBCWD_DEBUGM_CERR(x) do { } while(0)
#define LIBCWD_DEBUGM_ASSERT(x) do { } while(0)
#endif

namespace libcwd {

#if CWDEBUG_ALLOC
extern unsigned long list_allocations_on(debug_ct& debug_object, alloc_filter_ct const& format);
extern unsigned long list_allocations_on(debug_ct& debug_object);
#else // !CWDEBUG_ALLOC
inline void list_allocations_on(debug_ct&) { }
#endif // !CWDEBUG_ALLOC

} // namespace libcwd

#ifndef LIBCWD_DEBUGMALLOC_INTERNAL
#if CWDEBUG_ALLOC

#ifndef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
// Ugh, use macro kludge.
#include <cstdlib>	// Make sure the prototypes for malloc et al are declared
			// before defining the macros!
#define malloc __libcwd_malloc
#define calloc __libcwd_calloc
#define realloc __libcwd_realloc
#define free __libcwd_free
#endif

#ifndef LIBCWD_HAVE_DLOPEN
// Use macro kludge for these (too):
#if defined(LIBCWD_HAVE_POSIX_MEMALIGN) || defined(LIBCWD_HAVE_ALIGNED_ALLOC)
// Include this header before defining the macro 'posix_memalign' and/or 'aligned_alloc'.
#include <cstdlib>
#endif
#ifdef LIBCWD_HAVE_POSIX_MEMALIGN
#define posix_memalign __libcwd_posix_memalign
#endif
#ifdef LIBCWD_HAVE_ALIGNED_ALLOC
#define aligned_alloc __libcwd_aligned_alloc
#endif
// Include this header before defining the macro 'memalign' or 'valloc'.
#if defined(HAVE_MALLOC_H) && (defined(HAVE_MEMALIGN) || defined(HAVE_VALLOC))
#include <malloc.h>
#endif
#if defined(HAVE_UNISTD_H) && defined(HAVE_VALLOC)
#include <unistd.h>		// This is what is needed for valloc(3) on FreeBSD. Also needed for sysconf.
#endif
#ifdef LIBCWD_HAVE_MEMALIGN
#define memalign __libcwd_memalign
#endif
#ifdef LIBCWD_HAVE_VALLOC
#define valloc __libcwd_valloc
#endif
#endif // !LIBCWD_HAVE_DLOPEN

// Use external linkage to catch ALL calls to all malloc/calloc/realloc/free functions,
// also those that are done in libc, or any other shared library that might be linked.
// [ Note: if LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC wasn't defined, then these are the prototypes
// for __libcwd_malloc et al of course.  We still use external "C" linkage in that case
// in order to avoid a collision with possibily later included prototypes for malloc. ]
extern "C" void* malloc(size_t size) throw() __attribute__((__malloc__));
extern "C" void* calloc(size_t nmemb, size_t size) throw() __attribute__((__malloc__));
extern "C" void* realloc(void* ptr, size_t size) throw() __attribute__((__malloc__));
extern "C" void  free(void* ptr) throw();
#ifdef LIBCWD_HAVE_POSIX_MEMALIGN
#ifdef posix_memalign // Due to declaration conflicts with cstdlib, lets not define this when this isn't our macro.
extern "C" int posix_memalign(void** memptr, size_t alignment, size_t size) throw() __attribute__((__nonnull__(1))) __wur;
#endif
#endif
#ifdef LIBCWD_HAVE_ALIGNED_ALLOC
extern "C" void* aligned_alloc(size_t alignment, size_t size) throw() __attribute__((__malloc__)) __attribute__((__alloc_size__(2))) __wur;
#endif
#ifdef LIBCWD_HAVE_VALLOC
extern "C" void* valloc(size_t size) throw() __attribute__((__malloc__)) __wur;
#endif
#ifdef LIBCWD_HAVE_MEMALIGN
extern "C" void* memalign(size_t boundary, size_t size) throw() __attribute__((__malloc__));
#endif

#ifndef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
// Use same kludge for other libc functions that return malloc-ed pointers.
#define strdup __libcwd_strdup
#ifdef HAVE_WMEMCPY
#define wcsdup __libcwd_wcsdup
#endif

inline
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

inline
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
#endif // !LIBCWD_DEBUGMALLOC_INTERNAL

#endif // LIBCWD_DEBUGMALLOC_H
