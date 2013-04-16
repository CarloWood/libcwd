// $Header$
//
// \file debugmalloc.cc
// This file contains the memory allocations related debugging code.
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

//=============================================================================
//
// This file can be viewed at as an object, with __libcwd_tsd containing its
// private attributes.  The public 'manipulator' interface are the memory
// allocation functions:
//
// - malloc, calloc, realloc free (and register_external_allocation if
//   the macro kludge is being used (if we don't define these functions
//   with external linkage)).
// - new and delete.
//
// These functions maintain an internal data structure that allows for
// - checking if calls to free and realloc have a valid pointer.
// - finding allocated-memory statistics.
// - finding the memory block given an arbitrary pointer that points inside it.
//
// We maintain two main data 'trees' per thread:
// 1) A red/black tree (using STL's map class) which contains all
//    allocated memory blocks, except:
//    - those allocated in this file while `internal' is true.
//    - those allocated after a call to set_alloc_checking_off()
//      (and before the corresponding set_alloc_checking_on()).
// 2) A tree of objects that allows us to see which object allocated what.
//
// The first tree is used for two main things:
// - Checking whether or not a call to free(3) or realloc(3) contains a valid
//   pointer, and
// - Allowing us to quickly find back which allocated block belongs to any
//   arbitrary pointer (or interval if needed).
//   This allows us for example to find back the start of an instance of an
//   object, inside a method of a base class of this object.
// This tree also contains type of allocation (new/new[]/malloc/realloc/valloc/memalign/posix_memalign).
//
// The second tree, existing of linked dm_alloc_ct objects, stores the data related
// to the memory block itself, currently its start, size and - when provided by the
// user by means of AllocTag() - a type_info_ct of the returned pointer and a description.
// The second tree allows us to group allocations in a hierarchy: It's needed
// for the function that shows the current allocations, or the allocations
// made and/or left over since a call to some function or since a 'marker'
// was set.  It is also needed for memory leak detection (which just means
// that you set a marker somewhere and expect no extra allocated memory
// performed by the current thread at the point where you delete it).
//
// The memory management of these objects is as follows:
//
// new, new[], malloc and calloc create a memblk_key_ct and memblk_info_ct which
// are stored as a std::pair<memblk_key_ct const, memblk_info_ct> in the STL map.
//
// Creating the memblk_info_ct creates a related dm_alloc_ct.
//
// delete and free erase the std::pair<memblk_key_ct const, memblk_info_ct> from
// the map and as such destroy the memblk_key_ct and the memblk_info_ct.
//
// The memblk_info_ct has a lockable_auto_ptr to the allocated dm_alloc_ct: when
// the memblk_info_ct is destructed, so is the lockable_auto_ptr.  When the
// lockable_auto_ptr still is the owner of the dm_alloc_ct object, then the
// dm_alloc_ct is destructed too.
//
// Creation and destruction of the dm_alloc_ct takes care of its own insertion
// and deletion in the hierarchy automatically, including updating of total
// allocated memory size, number of allocated blocks and the current list that
// newly allocated blocks must be added to.
// 
// Two corresponding memblk_info_ct and dm_alloc_ct objects can be 'decoupled'
// in two ways:
// - The memblk_info_ct is erased but the corresponding dm_alloc_ct still has
//   a list of its own.  In which case the dm_alloc_ct is deleted as soon as its
//   last list element is removed.
// - The memory block is made 'invisible' (with `make_invisible') which means
//   that the dm_alloc_ct is deleted without deleting the corresponding
//   memblk_info_ct.
//
// The current 'accessor' functions are:
//
// - alloc_ct const* find_alloc(void const* ptr);
//		Find `alloc_ct' of memory block that `ptr' points to, or
//		points inside of.  Returns NULL when no block was found.
// - bool test_delete(void const* ptr)
//		Returns true if `ptr' points to the start of an allocated
//		memory block.
// - size_t mem_size(void)
//		Returns the total ammount of allocated memory in bytes
//		(the sum all blocks shown by `list_allocations_on').
// - unsigned long mem_blocks(void)
//		Returns the total number of allocated memory blocks
//		(Those that are shown by `list_allocations_on').
// - std::ostream& operator<<(std::ostream& o, debugmalloc_report_ct)
//		Allows to write a memory allocation report to std::ostream `o'.
// - unsigned long list_allocations_on(debug_ct& debug_object, alloc_filter_ct const& filter)
//		Prints out all visible allocated memory blocks and labels.
//
// The current 'manipulator' functions are:
//
// - void move_outside(marker_ct* marker, void const* ptr)
//		Move `ptr' outside the list of `marker'.
// - void make_invisible(void const* ptr)
//		Makes `ptr' invisible by deleting the corresponding alloc_ct
//		object.  `find_alloc' will return NULL for this allocation
//		and the allocation will not show up in memory allocation
//		overviews.
// - void make_all_allocations_invisible_except(void* ptr)
//		For internal use.
//

#define LIBCWD_DEBUGMALLOC_INTERNAL
#include "sys.h"
#include <libcwd/config.h>

#if CWDEBUG_ALLOC || defined(LIBCWD_DOXYGEN)

#include <cstring>
#include <string>
#include <map>
#include <new>
#include <csignal>
#include <inttypes.h>
#ifdef HAVE_POSIX_MEMALIGN
#include <cerrno>	// For EINVAL and ENOMEM
#endif
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif
#include <execinfo.h>	// For backtrace().

#if LIBCWD_THREAD_SAFE

// We got the C++ config earlier by <bits/stl_alloc.h>.

#ifdef __STL_THREADS
#include <bits/stl_threads.h>   // For interface to _STL_mutex_lock (node allocator lock)
#if defined(__STL_GTHREADS)
#include "bits/gthr.h"
#else
#error You have an unsupported configuraton of gcc. Please tell that you dont have gthreads along with the output of gcc -v to libcwd@alinoe.com.
#endif // __STL_GTHREADS


#endif // __STL_THREADS
#endif // LIBCWD_THREAD_SAFE
#include <iostream>
#include <iomanip>
#include <sys/time.h>		// Needed for gettimeofday(2)
#include "cwd_debug.h"
#include "ios_base_Init.h"
#include "match.h"
#include "zone.h"
#include <libcwd/cwprint.h>
#include <libcwd/bfd.h>
#ifdef HAVE_POSIX_MEMALIGN
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstdlib>
#endif
#if defined(HAVE_MALLOC_H) && (defined(HAVE_MEMALIGN) || defined(HAVE_VALLOC))
#include <malloc.h>
#endif
#if defined(HAVE_UNISTD_H) && defined(HAVE_VALLOC)
#include <unistd.h>		// This is what is needed for valloc(3) on FreeBSD. Also needed for sysconf.
#endif

#if LIBCWD_THREAD_SAFE
#if CWDEBUG_DEBUGT
#define UNSET_TARGETHREAD __libcwd_tsd.target_thread = NULL
#else
#define UNSET_TARGETHREAD
#endif
#include <libcwd/private_mutex.inl>
using libcwd::_private_::rwlock_tct;
using libcwd::_private_::mutex_tct;
using libcwd::_private_::mutex_ct;
using libcwd::_private_::threadlist_t;
using libcwd::_private_::threadlist;
using libcwd::_private_::location_cache_instance;
using libcwd::_private_::list_allocations_instance;
using libcwd::_private_::threadlist_instance;
using libcwd::_private_::dlclose_instance;
using libcwd::_private_::backtrace_instance;

// We can't use a read/write lock here because that leads to a deadlock.
// rwlocks have to use condition variables or semaphores and both try to get a
// (libpthread internal) self-lock that is already set by libthread when it calls
// free() in order to destroy thread specific data 1st level arrays.
// Note that we use rwlock_tct<threadlist_instance>, but that is safe because
// the free() from __pthread_destroy_specifics (in the pthread library) will only
// be trying to free memory that was allocated (for glibc-2.2.4 at specific.c:109)
// by the same thread and thus will not attempt to acquire the rwlock.
#if !CWDEBUG_DEBUGT
#define ACQUIRE_WRITE_LOCK(tt)	do { __libcwd_tsd.target_thread = tt; __libcwd_tsd.target_thread->thread_mutex.lock(); } while(0)
#define RELEASE_WRITE_LOCK	do { __libcwd_tsd.target_thread->thread_mutex.unlock(); UNSET_TARGETHREAD; } while(0)
#define ACQUIRE_READ_LOCK(tt)	do { __libcwd_tsd.target_thread = tt; __libcwd_tsd.target_thread->thread_mutex.lock(); } while(0)
#define RELEASE_READ_LOCK	do { __libcwd_tsd.target_thread->thread_mutex.unlock(); UNSET_TARGETHREAD; } while(0)
#else
#define ACQUIRE_WRITE_LOCK(tt)	do { __libcwd_tsd.target_thread = tt; __libcwd_tsd.target_thread->thread_mutex.lock(__libcwd_tsd); } while(0)
#define RELEASE_WRITE_LOCK	do { __libcwd_tsd.target_thread->thread_mutex.unlock(__libcwd_tsd); UNSET_TARGETHREAD; } while(0)
#define ACQUIRE_READ_LOCK(tt)	do { __libcwd_tsd.target_thread = tt; __libcwd_tsd.target_thread->thread_mutex.lock(__libcwd_tsd); } while(0)
#define RELEASE_READ_LOCK	do { __libcwd_tsd.target_thread->thread_mutex.unlock(__libcwd_tsd); UNSET_TARGETHREAD; } while(0)
#endif
#define ACQUIRE_READ2WRITE_LOCK	do { } while(0)
#define ACQUIRE_WRITE2READ_LOCK do { } while(0)
// We can rwlock_tct here, because this lock is never used from free(),
// only from new, new[], malloc and realloc.
#define ACQUIRE_LC_WRITE_LOCK		rwlock_tct<location_cache_instance>::wrlock()
#define RELEASE_LC_WRITE_LOCK		rwlock_tct<location_cache_instance>::wrunlock()
#define ACQUIRE_LC_READ_LOCK		rwlock_tct<location_cache_instance>::rdlock()
#define RELEASE_LC_READ_LOCK		rwlock_tct<location_cache_instance>::rdunlock()
#define ACQUIRE_LC_READ2WRITE_LOCK	rwlock_tct<location_cache_instance>::rd2wrlock()
#define ACQUIRE_LC_WRITE2READ_LOCK	rwlock_tct<location_cache_instance>::wr2rdlock()
#define DLCLOSE_ACQUIRE_LOCK            mutex_tct<dlclose_instance>::lock()
#define DLCLOSE_RELEASE_LOCK            mutex_tct<dlclose_instance>::unlock()
#define BACKTRACE_ACQUIRE_LOCK		mutex_tct<backtrace_instance>::lock()
#define BACKTRACE_RELEASE_LOCK		mutex_tct<backtrace_instance>::unlock()
#else // !LIBCWD_THREAD_SAFE
#define ACQUIRE_WRITE_LOCK(tt)		do { } while(0)
#define RELEASE_WRITE_LOCK	 	do { } while(0)
#define ACQUIRE_READ_LOCK(tt)	 	do { } while(0)
#define RELEASE_READ_LOCK		do { } while(0)
#define ACQUIRE_READ2WRITE_LOCK		do { } while(0)
#define ACQUIRE_WRITE2READ_LOCK		do { } while(0)
#define ACQUIRE_LC_WRITE_LOCK		do { } while(0)
#define RELEASE_LC_WRITE_LOCK		do { } while(0)
#define ACQUIRE_LC_READ_LOCK		do { } while(0)
#define RELEASE_LC_READ_LOCK		do { } while(0)
#define ACQUIRE_LC_READ2WRITE_LOCK	do { } while(0)
#define ACQUIRE_LC_WRITE2READ_LOCK	do { } while(0)
#define DLCLOSE_ACQUIRE_LOCK		do { } while(0)
#define DLCLOSE_RELEASE_LOCK		do { } while(0)
#define BACKTRACE_ACQUIRE_LOCK		do { } while(0)
#define BACKTRACE_RELEASE_LOCK		do { } while(0)
#endif // !LIBCWD_THREAD_SAFE

#if CWDEBUG_LOCATION
#define LIBCWD_COMMA_LOCATION(x) , x
#else
#define LIBCWD_COMMA_LOCATION(x)
#endif

#ifdef LIBCWD_DOXYGEN
// Doxygen doesn't parse the define below.  On linux this evaluates to 0.
#define USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE 0
#else
#define USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE \
    (defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC) && defined(HAVE_DLOPEN) \
     && !defined(LIBCWD_HAVE__LIBC_MALLOC) && !defined(LIBCWD_HAVE___LIBC_MALLOC))
#endif

#ifdef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC

#define __libcwd_malloc malloc
#define __libcwd_calloc calloc
#define __libcwd_realloc realloc
#define __libcwd_free free
#define dc_malloc dc::malloc

// This only works with a patched valgrind (So not for you).
// Also change the macro in threading.cc.
#define VALGRIND 0

#if defined(LIBCWD_HAVE__LIBC_MALLOC) || defined(LIBCWD_HAVE___LIBC_MALLOC)
#if VALGRIND
#define __libc_malloc valgrind_malloc
#define __libc_calloc valgrind_calloc
#define __libc_realloc valgrind_realloc
#define __libc_free valgrind_free
#else // !VALGRIND
#ifdef LIBCWD_HAVE__LIBC_MALLOC
#define __libc_malloc _libc_malloc
#define __libc_calloc _libc_calloc
#define __libc_realloc _libc_realloc
#define __libc_free _libc_free
#endif // LIBCWD_HAVE__LIBC_MALLOC
#endif // !VALGRIND
#else // USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE
#ifndef HAVE_DLOPEN
#error "configure bug: macros are inconsistent"
#endif
#define __libc_malloc (*libc_malloc)
#define __libc_calloc (*libc_calloc)
#define __libc_realloc (*libc_realloc)
#define __libc_free (*libc_free)
namespace libcwd {
void* malloc_bootstrap1(size_t size);
void* calloc_bootstrap1(size_t nmemb, size_t size);
} // namespace libcwd
void* __libc_malloc(size_t size) = libcwd::malloc_bootstrap1;
void* __libc_calloc(size_t nmemb, size_t size) = libcwd::calloc_bootstrap1;
void* __libc_realloc(void* ptr, size_t size);
void __libc_free(void* ptr);
void (*libc_free_final)(void* ptr) = (void (*)(void*))0;
#endif // USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE

#else // !LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC

#define __libc_malloc malloc
#define __libc_calloc calloc
#define __libc_realloc realloc
#define __libc_free free
#define dc_malloc dc::__libcwd_malloc

#endif // !LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC

#ifdef HAVE_DLOPEN

#ifdef HAVE_POSIX_MEMALIGN
#define __libcwd_posix_memalign posix_memalign
#define __libc_posix_memalign (*libc_posix_memalign)
int __libc_posix_memalign(void** memptr, size_t alignment, size_t size);
#endif
#ifdef HAVE_MEMALIGN
#define __libcwd_memalign memalign
#define __libc_memalign (*libc_memalign)
void* __libc_memalign(size_t boundary, size_t size);
#endif
#ifdef HAVE_VALLOC
#define __libcwd_valloc valloc
#define __libc_valloc (*libc_valloc)
void* __libc_valloc(size_t size);
#endif

#else // !HAVE_DLOPEN

#ifdef HAVE_POSIX_MEMALIGN
#define __libc_posix_memalign posix_memalign
#endif
#ifdef HAVE_MEMALIGN
#define __libc_memalign memalign
#endif
#ifdef HAVE_VALLOC
#define __libc_valloc valloc
#endif

#endif // !HAVE_DLOPEN

#if !USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE
extern "C" void* __libc_malloc(size_t size) throw() __attribute__((__malloc__));
extern "C" void* __libc_calloc(size_t nmemb, size_t size) throw() __attribute__((__malloc__));
extern "C" void* __libc_realloc(void* ptr, size_t size) throw() __attribute__((__malloc__));
extern "C" void __libc_free(void* ptr) throw();
#endif

#if defined(VALGRIND) && VALGRIND
void* valgrind_malloc(size_t) { return 0; }
void* valgrind_calloc(size_t, size_t) { return 0; }
void* valgrind_realloc(void*, size_t) { return 0; }
void valgrind_free(void*) { }
#endif

namespace libcwd {
    
namespace _private_ {

#if LIBCWD_THREAD_SAFE && CWDEBUG_DEBUGM
extern bool WST_multi_threaded;
#endif

void no_alloc_print_int_to(std::ostream* os, unsigned long val, bool hexadecimal)
{
  char buf[32];			// 32 > x where x is the number of digits of the largest unsigned long.
  char* p = &buf[32];
  int base = hexadecimal ? 16 : 10;
  do
  {
    int digit = val % base;
    if (digit < 10)
      *--p = (char)('0' + digit);
    else
      *--p = (char)('a' - 10 + digit);
    val /= base;
  }
  while(val > 0);
  if (hexadecimal)
  {
    *--p = 'x';
    *--p = '0';
  }
  os->write(p, &buf[32] - p);
}

void smart_ptr::decrement(LIBCWD_TSD_PARAM)
{
  if (M_string_literal)
    return;
  if (M_ptr && reinterpret_cast<refcnt_charptr_ct*>(M_ptr)->decrement())
  {
    set_alloc_checking_off(LIBCWD_TSD);
    delete reinterpret_cast<refcnt_charptr_ct*>(M_ptr);
    set_alloc_checking_on(LIBCWD_TSD);
  }
}

void smart_ptr::copy_from(smart_ptr const& ptr)
{
  if (M_ptr != ptr.M_ptr)
  {
    LIBCWD_TSD_DECLARATION;
    decrement(LIBCWD_TSD);
    M_ptr = ptr.M_ptr;
    M_string_literal = ptr.M_string_literal;
    increment();
  }
}

void smart_ptr::copy_from(char const* ptr)
{
  LIBCWD_TSD_DECLARATION;
  decrement(LIBCWD_TSD);
  M_string_literal = true;
  M_ptr = const_cast<char*>(ptr);
}

void smart_ptr::copy_from(char* ptr)
{
  LIBCWD_TSD_DECLARATION;
  decrement(LIBCWD_TSD);
  if (ptr)
  {
    set_alloc_checking_off(LIBCWD_TSD);
    M_ptr = new refcnt_charptr_ct(ptr);
    set_alloc_checking_on(LIBCWD_TSD);
    M_string_literal = false;
  }
  else
  {
    M_ptr = NULL;
    M_string_literal = true;
  }
}

} // namespace _private_

extern void ST_initialize_globals(LIBCWD_TSD_PARAM);

} // namespace libcwd

#if CWDEBUG_DEBUGM
#define DEBUGDEBUG_DoutInternal_MARKER DEBUGDEBUG_CERR( "DoutInternal at " << __FILE__ << ':' << __LINE__ )
#define DEBUGDEBUG_DoutFatalInternal_MARKER DEBUGDEBUG_CERR( "DoutFatalInternal at " << __FILE__ << ':' << __LINE__ )
#define DEBUGDEBUG_ELSE_DoutInternal(data) else DEBUGDEBUG_CERR( "library_call == " << __libcwd_tsd.library_call << "; DoutInternal skipped for: " << data )
#define DEBUGDEBUG_ELSE_DoutFatalInternal(data) FATALDEBUGDEBUG_CERR( "library_call == " << __libcwd_tsd.library_call << "; DoutFatalInternal skipped for: " << data )
#else
#define DEBUGDEBUG_DoutInternal_MARKER
#define DEBUGDEBUG_DoutFatalInternal_MARKER
#define DEBUGDEBUG_ELSE_DoutInternal(data)
#define DEBUGDEBUG_ELSE_DoutFatalInternal(data)
#endif

#define DoutInternalDo( debug_object, cntrl, data )												\
  do																\
  {																\
    DEBUGDEBUG_DoutInternal_MARKER;												\
    if (__libcwd_tsd.library_call == 0 && LIBCWD_DO_TSD_MEMBER_OFF(debug_object) < 0)						\
    {																\
      DEBUGDEBUG_CERR( "Entering 'DoutInternal(cntrl, \"" << data << "\")'.  internal == " << __libcwd_tsd.internal << '.' );	\
      channel_set_bootstrap_st channel_set(LIBCWD_DO_TSD(debug_object) LIBCWD_COMMA_TSD);					\
      bool on;															\
      {																\
        using namespace channels;												\
        on = (channel_set|cntrl).on;												\
      }																\
      if (on)															\
      {																\
        LIBCWD_DO_TSD(debug_object).start(debug_object, channel_set LIBCWD_COMMA_TSD);						\
	++ LIBCWD_DO_TSD_MEMBER_OFF(debug_object);										\
	_private_::no_alloc_ostream_ct no_alloc_ostream(*LIBCWD_DO_TSD_MEMBER(debug_object, current_bufferstream)); 			\
        no_alloc_ostream << data;												\
	-- LIBCWD_DO_TSD_MEMBER_OFF(debug_object);										\
        LIBCWD_DO_TSD(debug_object).finish(debug_object, channel_set LIBCWD_COMMA_TSD);						\
      }																\
      DEBUGDEBUG_CERR( "Leaving 'DoutInternal(cntrl, \"" << data << "\")'.  internal = " << __libcwd_tsd.internal << '.' ); 	\
    }																\
    DEBUGDEBUG_ELSE_DoutInternal(data);												\
  } while(0)

#define DoutInternal( cntrl, data ) DoutInternalDo( libcw_do, cntrl, data )												\

#define DoutFatalInternal( cntrl, data )											\
  do																\
  {																\
    DEBUGDEBUG_DoutFatalInternal_MARKER;											\
    if (__libcwd_tsd.library_call < 2)												\
    {																\
      DEBUGDEBUG_CERR( "Entering 'DoutFatalInternal(cntrl, \"" << data << "\")'.  internal == " <<				\
			__libcwd_tsd.internal << "; setting internal to 0." ); 							\
      __libcwd_tsd.internal = 0;												\
      channel_set_bootstrap_fatal_st channel_set(LIBCWD_DO_TSD(libcw_do) LIBCWD_COMMA_TSD);						\
      {																\
	using namespace channels;												\
	channel_set|cntrl;													\
      }																\
      LIBCWD_DO_TSD(libcw_do).start(libcw_do, channel_set LIBCWD_COMMA_TSD);							\
      ++ LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);											\
      _private_::no_alloc_ostream_ct no_alloc_ostream(*LIBCWD_DO_TSD_MEMBER(libcw_do, current_bufferstream)); 				\
      no_alloc_ostream << data;													\
      -- LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);											\
      LIBCWD_DO_TSD(libcw_do).fatal_finish(libcw_do, channel_set LIBCWD_COMMA_TSD);	/* Never returns */			\
      LIBCWD_DEBUG_ASSERT( !"Bug in libcwd!" );											\
      core_dump();														\
    }																\
    else															\
    {																\
      DEBUGDEBUG_ELSE_DoutFatalInternal(data);											\
      LIBCWD_DEBUG_ASSERT( !"See msg above." );											\
      core_dump();														\
    }																\
  } while(0)

namespace libcwd {

namespace _private_ {
  //
  // The following is about tackling recursive calls.
  //
  // The memory allocations performed by the users application are stored
  // in a red/black tree using the STL map container.  Using this container
  // does cause calls to malloc by itself.  Of course we can not also store
  // *those* allocations in the map, that would result in an endless loop.
  // Therefore we use a local variable `internal' and set that before we
  // do the calls to the map<> methods.  Then, when malloc is called with
  // `internal' set, we do not store these allocations in the map.
  // Note that we need to know *precisely* which pointers point to these
  // 'internal' allocations because we also need to set `internal' prior
  // to freeing these blocks - otherwise an error would be detected because
  // we can't find that pointer in the map (thinking the user is freeing
  // memory that he never allocated).
  //
  // Apart from the above there is a second cause of recursive calls:
  // Any library call (to libc or libstdc++) can in principle allocate or
  // free memory.  Writing debug output or writing directly to cerr also
  // causes a lot of calls to operator new and operator new[].  But in this
  // case we have no control from where these allocations are done, or from
  // where they are freed.  Therefore we assume that calls to Dout() and
  // writing data to cerr is _always_ done with `internal' set off.
  // That allows the user to use cerr too ;).
  //
  // This means however that it is possible to recursively enter the
  // allocation routines with `internal' set off.  In order to avoid an
  // endless loop in this case we have to store those allocations in the
  // map<> in total silence (no debug output what so ever).  The variable
  // that is used for this is `library_call'.  In other words, `library_call'
  // is set when we call a library function that could call malloc or new
  // (most notably operator<<(ostream&, ...), while writing debug output;
  // when creating a location_ct (which is a 'library call' of libcwd); or
  // while doing string manipulations).
  //
  // internal	library_call	Action
  //
  //  false	   false	Called from user space; store in the map and write debug output.
  //  true	   false	Called from our map<>, do not store in the map; allowed to write debug output.
  //  false	   true		Called from a library call while in new or malloc etc.
  //				  store in map but do not call any library function.
  //  true	   true		Called from our map<> while storing an allocation that
  //				  was done during a call to a library function from
  //				  one of our allocation routines, do not store in the map
  //				  and do not call any library functions.

#if LIBCWD_IOSBASE_INIT_ALLOCATES
  //
  // The following kludge is needed because of a bug in libstdc++-v3.
  //
  bool WST_ios_base_initialized = false;			// MT-safe: this is set to true before main() is reached and
  								// never changed anymore (see `inside_ios_base_Init_Init').

  // _private_::
  bool inside_ios_base_Init_Init(void)				// Single Threaded function.
  {
    LIBCWD_TSD_DECLARATION;
    LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.internal);
#ifndef _GLIBCPP_USE_WCHAR_T
    if (std::cerr.flags() != std::ios_base::unitbuf)		// Still didn't reach the end of std::ios_base::Init::Init()?
#else
    if (std::wcerr.flags() != std::ios_base::unitbuf)
#endif
      return true;
    WST_ios_base_initialized = true;
    ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
    make_all_allocations_invisible_except(NULL);		// Get rid of the <pre ios initialization> allocation list.
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
    DEBUGDEBUG_CERR( "Standard streams initialized." );
    return false;
  }
#endif // LIBCWD_IOSBASE_INIT_ALLOCATES

#if CWDEBUG_DEBUGM
// _private_::
void set_alloc_checking_off(LIBCWD_TSD_PARAM)
{
  LIBCWD_DEBUGM_CERR( "set_alloc_checking_off called from " << __builtin_return_address(0) << ": internal == " << __libcwd_tsd.internal << "; setting it to " << (__libcwd_tsd.internal + 1) << '.' );
  ++__libcwd_tsd.internal;
}

// _private_::
void set_alloc_checking_on(LIBCWD_TSD_PARAM)
{
  if (__libcwd_tsd.internal == 0)
    DoutFatal(dc::core, "Calling `set_alloc_checking_on' while ALREADY on.");
  LIBCWD_DEBUGM_CERR( "set_alloc_checking_on called from " << __builtin_return_address(0) << ": internal == " << __libcwd_tsd.internal << "; setting it to " << (__libcwd_tsd.internal - 1) << '.' );
  --__libcwd_tsd.internal;
}

// _private_::
int set_library_call_on(LIBCWD_TSD_PARAM)
{
  int saved_internal = __libcwd_tsd.internal;
  LIBCWD_DEBUGM_CERR( "set_library_call_on called from " << __builtin_return_address(0) << ": internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  if (saved_internal == 0)
    DoutFatal(dc::core, "Calling `set_library_call_on' while not internal.");
  ++__libcwd_tsd.library_call;
  ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
  return saved_internal;
}

// _private_::
void set_library_call_off(int saved_internal LIBCWD_COMMA_TSD_PARAM)
{
  --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
  if (__libcwd_tsd.internal)
    DoutFatal(dc::core, "Calling `set_library_call_off' while internal?!");
  if (__libcwd_tsd.library_call == 0)
    DoutFatal(dc::core, "Calling `set_library_call_off' while library_call == 0 ?!");
  LIBCWD_DEBUGM_CERR( "set_library_call_off called from " << __builtin_return_address(0) << ": internal == " << __libcwd_tsd.internal << "; setting it to " << saved_internal << '.' );
  __libcwd_tsd.internal = saved_internal;
  --__libcwd_tsd.library_call;
}
#endif // CWDEBUG_DEBUGM

} // namespace _private_

class dm_alloc_ct;
class memblk_key_ct;
class memblk_info_ct;

enum deallocated_from_nt { from_free, from_delete, from_delete_array, error };
  // Indicates if 'internal_free()' was called via __libcwd_free(), 'operator delete()' or 'operator delete[]()'.

static deallocated_from_nt const expected_from[] = {
  from_delete,			// memblk_type_new
  error,			// memblk_type_deleted
  from_delete_array,		// memblk_type_new_array
  error,			// memblk_type_deleted_array
  from_free,			// memblk_type_malloc
  from_free,			// memblk_type_realloc
  error,			// memblk_type_freed
#if CWDEBUG_MARKER
  from_delete,			// memblk_type_marker
  error,			// memblk_type_deleted_marker
#endif
  from_free,			// memblk_type_posix_memalign
  from_free,			// memblk_type_memalign
  from_free,			// memblk_type_valloc
  from_free			// memblk_type_external
};

/**
 * \brief Allow writing a \c memblk_types_nt directly to an ostream.
 *
 * Writes the name of the \c memblk_types_nt \a memblk_type to \c ostream \a os.
 */
std::ostream& operator<<(std::ostream& os, memblk_types_nt memblk_type)
{
  switch(memblk_type)
  {
    case memblk_type_new:
      os << "memblk_type_new";
      break;
    case memblk_type_deleted:
      os << "memblk_type_deleted";
      break;
    case memblk_type_new_array:
      os << "memblk_type_new_array";
      break;
    case memblk_type_deleted_array:
      os << "memblk_type_deleted_array";
      break;
    case memblk_type_malloc:
      os << "memblk_type_malloc";
      break;
    case memblk_type_realloc:
      os << "memblk_type_realloc";
      break;
    case memblk_type_freed:
      os << "memblk_type_freed";
      break;
#if CWDEBUG_MARKER
    case memblk_type_marker:
      os << "memblk_type_marker";
      break;
    case memblk_type_deleted_marker:
      os << "memblk_type_deleted_marker";
      break;
#endif
    case memblk_type_posix_memalign:
      os << "memblk_type_posix_memalign";
      break;
    case memblk_type_memalign:
      os << "memblk_type_memalign";
      break;
    case memblk_type_valloc:
      os << "memblk_type_valloc";
      break;
    case memblk_type_external:
      os << "memblk_type_external";
      break;
  }
  return os;
}

// Used to print the label in an Memory Allocation Overview.
class memblk_types_label_ct {
private:
  memblk_types_nt M_memblk_type;
public:
  memblk_types_label_ct(memblk_types_nt memblk_type) : M_memblk_type(memblk_type) { }
  void print_on(std::ostream& os) const;
};

void memblk_types_label_ct::print_on(std::ostream& os) const
{
  switch (M_memblk_type)
  {
    case memblk_type_new:
      os.write("          ", 10);
      break;
    case memblk_type_new_array:
      os.write("new[]     ", 10);
      break;
    case memblk_type_malloc:
      os.write("malloc    ", 10);
      break;
    case memblk_type_realloc:
      os.write("realloc   ", 10);
      break;
#if CWDEBUG_MARKER
    case memblk_type_marker:
      os.write("(MARKER)  ", 10);
      break;
    case memblk_type_deleted_marker:
#endif
    case memblk_type_deleted:
    case memblk_type_deleted_array:
      os.write("(deleted) ", 10);
      break;
    case memblk_type_freed:
      os.write("(freed)   ", 10);
      break;
    case memblk_type_posix_memalign:
      os.write("pmemalign ", 10);
      break;
    case memblk_type_memalign:
      os.write("memalign  ", 10);
      break;
    case memblk_type_valloc:
      os.write("valloc    ", 10);
      break;
    case memblk_type_external:
      os.write("external  ", 10);
      break;
  }
}

#if CWDEBUG_DEBUGM
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, memblk_key_ct const&)
{
  write(2, "<memblk_key_ct>", 15);
  return raw_write;
}
#endif
#if CWDEBUG_LOCATION && CWDEBUG_DEBUG
inline
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& os, location_ct const& location)
{
  _private_::print_location_on(os, location);
  return os;
}
#endif

class dm_alloc_base_ct : public alloc_ct {
public:
  dm_alloc_base_ct(void const* s, size_t sz, memblk_types_nt type,
      type_info_ct const& ti, struct timeval const& t LIBCWD_COMMA_LOCATION(location_ct const* l))
    : alloc_ct(s, sz, type, ti, t LIBCWD_COMMA_LOCATION(l)) { }
  void print_description(debug_ct& debug_object, alloc_filter_ct const& filter LIBCWD_COMMA_TSD_PARAM) const;
};

#if LIBCWD_THREAD_SAFE
#define BASE_ALLOC_LIST (__libcwd_tsd.target_thread->base_alloc_list)
#define CURRENT_ALLOC_LIST (__libcwd_tsd.target_thread->current_alloc_list)
#define CURRENT_OWNER_NODE (__libcwd_tsd.target_thread->current_owner_node)
#define MEMSIZE (__libcwd_tsd.target_thread->memsize)
#define MEMBLKS (__libcwd_tsd.target_thread->memblks)
#else // !LIBCWD_THREAD_SAFE
static dm_alloc_ct* ST_base_alloc_list = NULL;				// The base list with `dm_alloc_ct' objects.
									// Each of these objects has a list of it's own.
static dm_alloc_ct** ST_current_alloc_list =  &ST_base_alloc_list;	// The current list to which newly allocated memory blocks are added.
static dm_alloc_ct* ST_current_owner_node = NULL;			// If the CURRENT_ALLOC_LIST != &BASE_ALLOC_LIST, then this variable
									// points to the dm_alloc_ct node who owns the current list.
static size_t ST_memsize = 0;						// Total number of allocated bytes (excluding internal allocations).
static unsigned long ST_memblks = 0;					// Total number of allocated blocks (excluding internal allocations).
// Access macros for the single threaded case.
#define BASE_ALLOC_LIST ST_base_alloc_list
#define CURRENT_ALLOC_LIST ST_current_alloc_list
#define CURRENT_OWNER_NODE ST_current_owner_node
#define MEMSIZE ST_memsize
#define MEMBLKS ST_memblks
#endif // !LIBCWD_THREAD_SAFE

#define memblk_iter_write reinterpret_cast<memblk_map_ct::iterator&>(const_cast<memblk_map_ct::const_iterator&>(iter))
#define target_memblk_iter_write memblk_iter_write

//===========================================================================
//
// class dm_alloc_ct
//
// The object that is stored in the `alloc list' (see 2) above).
// Each `memblk_ct' is related with exactly one `dm_alloc_ct' object.
// All `dm_alloc_ct' objects do is maintaining an extra relationship between
// certain `memblk_ct' objects. ('certain' because `memblk_ct' can be made
// 'invisible', in which case they do not participate in this relation and
// are never showed in any overview).
// Unfortunately the STL doesn't allow an object to be simultaneously a map and
// a list, therefore we have to implement our own list (prev/next pointers).

class dm_alloc_copy_ct;				// A class to temporarily copy dm_alloc_ct objects to in order to print them.
class dm_alloc_ct : public dm_alloc_base_ct {
  friend class dm_alloc_copy_ct;
#if CWDEBUG_MARKER
  friend class marker_ct;
  friend void move_outside(marker_ct* marker, void const* ptr);
#endif
private:
  dm_alloc_ct* next;		// Next memblk in list `my_list'
  dm_alloc_ct* prev;		// Previous memblk in list `my_list'
  dm_alloc_ct* a_next_list;	// First element of my childeren (if any)
  dm_alloc_ct** my_list;	// Pointer to my list, never NULL except after deinitialization.
  dm_alloc_ct* my_owner_node;	// Pointer to node who's a_next_list contains this object.

public:
  dm_alloc_ct(void const* s, size_t sz, memblk_types_nt f, struct timeval const& t
              LIBCWD_COMMA_TSD_PARAM LIBCWD_COMMA_LOCATION(location_ct const* l)) :		// MT-safe: write lock is set.
      dm_alloc_base_ct(s, sz , f, unknown_type_info_c, t LIBCWD_COMMA_LOCATION(l)), prev(NULL), a_next_list(NULL)
      {
	LIBCWD_DEBUGT_ASSERT(__libcwd_tsd.target_thread == &(*__libcwd_tsd.thread_iter));
	next = *CURRENT_ALLOC_LIST;
	my_list = CURRENT_ALLOC_LIST;
	my_owner_node = CURRENT_OWNER_NODE;
	dm_alloc_ct** foo = CURRENT_ALLOC_LIST;
        *foo = this;
        if (next)
          next->prev = this;
	MEMSIZE += sz;
	++MEMBLKS;
      }
    // Constructor: Add `node' at the start of `list'
#if CWDEBUG_DEBUGM
  dm_alloc_ct(dm_alloc_ct const& __dummy) : dm_alloc_base_ct(__dummy) { LIBCWD_TSD_DECLARATION; DoutFatalInternal( dc::fatal, "Calling dm_alloc_ct::dm_alloc_ct(dm_alloc_ct const&)" ); }
    // No copy constructor allowed.
#endif
  void deinit(LIBCWD_TSD_PARAM);
  ~dm_alloc_ct() { if (my_list) { LIBCWD_TSD_DECLARATION; deinit(LIBCWD_TSD); } }
  void new_list(LIBCWD_TSD_PARAM)							// MT-safe: write lock is set.
      {
	// A new list is always added for the current thread.
	LIBCWD_DEBUGT_ASSERT(__libcwd_tsd.target_thread == &(*__libcwd_tsd.thread_iter));
	CURRENT_ALLOC_LIST = &a_next_list;
        CURRENT_OWNER_NODE = this;
      }
    // Start a new list in this node.
  void change_flags(memblk_types_nt new_memblk_type) { a_memblk_type = new_memblk_type; }
  void change_label(type_info_ct const& ti, _private_::smart_ptr description) { type_info_ptr = &ti; a_description = description; }
  type_info_ct const* typeid_ptr(void) const { return type_info_ptr; }
  _private_::smart_ptr description(void) const { return a_description; }
  dm_alloc_ct const* next_node(void) const { return next; }
  dm_alloc_ct const* prev_node(void) const { return prev; }
  dm_alloc_ct const* next_list(void) const { return a_next_list; }
  void const* start(void) const { return a_start; }
  bool is_deleted(void) const;
  size_t size(void) const { return a_size; }
  void printOn(std::ostream& os) const;
  static void descend_current_alloc_list(LIBCWD_TSD_PARAM);			// MT-safe: write lock is set.
  friend inline std::ostream& operator<<(std::ostream& os, dm_alloc_ct const& alloc) { alloc.printOn(os); return os; }
  friend inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, dm_alloc_ct const& alloc) { alloc.printOn(os.M_os); return os; }
};

class dm_alloc_copy_ct : public dm_alloc_base_ct {
private:
  dm_alloc_copy_ct* M_next;
  dm_alloc_copy_ct* M_next_list;
public:
  dm_alloc_copy_ct(dm_alloc_ct const& alloc) : dm_alloc_base_ct(alloc), M_next(NULL), M_next_list(NULL) { }
  ~dm_alloc_copy_ct();
  static dm_alloc_copy_ct* deep_copy(dm_alloc_ct const* alloc);
  unsigned long show_alloc_list(debug_ct& debug_object, int depth, channel_ct const& channel, alloc_filter_ct const& filter) const;
  dm_alloc_copy_ct const* next(void) const { return M_next; }
};

dm_alloc_copy_ct* dm_alloc_copy_ct::deep_copy(dm_alloc_ct const* alloc)
{
  dm_alloc_copy_ct* dm_alloc_copy = new dm_alloc_copy_ct(*alloc);
  if (alloc->a_next_list)
    dm_alloc_copy->M_next_list = deep_copy(alloc->a_next_list);
  dm_alloc_copy_ct* prev = dm_alloc_copy;
  for(;;)
  {
    alloc = alloc->next;
    if (!alloc)
      break;
    prev->M_next = new dm_alloc_copy_ct(*alloc);
    prev = prev->M_next;
    if (alloc->a_next_list)
      prev->M_next_list = dm_alloc_copy_ct::deep_copy(alloc->a_next_list);
  }
  return dm_alloc_copy;
}

dm_alloc_copy_ct::~dm_alloc_copy_ct()
{
  delete M_next_list;
  //delete M_next;
  // In order to avoid a stack overflow in the case that this is
  // a very long list, delete all M_next objects in the chain
  // here (patch by Joel Nordell).
  dm_alloc_copy_ct* next;
  for (dm_alloc_copy_ct* p = M_next; p != NULL; p = next)
  {
    next = p->M_next;
    p->M_next = NULL;
    delete p;
  }
}

typedef dm_alloc_ct const const_dm_alloc_ct;

// Set `CURRENT_ALLOC_LIST' back to its parent list.
void dm_alloc_ct::descend_current_alloc_list(LIBCWD_TSD_PARAM)			// MT-safe: write lock is set.
{
  if (CURRENT_OWNER_NODE)
  {
    CURRENT_ALLOC_LIST = CURRENT_OWNER_NODE->my_list;
    CURRENT_OWNER_NODE = (*CURRENT_ALLOC_LIST)->my_owner_node;
  }
  else
    CURRENT_ALLOC_LIST = &BASE_ALLOC_LIST;
}

//=============================================================================
//
// classes memblk_key_ct and memblk_info_ct
//
// The object memblk_ct (std::pair<memblk_key_ct const, memblk_info_ct) that is
// stored in the red/black tree.
// Each of these objects is related with exactly one allocated memory block.
// Note that ALL allocated memory blocks have a related `memblk_ct'
// unless they are allocated from within this file (with `internal' set).

class appblock;
class memblk_key_ct {
private:
  void const* a_start;		// Start of allocated memory block
  void const* a_end;		// End of allocated memory block
public:
  memblk_key_ct(appblock const* s, size_t size) : a_start(s), a_end(reinterpret_cast<char const*>(s) + size) { }
  void const* start(void) const { return a_start; }
  void const* end(void)   const { return a_end; }
  size_t size(void)       const { return (char*)a_end - (char*)a_start; }
  bool operator<(memblk_key_ct b) const
      { return a_end < b.start() || (a_end == b.start() && size() > 0); }
  bool operator>(memblk_key_ct b) const
      { return a_start > b.end() || (a_start == b.end() && b.size() > 0); }
  bool operator==(memblk_key_ct b) const
  {
#if CWDEBUG_DEBUGM
    {
      LIBCWD_TSD_DECLARATION;
      LIBCWD_DEBUGM_ASSERT( __libcwd_tsd.internal );
      DoutInternal( dc::warning, "Calling memblk_key_ct::operator==(" << *this << ", " << b << ")" );
    }
#endif
    return a_start == b.start();
  }
  void printOn(std::ostream& os) const;
  friend inline std::ostream& operator<<(std::ostream& os, memblk_key_ct const& memblk) { memblk.printOn(os); return os; }
  friend inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, memblk_key_ct const& memblk) { memblk.printOn(os.M_os); return os; }
  memblk_key_ct(void) { }
    // Never use this.  It's needed for the implementation of the std::pair<>.
};

uint16_t const memblk_info_flag_watch = 1;

class memblk_info_base_ct {
protected:
  uint16_t M_memblk_type;
  mutable uint16_t M_flags;	// Warning: the mutable needs special attention with locking,
  				// or else this is not thread safe.
public:
  memblk_info_base_ct() { }
  memblk_info_base_ct(memblk_types_nt memblk_type) : M_memblk_type(memblk_type), M_flags(0) { }
  memblk_info_base_ct(memblk_info_base_ct const& memblk_info_base) :
      M_memblk_type(memblk_info_base.M_memblk_type), M_flags(memblk_info_base.M_flags) { }
  memblk_types_nt flags(void) const { return (memblk_types_nt)M_memblk_type; }
  void set_watch(void) const { M_flags |= memblk_info_flag_watch; }
  bool is_watched(void) const { return (M_flags & memblk_info_flag_watch); }
};

class memblk_info_ct : public memblk_info_base_ct {
#if CWDEBUG_MARKER
  friend class marker_ct;
  friend void move_outside(marker_ct* marker, void const* ptr);
#endif
private:
  lockable_auto_ptr<dm_alloc_ct> a_alloc_node;
    // The associated `dm_alloc_ct' object.
    // This must be a pointer because the allocated `dm_alloc_ct' can persist
    // after this memblk_info_ct is deleted (dm_alloc_ct marked `is_deleted'),
    // when it still has allocated 'childeren' in `a_next_list' of its own.
public:
  memblk_info_ct(appblock const* s, size_t sz, memblk_types_nt f,
      struct timeval const& t LIBCWD_COMMA_TSD_PARAM LIBCWD_COMMA_LOCATION(location_ct const* l));
  memblk_info_ct(memblk_types_nt f);
  bool erase(bool owner LIBCWD_COMMA_TSD_PARAM);
  void lock(void) { a_alloc_node.lock(); }
  dm_alloc_ct* release_alloc_node(void) const { return a_alloc_node.release(); }
  void make_invisible(void);
  bool has_alloc_node(void) const { return a_alloc_node.get(); }
  alloc_ct* get_alloc_node(void) const
      { if (has_alloc_node()) return a_alloc_node.get(); return NULL; }
  type_info_ct const* typeid_ptr(void) const { return a_alloc_node.get()->typeid_ptr(); }
  _private_::smart_ptr description(void) const { return a_alloc_node.get()->description(); }
  void change_label(type_info_ct const& ti, _private_::smart_ptr description) const
      { if (has_alloc_node()) a_alloc_node.get()->change_label(ti, description); }
  void change_label(type_info_ct const& ti, char const* description) const
      { _private_::smart_ptr desc(description); change_label(ti, desc); }
  void alloctag_called(void) { a_alloc_node.get()->alloctag_called(); }
  void change_flags(memblk_types_nt new_flag)
      { M_memblk_type = new_flag; if (has_alloc_node()) a_alloc_node.get()->change_flags(new_flag); }
  void new_list(LIBCWD_TSD_PARAM) const { a_alloc_node.get()->new_list(LIBCWD_TSD); }			// MT-safe: write lock is set.
  void printOn(std::ostream& os) const;
  friend inline std::ostream& operator<<(std::ostream& os, memblk_info_ct const& memblk) { memblk.printOn(os); return os; }
private:
  memblk_info_ct(void) { }
    // Never use this.  It's needed for the implementation of the std::pair<>.
};

//=============================================================================
//
// The memblk map:
//

typedef std::pair<memblk_key_ct const, memblk_info_ct> memblk_ct;
  // The value type of the map (memblk_map_ct::value_type).

typedef std::map<memblk_key_ct, memblk_info_ct, std::less<memblk_key_ct>,
                 _private_::memblk_map_allocator::rebind<memblk_ct>::other> memblk_map_ct;
  // The map containing all `memblk_ct' objects.

#if LIBCWD_THREAD_SAFE
// Should only be used after an ACQUIRE_WRITE_LOCK and before the corresponding RELEASE_WRITE_LOCK.
#define memblk_map_write (reinterpret_cast<memblk_map_ct*>((*__libcwd_tsd.thread_iter).memblk_map))
#define target_memblk_map_write (reinterpret_cast<memblk_map_ct*>(__libcwd_tsd.target_thread->memblk_map))
// Should only be used after an ACQUIRE_READ_LOCK and before the corresponding RELEASE_READ_LOCK.
#define memblk_map_read  (reinterpret_cast<memblk_map_ct const*>((*__libcwd_tsd.thread_iter).memblk_map))
#define target_memblk_map_read  (reinterpret_cast<memblk_map_ct const*>(__libcwd_tsd.target_thread->memblk_map))
#else // !LIBCWD_THREAD_SAFE
//=============================================================================
//
// static variables
//
static memblk_map_ct* ST_memblk_map;
// Access macros for the single threaded case.
#define memblk_map_write ST_memblk_map
#define target_memblk_map_write ST_memblk_map
#define memblk_map_read  ST_memblk_map
#define target_memblk_map_read ST_memblk_map
#endif // !LIBCWD_THREAD_SAFE

#define memblk_iter_write reinterpret_cast<memblk_map_ct::iterator&>(const_cast<memblk_map_ct::const_iterator&>(iter))
#define target_memblk_iter_write memblk_iter_write

alloc_filter_ct const default_ooam_filter(0);

#if CWDEBUG_LOCATION
//=============================================================================
//
// The location_cache map:
//

typedef std::pair<void const* const, location_ct> location_cache_ct;
  // The value type of the map (location_cache_map_ct::value_type).

typedef std::map<void const*, location_ct, std::less<void const*>,
                 _private_::internal_allocator::rebind<location_cache_ct>::other> location_cache_map_ct;
  // The map used to cache locations at which memory was allocated.

union location_cache_map_t {
#if LIBCWD_THREAD_SAFE
  // See http://www.cuj.com/experts/1902/alexandr.htm?topic=experts
  location_cache_map_ct const volatile* MT_unsafe;
#else
  location_cache_map_ct const* MT_unsafe;
#endif
  location_cache_map_ct* write;		// Should only be used after an ACQUIRE_LC_WRITE_LOCK and before the corresponding RELEASE_LC_WRITE_LOCK.
  location_cache_map_ct const* read;	// Should only be used after an ACQUIRE_LC_READ_LOCK and before the corresponding RELEASE_LC_READ_LOCK.
};

static location_cache_map_t location_cache_map;		// MT-safe: initialized before WST_initialization_state == 1.

#define location_cache_map_write (location_cache_map.write)
#define location_cache_map_read  (location_cache_map.read)
#define location_cache_iter_write reinterpret_cast<location_cache_map_ct::iterator&>(const_cast<location_cache_map_ct::const_iterator&>(iter))

//=============================================================================
//
// Location cache
//

location_ct const* location_cache(void const* addr LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_ASSERT(!__libcwd_tsd.internal);
  bool found;
  location_ct* location_info = NULL;	// Avoid compiler warning.
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_LC_READ_LOCK;
  location_cache_map_ct::const_iterator const_iter(location_cache_map_read->find(addr));
  found = (const_iter != location_cache_map_read->end());
  if (found)
    location_info = const_cast<location_ct*>(&(*const_iter).second);
  RELEASE_LC_READ_LOCK;
  LIBCWD_RESTORE_CANCEL;
  if (!found)
  {
    location_ct loc(addr);	// This construction must be done with no lock set at all.
    LIBCWD_DEFER_CANCEL;
    ACQUIRE_LC_WRITE_LOCK;
    DEBUGDEBUG_CERR( "location_cache: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
    __libcwd_tsd.internal = 1;
    std::pair<location_cache_map_ct::iterator, bool> const&
	iter(location_cache_map_write->insert(location_cache_ct(addr, loc)));
    DEBUGDEBUG_CERR( "location_cache: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
    __libcwd_tsd.internal = 0;
    location_info = &(*iter.first).second;
    if (iter.second)		// Test for insertion because another thread could have added this location
				// to the cache between RELEASE_LC_READ_LOCK and ACQUIRE_LC_WRITE_LOCK above.
      location_info->lock_ownership();
    RELEASE_LC_WRITE_LOCK;
    LIBCWD_RESTORE_CANCEL;
  }
  // Don't try to resolve a location if we're from a library call, this to avoid possible infinite loops.
  // We need to at least try to resolve when library_call == 1, because we might get here from internal_malloc
  // et al, which increments library_call before calling location_cache().
  else if (__libcwd_tsd.library_call < 2 && location_info->initialization_delayed())
    location_info->handle_delayed_initialization(default_ooam_filter);
  return location_info;
}

// Synchronize filtering in regard with filter.M_sourcefile_masks and filter.M_function_masks (only).
void location_ct::synchronize_with(alloc_filter_ct const& filter) const
{
  if (!M_object_file)					// Then also !M_known.
    M_hide = _private_::unfiltered_location;
  else if (!M_known)
  {
    if (M_func == unknown_function_c ||
        M_func == S_uninitialized_location_ct_c ||
	M_func == S_pre_ios_initialization_c ||
	M_func == S_pre_libcwd_initialization_c ||
	M_func == S_cleared_location_ct_c)
      M_hide = _private_::unfiltered_location;
    else
      M_hide = filter.check_hide(M_object_file, M_func);
  }
  else
  {
    M_hide = filter.check_hide(M_filepath.get());
    if (M_hide != _private_::filtered_location)
      M_hide = filter.check_hide(M_object_file, M_func);
  }
}

void alloc_filter_ct::M_synchronize_locations(void) const
{
  ACQUIRE_LC_WRITE_LOCK;
  for (location_cache_map_ct::iterator iter = location_cache_map_write->begin(); iter != location_cache_map_write->end(); ++iter)
    (*iter).second.synchronize_with(*this);
  RELEASE_LC_WRITE_LOCK;
}

void location_ct::handle_delayed_initialization(alloc_filter_ct const& filter)
{
  LIBCWD_TSD_DECLARATION;
  M_pc_location(M_initialization_delayed LIBCWD_COMMA_TSD);
  synchronize_with(filter);
}
#endif // CWDEBUG_LOCATION

static int WST_initialization_state;		// MT-safe: We will reach state '1' the first call to malloc.
						// We *assume* that the first call to malloc is before we reach
						// main(), or at least before threads are created.

  //  0: memblk_map, location_cache_map and libcwd both not initialized yet.
  //  1: memblk_map, location_cache_map and libcwd both initialized.
  // -1: memblk_map and location_cache_map initialized but libcwd not initialized yet.

//=============================================================================
//
// dm_alloc_ct methods
//

inline bool dm_alloc_ct::is_deleted(void) const
{
  return (a_memblk_type == memblk_type_deleted ||
#if CWDEBUG_MARKER
      a_memblk_type == memblk_type_deleted_marker ||
#endif
      a_memblk_type == memblk_type_freed);
}

void dm_alloc_ct::deinit(LIBCWD_TSD_PARAM)					// MT-safe: write lock is set.
{
  if (!my_list)
    return;
#if CWDEBUG_DEBUGM
  LIBCWD_DEBUGM_ASSERT(__libcwd_tsd.internal);
  if (a_next_list)
    DoutFatalInternal( dc::core, "Removing an dm_alloc_ct that still has an own list" );
  dm_alloc_ct* tmp;
  for(tmp = *my_list; tmp && tmp != this; tmp = tmp->next) ;
  if (!tmp)
    DoutFatalInternal( dc::core, "Removing an dm_alloc_ct that is not part of its own list" );
#endif
  MEMSIZE -= size();
  --MEMBLKS;
  if (CURRENT_ALLOC_LIST == &a_next_list)
    descend_current_alloc_list(LIBCWD_TSD);
  // Delink this node from `my_list'
  if (next)
    next->prev = prev;
  if (prev)
    prev->next = next;
  else if (!(*my_list = next) && my_owner_node && my_owner_node->is_deleted())
    delete my_owner_node;
  my_list = NULL;	// Mark that we are already de-initialized.
}

namespace _private_ {
  extern void demangle_symbol(char const* in, _private_::internal_string& out);
} // namespace _private_

static char const* const twentyfive_spaces_c = "                         ";

// Print integer without allocating memory.
static void print_integer(std::ostream& os, unsigned int val, int width)
{
  char buf[12];
  char* p = buf + sizeof(buf);
  int c = width;
  do
  {
    *--p = '0' + val % 10;
    val /= 10;
  }
  while(--c > 0 || val > 0);
  while(c++ < width)
    os << *p++;
}

void dm_alloc_base_ct::print_description(debug_ct& debug_object, alloc_filter_ct const& filter LIBCWD_COMMA_TSD_PARAM) const
{
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.internal && !__libcwd_tsd.library_call);
#if CWDEBUG_LOCATION
  LibcwDoutScopeBegin(channels, debug_object, dc::continued);
  if ((filter.M_flags & show_objectfile))
  {
    object_file_ct const* object_file = M_location->object_file();
    if (object_file)
      LibcwDoutStream << object_file->filename() << ':';
    else
      LibcwDoutStream << "<unknown object file> (at " << M_location->unknown_pc() << ") :";
  }
  bool print_mangled_function_name = filter.M_flags & show_function;
  if (print_mangled_function_name)
    LibcwDoutStream << M_location->mangled_function_name();
  if (M_location->is_known())
  {
    if ((filter.M_flags & show_path))
    {
      size_t len = M_location->filepath_length();
      if (len < 20)
	LibcwDoutStream.write(twentyfive_spaces_c, 20 - len);
      else if (print_mangled_function_name)
        LibcwDoutStream.put(':');
      M_location->print_filepath_on(LibcwDoutStream);
    }
    else
    {
      size_t len = M_location->filename_length();
      if (len < 20)
	LibcwDoutStream.write(twentyfive_spaces_c, 20 - len);
      else if (print_mangled_function_name)
        LibcwDoutStream.put(':');
      M_location->print_filename_on(LibcwDoutStream);
    }
    LibcwDoutStream.put(':');
    print_integer(LibcwDoutStream, M_location->line(), 1);
    int l = M_location->line();
    int cnt = 0;
    while(l < 10000)
    {
      ++cnt;
      l *= 10;
    }
    LibcwDoutStream.write(twentyfive_spaces_c, cnt);
  }
  else
  {
    char const* mangled_function_name = M_location->mangled_function_name();
    if (mangled_function_name != unknown_function_c &&
      (!print_mangled_function_name || (mangled_function_name[0] == '_' && mangled_function_name[1] == 'Z')))
    {
      size_t s;
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      do
      {
	_private_::internal_string f;
	_private_::demangle_symbol(mangled_function_name, f);
	_private_::set_alloc_checking_on(LIBCWD_TSD);
	s = f.size();
	if (print_mangled_function_name)
	  LibcwDoutStream.put(':');
	LibcwDoutStream.write(f.data(), s);
	_private_::set_alloc_checking_off(LIBCWD_TSD);
      }
      while(0);
      _private_::set_alloc_checking_on(LIBCWD_TSD);
      if (s < 25)
	LibcwDoutStream.write(twentyfive_spaces_c, 25 - s);
      LibcwDoutStream.put(' ');
    }
    else
      LibcwDoutStream.write(twentyfive_spaces_c, 25);
  }
  LibcwDoutScopeEnd;
#endif

#if CWDEBUG_MARKER
  if (a_memblk_type == memblk_type_marker || a_memblk_type == memblk_type_deleted_marker)
    DoutInternalDo( debug_object, dc::continued, "<marker>;" );
  else
#endif

  {
    char const* a_type = type_info_ptr->demangled_name();
    size_t s = a_type ? strlen(a_type) : 0;		// Can be 0 while deleting a qualifiers_ct object in demangle3.cc
    if (s > 0)
    {
      if (a_type[s - 1] == '*' && type_info_ptr->ref_size() != 0)
      {
	__libcwd_tsd.internal = 1;
	char* buf = new char [s + 34];	// s-1 + '[' + 32 + ']' + '\0'.
	if (a_memblk_type == memblk_type_new || a_memblk_type == memblk_type_deleted)
	{
	  if (s > 1 && a_type[s - 2] == ' ')
	  {
	    std::strncpy(buf, a_type, s - 2);
	    buf[s - 2] = 0;
	  }
	  else
	  {
	    std::strncpy(buf, a_type, s - 1);
	    buf[s - 1] = 0;
	  }
	}
	else /* if (a_memblk_type == memblk_type_new_array || a_memblk_type == memblk_type_deleted_array) */
	{
	  std::strncpy(buf, a_type, s - 1);
	  // We can't use operator<<(ostream&, int) because that uses std::alloc.
	  buf[s - 1] = '[';
	  unsigned long val = a_size / type_info_ptr->ref_size();
	  char intbuf[32];	// 32 > x where x is the number of digits of the largest unsigned long.
	  char* p = &intbuf[32];
	  do
	  {
	    int digit = val % 10;
	    *--p = (char)('0' + digit);
	    val /= 10;
	  }
	  while(val > 0);
	  std::strncpy(buf + s, p, &intbuf[32] - p);
	  buf[s + &intbuf[32] - p] = ']';
	  buf[s + &intbuf[32] - p + 1] = 0;
	}
	DoutInternalDo( debug_object, dc::continued, buf );
	delete [] buf;
	__libcwd_tsd.internal = 0;
      }
      else
	DoutInternalDo( debug_object, dc::continued, a_type );
    }

    DoutInternalDo( debug_object, dc::continued, ';' );
  }

  DoutInternalDo( debug_object, dc::continued, " (sz = " << a_size << ") " );

  if (!a_description.is_null())
    DoutInternalDo( debug_object, dc::continued, ' ' << a_description );
}

void dm_alloc_ct::printOn(std::ostream& os) const
{
  _private_::no_alloc_ostream_ct no_alloc_ostream(os); 
  no_alloc_ostream << "{ start = " << a_start << ", size = " << a_size << ", a_memblk_type = " << a_memblk_type <<
      ",\n\ttype = \"" << type_info_ptr->demangled_name() <<
      "\", description = \"" << (a_description.is_null() ? "NULL" : static_cast<char const*>(a_description)) <<
      "\", next = " << (void*)next << ", prev = " << (void*)prev <<
      ",\n\tnext_list = " << (void*)a_next_list << ", my_list = " << (void*)my_list << "\n\t( = " << (void*)*my_list << " ) }";
}

unsigned long dm_alloc_copy_ct::show_alloc_list(debug_ct& debug_object, int depth, channel_ct const& channel, alloc_filter_ct const& filter) const
{
  unsigned long printed_memblks = 0;
  dm_alloc_copy_ct const* alloc;
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT(!__libcwd_tsd.internal);		// localtime() can allocate memory and is a library call.
  for (alloc = this; alloc; alloc = alloc->M_next)
  {
    if ((filter.M_flags & hide_untagged) && !alloc->is_tagged())
      continue;
#if CWDEBUG_LOCATION
    if (alloc->location().initialization_delayed())
      const_cast<location_ct*>(&alloc->location())->handle_delayed_initialization(filter);
    if ((filter.M_flags & hide_unknown_loc) && !alloc->location().is_known())
      continue;
    if (alloc->location().new_location())
      alloc->location().synchronize_with(filter);
    if (alloc->location().hide_from_alloc_list())
      continue;
    object_file_ct const* object_file = alloc->location().object_file();
    if (object_file && object_file->hide_from_alloc_list())
      continue;
#endif
    if (filter.M_start.tv_sec != 1)
    {
      if (alloc->a_time.tv_sec < filter.M_start.tv_sec || 
	  (alloc->a_time.tv_sec == filter.M_start.tv_sec && alloc->a_time.tv_usec < filter.M_start.tv_usec))
	continue;
    }
    if (filter.M_end.tv_sec != 1)
    {
      if (alloc->a_time.tv_sec > filter.M_end.tv_sec ||
	  (alloc->a_time.tv_sec == filter.M_end.tv_sec && alloc->a_time.tv_usec > filter.M_end.tv_usec))
	continue;
    }
    struct tm* tbuf_ptr = 0;				// Initialize in order to avoid warning.
    if ((filter.M_flags & show_time))
    {
      ++LIBCWD_DO_TSD_MEMBER_OFF(debug_object);		// localtime() can allocate memory, don't show it.
      _private_::set_invisible_on(LIBCWD_TSD);
      time_t tv_sec = alloc->a_time.tv_sec;		// On some OS, tv_sec is long.
#if LIBCWD_THREAD_SAFE
      struct tm tbuf;
      tbuf_ptr = localtime_r(&tv_sec, &tbuf);
#else
      tbuf_ptr = localtime(&tv_sec);
#endif
      _private_::set_invisible_off(LIBCWD_TSD);
      --LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
    }
    LibcwDoutScopeBegin(channels, debug_object, channel|nolabel_cf|continued_cf);
    for (int i = depth; i > 1; i--)
      LibcwDoutStream << "    ";
    if ((filter.M_flags & show_time))
    {
      print_integer(LibcwDoutStream, tbuf_ptr->tm_hour, 2);
      LibcwDoutStream << ':';
      print_integer(LibcwDoutStream, tbuf_ptr->tm_min, 2);
      LibcwDoutStream << ':';
      print_integer(LibcwDoutStream, tbuf_ptr->tm_sec, 2);
      LibcwDoutStream << '.';
      print_integer(LibcwDoutStream, alloc->a_time.tv_usec, 6);
      LibcwDoutStream << ' ';
    }
    // Print label and start.
    LibcwDoutStream << cwprint(memblk_types_label_ct(alloc->memblk_type()));
    LibcwDoutStream << alloc->a_start << ' ';
    LibcwDoutScopeEnd;
    alloc->print_description(debug_object, filter LIBCWD_COMMA_TSD);
    LibcwDout(LIBCWD_DEBUGCHANNELS, debug_object, dc::finish, "");
    ++printed_memblks;
    if (alloc->M_next_list)
      printed_memblks += alloc->M_next_list->show_alloc_list(debug_object, depth + 1, channel, filter);
  }
  return printed_memblks;
}

//=============================================================================
//
// memblk_ct methods
//

inline memblk_info_ct::memblk_info_ct(appblock const* s, size_t sz,
    memblk_types_nt f, struct timeval const& t LIBCWD_COMMA_TSD_PARAM LIBCWD_COMMA_LOCATION(location_ct const* l))
  : memblk_info_base_ct(f), a_alloc_node(new dm_alloc_ct(s, sz, f, t LIBCWD_COMMA_TSD LIBCWD_COMMA_LOCATION(l))) { }	// MT-safe: write lock is set.

// Used for invisible memory blocks:
inline memblk_info_ct::memblk_info_ct(memblk_types_nt f) : memblk_info_base_ct(f), a_alloc_node(NULL) { }

void memblk_key_ct::printOn(std::ostream& os) const
{
  _private_::no_alloc_ostream_ct no_alloc_ostream(os);
  no_alloc_ostream << "{ a_start = " << a_start << ", a_end = " << a_end << " (size = " << size() << ") }";
}

void memblk_info_ct::printOn(std::ostream& os) const
{
  _private_::no_alloc_ostream_ct no_alloc_ostream(os);
  no_alloc_ostream << "{ alloc_node = { owner = " << a_alloc_node.is_owner() << ", locked = " << a_alloc_node.strict_owner()
      << ", px = " << a_alloc_node.get() << "\n\t( = " << *a_alloc_node.get() << " ) }";
}

bool memblk_info_ct::erase(bool owner LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_DEBUGM_ASSERT(__libcwd_tsd.internal);
  dm_alloc_ct* ap = a_alloc_node.get();
#if CWDEBUG_DEBUGM
  if (ap)
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: memblk_info_ct::erase for " << ap->start() );
#endif
  if (ap && ap->next_list())
  {
    if (owner)
      a_alloc_node.release();
    memblk_types_nt new_flag = ap->memblk_type();
    switch(new_flag)
    {
      case memblk_type_new:
	new_flag = memblk_type_deleted;
	break;
      case memblk_type_new_array:
        new_flag = memblk_type_deleted_array;
	break;
      case memblk_type_malloc:
      case memblk_type_realloc:
      case memblk_type_posix_memalign:
      case memblk_type_memalign:
      case memblk_type_valloc:
      case memblk_type_external:
	new_flag = memblk_type_freed;
	break;
#if CWDEBUG_MARKER
      case memblk_type_marker:
	new_flag = memblk_type_deleted_marker;
	break;
      case memblk_type_deleted_marker:
#endif
      case memblk_type_deleted:
      case memblk_type_deleted_array:
      case memblk_type_freed:
	DoutFatalInternal( dc::core, "Deleting a memblk_info_ct twice ?" );
    }
    ap->change_flags(new_flag);
    return true;
  }
  return false;
}

void memblk_info_ct::make_invisible(void)
{
#if LIBCWD_THREAD_SAFE && CWDEBUG_DEBUGOUTPUT
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUG_ASSERT((*__libcwd_tsd.thread_iter).thread_mutex.is_locked()); // MT-safe: write lock is set (needed for ~dm_alloc_ct).
#endif
  LIBCWD_ASSERT( a_alloc_node.strict_owner() );

  if (a_alloc_node.get()->next_list())
#if LIBCWD_THREAD_SAFE
    DoutFatal(dc::core, "Trying to make a memory block invisible that has allocation \"children\" (like a marker has).  "
	"Did you call 'make_invisible' for an allocation that was allocated by another thread?");
#else
    DoutFatal(dc::core, "Trying to make a memory block invisible that has allocation \"children\" (like a marker has).");
#endif

  a_alloc_node.reset(); 
}

// A dummy struct.  We'll only ever use 'appblock*'.
struct appblock {
  char appblock_id;
};

#if CWDEBUG_MAGIC

// Assertions, used to make sure we don't accidently use the wrong pointer (both NOOPS):
#define ASSERT_APPBLOCK(ptr2) ((void)((ptr2)->appblock_id), ptr2)
#define ASSERT_PREZONE(ptr1) ((void)((ptr1)->size), ptr1)

// The characters used to fill the red zones.
static int const prezone_char = 0x9a;
static int const postzone_char = 0xa9;

// Memory is allocated as follows:
//
// Pointer returned by libc's malloc (ptr1)
// |<-------------- allocated size (as) ------------------->
// v        <---- requested size (rs) --->
// [prezone][block returned to allocation][OFFSET][postzone]
// ^        ^                                     ^
// ptr1     ptr2                                  3
// Addresses at 1, 2 and 3 will be aligned to sizeof(size_t) boundaries.
// If the address returned by malloc is aligned on an 8 byte boundary,
// then so are the addresses 1 and 2.
//
// In order to make the code in this file more robust, we
// use the following convention (inside the functions:
// internal_malloc, internal_free, __libcwd_malloc, __libcwd_calloc
// __libcwd_realloc, operator delete, operator new, operator new[],
// operator delete and operator delete[]):
//
// Pointer 1 is written as ptr1 has always type 'prezone*'.
// Pointer 2 is written as ptr2 and has type 'appblock*'
// (where appblock is a dummy class).
//
// The size of OFFSET (in bytes) is thus a function of rs (in bytes) (2 OPS):
// This uses the fact that -rs == ~rs + 1 in twos complement.
#define OFFSET(rs) (-(rs) & (sizeof(size_t) - 1))
// The postzone offset is then rs + OFFSET(rs), which can be written as (2 OPS):
#define RS_OFFSET(rs) ((sizeof(size_t) - 1 + (rs)) & ~(sizeof(size_t) - 1))
// The to-be-allocated size (as) is therefore (3 OPS):
#define REAL_SIZE(rs) (sizeof(prezone) + sizeof(postzone) + RS_OFFSET(rs))
// The prezone structure can be reached with (NOOP):
#define PREZONE(ptr1) ASSERT_PREZONE(ptr1)[0]
// The 'size' field in the prezone structure is filled with RS_OFFSET + OFFSET
// for efficiency reasons (see SET_MAGIC). That means that the last three bits
// are equal to OFFSET, while the higher bits are equal to RS_OFFSET.
// Thus, to get the OFFSET back from a prezone pointer (1 OP):
#define ZONE2OFFSET(ptr1) (PREZONE(ptr1).size & (sizeof(size_t) - 1))
// And to get RS_OFFSET back from a prezone pointer (1 OP):
#define ZONE2RS_OFFSET(ptr1) (PREZONE(ptr1).size & ~(sizeof(size_t) - 1))
// In order to get back the REAL_SIZE from a ptr1, we need therefore (2 OPS):
#define ZONE2REAL_SIZE(ptr1) (sizeof(prezone) + sizeof(postzone) + ZONE2RS_OFFSET(ptr1))
// And the requested size (3 OPS):
#define ZONE2RS(ptr1) (ZONE2RS_OFFSET(ptr1) - ZONE2OFFSET(ptr1))
// The postzone structure can be reached as follows (3 OPS):
#define POSTZONE(ptr1) reinterpret_cast<postzone*>((char*)ptr1 + ZONE2REAL_SIZE(ptr1))[-1]
// Convert from one pointer to the other (both 1 OP):
#define ZONE2APP(ptr1) (reinterpret_cast<appblock*>(ASSERT_PREZONE(ptr1) + 1))
#define APP2ZONE(ptr2) (reinterpret_cast<prezone*>(ASSERT_APPBLOCK(ptr2)) - 1)
// Set the 'size' field, magic numbers and zone.
#define SET_MAGIC(ptr1, rs, magic_begin, magic_end) \
  do { \
    size_t offset = OFFSET(rs); \
    PREZONE(ptr1).magic = magic_begin; \
    PREZONE(ptr1).size = RS_OFFSET(rs) + offset; \
    POSTZONE(ptr1).magic = magic_end; \
    FILLREDZONES(ptr1, offset); \
  } while(0)
// Return true if the magic is corrupt.
#define CHECK_MAGIC_BEGIN(ptr1, magic_begin) \
  (PREZONE(ptr1).magic != magic_begin || CHECK_REDZONE_BEGIN(ptr1))
#define CHECK_MAGIC_END(ptr1, magic_end) \
  (POSTZONE(ptr1).magic != magic_end || CHECK_REDZONE_END(ptr1))
#define CHECK_MAGIC(ptr1, magic_begin, magic_end) \
  (CHECK_MAGIC_BEGIN(ptr1, magic_begin) || CHECK_MAGIC_END(ptr1, magic_end))

static size_t offsetfill;
static size_t offsetmask[sizeof(size_t)];
#define INITOFFSETZONE \
  do { \
    std::memset(&offsetfill, postzone_char, sizeof(offsetfill)); \
    for (unsigned int offset = 0; offset < sizeof(size_t); ++offset) \
    { \
      offsetmask[offset] = ~(size_t)0; \
      char* p = reinterpret_cast<char*>(&offsetmask[offset]); \
      for (unsigned int cnt = 0; cnt < sizeof(size_t) - offset; ++cnt, ++p) \
        *p = 0; \
    } \
  } while(0)
#if LIBCWD_REDZONE_BLOCKS > 0
static size_t preredzone[redzone_size];
static size_t postredzone[redzone_size + 1];	// One extra for the OFFSET.
#define FILLREDZONES(ptr1, offset) \
  std::memset(PREZONE(ptr1).redzone, prezone_char, sizeof(preredzone)); \
  std::memset((char*)POSTZONE(ptr1).redzone - offset, postzone_char, sizeof(size_t) * redzone_size + offset);
#define INITREDZONES \
  do { \
    std::memset(preredzone, prezone_char, sizeof(preredzone)); \
    std::memset(postredzone, postzone_char, sizeof(postredzone)); \
    INITOFFSETZONE; \
  } while(0)
#define CHECK_REDZONE_BEGIN(ptr1) \
  (std::memcmp(PREZONE(ptr1).redzone, preredzone, sizeof(preredzone)))
#define CHECK_REDZONE_END(ptr1) \
  (std::memcmp((char*)POSTZONE(ptr1).redzone - ZONE2OFFSET(ptr1), postredzone, \
      sizeof(size_t) * redzone_size + ZONE2OFFSET(ptr1)))
#else
#define FILLREDZONES(ptr1, offset) \
  do { \
    if (offset) \
    { \
      size_t* ptr = reinterpret_cast<size_t*>(&POSTZONE(ptr1)) - 1; \
      size_t value = *ptr; \
      size_t mask = offsetmask[offset]; \
      *ptr = (value & ~mask) | (offsetfill & mask); \
    } \
  } while(0)
#define INITREDZONES INITOFFSETZONE
#define CHECK_REDZONE_BEGIN(ptr1) false
#define CHECK_REDZONE_END(ptr1) \
  ((reinterpret_cast<size_t*>(&POSTZONE(ptr1))[-1] & offsetmask[ZONE2OFFSET(ptr1)]) != \
   (offsetfill & offsetmask[ZONE2OFFSET(ptr1)]))
#endif

size_t const INTERNAL_MAGIC_NEW_BEGIN = 0x7af45b1c;
size_t const INTERNAL_MAGIC_NEW_END = 0x3b9f018a;
size_t const INTERNAL_MAGIC_NEW_ARRAY_BEGIN = 0xf101cc33;
size_t const INTERNAL_MAGIC_NEW_ARRAY_END = 0x60fa30e2;
size_t const INTERNAL_MAGIC_MALLOC_BEGIN = 0xcf218aa3;
size_t const INTERNAL_MAGIC_MALLOC_END = 0x81a2bea9;
size_t const MAGIC_NEW_BEGIN = 0x4b28ca20;
size_t const MAGIC_NEW_END = 0x585babe0;
size_t const MAGIC_NEW_ARRAY_BEGIN = 0x83d14701;
size_t const MAGIC_NEW_ARRAY_END = 0x31415927;
size_t const MAGIC_MALLOC_BEGIN = 0xf4c433a1;
size_t const MAGIC_MALLOC_END = 0x335bc0fa;
size_t const MAGIC_POSIX_MEMALIGN_BEGIN = 0xb3f80179;
size_t const MAGIC_POSIX_MEMALIGN_END = 0xac0a6548;
size_t const MAGIC_MEMALIGN_BEGIN = 0x4ee299af;
size_t const MAGIC_MEMALIGN_END = 0x0e60f529;
size_t const MAGIC_VALLOC_BEGIN = 0x24756590;
size_t const MAGIC_VALLOC_END = 0xd2d8a14f;

char const* diagnose_from(deallocated_from_nt from, bool internal, bool visible = true)
{
  switch(from)
  {
    case from_free:
      return internal ?
	  "You are 'free()'-ing a pointer with alloc checking OFF ('internal' allocation) (" :
          visible ? "You are 'free()'-ing a pointer (" : "You are 'free()'-ing an invisible memory block (at ";
    case from_delete:
      return internal ?
	  "You are 'delete'-ing a pointer with alloc checking OFF ('internal' allocation) (" :
          visible ? "You are 'delete'-ing a pointer (" : "You are 'delete'-ing an invisible memory block (at ";
    case from_delete_array:
      return internal ?
	  "You are 'delete[]'-ing a pointer with alloc checking OFF ('internal' allocation) (" :
          visible ? "You are 'delete[]'-ing a pointer (" : "You are 'delete[]'-ing an invisible memory block (at ";
    case error:
      break;
  }
  return "Huh? Bug in libcwd!";
}

#define REST_OF_EXPLANATION ") that is a libcwd internal allocation "

char const* diagnose_magic(prezone const* ptr1, size_t expected_magic_begin, size_t expected_magic_end)
{
  size_t magic_begin = PREZONE(ptr1).magic;
  size_t* magic_end = &POSTZONE(ptr1).magic;
  if (magic_begin == expected_magic_begin)
  {
    if (CHECK_MAGIC_BEGIN(ptr1, expected_magic_begin))
    {
      switch(magic_begin)
      {
	case INTERNAL_MAGIC_NEW_BEGIN:
        case INTERNAL_MAGIC_NEW_ARRAY_BEGIN:
	case INTERNAL_MAGIC_MALLOC_BEGIN:
	  return REST_OF_EXPLANATION "with a corrupt redzone prefix (you wrote to a pointer that was already freed?)!";
        case MAGIC_NEW_BEGIN:
        case MAGIC_NEW_ARRAY_BEGIN:
        case MAGIC_MALLOC_BEGIN:
	case MAGIC_POSIX_MEMALIGN_BEGIN:
	case MAGIC_MEMALIGN_BEGIN:
	case MAGIC_VALLOC_BEGIN:
	  return ") with a corrupt redzone prefix (buffer underrun?)!";
      }
    }
    if (*magic_end == expected_magic_end)
    {
      if (CHECK_MAGIC_END(ptr1, expected_magic_end))
      {
        switch(*magic_end)
	{
	  case INTERNAL_MAGIC_NEW_END:
	  case INTERNAL_MAGIC_NEW_ARRAY_END:
	  case INTERNAL_MAGIC_MALLOC_END:
	    return REST_OF_EXPLANATION "with a corrupt redzone postfix (you wrote to a pointer that was already freed?)!";
	  case MAGIC_NEW_END:
	  case MAGIC_NEW_ARRAY_END:
	  case MAGIC_MALLOC_END:
	  case MAGIC_POSIX_MEMALIGN_END:
	  case MAGIC_MEMALIGN_END:
	  case MAGIC_VALLOC_END:
	    return ") with a corrupt redzone postfix (buffer overrun?)!";
	}
      }
    }
  }
  switch(magic_begin)
  {
    case INTERNAL_MAGIC_NEW_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_NEW_END)
        return REST_OF_EXPLANATION "(allocated with 'new').";
      break;
    case INTERNAL_MAGIC_NEW_ARRAY_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_NEW_ARRAY_END)
        return REST_OF_EXPLANATION "(allocated with 'new[]).";
      break;
    case INTERNAL_MAGIC_MALLOC_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_MALLOC_END)
        return REST_OF_EXPLANATION "(allocated with 'malloc()').";
      break;
    case MAGIC_NEW_BEGIN:
      if (*magic_end == MAGIC_NEW_END)
        return ") that was allocated with 'new'.  Use 'delete' instead.";
      break;
    case MAGIC_NEW_ARRAY_BEGIN:
      if (*magic_end == MAGIC_NEW_ARRAY_END)
        return ") that was allocated with 'new[]'.  Use 'delete[]' instead";
      break;
    case MAGIC_MALLOC_BEGIN:
      if (*magic_end == MAGIC_MALLOC_END)
        return ") that was allocated with 'malloc()'.  Use 'free' instead.";
      break;
    case MAGIC_POSIX_MEMALIGN_BEGIN:
      if (*magic_end == MAGIC_POSIX_MEMALIGN_END)
        return ") that was allocated with 'posix_memalign()'.  Use 'free' instead.";
      break;
    case MAGIC_MEMALIGN_BEGIN:
      if (*magic_end == MAGIC_MEMALIGN_END)
        return ") that was allocated with 'memalign()'.  Use 'free' instead.";
      break;
    case MAGIC_VALLOC_BEGIN:
      if (*magic_end == MAGIC_VALLOC_END)
        return ") that was allocated with 'valloc()'.  Use 'free' instead.";
      break;
    default:
      switch(magic_begin)
      {
        case ~INTERNAL_MAGIC_NEW_BEGIN:
	  return ") which appears to be a deleted block that was originally a libcwd internal allocation (allocated with 'new').";
        case ~INTERNAL_MAGIC_NEW_ARRAY_BEGIN:
	  return ") which appears to be a deleted block that was originally a libcwd internal allocation (allocated with 'new[]).";
	case ~INTERNAL_MAGIC_MALLOC_BEGIN:
	  return ") which appears to be a deleted block that was originally a libcwd internal allocation (allocated with 'malloc()).";
        case ~MAGIC_NEW_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'new'.";
        case ~MAGIC_NEW_ARRAY_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'new[]'.";
        case ~MAGIC_MALLOC_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'malloc()'.";
        case ~MAGIC_POSIX_MEMALIGN_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'posix_memalign()'.";
        case ~MAGIC_MEMALIGN_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'memalign()'.";
        case ~MAGIC_VALLOC_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'valloc()'.";
      }
      return ") which seems uninitialized (already deleted?), or has a corrupt first magic number (buffer underrun?)!";
  }
  switch (*magic_end)
  {
    case INTERNAL_MAGIC_NEW_END:
    case INTERNAL_MAGIC_NEW_ARRAY_END:
    case INTERNAL_MAGIC_MALLOC_END:
    case MAGIC_NEW_END:
    case MAGIC_NEW_ARRAY_END:
    case MAGIC_MALLOC_END:
    case MAGIC_POSIX_MEMALIGN_END:
    case MAGIC_MEMALIGN_END:
    case MAGIC_VALLOC_END:
    case ~INTERNAL_MAGIC_NEW_END:
    case ~INTERNAL_MAGIC_NEW_ARRAY_END:
    case ~INTERNAL_MAGIC_MALLOC_END:
    case ~MAGIC_NEW_END:
    case ~MAGIC_NEW_ARRAY_END:
    case ~MAGIC_MALLOC_END:
    case ~MAGIC_POSIX_MEMALIGN_END:
    case ~MAGIC_MEMALIGN_END:
    case ~MAGIC_VALLOC_END:
      return ") with inconsistent magic numbers!";
  }
  return ") with a corrupt second magic number (buffer overrun?)!";
}
#else // !CWDEBUG_MAGIC
// Dummy class
struct prezone {
  char prezone_id;
};
// Convert from one pointer to the other.
#define ZONE2APP(ptr1) (reinterpret_cast<appblock*>(ASSERT_PREZONE(ptr1)))
#define APP2ZONE(ptr2) (reinterpret_cast<prezone*>(ASSERT_APPBLOCK(ptr2)))
// Assertions, used to make sure we don't accidently use the wrong pointer.
#define ASSERT_APPBLOCK(ptr2) ((void)((ptr2)->appblock_id), ptr2)
#define ASSERT_PREZONE(ptr1) ((void)((ptr1)->prezone_id), ptr1)
// Size doesn't change.
#define REAL_SIZE(rs) (rs)
#endif // !CWDEBUG_MAGIC
// Used to print a pointer.
#define PRINT_PTR(ptr2) (void*)ASSERT_APPBLOCK(ptr2)

#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
#define LIBCWD_DEBUGM_OPT(x) x
#define LIBCWD_COMMA_DEBUGM_OPT(x) , x
#else
#define LIBCWD_DEBUGM_OPT(x)
#define LIBCWD_COMMA_DEBUGM_OPT(x)
#endif
#if CWDEBUG_LOCATION
#define LIBCWD_COMMA_LOCATION_OPT(x) , x
#else
#define LIBCWD_COMMA_LOCATION_OPT(x)
#endif
#if CWDEBUG_LOCATION && !CWDEBUG_DEBUGT
#define LIBCWD_LOCATION_OPT(x) x
#else
#define LIBCWD_LOCATION_OPT(x)
#endif

void (*backtrace_hook)(void** buffer, int frames LIBCWD_COMMA_TSD_PARAM);

//=============================================================================
//
// internal_malloc
//
// Allocs a new block of size `size' and updates the internal administration.
//
// Note: This function is called by `__libcwd_malloc', `__libcwd_calloc' and
// `operator new' which end with a call to DoutInternal( dc_malloc|continued_cf, ...)
// and should therefore end with a call to DoutInternal( dc::finish, ptr ).
// This function can also be called from `posix_memalign', `memalign' or `valloc',
// in which case `alignment' is set to a non-zero value.
//
static appblock* internal_malloc(size_t size, memblk_types_nt flag
    LIBCWD_COMMA_LOCATION_OPT(void* call_addr) LIBCWD_COMMA_TSD_PARAM
    LIBCWD_COMMA_DEBUGM_OPT(int saved_marker), size_t alignment = 0)
{
  if (WST_initialization_state <= 0)		// Only true prior to initialization of std::ios_base::Init.
  {
#if CWDEBUG_DEBUG
    bool continued_debug_output = (__libcwd_tsd.library_call == 0 && LIBCWD_DO_TSD_MEMBER_OFF(libcw_do) < 0);
#endif
    init_debugmalloc();
#if CWDEBUG_DEBUG
    // It is possible that libcwd is not initialized at this point,  LIBCWD_DO_TSD_MEMBER_OFF(libcw_do) == 0 (turned off)
    // and thus no unfinished debug output was printed before entering this function.
    // Initialization of libcwd with CWDEBUG_DEBUG set turns on libcwd_do.  In order to balance the
    // continued stack, we print an unfinished debug message here.
    if (continued_debug_output != (__libcwd_tsd.library_call == 0 && LIBCWD_DO_TSD_MEMBER_OFF(libcw_do) < 0))
      DoutInternal( dc_malloc|continued_cf, "internal_malloc(" << size << ", " << flag << ") = " );
#endif
  }

  prezone* ptr1;
  size_t real_size;
  if (alignment == 0)
  {
    real_size = REAL_SIZE(size);
#if CWDEBUG_MAGIC
    if (size > real_size)	// Overflow?
    {
      DoutInternal(dc::finish, "NULL" LIBCWD_DEBUGM_OPT(" [" << saved_marker << ']'));
      DoutInternal( dc_malloc, "Size too large: no space left for magic numbers." );
      return NULL;	// A fatal error should occur directly after this
    }
#endif // !CWDEBUG_MAGIC
    ptr1 = static_cast<prezone*>(__libc_malloc(real_size));
  }
  else
  {
#if CWDEBUG_MAGIC
    bool alignment_is_power_of_two = (alignment & (alignment - 1)) == 0;
    LIBCWD_ASSERT(alignment_is_power_of_two);
    if (alignment < sizeof(size_t))
      alignment = sizeof(size_t);
    // Reserve an extra size_t before the prezone to store the alignment offset in.
    size_t alignment_offset = (sizeof(size_t) + sizeof(prezone) + alignment - 1) & ~(alignment - 1);
    real_size = alignment_offset + RS_OFFSET(size) + sizeof(postzone);
#else
    real_size = size;
#endif
    switch (flag)
    {
#ifdef HAVE_POSIX_MEMALIGN
      case memblk_type_posix_memalign:
	if (__libc_posix_memalign((void**)(&ptr1), alignment, real_size) != 0)
	  ptr1 = NULL;
	break;
#endif
#ifdef HAVE_MEMALIGN
      case memblk_type_memalign:
        ptr1 = static_cast<prezone*>(__libc_memalign(alignment, real_size));
        break;
#endif
#ifdef HAVE_VALLOC
      case memblk_type_valloc:
        ptr1 = static_cast<prezone*>(__libc_valloc(real_size));
        break;
#endif
      default:
        /* Never happens */
	ptr1 = NULL;
        break;
    }
#if CWDEBUG_MAGIC
    if (ptr1)
    {
      // The minimum offset we need.
      appblock* ptr2 = reinterpret_cast<appblock*>((char*)ptr1 + sizeof(size_t) + sizeof(prezone));
      // And rounded up to nearest alignment.
      ptr2 = reinterpret_cast<appblock*>(((size_t)ptr2 + alignment - 1) & ~(alignment - 1));
      ptr1 = APP2ZONE(ptr2);
      // Fill in extra information field.
      reinterpret_cast<size_t*>(ptr1)[-1] = alignment_offset;
    }
#endif
  }
  if (ptr1 == NULL)
  {
    DoutInternal(dc::finish, "NULL" LIBCWD_DEBUGM_OPT(" [" << saved_marker << ']'));
    DoutInternal( dc_malloc, "Out of memory ! this is only a pre-detection!" );
    return NULL;	// A fatal error should occur directly after this
  }
  appblock* ptr2 = ZONE2APP(ptr1);

#if CWDEBUG_LOCATION
  if (__libcwd_tsd.library_call++)
    ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);	// Otherwise debug output will be generated from bfd.cc (location_ct)
  location_ct const* loc = location_cache(call_addr LIBCWD_COMMA_TSD);
  if (--__libcwd_tsd.library_call)
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
#endif

#if CWDEBUG_DEBUGM
  bool error;
#endif
  LIBCWD_DEFER_CANCEL;

  DEBUGDEBUG_CERR( "internal_malloc: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;

  // Update our administration:
  if (__libcwd_tsd.invisible)
  {
    ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
#if CWDEBUG_DEBUGM
    std::pair<memblk_map_ct::iterator, bool> const& iter =
#endif
    memblk_map_write->insert(memblk_ct(memblk_key_ct(ptr2, size), memblk_info_ct(flag)));
#if CWDEBUG_DEBUGM
    error = !iter.second;
#endif
    RELEASE_WRITE_LOCK;
  }
  else
  {
    struct timeval alloc_time;
    gettimeofday(&alloc_time, 0);
    ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
    std::pair<memblk_map_ct::iterator, bool> const&
	iter(memblk_map_write->insert(memblk_ct(memblk_key_ct(ptr2, size),
		memblk_info_ct(ptr2, size, flag, alloc_time LIBCWD_COMMA_TSD LIBCWD_COMMA_LOCATION(loc)))));
    (*iter.first).second.lock();				// Lock ownership (doesn't call malloc).
#if CWDEBUG_DEBUGM
    error = !iter.second;
#endif
    RELEASE_WRITE_LOCK;
  }

  DEBUGDEBUG_CERR( "internal_malloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;

  LIBCWD_RESTORE_CANCEL;

#if CWDEBUG_DEBUGM
  if (error)
    DoutFatalInternal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );
#endif

  // Backtrace.
  if (backtrace_hook && __libcwd_tsd.library_call == 0)
  {
    void* buffer[max_frames];
    ++__libcwd_tsd.library_call;
    ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
    int frames = backtrace(buffer, max_frames);
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
    BACKTRACE_ACQUIRE_LOCK;
    if (backtrace_hook)
      (*backtrace_hook)(buffer, frames LIBCWD_COMMA_TSD);
    BACKTRACE_RELEASE_LOCK;
    --__libcwd_tsd.library_call;
  }

  DoutInternal(dc::finish, PRINT_PTR(ptr2)
      LIBCWD_LOCATION_OPT(<< " [" << *loc << ']')
      << (__libcwd_tsd.invisible ? " (invisible)" : "")
      LIBCWD_DEBUGM_OPT(<< " [" << saved_marker << ']'));

  return ptr2;
}

#if LIBCWD_THREAD_SAFE
static bool search_in_maps_of_other_threads(void const* void_ptr, memblk_map_ct::const_iterator& iter LIBCWD_COMMA_TSD_PARAM)
{
  appblock const* ptr2 = static_cast<appblock const*>(void_ptr);
  bool found = false;
  rwlock_tct<threadlist_instance>::rdlock(true);
  // Using threadlist_t::iterator instead of threadlist_t::const_iterator because
  // we need to pass &(*thread_iter) to ACQUIRE_READ_LOCK.  This should be safe
  // because inside the critical area of a READ_LOCK we should only be reading,
  // of course, except trying to lock the target_thread->thread_mutex.  This means
  // that despite this threadlist_instance rdlock(), multiple threads may be trying
  // to write to (threadlist_element).thread_mutex in terms of trying to acquire
  // that lock.  This is of course ok.
  for(threadlist_t::iterator thread_iter = threadlist->begin(); thread_iter != threadlist->end(); ++thread_iter)
  {
    if (thread_iter == __libcwd_tsd.thread_iter)
      continue;	// Already searched.
    ACQUIRE_READ_LOCK(&(*thread_iter));
    DEBUGDEBUG_CERR("Looking inside memblk_map " << (void*)target_memblk_map_read << "(thread_ct at  " << (void*)__libcwd_tsd.target_thread << ", thread_iter at " << (void*)&thread_iter << ')');
    iter = target_memblk_map_read->find(memblk_key_ct(ptr2, 0));
    found = (iter != target_memblk_map_read->end());
    if (found)
      break;
    RELEASE_READ_LOCK;
  }
  rwlock_tct<threadlist_instance>::rdunlock();
  return found;
}
#endif

#if CWDEBUG_DEBUGM && CWDEBUG_MAGIC
#if LIBCWD_THREAD_SAFE
static pthread_mutex_t annotation_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static size_t internal_size;
static size_t max_internal_size;
static int internal_size_count;
static int sizes[100000];
static int max_size_nr[8];
static size_t max_size_nr_size[8];
static void annotation_alloc(size_t size)
{
#if LIBCWD_THREAD_SAFE
  pthread_mutex_lock(&annotation_lock);
#endif
  internal_size += size;
  if (size < 100000)
  {
    sizes[size] += 1;
    for (int i = 0; i < 8; ++i)
    {
      if (sizes[size] > max_size_nr[i])
      {
	max_size_nr[i] = sizes[size];
	int j = i;
	while (j > 0)
	{
	  if (max_size_nr[j] < max_size_nr[j - 1])
	    break;
	  int t = max_size_nr[j];
	  max_size_nr[j - 1] = max_size_nr[j];
	  max_size_nr[j] = t;
	  size_t s = max_size_nr_size[j];
	  max_size_nr_size[j - 1] = max_size_nr_size[j];
	  max_size_nr_size[j] = s;
	  --j;
	}
	DEBUGDEBUG_CERR("ANNOTATION: Got " << max_size_nr[i] << " blocks of size " << size << " (from " << (void*)__builtin_return_address(1) << ")");
	max_size_nr_size[i] = size;
      }
      if (size == max_size_nr_size[i])
        break;
    }
  }
  if (internal_size > max_internal_size || ++internal_size_count > 4000)
  {
    if (internal_size_count > 4000)
    {
      DEBUGDEBUG_CERR("ANNOTATION: Resetting internal allocation annotations.");
      memset(max_size_nr, 0 , sizeof(max_size_nr));
      memset(max_size_nr_size, 0 , sizeof(max_size_nr_size));
    }
    internal_size_count = 0;
    max_internal_size = internal_size;
    DEBUGDEBUG_CERR("ANNOTATION: max_internal_size now " << max_internal_size);
  }
#if LIBCWD_THREAD_SAFE
  pthread_mutex_unlock(&annotation_lock);
#endif
}

static void annotation_free(size_t size)
{
#if LIBCWD_THREAD_SAFE
  pthread_mutex_lock(&annotation_lock);
#endif
  internal_size -= size;
  if (size < 100000)
    sizes[size] -= 1;
#if LIBCWD_THREAD_SAFE
  pthread_mutex_unlock(&annotation_lock);
#endif
}
#endif

#if LIBCWD_THREAD_SAFE
static void delete_zombie(_private_::thread_ct* zombie_thread LIBCWD_COMMA_TSD_PARAM)
{
  rwlock_tct<threadlist_instance>::wrlock();
  memblk_map_ct* memblk_map = reinterpret_cast<memblk_map_ct*>(zombie_thread->memblk_map);
  zombie_thread->memblk_map = NULL;
  delete memblk_map;
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  set_alloc_checking_off(__libcwd_tsd);
  for(threadlist_t::iterator thread_iter = threadlist->begin(); thread_iter != threadlist->end(); ++thread_iter)
  {
    if (&(*thread_iter) == zombie_thread)
    {
      threadlist->erase(thread_iter);
      break;
    }
  }
  set_alloc_checking_on(__libcwd_tsd);
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;
  rwlock_tct<threadlist_instance>::wrunlock();
}
#endif

static void internal_free(appblock* ptr2, deallocated_from_nt from LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_DEBUGM_ASSERT(__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal);
#if CWDEBUG_DEBUGM && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC) && LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT(_private_::WST_ios_base_initialized || __libcwd_tsd.internal);
#endif
  if (__libcwd_tsd.internal)
  {
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
    if (from == from_delete)
    {
      LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal `delete(" << PRINT_PTR(ptr2) << ")' [" << ++__libcwd_tsd.marker << ']' );
    }
    else if (from == from_delete_array)
    {
      LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal `delete[](" << PRINT_PTR(ptr2) << ")' [" << ++__libcwd_tsd.marker << ']' );
    }
    else
    {
      LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal `free(" << PRINT_PTR(ptr2) << ")' [" << ++__libcwd_tsd.marker << ']' );
    }
#endif // CWDEBUG_DEBUGM
#if CWDEBUG_MAGIC
    if (!ptr2)
      return;
    prezone* ptr1 = APP2ZONE(ptr2);
    if (from == from_delete)
    {
      if (CHECK_MAGIC(ptr1, INTERNAL_MAGIC_NEW_BEGIN, INTERNAL_MAGIC_NEW_END))
        DoutFatalInternal(dc::core, "internal delete: " << diagnose_from(from, true) <<
	    PRINT_PTR(ptr2) << diagnose_magic(ptr1, INTERNAL_MAGIC_NEW_BEGIN, INTERNAL_MAGIC_NEW_END));
    }
    else if (from == from_delete_array)
    {
      if (CHECK_MAGIC(ptr1, INTERNAL_MAGIC_NEW_ARRAY_BEGIN, INTERNAL_MAGIC_NEW_ARRAY_END))
        DoutFatalInternal(dc::core, "internal delete[]: " << diagnose_from(from, true) <<
	    PRINT_PTR(ptr2) << diagnose_magic(ptr1, INTERNAL_MAGIC_NEW_ARRAY_BEGIN, INTERNAL_MAGIC_NEW_ARRAY_END));
    }
    else
    {
      if (CHECK_MAGIC(ptr1, INTERNAL_MAGIC_MALLOC_BEGIN, INTERNAL_MAGIC_MALLOC_END))
        DoutFatalInternal(dc::core, "internal free: " << diagnose_from(from, true) <<
	    PRINT_PTR(ptr2) << diagnose_magic(ptr1, INTERNAL_MAGIC_MALLOC_BEGIN, INTERNAL_MAGIC_MALLOC_END));
    }
    PREZONE(ptr1).magic ^= (size_t)-1;
    POSTZONE(ptr1).magic ^= (size_t)-1;
#if CWDEBUG_DEBUGM
    if (!__libcwd_tsd.annotation)
    {
      __libcwd_tsd.annotation = 1;
      annotation_free(ZONE2RS(ptr1));
      __libcwd_tsd.annotation = 0;
    }
#endif
#endif // CWDEBUG_MAGIC
    __libc_free(APP2ZONE(ptr2));
    return;
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
  if (!ptr2)
  {
    DoutInternal( dc_malloc, "Trying to free NULL - ignored" LIBCWD_DEBUGM_OPT(" [" << ++__libcwd_tsd.marker << "]") "." );
    --__libcwd_tsd.inside_malloc_or_free;
    return;
  }

#if LIBCWD_THREAD_SAFE
  LIBCWD_DEFER_CANCEL_NO_BRACE;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr2, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr2, 0)));
#endif
  bool found = (iter != target_memblk_map_read->end() && (*iter).first.start() == ptr2);
#if LIBCWD_THREAD_SAFE
  bool found_in_current_thread = found;
  if (!found)
  {
    RELEASE_READ_LOCK;
    // The following will acquire a lock in another target thread if the
    // ptr is found, when the ptr is not found then no lock will be set.
    found = search_in_maps_of_other_threads(ptr2, iter, __libcwd_tsd);
  }
#endif
  if (!found
#if LIBCWD_THREAD_SAFE
      || (*iter).first.start() != ptr2
#endif
    )
  {
    if (found)			// We found the wrong block (different start pointer).
      RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
#if CWDEBUG_MAGIC
    prezone* ptr1 = APP2ZONE(ptr2);
    if (PREZONE(ptr1).magic == INTERNAL_MAGIC_NEW_BEGIN ||
        PREZONE(ptr1).magic == INTERNAL_MAGIC_NEW_ARRAY_BEGIN ||
	PREZONE(ptr1).magic == INTERNAL_MAGIC_MALLOC_BEGIN)
    {
      size_t expected_magic_begin, expected_magic_end;
      if (from == from_delete)
      {
	expected_magic_begin = MAGIC_NEW_BEGIN;
	expected_magic_end = MAGIC_NEW_END;
      }
      else if (from == from_free)
      {
	expected_magic_begin = MAGIC_MALLOC_BEGIN;
	expected_magic_end = MAGIC_MALLOC_END;
      }	
      else
      {
	expected_magic_begin = MAGIC_NEW_ARRAY_BEGIN;
	expected_magic_end = MAGIC_NEW_ARRAY_END;
      }
      DoutFatalInternal(dc::core, "Trying to " <<
          ((from == from_delete) ? "delete" : ((from == from_free) ? "free" : "delete[]")) <<
	  " a pointer (" << PRINT_PTR(ptr2) <<
	  ") that appears to be a libcwd internal allocation!  The magic number diagnostic gives: " <<
	  diagnose_from(from, false) << PRINT_PTR(ptr2) << diagnose_magic(ptr1, expected_magic_begin, expected_magic_end));
    }
#endif
    DoutFatalInternal(dc::core, "Trying to " <<
        ((from == from_delete) ? "delete" : ((from == from_free) ? "free" : "delete[]")) <<
	" an invalid pointer (" << PRINT_PTR(ptr2) << ')' );
  }
  else
  {
    dm_alloc_ct* alloc_node = 0;	// Only used when `visible' is true.  Set to 0 in order to avoid compiler warning.
    memblk_types_nt f = (*iter).second.flags();
    if ((*iter).second.is_watched())
      raise(SIGTRAP);
    bool const visible = (!__libcwd_tsd.library_call && (*iter).second.has_alloc_node());
    if (visible)
    {
      // Needed to print description later on.
      alloc_node = (*iter).second.release_alloc_node();
    }
    if (expected_from[f] != from)
    {
      RELEASE_READ_LOCK;
      if (visible)
      {
	DoutInternal( dc_malloc|continued_cf,
	    ((from == from_free) ? "free(" : ((from == from_delete) ? "delete " : "delete[] "))
	    << PRINT_PTR(ptr2) << ((from == from_free) ? ") " : " ") );
	if (channels::dc_malloc.is_on(LIBCWD_TSD))
	  alloc_node->print_description(libcw_do, default_ooam_filter LIBCWD_COMMA_TSD);
	DoutInternal(dc::continued, " " LIBCWD_DEBUGM_OPT("[" << ++__libcwd_tsd.marker << "] "));
      }
      if (from == from_delete)
      {
	if (f == memblk_type_malloc)
	  DoutFatalInternal( dc::core, "You are `delete'-ing a block that was allocated with `malloc()'!  Use `free()' instead." );
	else if (f == memblk_type_external)
	  DoutFatalInternal( dc::core, "You are `delete'-ing a block that was externally allocated!  Use `free()' instead." );
	else if (f == memblk_type_realloc)
	  DoutFatalInternal( dc::core, "You are `delete'-ing a block that was returned by `realloc()'!  Use `free()' instead." );
	else if (f == memblk_type_new_array)
	  DoutFatalInternal( dc::core, "You are `delete'-ing a block that was allocated with `new[]'!  Use `delete[]' instead." );
        else if (f == memblk_type_posix_memalign)
	  DoutFatalInternal( dc::core, "You are `delete'-ing a block that was allocated with `posix_memalign()'!  Use `free()' instead." );
        else if (f == memblk_type_memalign)
	  DoutFatalInternal( dc::core, "You are `delete'-ing a block that was allocated with `memalign()'!  Use `free()' instead." );
        else if (f == memblk_type_valloc)
	  DoutFatalInternal( dc::core, "You are `delete'-ing a block that was allocated with `valloc()'!  Use `free()' instead." );
      }
      else if (from == from_delete_array)
      {
	if (f == memblk_type_malloc)
	  DoutFatalInternal( dc::core, "You are `delete[]'-ing a block that was allocated with `malloc()'!  Use `free()' instead." );
	else if (f == memblk_type_external)
	  DoutFatalInternal( dc::core, "You are `delete[]'-ing a block that externally allocated!  Use `free()' instead." );
	else if (f == memblk_type_realloc)
	  DoutFatalInternal( dc::core, "You are `delete[]'-ing a block that was returned by `realloc()'!  Use `free()' instead." );
	else if (f == memblk_type_new)
	  DoutFatalInternal( dc::core, "You are `delete[]'-ing a block that was allocated with `new'!  Use `delete' instead." );
        else if (f == memblk_type_posix_memalign)
	  DoutFatalInternal( dc::core, "You are `delete[]'-ing a block that was allocated with `posix_memalign()'!  Use `free()' instead." );
        else if (f == memblk_type_memalign)
	  DoutFatalInternal( dc::core, "You are `delete[]'-ing a block that was allocated with `memalign()'!  Use `free()' instead." );
        else if (f == memblk_type_valloc)
	  DoutFatalInternal( dc::core, "You are `delete[]'-ing a block that was allocated with `valloc()'!  Use `free()' instead." );
      }
      else if (from == from_free)
      {
	if (f == memblk_type_new)
	  DoutFatalInternal( dc::core, "You are `free()'-ing a block that was allocated with `new'!  Use `delete' instead." );
	else if (f == memblk_type_new_array)
	  DoutFatalInternal( dc::core, "You are `free()'-ing a block that was allocated with `new[]'!  Use `delete[]' instead." );
      }
      DoutFatalInternal( dc::core, "Huh? Bug in libcwd" );
    }

    DEBUGDEBUG_CERR( "__libcwd_free: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
    __libcwd_tsd.internal = 1;

#if CWDEBUG_MAGIC
    bool has_alloc_node = (*iter).second.has_alloc_node();	// Needed for diagnostic message below.
    // Determine the offset between the real ptr1 and ptr2.
    size_t alignment_offset;
    if (f >= memblk_type_posix_memalign)	// posix_memalign, memalign or valloc.
      alignment_offset = reinterpret_cast<size_t*>(APP2ZONE(ptr2))[-1];
    else
      alignment_offset = sizeof(prezone);	// Normal.
#endif

    // Convert lock into a writers lock without allowing other writers to start.
    // First unlocking the reader and then locking a writer would make it possible
    // that the application would crash when two threads try to free simultaneously
    // the same memory block (instead of detecting that and exiting gracefully).
    ACQUIRE_READ2WRITE_LOCK;
    bool keep = (*memblk_iter_write).second.erase(!visible LIBCWD_COMMA_TSD);	// Update flags and optional decouple
    if (visible && !keep)
      alloc_node->deinit(LIBCWD_TSD);						// Perform deinitialization that needs lock.
    target_memblk_map_write->erase(memblk_iter_write);				// Update administration
#if LIBCWD_THREAD_SAFE
    _private_::thread_ct* zombie_thread = __libcwd_tsd.target_thread;
    bool can_delete_zombie = !found_in_current_thread && zombie_thread->is_zombie() && target_memblk_map_write->size() == 0;
#endif
    RELEASE_WRITE_LOCK;
#if LIBCWD_THREAD_SAFE
    if (can_delete_zombie)
      delete_zombie(zombie_thread, __libcwd_tsd);
#endif

    DEBUGDEBUG_CERR( "__libcwd_free: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
    __libcwd_tsd.internal = 0;

    LIBCWD_RESTORE_CANCEL_NO_BRACE;

    if (visible)
    {
      DoutInternal( dc_malloc|continued_cf,
	  ((from == from_free) ? "free(" : ((from == from_delete) ? "delete " : "delete[] "))
	  << PRINT_PTR(ptr2) << ((from == from_free) ? ") " : " ") );
      if (channels::dc_malloc.is_on(LIBCWD_TSD))
	alloc_node->print_description(libcw_do, default_ooam_filter LIBCWD_COMMA_TSD);
      if (!keep)
      {
	DEBUGDEBUG_CERR( "__libcwd_free: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
	__libcwd_tsd.internal = 1;
	delete alloc_node;
	DEBUGDEBUG_CERR( "__libcwd_free: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
	__libcwd_tsd.internal = 0;
      }
      DoutInternal(dc::continued, " " LIBCWD_DEBUGM_OPT("[" << ++__libcwd_tsd.marker << "] "));
    }

#if CWDEBUG_MAGIC
    prezone* ptr1 = APP2ZONE(ptr2);
    if (f != memblk_type_external)
    {
      size_t magic_begin, magic_end;
      if (from == from_delete)
      {
        magic_begin = MAGIC_NEW_BEGIN;
	magic_end = MAGIC_NEW_END;
      }
      else if (from == from_delete_array)
      {
        magic_begin = MAGIC_NEW_ARRAY_BEGIN;
	magic_end = MAGIC_NEW_ARRAY_END;
      }
      else if (f == memblk_type_malloc || f == memblk_type_realloc)
      {
	magic_begin = MAGIC_MALLOC_BEGIN;
	magic_end = MAGIC_MALLOC_END;
      }
      else if (f == memblk_type_posix_memalign)
      {
        magic_begin = MAGIC_POSIX_MEMALIGN_BEGIN;
        magic_end = MAGIC_POSIX_MEMALIGN_END;
      }
      else if (f == memblk_type_memalign)
      {
        magic_begin = MAGIC_MEMALIGN_BEGIN;
        magic_end = MAGIC_MEMALIGN_END;
      }
      else // if (f == memblk_type_valloc)
      {
        magic_begin = MAGIC_VALLOC_BEGIN;
        magic_end = MAGIC_VALLOC_END;
      }
      if (CHECK_MAGIC(ptr1, magic_begin, magic_end))
	DoutFatalInternal(dc::core, diagnose_from(from, false, has_alloc_node) <<
	    PRINT_PTR(ptr2) << diagnose_magic(ptr1, magic_begin, magic_end));
      PREZONE(ptr1).magic ^= (size_t)-1;
      POSTZONE(ptr1).magic ^= (size_t)-1;
    }
    __libc_free((char*)ptr2 - alignment_offset);	// Free memory block
#else
    __libc_free(APP2ZONE(ptr2));			// Free memory block
#endif // CWDEBUG_MAGIC

    if (visible)
      DoutInternal( dc::finish, "" );
  }
  --__libcwd_tsd.inside_malloc_or_free;
}

#if USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE
//---------------------------------------------------------------------------------------------
// This is a tiny 'malloc library' that is used while
// looking up the real malloc functions in libc.
// We expect only a single call to malloc() from dlopen (from init_malloc_function_pointers)
// but provide a slightly more general set of stubs here anyway.
//
static size_t const assert_reserve_heap_size = 1024;
static size_t const assert_reserve_ptrs_size = 6;
static char allocation_heap[1024 + assert_reserve_heap_size];
static void* allocation_ptrs[8 + assert_reserve_ptrs_size];
static unsigned int allocation_counter = 0;
static char* allocation_ptr = allocation_heap;

void* malloc_bootstrap2(size_t size)
{
  static size_t _assert_reserve_heap_size = assert_reserve_heap_size;
  static size_t _assert_reserve_ptrs_size = assert_reserve_ptrs_size;
  if (allocation_counter > sizeof(allocation_ptrs) / sizeof(void*) - _assert_reserve_ptrs_size
      || allocation_ptr + size > allocation_heap + sizeof(allocation_heap) - _assert_reserve_heap_size)
  {
    _assert_reserve_heap_size = 0;
    _assert_reserve_ptrs_size = 0;
    assert(allocation_counter <= sizeof(allocation_ptrs) / sizeof(void*) - assert_reserve_ptrs_size);
    assert(allocation_ptr + size <= allocation_heap + sizeof(allocation_heap) - assert_reserve_heap_size);
  }
  void* ptr = allocation_ptr;
  allocation_ptrs[allocation_counter++] = ptr;
  allocation_ptr += size;
  return ptr;
}

void free_bootstrap2(void* ptr)
{
  for (unsigned int i = 0; i < allocation_counter; ++i)
    if (allocation_ptrs[i] == ptr)
    {
      allocation_ptrs[i] = allocation_ptrs[allocation_counter - 1];
      allocation_ptrs[allocation_counter - 1] = NULL;
      if (--allocation_counter == 0 && libc_free_final)	// Done?
	libc_free = libc_free_final;
      return;
    }
  (*libc_free_final)(ptr);
}

// This appears not to be called by dlopen/dlsym/dlclose, but lets keep it anyway.
void* calloc_bootstrap2(size_t nmemb, size_t size)
{
  void* ptr = malloc_bootstrap2(nmemb * size);
  std::memset(ptr, 0, nmemb * size);
  return ptr;
}

// This appears not to be called by dlopen/dlsym/dlclose, but lets keep it anyway.
void* realloc_bootstrap2(void* ptr, size_t size)
{
  // This assumes that allocations during dlopen()/dlclose()
  // never use the fact that decreasing an allocation is
  // garanteed not to relocate it.
  void* res = malloc_bootstrap2(size);
  free_bootstrap2(ptr);
  return res;
}

void init_malloc_function_pointers(void)
{
  // Point functions to next phase.
  libc_malloc = malloc_bootstrap2;
  libc_calloc = calloc_bootstrap2;
  libc_realloc = realloc_bootstrap2;
  libc_free = free_bootstrap2;
  void* (*libc_malloc_tmp)(size_t size);
  void* (*libc_calloc_tmp)(size_t nmemb, size_t size);
  void* (*libc_realloc_tmp)(void* ptr, size_t size);
  void (*libc_free_tmp)(void* ptr);
  libc_malloc_tmp = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
  assert(libc_malloc_tmp);
  libc_calloc_tmp = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
  assert(libc_calloc_tmp);
  libc_realloc_tmp = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
  assert(libc_realloc_tmp);
  libc_free_tmp = (void (*)(void*))dlsym(RTLD_NEXT, "free");
  assert(libc_free_tmp);
  libc_malloc = libc_malloc_tmp;
  libc_calloc = libc_calloc_tmp;
  libc_realloc = libc_realloc_tmp;
  if (allocation_counter == 0)	// Done?
    libc_free = libc_free_tmp;
  else
    // There are allocations left, we have to check
    // every free to see if it's one that was
    // allocated here... until it is finally freed.
    libc_free_final = libc_free_tmp;
}

// Very first time that malloc(3) or calloc(3) are called; initialize the function pointers.

void* malloc_bootstrap1(size_t size)
{
  init_malloc_function_pointers();
  return __libc_malloc(size);
}

void* calloc_bootstrap1(size_t nmemb, size_t size)
{
  init_malloc_function_pointers();
  return __libc_calloc(nmemb, size);
}
//---------------------------------------------------------------------------------------------
#endif // USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE

#if LIBCWD_THREAD_SAFE
namespace _private_ {

void* new_memblk_map(LIBCWD_TSD_PARAM)
{
  // Already internal.  Called from thread_ct::initialize()
  __libcwd_tsd.target_thread = &(*__libcwd_tsd.thread_iter);
  memblk_map_ct* memblk_map = new memblk_map_ct;
  UNSET_TARGETHREAD;
  return memblk_map;
}

bool delete_memblk_map(void* ptr LIBCWD_COMMA_TSD_PARAM)
{
  DEBUGDEBUG_CERR("Entering delete_memblk_map()");
  // Already internal.  Called from thread_ct::terminated()
  memblk_map_ct* memblk_map = reinterpret_cast<memblk_map_ct*>(ptr);
  bool deleted;
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  if (memblk_map->size() == 0)
  {
    DEBUGDEBUG_CERR("Destroying memblk_map " << (void*)memblk_map);
    delete memblk_map;
    deleted = true;
  }
  else
  {
    DEBUGDEBUG_CERR("Terminated thread with " << memblk_map->size() << " blocks still allocated.");
    deleted = false;
  }
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;
  return deleted;
}

} // namespace _private_
#endif // LIBCWD_THREAD_SAFE

void init_debugmalloc(void)
{
  if (WST_initialization_state <= 0)
  {
    LIBCWD_TSD_DECLARATION;
    // This block is Single Threaded.
    if (WST_initialization_state == 0)			// Only true once.
    {
#if CWDEBUG_MAGIC
      INITREDZONES;
#endif
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      // MT-safe: There are no threads created yet when we get here.
#if CWDEBUG_LOCATION
      location_cache_map.MT_unsafe = new location_cache_map_ct;
#endif
#if CWDEBUG_DEBUG && LIBCWD_THREAD_SAFE
      LIBCWD_DEBUG_ASSERT(!_private_::WST_multi_threaded);
#endif
#if !LIBCWD_THREAD_SAFE
      // With threads, memblk_map is initialized in 'LIBCWD_TSD_DECLARATION'.
      ST_memblk_map = new memblk_map_ct;		// LEAK4
#endif
      WST_initialization_state = -1;
      _private_::set_alloc_checking_on(LIBCWD_TSD);

#ifdef HAVE_DLOPEN
      // Initialize the function pointers for posix_memalign et al.
#ifdef HAVE_POSIX_MEMALIGN
      libc_posix_memalign = (int (*)(void **, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");
#endif
#ifdef HAVE_MEMALIGN
      libc_memalign = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "memalign");
#endif
#ifdef HAVE_VALLOC
      libc_valloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "valloc");
#endif
    }
#if LIBCWD_IOSBASE_INIT_ALLOCATES
    if (!_private_::WST_ios_base_initialized && !_private_::inside_ios_base_Init_Init())
#else
    std::ios_base::Init::Init dummy_init;
#endif
    {
      WST_initialization_state = 1;		// ST_initialize_globals() calls malloc again of course.
      int recursive_store = __libcwd_tsd.inside_malloc_or_free;
      __libcwd_tsd.inside_malloc_or_free = 0;	// Allow that (this call to malloc will not have done from STL allocator).
      libcwd::ST_initialize_globals(LIBCWD_TSD);	// This doesn't belong in the malloc department at all, but malloc()
      							// happens to be a function that is called _very_ early - and hence
							// this is a good moment to initialize ALL of libcwd.
      __libcwd_tsd.inside_malloc_or_free = recursive_store;
#endif // HAVE_DLOPEN
    }
  }
}

//=============================================================================
//
// 'Accessor' functions
//

/**
 * \brief Test if a pointer points to the start of an allocated memory block.
 * \ingroup book_allocations
 *
 * \returns \c true when \a ptr does \em not point to the start of an allocated memory block.&nbsp;
 * No checks are performed on the type of allocator that was used: that is done when
 * the memory block is actually deleted, see \ref chapter_validation.
 *
 * Unlike \ref find_alloc, \c test_delete also works for \ref group_invisible "invisible" memory blocks.
 */
bool test_delete(void const* void_ptr)
{
  appblock const* ptr2 = static_cast<appblock const*>(void_ptr);
  bool found;
#if LIBCWD_THREAD_SAFE
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr2, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr2, 0)));
#endif
  // MT: Because the expression `(*iter).first.start()' is included inside the locked
  //     area too, no core dump will occur when another thread would be deleting
  //     this allocation at the same time.  The behaviour of the application would
  //     still be undefined however because it makes it possible that this function
  //     returns false (not deleted) for a deleted memory block.
  found = (iter != target_memblk_map_read->end() && (*iter).first.start() == ptr2);
#if LIBCWD_THREAD_SAFE
  if (!found)
  {
    RELEASE_READ_LOCK;
    found = search_in_maps_of_other_threads(ptr2, iter, __libcwd_tsd) && (*iter).first.start() == ptr2;
  }
  RELEASE_READ_LOCK;
  LIBCWD_RESTORE_CANCEL;
#endif
  return !found;
}

/**
 * \brief Returns the total number of allocated bytes.
 * \ingroup book_allocations
 */
size_t mem_size(void)
{
  size_t memsize = 0;
#if LIBCWD_THREAD_SAFE
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEFER_CANCEL;
  rwlock_tct<threadlist_instance>::rdlock();
  // See comment in search_in_maps_of_other_threads.
  for(threadlist_t::iterator thread_iter = threadlist->begin(); thread_iter != threadlist->end(); ++thread_iter)
  {
    ACQUIRE_READ_LOCK(&(*thread_iter));
#endif
    memsize += MEMSIZE;
#if LIBCWD_THREAD_SAFE
    RELEASE_READ_LOCK;
  }
  rwlock_tct<threadlist_instance>::rdunlock();
  LIBCWD_RESTORE_CANCEL;
#endif
  return memsize;
}

/**
 * \brief Returns the total number of allocated memory blocks.
 * \ingroup book_allocations
 */
unsigned long mem_blocks(void)
{
  unsigned long memblks = 0;
#if LIBCWD_THREAD_SAFE
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEFER_CANCEL;
  rwlock_tct<threadlist_instance>::rdlock();
  // See comment in search_in_maps_of_other_threads.
  for(threadlist_t::iterator thread_iter = threadlist->begin(); thread_iter != threadlist->end(); ++thread_iter)
  {
    ACQUIRE_READ_LOCK(&(*thread_iter));
#endif
    memblks += MEMBLKS;
#if LIBCWD_THREAD_SAFE
    RELEASE_READ_LOCK;
  }
  rwlock_tct<threadlist_instance>::rdunlock();
  LIBCWD_RESTORE_CANCEL;
#endif
  return memblks;
}

/**
 * \brief Allow writing of enum malloc_report_nt to an ostream.
 * \ingroup group_overview
 *
 * \sa \ref malloc_report_nt
 */
std::ostream& operator<<(std::ostream& o, malloc_report_nt)
{
  size_t memsize = 0;
  unsigned long memblks = 0;
#if LIBCWD_THREAD_SAFE
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEFER_CANCEL;
  rwlock_tct<threadlist_instance>::rdlock();
  // See comment in search_in_maps_of_other_threads.
  for(threadlist_t::iterator thread_iter = threadlist->begin(); thread_iter != threadlist->end(); ++thread_iter)
  {
    ACQUIRE_READ_LOCK(&(*thread_iter));
#endif
    memsize += MEMSIZE;
    memblks += MEMBLKS;
#if LIBCWD_THREAD_SAFE
    RELEASE_READ_LOCK;
  }
  rwlock_tct<threadlist_instance>::rdunlock();
  LIBCWD_RESTORE_CANCEL;
#endif
  o << "Allocated memory: " << memsize << " bytes in " << memblks << " blocks";
  return o;
}

/**
 * \brief List all current allocations to a given %debug object.
 * \ingroup group_overview
 *
 * With \link enable_alloc CWDEBUG_ALLOC \endlink set to 1, it is possible
 * to write the \ref group_overview "overview of allocated memory" to
 * a \ref group_debug_object "Debug Object".
 *
 * By default only the allocations that were made by the <EM>current</EM> thread
 * will be printed.&nbsp; Use \ref show_allthreads, together with a filter object,
 * in order to get a listing of the allocations of all threads (see below).
 *
 * The syntax to do this is:
 *
 * \code
 * Debug( list_allocations_on(libcw_do) );	// libcw_do is the (default) debug object.
 * \endcode
 *
 * which would print on \link libcwd::libcw_do libcw_do \endlink using
 * \ref group_debug_channels "debug channel" \link libcwd::channels::dc::malloc dc::malloc \endlink.
 *
 * Note that not passing formatting information is equivalent with,
 *
 * \code
 * Debug(
 *     alloc_filter_ct format(0);
 *     list_allocations_on(debug_object, format)
 * );
 * \endcode
 *
 * meaning that all allocations are shown without time, without path and without to which library they belong to.
 *
 * \returns the number of actually printed allocations.
 */
unsigned long list_allocations_on(debug_ct& debug_object)
{
  return list_allocations_on(debug_object, default_ooam_filter);
}

#if LIBCWD_THREAD_SAFE
static void list_allocations_cleanup(void)
{
  LIBCWD_TSD_DECLARATION;
  rwlock_tct<threadlist_instance>::rdunlock();
  if (LIBCWD_TSD.list_allocations_on_show_allthreads)
    DLCLOSE_RELEASE_LOCK;
}
#endif

/**
 * \brief List all current allocations to a given %debug object using a specified format.
 * \ingroup group_overview
 *
 * With \link enable_alloc CWDEBUG_ALLOC \endlink set to 1, it is possible
 * to write the \ref group_overview "overview of allocated memory" to
 * a \ref group_debug_object "Debug Object".
 *
 * For example:
 *
 * \code
 * Debug(
 *   alloc_filter_ct alloc_filter(show_objectfile);
 *   std::vector<std::string> masks;
 *   masks.push_back("libc.so*");
 *   masks.push_back("libstdc++*");
 *   alloc_filter.hide_objectfiles_matching(masks);
 *   alloc_filter.hide_unknown_locations();
 *   list_allocations_on(libcw_do, alloc_filter)
 * );
 * \endcode
 *
 * which would print on \link libcwd::libcw_do libcw_do \endlink using
 * \ref group_debug_channels "debug channel" \link libcwd::channels::dc::malloc dc::malloc \endlink,
 * not showing allocations that belong to shared libraries matching "libc.so*" or "libstdc++*".
 * The remaining items would show which object file (shared library name or the executable name)
 * they belong to, because we used \c show_objectfile as flag.
 *
 * \returns the number of unfiltered (listed) allocations.
 *
 * \sa group_alloc_format
 */
unsigned long list_allocations_on(debug_ct& debug_object, alloc_filter_ct const& filter)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);

  unsigned long printed_memblks = 0;
#if LIBCWD_THREAD_SAFE
  size_t total_memsize = 0;
  unsigned long total_memblks = 0;
  LIBCWD_DEFER_CLEANUP_PUSH(list_allocations_cleanup, NULL);
  LIBCWD_TSD.list_allocations_on_show_allthreads = filter.M_flags & show_allthreads;
  if ((filter.M_flags & show_allthreads))
    DLCLOSE_ACQUIRE_LOCK;
  rwlock_tct<threadlist_instance>::rdlock();
  // See comment in search_in_maps_of_other_threads.
  for(threadlist_t::iterator thread_iter = threadlist->begin(); thread_iter != threadlist->end(); ++thread_iter)
  {
    ACQUIRE_READ_LOCK(&(*thread_iter));
    total_memsize += MEMSIZE;
    total_memblks += MEMBLKS;
    if ((MEMBLKS == 0 && (*thread_iter).is_terminating()) ||
	(!(filter.M_flags & show_allthreads) && thread_iter != __libcwd_tsd.thread_iter))
    {
      RELEASE_READ_LOCK;
      continue;
    }
#endif
    size_t memsize = MEMSIZE;
    unsigned long memblks = MEMBLKS;
    dm_alloc_copy_ct* list = NULL;
    if (BASE_ALLOC_LIST)
    {
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      list = dm_alloc_copy_ct::deep_copy(BASE_ALLOC_LIST);
      _private_::set_alloc_checking_on(LIBCWD_TSD);
    }
#if LIBCWD_THREAD_SAFE
    pthread_t tid = __libcwd_tsd.target_thread->tid;
    RELEASE_READ_LOCK;
    LibcwDout( channels, debug_object, dc_malloc, "Allocated memory by thread " << tid << ": " << memsize << " bytes in " << memblks << " blocks:" );
#else
    LibcwDout( channels, debug_object, dc_malloc, "Allocated memory: " << memsize << " bytes in " << memblks << " blocks." );
#endif
    if (list)
    {
#if LIBCWD_THREAD_SAFE
      LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
      mutex_tct<list_allocations_instance>::lock();
#endif
#if CWDEBUG_LOCATION
      filter.M_check_synchronization();
#endif
#if LIBCWD_THREAD_SAFE
      LIBCWD_CLEANUP_POP_RESTORE(true);
#endif
      printed_memblks += list->show_alloc_list(debug_object, 1, channels::dc_malloc, filter);
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      delete list;
      _private_::set_alloc_checking_on(LIBCWD_TSD);
    }
#if LIBCWD_THREAD_SAFE
  }
  LIBCWD_CLEANUP_POP_RESTORE(true);
  LibcwDout(channels, debug_object, dc_malloc, "Total allocated memory: " << total_memsize << " bytes in " << total_memblks << " blocks (" << printed_memblks << " shown).");
#else
  if (printed_memblks > 0 && printed_memblks != memblks)
    LibcwDout(channels, debug_object, dc_malloc, "Number of visible memory blocks: " << printed_memblks << ".");
#endif
  return printed_memblks;
}

//=============================================================================
//
// 'Manipulator' functions
//

/**
 * \brief Make allocation pointed to by \a ptr invisible.
 * \ingroup group_invisible
 *
 * The allocation pointed to by \a ptr is made invisible; it won't show up
 * anymore in the \ref group_overview "overview of allocated memory".
 *
 * \sa \ref group_invisible
 * \sa \ref group_overview
 *
 * <b>Example:</b>
 *
 * \code
 * Debug( make_invisible(p) );
 * \endcode
 */
void make_invisible(void const* void_ptr)
{
  appblock const* ptr2 = static_cast<appblock const*>(void_ptr);
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.internal);
#if LIBCWD_THREAD_SAFE
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr2, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr2, 0)));
#endif
  bool found = (iter != target_memblk_map_read->end() && (*iter).first.start() == ptr2);
#if LIBCWD_THREAD_SAFE
  if (!found)
  {
    RELEASE_READ_LOCK;
    found = search_in_maps_of_other_threads(ptr2, iter, __libcwd_tsd);
  }
#endif
  if (!found
#if LIBCWD_THREAD_SAFE
      || (*iter).first.start() != ptr2
#endif
      )
  {
    if (found)
      RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatalInternal( dc::core, "Trying to turn non-existing memory block (" <<
        PRINT_PTR(ptr2) << ") into an 'internal' block" );
  }
  DEBUGDEBUG_CERR( "make_invisible: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;
  ACQUIRE_READ2WRITE_LOCK;
  (*memblk_iter_write).second.make_invisible();
  RELEASE_WRITE_LOCK;
  DEBUGDEBUG_CERR( "make_invisible: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  LIBCWD_RESTORE_CANCEL;
}

/**
 * \brief Make all current allocations invisible except the given pointer.
 * \ingroup group_invisible
 *
 * All allocations, except the given pointer, are made invisible; they won't show up
 * anymore in the \ref group_overview "overview of allocated memory".
 *
 * If you want to make \em all allocations invisible, just pass \c NULL as parameter.
 *
 * \sa \ref group_invisible
 * \sa \ref group_overview
 *
 * <b>Example:</b>
 *
 * \code
 * Debug( make_all_allocations_invisible_except(NULL) );
 * \endcode
 */
void make_all_allocations_invisible_except(void const* ptr)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.internal);
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  for (memblk_map_ct::iterator iter(memblk_map_write->begin()); iter != memblk_map_write->end(); ++iter)
    if ((*iter).second.has_alloc_node() && (*iter).first.start() != ptr)
    {
      DEBUGDEBUG_CERR( "make_all_allocations_invisible_except: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
      __libcwd_tsd.internal = 1;
      (*iter).second.make_invisible();
      DEBUGDEBUG_CERR( "make_all_allocations_invisible_except: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
      __libcwd_tsd.internal = 0;
    }
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;
}

#if CWDEBUG_ALLOC
namespace _private_ {
  struct exit_function_list** __exit_funcs_ptr;
}

/**
 * \brief Make allocations done in libc.so:__new_exitfn invisible.
 * \ingroup group_invisible
 *
 * This makes the allocation done in __new_exitfn (libc.so)
 * invisible because it is not freed until after all
 * __cxa_atexit functions have been called and would therefore
 * always falsely trigger a memory leak detection.
 * This function can be called first thing in main().
 *
 * \sa \ref group_invisible
 * \sa \ref group_overview
 *
 * <b>Example:</b>
 *
 * \code
 * Debug( make_exit_function_list_invisible() );
 * \endcode
 */
void make_exit_function_list_invisible(void)
{
  if (libcwd::_private_::__exit_funcs_ptr)
    for (libcwd::_private_::exit_function_list* l = *libcwd::_private_::__exit_funcs_ptr; l->next; l = l->next)
      libcwd::make_invisible(l);
}
#endif

// Undocumented (used in macro AllocTag)
void set_alloc_label(void const* void_ptr, type_info_ct const& ti, char const* description LIBCWD_COMMA_TSD_PARAM)
{
  appblock const* ptr2 = static_cast<appblock const*>(void_ptr);
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(ptr2, 0)));
  if (iter != memblk_map_write->end() && (*iter).first.start() == ptr2)
  {
    (*iter).second.change_label(ti, description);
    (*iter).second.alloctag_called();
  }
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;
}

void set_alloc_label(void const* void_ptr, type_info_ct const& ti, _private_::smart_ptr description LIBCWD_COMMA_TSD_PARAM)
{
  appblock const* ptr2 = static_cast<appblock const*>(void_ptr);
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(ptr2, 0)));
  if (iter != memblk_map_write->end() && (*iter).first.start() == ptr2)
  {
    (*iter).second.change_label(ti, description);
    (*iter).second.alloctag_called();
  }
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;
}

// Used in AllocTag. This function is put here so that the leak can
// easier be suppressed by valgrind.
char* allocate_AllocTag_WS_desc(size_t size)
{
  return new char [size];	// LEAK45
}

#if CWDEBUG_LOCATION
namespace _private_ {

// This is called from bfile_ct::deinitialize.
void remove_type_info_references(object_file_ct const* object_file_ptr LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  
  for (memblk_map_ct::const_iterator iter = memblk_map_read->begin(); iter != memblk_map_read->end(); ++iter)
  {
    alloc_ct* alloc = iter->second.get_alloc_node();
    if (alloc && alloc->location().object_file() == object_file_ptr)
      alloc->reset_type_info();
  }

  RELEASE_READ_LOCK;
  LIBCWD_RESTORE_CANCEL;
}

} // namespace _private_
#endif

#undef CALL_ADDRESS
#if CWDEBUG_LOCATION
#define CALL_ADDRESS , reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset
#else
#define CALL_ADDRESS
#endif

#if CWDEBUG_MARKER
void marker_ct::register_marker(char const* label)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);
  Dout( dc_malloc, "New libcwd::marker_ct at " << this );
  bool error = false;
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(reinterpret_cast<appblock*>(this), 0)));
  memblk_info_ct& info((*iter).second);
  if (iter == memblk_map_write->end() || (*iter).first.start() != this || info.flags() != memblk_type_new)
    error = true;
  else
  {
    info.change_label(type_info_of(this), label);
    info.alloctag_called();
    info.change_flags(memblk_type_marker);
    info.new_list(LIBCWD_TSD);					// MT: needs write lock set.
  }
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;
  if (error)
    DoutFatal( dc::core, "Use 'new' for libcwd::marker_ct" );
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  Debug( list_allocations_on(libcw_do) );
#endif
}

/**
 * \brief Destructor.
 */
marker_ct::~marker_ct()
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);

  _private_::smart_ptr description;

  LIBCWD_DEFER_CANCEL_NO_BRACE;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(reinterpret_cast<appblock*>(this), 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != this)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatal( dc::core, "Trying to delete an invalid marker" );
  }

  description = (*iter).second.description();		// This won't call malloc.

  dm_alloc_ct* marker_alloc_node = (*iter).second.a_alloc_node.get();

  if (*CURRENT_ALLOC_LIST != marker_alloc_node->next_list())
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    Dout( dc_malloc, "Removing libcwd::marker_ct at " << this << " (" << description.get() << ')' );
    DoutFatal( dc::core, "Deleting a marker must be done in the same \"scope\" as where it was allocated; for example, "
	"you cannot allocate marker A, then allocate marker B and then delete marker A before deleting first marker B." );
  }
  ACQUIRE_READ2WRITE_LOCK;
  // Set `CURRENT_ALLOC_LIST' one list back
  dm_alloc_ct::descend_current_alloc_list(LIBCWD_TSD);		// MT: needs write lock.
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL_NO_BRACE;

  Dout( dc_malloc, "Removing libcwd::marker_ct at " << this << " (" << description.get() << ')' );
  if (marker_alloc_node->next_list())
  {
    dm_alloc_copy_ct* list = NULL; // Initialize to avoid 'may be used uninitialized' compiler warning.
    bool memory_leak;
#if LIBCWD_THREAD_SAFE
    LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<list_allocations_instance>::cleanup, NULL);
    mutex_tct<list_allocations_instance>::lock();
#endif
#if CWDEBUG_LOCATION
    M_filter.M_check_synchronization();
#endif
    for (dm_alloc_ct* alloc_node = marker_alloc_node->a_next_list; alloc_node;)
    {
      dm_alloc_ct* next_alloc_node = alloc_node->next;
#if CWDEBUG_LOCATION
      object_file_ct const* object_file = alloc_node->location().object_file();
      if (alloc_node->location().new_location())
	alloc_node->location().synchronize_with(M_filter);
#endif
      if (((M_filter.M_flags & hide_untagged) && !alloc_node->is_tagged()) ||
#if CWDEBUG_LOCATION
	  (alloc_node->location().hide_from_alloc_list()) ||
	  (object_file && object_file->hide_from_alloc_list()) ||
#endif
          ((M_filter.M_start.tv_sec != 1) &&
	   (alloc_node->time().tv_sec < M_filter.M_start.tv_sec || 
	       (alloc_node->time().tv_sec == M_filter.M_start.tv_sec && alloc_node->time().tv_usec < M_filter.M_start.tv_usec))) ||
          ((M_filter.M_end.tv_sec != 1) &&
	    (alloc_node->time().tv_sec > M_filter.M_end.tv_sec ||
	        (alloc_node->time().tv_sec == M_filter.M_end.tv_sec && alloc_node->time().tv_usec > M_filter.M_end.tv_usec))))
      {
        if (M_make_invisible)
	  make_invisible(alloc_node->start());
	else
	{
	  // Delink it:
	  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
	  if (alloc_node->next)
	    alloc_node->next->prev = alloc_node->prev;
	  if (alloc_node->prev)
	    alloc_node->prev->next = alloc_node->next;
	  else if (!(*alloc_node->my_list = alloc_node->next) && alloc_node->my_owner_node->is_deleted())
	    delete alloc_node->my_owner_node;
	  // Relink it:
	  alloc_node->prev = NULL;
	  alloc_node->next = *marker_alloc_node->my_list;
	  *marker_alloc_node->my_list = alloc_node;
	  alloc_node->next->prev = alloc_node;
	  alloc_node->my_list = marker_alloc_node->my_list;
	  alloc_node->my_owner_node = marker_alloc_node->my_owner_node;
	  RELEASE_WRITE_LOCK;
	}
      }
      alloc_node = next_alloc_node;
    }
    memory_leak = marker_alloc_node->next_list();
    if (memory_leak)
    {
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      list = dm_alloc_copy_ct::deep_copy(marker_alloc_node->next_list());
      _private_::set_alloc_checking_on(LIBCWD_TSD);
    }
    LIBCWD_CLEANUP_POP_RESTORE(true);
    if (memory_leak)
    {
      libcw_do.push_margin();
      libcw_do.margin().append("  * ", 4);
      Dout( dc::warning, "Memory leak detected!" );
      list->show_alloc_list(libcw_do, 1, channels::dc::warning, M_filter);
      libcw_do.pop_margin();
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      delete list;
      _private_::set_alloc_checking_on(LIBCWD_TSD);
    }
  }
}

/**
 * \brief Move memory allocation pointed to by \a ptr outside \a marker.
 * \ingroup group_markers
 */
void move_outside(marker_ct* marker, void const* void_ptr)
{
  appblock const* ptr2 = static_cast<appblock const*>(void_ptr);
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);

  LIBCWD_DEFER_CANCEL_NO_BRACE;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(ptr2, 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != ptr2)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatal( dc::core, "Trying to move non-existing memory block (" <<
        PRINT_PTR(ptr2) << ") outside memory leak test marker" );
  }
  memblk_map_ct::const_iterator const& iter2(memblk_map_read->find(memblk_key_ct(reinterpret_cast<appblock*>(marker), 0)));
  if (iter2 == memblk_map_read->end() || (*iter2).first.start() != marker)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatal( dc::core, "No such marker (in this thread): " << (void*)marker );
  }
  dm_alloc_ct* alloc_node = (*iter).second.a_alloc_node.get();
  if (!alloc_node)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatal( dc::core, "Trying to move an invisible memory block outside memory leak test marker" );
  }
  dm_alloc_ct* marker_alloc_node = (*iter2).second.a_alloc_node.get();
  if (!marker_alloc_node || marker_alloc_node->a_memblk_type != memblk_type_marker)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatal( dc::core, "That is not a marker: " << (void*)marker );
  }

  // Look if it is in the right list
  for(dm_alloc_ct* node = alloc_node; node;)
  {
    node = node->my_owner_node;
    if (node == marker_alloc_node)
    {
      // Correct.
      // Delink it:
      ACQUIRE_READ2WRITE_LOCK;
      if (alloc_node->next)
	alloc_node->next->prev = alloc_node->prev;
      if (alloc_node->prev)
	alloc_node->prev->next = alloc_node->next;
      else if (!(*alloc_node->my_list = alloc_node->next) && alloc_node->my_owner_node->is_deleted())
	delete alloc_node->my_owner_node;
      // Relink it:
      alloc_node->prev = NULL;
      alloc_node->next = *marker_alloc_node->my_list;
      *marker_alloc_node->my_list = alloc_node;
      alloc_node->next->prev = alloc_node;
      alloc_node->my_list = marker_alloc_node->my_list;
      alloc_node->my_owner_node = marker_alloc_node->my_owner_node;
      RELEASE_WRITE_LOCK;
      LIBCWD_RESTORE_CANCEL_NO_BRACE;
      return;
    }
  }
  RELEASE_READ_LOCK;
  LIBCWD_RESTORE_CANCEL_NO_BRACE;
  Dout( dc::warning, "Memory block at " << PRINT_PTR(ptr2) << " is already outside the marker at " <<
      (void*)marker << " (" << marker_alloc_node->type_info_ptr->demangled_name() << ") area!" );
}
#endif // CWDEBUG_MARKER

static alloc_ct* find_memblk_info(memblk_info_base_ct& result, bool set_watch, void const* void_ptr LIBCWD_COMMA_TSD_PARAM)
{ 
  appblock const* ptr2 = static_cast<appblock const*>(void_ptr);
  alloc_ct* alloc;
#if LIBCWD_THREAD_SAFE
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct const* target_memblk_map = target_memblk_map_read;
  if (!target_memblk_map)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    return NULL;
  }
  memblk_map_ct::const_iterator iter = target_memblk_map->find(memblk_key_ct(ptr2, 0));
#else
  memblk_map_ct const* target_memblk_map = target_memblk_map_read;
  if (!target_memblk_map)
    return NULL;
  memblk_map_ct::const_iterator const& iter(target_memblk_map->find(memblk_key_ct(ptr2, 0)));
#endif
  bool found = (iter != target_memblk_map->end());
#if LIBCWD_THREAD_SAFE
  if (!found)
  {
    RELEASE_READ_LOCK;
    found = search_in_maps_of_other_threads(ptr2, iter, __libcwd_tsd);
  }
#endif
  if (!found)
  {
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    return NULL;
  }
  result = iter->second;
  if (set_watch)
  {
    ACQUIRE_READ2WRITE_LOCK;
    iter->second.set_watch();
    ACQUIRE_WRITE2READ_LOCK;
  }
  alloc = iter->second.get_alloc_node();
  RELEASE_READ_LOCK;
  LIBCWD_RESTORE_CANCEL;
  return alloc;
}

/**
 * \brief Find information about a memory allocation.
 * \ingroup group_finding
 *
 * Given a pointer, which points to the start of or inside an allocated memory block,
 * it is possible to find information about this memory block using the libcwd function
 * \c find_alloc.
 *
 * \returns a const pointer to an object of class alloc_ct.
 *
 * \sa \ref test_delete()
 *
 * <b>Example:</b>
 *
 * \code
 * char* buf = new char [40];
 * AllocTag(buf, "A buffer");
 * libcwd::alloc_ct const* alloc = libcwd::find_alloc(&buf[10]);
 * std::cout << '"' << alloc->description() << "\" is " << alloc->size() << " bytes.\n";
 * \endcode
 *
 * gives as output,
 *
 * \exampleoutput <PRE>
 * "A buffer" is 40 bytes.</PRE>
 * \endexampleoutput
 */
alloc_ct const* find_alloc(void const* ptr)
{ 
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);

  memblk_info_base_ct memblk_info_dummy;
  return find_memblk_info(memblk_info_dummy, false, ptr LIBCWD_COMMA_TSD);
}

#ifndef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC

// If we cannot call the libc malloc functions directly, then we cannot declare
// a malloc function with external linkage but instead have to use a macro `malloc'
// that causes the application to call __libcwd_malloc.  However, that means that
// allocations done with malloc et.al from other libraries that are linked are
// not registered - which is a problem if the application has to free such an
// allocation (as is the case with for instance strdup(3)).  For such cases we
// provide the following function: to register an 'externally' allocated allocation.
// THIS IS DANGEROUS however: If you call this function with a pointer that is
// is NOT externally allocated, the application will crash.  Or when the external
// library DOES free the allocation then a leak is detected and freeing this allocation
// again in the application will lead to a crash without that it is detected that
// free(3) was called twice.
// 
void register_external_allocation(void const* void_ptr, size_t size)
{
  appblock const* ptr2 = static_cast<appblock const*>(void_ptr);
  LIBCWD_TSD_DECLARATION;
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);
#if CWDEBUG_DEBUGM && LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT(_private_::WST_ios_base_initialized);
#endif
  if (__libcwd_tsd.internal)
    DoutFatalInternal( dc::core, "Calling register_external_allocation while `internal' is non-zero!  "
                                 "You can't use RegisterExternalAlloc() inside a Dout() et. al. "
				 "(or whenever alloc_checking is off)." );
  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal( dc_malloc, "register_external_allocation(" << PRINT_PTR(ptr2) << ", " << size << ')' );

  // This block is Single Threaded.
  if (WST_initialization_state == 0)			// Only true once.
  {
#if CWDEBUG_MAGIC
    INITREDZONES;
#endif
    __libcwd_tsd.internal = 1;
    // MT-safe: There are no threads created yet when we get here.
#if CWDEBUG_LOCATION
    location_cache_map.MT_unsafe = new location_cache_map_ct;
#endif
#if CWDEBUG_DEBUG && LIBCWD_THREAD_SAFE
    LIBCWD_DEBUG_ASSERT(!_private_::WST_multi_threaded);
#endif
#if !LIBCWD_THREAD_SAFE
    // With threads, memblk_map is initialized in 'LIBCWD_TSD_DECLARATION'.
    ST_memblk_map = new memblk_map_ct;			// LEAK3
#endif
    WST_initialization_state = -1;
    __libcwd_tsd.internal = 0;
  }

#if CWDEBUG_LOCATION
  if (__libcwd_tsd.library_call++)
    ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);	// Otherwise debug output will be generated from bfd.cc (location_ct)
  location_ct* loc = location_cache(reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset LIBCWD_COMMA_TSD);
  if (--__libcwd_tsd.library_call)
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
#endif

  DEBUGDEBUG_CERR( "register_external_allocation: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;

  // Update our administration:
  std::pair<memblk_map_ct::iterator, bool> iter;
  LIBCWD_DEFER_CANCEL;
  if (__libcwd_tsd.invisible)
  {
    memblk_ct memblk(memblk_key_ct(ptr2, size), memblk_info_ct(memblk_type_external));
    ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
    iter = memblk_map_write->insert(memblk);
    RELEASE_WRITE_LOCK;
  }
  else
  {
    struct timeval alloc_time;
    gettimeofday(&alloc_time, 0);
    ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));	// MT: visible memblk_info_ct() needs wrlock too.
    memblk_ct memblk(memblk_key_ct(ptr2, size), memblk_info_ct(ptr2, size, memblk_type_external, alloc_time LIBCWD_COMMA_LOCATION(loc)));
    iter = memblk_map_write->insert(memblk);
    RELEASE_WRITE_LOCK;
  }
  LIBCWD_RESTORE_CANCEL;

  DEBUGDEBUG_CERR( "register_external_allocation: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  
  // MT-safe: iter points to an element that is garanteed unique to this thread (we just allocated it).
  if (!iter.second)
    DoutFatalInternal( dc::core, "register_external_allocation: externally (supposedly newly) allocated block collides with *existing* memblk!  Are you sure this memory block was externally allocated, or did you call RegisterExternalAlloc twice for the same pointer?" );

  if (!__libcwd_tsd.invisible)
  {
    memblk_info_ct& memblk_info((*iter.first).second);
    memblk_info.lock();		// Lock ownership (doesn't call malloc).
  }
  --__libcwd_tsd.inside_malloc_or_free;
}
#endif // !LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC

} // namespace libcwd

using namespace ::libcwd;

extern "C" {

//=============================================================================
//
// __libcwd_free, replacement for free(3)
//
// frees a block and updates the internal administration.
//

void __libcwd_free(void* void_ptr) throw()
{
  appblock* ptr2 = static_cast<appblock*>(void_ptr);
#if LIBCWD_THREAD_SAFE
  // This marks the returned tsd as 'inside_free'.  Such tsds are static if the thread is
  // terminating and are never overwritten by other threads.
  libcwd::_private_::TSD_st& __libcwd_tsd(libcwd::_private_::TSD_st::instance_free());
#endif
  internal_free(ptr2, from_free LIBCWD_COMMA_TSD);
#if LIBCWD_THREAD_SAFE
  // Mark the end of free() - now a static tsd might be re-used by other threads.
  libcwd::_private_::TSD_st::free_instance(__libcwd_tsd);
#endif
}

//=============================================================================
//
// malloc(3) and calloc(3) replacements:
//

void* __libcwd_malloc(size_t size) throw()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    FATALDEBUGDEBUG_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
#if CWDEBUG_DEBUGM && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC) && LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT(_private_::WST_ios_base_initialized || __libcwd_tsd.internal);
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `__libcwd_malloc(" << size << ")' [" << saved_marker << ']' << " from " << (void*)__builtin_return_address(0) );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_malloc(size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

    size_t real_size = REAL_SIZE(size);
#if CWDEBUG_MAGIC
    if (size > real_size)	// Overflow?
      return NULL;
#endif
    prezone* ptr1 = static_cast<prezone*>(__libc_malloc(real_size));
    if (!ptr1)
      return NULL;
#if CWDEBUG_MAGIC
    SET_MAGIC(ptr1, size, INTERNAL_MAGIC_MALLOC_BEGIN, INTERNAL_MAGIC_MALLOC_END);
#if CWDEBUG_DEBUGM
    if (!__libcwd_tsd.annotation)
    {
      __libcwd_tsd.annotation = 1;
      annotation_alloc(size);
      __libcwd_tsd.annotation = 0;
    }
#endif
#endif // CWDEBUG_MAGIC
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_malloc': " <<
        PRINT_PTR(ZONE2APP(ptr1)) << " [" << saved_marker << ']' );
    return ZONE2APP(ptr1);

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "malloc(" << size << ") = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  appblock* ptr2 = internal_malloc(size, memblk_type_malloc CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker));
#if CWDEBUG_MAGIC
  if (ptr2)
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_MALLOC_BEGIN, MAGIC_MALLOC_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}

void* __libcwd_calloc(size_t nmemb, size_t size) throw()
{
#if LIBCWD_THREAD_SAFE && !VALGRIND
  static bool WST_libpthread_initialized = false;
  struct delayed_calloc_st {
    appblock* ptr;
    size_t size;
  };
  static delayed_calloc_st WST_delayed_calloc[2];
  if (!WST_libpthread_initialized)
  {
    static int ST_libpthread_init_count = 0;
    if (ST_libpthread_init_count == 2)
    {
      WST_libpthread_initialized = true;
      LIBCWD_TSD_DECLARATION;
      LIBCWD_DEFER_CANCEL;
      __libcwd_tsd.internal = 1;
      ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
      memblk_map_write->insert(memblk_ct(memblk_key_ct(WST_delayed_calloc[0].ptr, WST_delayed_calloc[0].size), memblk_info_ct(memblk_type_malloc)));
      memblk_map_write->insert(memblk_ct(memblk_key_ct(WST_delayed_calloc[1].ptr, WST_delayed_calloc[1].size), memblk_info_ct(memblk_type_malloc)));
      RELEASE_WRITE_LOCK;
      __libcwd_tsd.internal = 0;
      LIBCWD_RESTORE_CANCEL;
    }
    else
    {
      // We can't call pthread_self() or any other function of libpthread yet.
      // Doing LIBCWD_TSD_DECLARATION would core without creating a usable backtrace.
#if CWDEBUG_MAGIC
      size_t real_size = REAL_SIZE(nmemb * size);
      if (nmemb * size > real_size)	// Overflow?
	return NULL;
      prezone* ptr1 = static_cast<prezone*>(__libc_malloc(real_size));
      if (!ptr1)
	return NULL;
      appblock* ptr2 = ZONE2APP(ptr1);
      std::memset(ptr2, 0, nmemb * size);
      SET_MAGIC(ptr1, nmemb * size, MAGIC_MALLOC_BEGIN, MAGIC_MALLOC_END);
#else
      prezone* ptr1 = static_cast<prezone*>(__libc_calloc(nmemb, size));
      if (!ptr1)
	return NULL;
      appblock* ptr2 = ZONE2APP(ptr1);
#endif
      WST_delayed_calloc[ST_libpthread_init_count].ptr = ptr2;
      WST_delayed_calloc[ST_libpthread_init_count++].size = nmemb * size;
      return ASSERT_APPBLOCK(ptr2);
    }
  }
#endif // LIBCWD_THREAD_SAFE && !VALGRIND
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
#if CWDEBUG_DEBUGM && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC) && LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT(_private_::WST_ios_base_initialized || __libcwd_tsd.internal);
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `__libcwd_calloc(" << nmemb << ", " << size << ")' [" << saved_marker << ']' << " from " << (void*)__builtin_return_address(0) );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_calloc(nmemb, size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

#if CWDEBUG_MAGIC
    size_t real_size = REAL_SIZE(nmemb * size);
    if (nmemb * size > real_size)	// Overflow?
      return NULL;
    prezone* ptr1 = static_cast<prezone*>(__libc_malloc(real_size));
    if (!ptr1)
      return NULL;
    std::memset(ZONE2APP(ptr1), 0, nmemb * size);
    SET_MAGIC(ptr1, nmemb * size, INTERNAL_MAGIC_MALLOC_BEGIN, INTERNAL_MAGIC_MALLOC_END);
#if CWDEBUG_DEBUGM
    if (!__libcwd_tsd.annotation)
    {
      __libcwd_tsd.annotation = 1;
      annotation_alloc(nmemb * size);
      __libcwd_tsd.annotation = 0;
    }
#endif
#else
    prezone* ptr1 = static_cast<prezone*>(__libc_calloc(nmemb, size));
#endif // CWDEBUG_MAGIC
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_calloc': " <<
        PRINT_PTR(ZONE2APP(ptr1)) << " [" << saved_marker << ']' );
    return ZONE2APP(ptr1);

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "calloc(" << nmemb << ", " << size << ") = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  appblock* ptr2;
  size *= nmemb;
  if ((ptr2 = internal_malloc(size, memblk_type_malloc CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker))))
    std::memset(ptr2, 0, size);
#if CWDEBUG_MAGIC
  if (ptr2)
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_MALLOC_BEGIN, MAGIC_MALLOC_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}

//=============================================================================
//
// __libcwd_realloc, replacement for realloc(3)
//
// reallocates a block and updates the internal administration.
//

void* __libcwd_realloc(void* void_ptr, size_t size) throw()
{
  appblock* ptr2 = static_cast<appblock*>(void_ptr);
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
#if CWDEBUG_DEBUGM && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC) && LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT(_private_::WST_ios_base_initialized || __libcwd_tsd.internal);
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `__libcwd_realloc(" << PRINT_PTR(ptr2) << ", " << size << ")' [" << saved_marker << ']' << " from " << (void*)__builtin_return_address(0) );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_realloc(ptr2, size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

#if !CWDEBUG_MAGIC
    prezone* ptr1 = static_cast<prezone*>(__libc_realloc(APP2ZONE(ptr2), size));
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_realloc': " <<
        PRINT_PTR(ZONE2APP(ptr1)) << " [" << saved_marker << ']' );
    return ZONE2APP(ptr1);
#else // CWDEBUG_MAGIC
    prezone* ptr1;
    size_t real_size = REAL_SIZE(size);
    if (size > real_size)	// Overflow?
    {
      DoutInternal( dc_malloc, "Size too large: no space left for magic numbers." );
      LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_realloc': NULL [" << saved_marker << ']' );
      return NULL;
    }
    if (ptr2)
    {
      ptr1 = APP2ZONE(ptr2);
      if (PREZONE(ptr1).magic != INTERNAL_MAGIC_MALLOC_BEGIN || POSTZONE(ptr1).magic != INTERNAL_MAGIC_MALLOC_END)
	DoutFatalInternal( dc::core, "internal realloc: magic number corrupt!" );
#if CWDEBUG_DEBUGM
      if (!__libcwd_tsd.annotation)
      {
	__libcwd_tsd.annotation = 1;
	annotation_free(ZONE2RS(ptr1));
	__libcwd_tsd.annotation = 0;
      }
#endif
      if (size == 0)
      {
        __libc_free(ptr1);
	LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_realloc': NULL [" << saved_marker << ']' );
	return NULL;
      }
      ptr1 = static_cast<prezone*>(__libc_realloc(ptr1, real_size));
    }
    else
      ptr1 = static_cast<prezone*>(__libc_malloc(real_size));
    SET_MAGIC(ptr1, size, INTERNAL_MAGIC_MALLOC_BEGIN, INTERNAL_MAGIC_MALLOC_END);
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_realloc': " <<
        PRINT_PTR(ZONE2APP(ptr1)) << " [" << saved_marker << ']' );
#if CWDEBUG_DEBUGM
    if (!__libcwd_tsd.annotation)
    {
      __libcwd_tsd.annotation = 1;
      annotation_alloc(size);
      __libcwd_tsd.annotation = 0;
    }
#endif
    return ZONE2APP(ptr1);
#endif // CWDEBUG_MAGIC

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "realloc(" << PRINT_PTR(ptr2) << ", " << size << ") = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));

  if (ptr2 == NULL)
  {
    ptr2 = internal_malloc(size, memblk_type_realloc CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker));

#if CWDEBUG_MAGIC
    if (ptr2)
    {
      prezone* ptr1 = APP2ZONE(ptr2);
      SET_MAGIC(ptr1, size, MAGIC_MALLOC_BEGIN, MAGIC_MALLOC_END);
    }
#endif
    --__libcwd_tsd.inside_malloc_or_free;
    return ASSERT_APPBLOCK(ptr2);
  }

#if CWDEBUG_LOCATION
  if (__libcwd_tsd.library_call++)
    ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);    // Otherwise debug output will be generated from bfd.cc (location_ct)
  location_ct const* loc = location_cache(reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset LIBCWD_COMMA_TSD);
  if (--__libcwd_tsd.library_call)
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
#endif

  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;

#if LIBCWD_THREAD_SAFE
  LIBCWD_DEFER_CANCEL_NO_BRACE;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr2, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr2, 0)));
#endif
  bool found = (iter != target_memblk_map_read->end() && (*iter).first.start() == ptr2);
#if LIBCWD_THREAD_SAFE
  _private_::thread_ct* other_target_thread;
  if (!found)
  {
    RELEASE_READ_LOCK;
    found = search_in_maps_of_other_threads(ptr2, iter, __libcwd_tsd);
    other_target_thread = __libcwd_tsd.target_thread;
  }
  else
    other_target_thread = NULL;
#endif
  if (!found
#if LIBCWD_THREAD_SAFE
      || (*iter).first.start() != ptr2
#endif
      )
  {
    if (found)
      RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
    __libcwd_tsd.internal = 0;
    DoutInternal( dc::finish, "" );
    DoutFatalInternal(dc::core, "Trying to realloc() an invalid pointer (" << PRINT_PTR(ptr2) << ")" LIBCWD_DEBUGM_OPT(" [" << saved_marker << ']'));
  }

  if (size == 0)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    // MT: At this point, the memory block that `ptr' points at could be freed by
    //     another thread.  This is not a problem because `internal_free' will
    //     again set a lock and again try to find the ptr in the memblk_map.
    //     It might print "free" instead of "realloc", but the program is ill-formed
    //     anyway in this case.
    --__libcwd_tsd.inside_malloc_or_free;
    __libcwd_tsd.internal = 0;
    internal_free(ptr2, from_free LIBCWD_COMMA_TSD);
    ++__libcwd_tsd.inside_malloc_or_free;
    DoutInternal(dc::finish, "NULL" LIBCWD_DEBUGM_OPT(" [" << saved_marker << ']'));
    --__libcwd_tsd.inside_malloc_or_free;
    return NULL;
  }

  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;

  prezone* ptr1;
  size_t real_size = REAL_SIZE(size);
#if CWDEBUG_MAGIC
  ptr1 = APP2ZONE(ptr2);
  if (PREZONE(ptr1).magic != MAGIC_MALLOC_BEGIN || POSTZONE(ptr1).magic != MAGIC_MALLOC_END)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    if (PREZONE(ptr1).magic == MAGIC_NEW_BEGIN && POSTZONE(ptr1).magic == MAGIC_NEW_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `new'!" );
    if (PREZONE(ptr1).magic == MAGIC_NEW_ARRAY_BEGIN && POSTZONE(ptr1).magic == MAGIC_NEW_ARRAY_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `new[]'!" );
    if (PREZONE(ptr1).magic == MAGIC_POSIX_MEMALIGN_BEGIN && POSTZONE(ptr1).magic == MAGIC_POSIX_MEMALIGN_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `posix_memalign()'!" );
    if (PREZONE(ptr1).magic == MAGIC_MEMALIGN_BEGIN && POSTZONE(ptr1).magic == MAGIC_MEMALIGN_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `memalign()'!" );
    if (PREZONE(ptr1).magic == MAGIC_VALLOC_BEGIN && POSTZONE(ptr1).magic == MAGIC_VALLOC_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `valloc()'!" );
    DoutFatalInternal( dc::core, "realloc: magic number corrupt!" );
  }
  if (size > real_size)	// Overflow?
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutInternal( dc::finish, "NULL" LIBCWD_DEBUGM_OPT(" [" << saved_marker << ']'));
    DoutInternal( dc_malloc, "Size too large: no space left for magic numbers." );
    return NULL;	// A fatal error should occur directly after this
  }
#endif
  if ((ptr1 = static_cast<prezone*>(__libc_realloc(APP2ZONE(ptr2), real_size))) == NULL)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutInternal(dc::finish, "NULL" LIBCWD_DEBUGM_OPT(" [" << saved_marker << ']'));
    DoutInternal( dc_malloc, "Out of memory! This is only a pre-detection!" );
    --__libcwd_tsd.inside_malloc_or_free;
    return NULL; // A fatal error should occur directly after this
  }
#if CWDEBUG_MAGIC
  SET_MAGIC(ptr1, size, MAGIC_MALLOC_BEGIN, MAGIC_MALLOC_END);
#endif
  ptr2 = ZONE2APP(ptr1);

  // Update administration
  bool insertion_succeeded;
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;
  bool invisible = __libcwd_tsd.invisible || !(*iter).second.has_alloc_node();
  if (invisible)
  {
#if LIBCWD_THREAD_SAFE
    if (other_target_thread)
      ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));	// MT: visible memblk_info_ct() needs wrlock too.
    else
      ACQUIRE_READ2WRITE_LOCK;
#endif
    memblk_ct memblk(memblk_key_ct(ptr2, size), memblk_info_ct(memblk_type_realloc));
#if LIBCWD_THREAD_SAFE
    if (other_target_thread)
    {
      __libcwd_tsd.target_thread = other_target_thread;	// We still hold a read lock on this.
      ACQUIRE_READ2WRITE_LOCK;				// iter was changed to point to other_target_thread's memblk_map.
    }
#endif
    target_memblk_map_write->erase(memblk_iter_write);
#if LIBCWD_THREAD_SAFE
    if (other_target_thread)
    {
      _private_::thread_ct* zombie_thread = __libcwd_tsd.target_thread;
      bool can_delete_zombie = zombie_thread->is_zombie() && target_memblk_map_write->size() == 0;
      RELEASE_WRITE_LOCK;
      __libcwd_tsd.target_thread = &(*__libcwd_tsd.thread_iter);	// We still hold a write lock on this.
      if (can_delete_zombie)
	delete_zombie(zombie_thread, __libcwd_tsd);
    }
#endif
    std::pair<memblk_map_ct::iterator, bool> const& iter2(memblk_map_write->insert(memblk));
    insertion_succeeded = iter2.second;
  }
  else
  {
    type_info_ct const* type_info_ptr = (*iter).second.typeid_ptr();
    _private_::smart_ptr d((*iter).second.description());
    struct timeval realloc_time;
    gettimeofday(&realloc_time, 0);
#if LIBCWD_THREAD_SAFE
    if (other_target_thread)
      ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));	// MT: visible memblk_info_ct() needs wrlock too.
    else
      ACQUIRE_READ2WRITE_LOCK;
#endif
    memblk_ct memblk(memblk_key_ct(ptr2, size),
		     memblk_info_ct(ptr2, size, memblk_type_realloc,
		     realloc_time LIBCWD_COMMA_TSD LIBCWD_COMMA_LOCATION(loc)));
#if LIBCWD_THREAD_SAFE
    if (other_target_thread)
    {
      __libcwd_tsd.target_thread = other_target_thread;	// We still hold a read lock on this.
      ACQUIRE_READ2WRITE_LOCK;				// iter was changed to point to other_target_thread's memblk_map.
    }
#endif
    target_memblk_map_write->erase(memblk_iter_write);
#if LIBCWD_THREAD_SAFE
    if (other_target_thread)
    {
      _private_::thread_ct* zombie_thread = __libcwd_tsd.target_thread;
      bool can_delete_zombie = zombie_thread->is_zombie() && target_memblk_map_write->size() == 0;
      RELEASE_WRITE_LOCK;
      __libcwd_tsd.target_thread = &(*__libcwd_tsd.thread_iter);	// We still hold a write lock on this.
      if (can_delete_zombie)
	delete_zombie(zombie_thread, __libcwd_tsd);
    }
#endif
    std::pair<memblk_map_ct::iterator, bool> const& iter2(memblk_map_write->insert(memblk));
    insertion_succeeded = iter2.second;
    if (insertion_succeeded)
    {
      memblk_info_ct& memblk_info((*(iter2.first)).second);
      memblk_info.change_label(*type_info_ptr, d);
      memblk_info.lock();	// Lock ownership.
    }
  }
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL_NO_BRACE;

  if (!insertion_succeeded)
    DoutFatalInternal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );

  // Backtrace.
  if (backtrace_hook && __libcwd_tsd.library_call == 0)
  {
    void* buffer[max_frames];
    ++__libcwd_tsd.library_call;
    ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
    int frames = backtrace(buffer, max_frames);
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
    BACKTRACE_ACQUIRE_LOCK;
    if (backtrace_hook)
      (*backtrace_hook)(buffer, frames LIBCWD_COMMA_TSD);
    BACKTRACE_RELEASE_LOCK;
    --__libcwd_tsd.library_call;
  }

  DoutInternal(dc::finish, PRINT_PTR(ptr2)
      LIBCWD_LOCATION_OPT(<< " [" << *loc << ']')
      << (__libcwd_tsd.invisible ? " (invisible)" : "")
      LIBCWD_DEBUGM_OPT(<< " [" << saved_marker << ']'));
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}

//=============================================================================
//
// posix_memalign(3), memalign(3) and valloc(3) replacements:
//

#ifdef HAVE_POSIX_MEMALIGN
int __libcwd_posix_memalign(void **memptr, size_t alignment, size_t size) throw()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);
#if LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT(_private_::WST_ios_base_initialized);
#endif
#endif
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "posix_memalign(" << memptr << ", " << alignment << ", " << size << ") = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  if ((alignment & (alignment - 1)) != 0)
  {
    DoutInternal(dc::finish, "EINVAL" LIBCWD_DEBUGM_OPT(" [" << saved_marker << ']'));
    DoutInternal( dc_malloc|dc::warning, "Requested alignment for posix_memalign is not a power of two!" );
    return EINVAL;	// A fatal error should occur directly after this
  }
  appblock* ptr2 = internal_malloc(size, memblk_type_posix_memalign CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker), alignment);
#if CWDEBUG_MAGIC
  if (ptr2)
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_POSIX_MEMALIGN_BEGIN, MAGIC_POSIX_MEMALIGN_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  if (!ptr2)
    return ENOMEM;
  *memptr = ASSERT_APPBLOCK(ptr2);
  return 0;
}
#endif // HAVE_POSIX_MEMALIGN

#ifdef HAVE_MEMALIGN
void* __libcwd_memalign(size_t alignment, size_t size) throw()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);
#if LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT(_private_::WST_ios_base_initialized);
#endif
#endif
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "memalign(" << alignment << ", " << size << ") = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  appblock* ptr2 = internal_malloc(size, memblk_type_memalign CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker), alignment);
#if CWDEBUG_MAGIC
  if (ptr2)
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_MEMALIGN_BEGIN, MAGIC_MEMALIGN_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}
#endif // HAVE_MEMALIGN

#ifdef HAVE_VALLOC
void* __libcwd_valloc(size_t size) throw()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  LIBCWD_DEBUGM_ASSERT(!__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal);
#if LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT( _private_::WST_ios_base_initialized );
#endif
#endif
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "valloc(" << size << ") = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  appblock* ptr2 = internal_malloc(size, memblk_type_valloc CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker), sysconf(_SC_PAGESIZE));
#if CWDEBUG_MAGIC
  if (ptr2)
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_VALLOC_BEGIN, MAGIC_VALLOC_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}
#endif // HAVE_VALLOC

} // extern "C"

//=============================================================================
//
// operator `new' and `new []' replacements.
//

void* operator new(size_t size) throw (std::bad_alloc)
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `operator new', size = " << size << " [" << saved_marker << ']' << " from " << (void*)__builtin_return_address(0) );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_malloc(size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

    size_t real_size = REAL_SIZE(size);
    if (size > real_size)
      DoutFatalInternal(dc::core, "Size too large: no space left for magic numbers in `operator new'");
    prezone* ptr1 = static_cast<prezone*>(__libc_malloc(real_size));
    if (!ptr1)
      DoutFatalInternal(dc::core, "Out of memory in `operator new'");
#if CWDEBUG_MAGIC
    SET_MAGIC(ptr1, size, INTERNAL_MAGIC_NEW_BEGIN, INTERNAL_MAGIC_NEW_END);
#if CWDEBUG_DEBUGM
    if (!__libcwd_tsd.annotation)
    {
      __libcwd_tsd.annotation = 1;
      annotation_alloc(size);
      __libcwd_tsd.annotation = 0;
    }
#endif
#endif // CWDEBUG_MAGIC
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `operator new': " <<
        PRINT_PTR(ZONE2APP(ptr1)) << " [" << saved_marker << ']' );
    return ZONE2APP(ptr1);

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "operator new (size = " << size << ") = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  appblock* ptr2 = internal_malloc(size, memblk_type_new CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker));
  if (!ptr2)
    DoutFatalInternal( dc::core, "Out of memory in `operator new'" );
#if CWDEBUG_MAGIC
  else
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_NEW_BEGIN, MAGIC_NEW_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}

void* operator new(size_t size, std::nothrow_t const&) throw ()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call || __libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call && !__libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif

  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "operator new (size = " << size << ", std::nothrow_t const&) = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  appblock* ptr2 = internal_malloc(size, memblk_type_new CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker));
  if (!ptr2)
    DoutFatalInternal( dc::core, "Out of memory in `operator new'" );
#if CWDEBUG_MAGIC
  else
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_NEW_BEGIN, MAGIC_NEW_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}

void* operator new[](size_t size) throw (std::bad_alloc)
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `operator new[]', size = " << size << " [" << saved_marker << ']' << " from " << (void*)__builtin_return_address(0) );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_malloc(size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

    size_t real_size = REAL_SIZE(size);
    if (size > real_size)
      DoutFatalInternal(dc::core, "Size too large: no space left for magic numbers in `operator new[]'");
    prezone* ptr1 = static_cast<prezone*>(__libc_malloc(real_size));
    if (!ptr1)
      DoutFatalInternal(dc::core, "Out of memory in `operator new[]'");
#if CWDEBUG_MAGIC
    SET_MAGIC(ptr1, size, INTERNAL_MAGIC_NEW_ARRAY_BEGIN, INTERNAL_MAGIC_NEW_ARRAY_END);
#if CWDEBUG_DEBUGM
    if (!__libcwd_tsd.annotation)
    {
      __libcwd_tsd.annotation = 1;
      annotation_alloc(size);
      __libcwd_tsd.annotation = 0;
    }
#endif
#endif // CWDEBUG_MAGIC
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `operator new[]': " <<
        PRINT_PTR(ZONE2APP(ptr1)) << " [" << saved_marker << ']' );
    return ZONE2APP(ptr1);

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "operator new[] (size = " << size << ") = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  appblock* ptr2 = internal_malloc(size, memblk_type_new_array CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker));
  if (!ptr2)
    DoutFatalInternal( dc::core, "Out of memory in `operator new[]'" );
#if CWDEBUG_MAGIC
  else
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_NEW_ARRAY_BEGIN, MAGIC_NEW_ARRAY_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}

void* operator new[](size_t size, std::nothrow_t const&) throw ()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call || __libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call && !__libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif

  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal(dc_malloc|continued_cf, "operator new[] (size = " << size << ", std::nothrow_t const&) = " LIBCWD_DEBUGM_OPT("[" << saved_marker << ']'));
  appblock* ptr2 = internal_malloc(size, memblk_type_new_array CALL_ADDRESS LIBCWD_COMMA_TSD LIBCWD_COMMA_DEBUGM_OPT(saved_marker));
  if (!ptr2)
    DoutFatalInternal( dc::core, "Out of memory in `operator new[]'" );
#if CWDEBUG_MAGIC
  else
  {
    prezone* ptr1 = APP2ZONE(ptr2);
    SET_MAGIC(ptr1, size, MAGIC_NEW_ARRAY_BEGIN, MAGIC_NEW_ARRAY_END);
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ASSERT_APPBLOCK(ptr2);
}

//=============================================================================
//
// operator `delete' and `delete []' replacements.
//

void operator delete(void* void_ptr) throw()
{
  appblock* ptr2 = static_cast<appblock*>(void_ptr);
#if LIBCWD_THREAD_SAFE
  // This marks the returned tsd as 'inside_free'.  Such tsds are static if the thread is
  // terminating and are never overwritten by other threads.
  libcwd::_private_::TSD_st& __libcwd_tsd(libcwd::_private_::TSD_st::instance_free());
#endif
  LIBCWD_DEBUGM_ASSERT(__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal);
#if CWDEBUG_DEBUGM && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC) && LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT( _private_::WST_ios_base_initialized || __libcwd_tsd.internal );
#endif
  internal_free(ptr2, from_delete LIBCWD_COMMA_TSD);
#if LIBCWD_THREAD_SAFE
  libcwd::_private_::TSD_st::free_instance(__libcwd_tsd);
#endif
}

void operator delete(void* void_ptr, std::nothrow_t const&) throw()
{
  appblock* ptr2 = static_cast<appblock*>(void_ptr);
#if LIBCWD_THREAD_SAFE
  // This marks the returned tsd as 'inside_free'.  Such tsds are static if the thread is
  // terminating and are never overwritten by other threads.
  libcwd::_private_::TSD_st& __libcwd_tsd(libcwd::_private_::TSD_st::instance_free());
#endif
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call || __libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call && !__libcwd_tsd.internal' failed.");
    core_dump();
  }
#endif
  internal_free(ptr2, from_delete LIBCWD_COMMA_TSD);
#if LIBCWD_THREAD_SAFE
  libcwd::_private_::TSD_st::free_instance(__libcwd_tsd);
#endif
}

void operator delete[](void* void_ptr) throw()
{
  appblock* ptr2 = static_cast<appblock*>(void_ptr);
#if LIBCWD_THREAD_SAFE
  // This marks the returned tsd as 'inside_free'.  Such tsds are static if the thread is
  // terminating and are never overwritten by other threads.
  libcwd::_private_::TSD_st& __libcwd_tsd(libcwd::_private_::TSD_st::instance_free());
#endif
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#endif
#if CWDEBUG_DEBUGM && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC) && LIBCWD_IOSBASE_INIT_ALLOCATES
  LIBCWD_DEBUGM_ASSERT(_private_::WST_ios_base_initialized || __libcwd_tsd.internal);
#endif
  internal_free(ptr2, from_delete_array LIBCWD_COMMA_TSD);	// Note that the standard demands that we call free(), and not delete().
  								// This forces everyone to overload both, operator delete() and operator
								// delete[]() and not only operator delete().
#if LIBCWD_THREAD_SAFE
  libcwd::_private_::TSD_st::free_instance(__libcwd_tsd);
#endif
}

void operator delete[](void* void_ptr, std::nothrow_t const&) throw()
{
  appblock* ptr2 = static_cast<appblock*>(void_ptr);
#if LIBCWD_THREAD_SAFE
  // This marks the returned tsd as 'inside_free'.  Such tsds are static if the thread is
  // terminating and are never overwritten by other threads.
  libcwd::_private_::TSD_st& __libcwd_tsd(libcwd::_private_::TSD_st::instance_free());
#endif
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call || __libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << (__LINE__ - 2) << ": " << __PRETTY_FUNCTION__ <<
	": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call && !__libcwd_tsd.internal' failed.");
    core_dump();
  }
#endif
  internal_free(ptr2, from_delete_array LIBCWD_COMMA_TSD);
#if LIBCWD_THREAD_SAFE
  libcwd::_private_::TSD_st::free_instance(__libcwd_tsd);
#endif
}

extern "C" {

void const* cwdebug_watch(void const* ptr) __attribute__ ((unused));

/**
 * \fn void const* cwdebug_watch(void const* ptr)
 * \ingroup chapter_gdb
 *
 * \brief Add a watch point for freeing \a ptr
 *
 * This function can be called from inside gdb.  After continuing the
 * application, gdb will stop when the memory that \a ptr is pointing
 * to is being freed.
 */
void const* cwdebug_watch(void const* ptr)
{
  using namespace libcwd;
  LIBCWD_TSD_DECLARATION;
  ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
  _private_::set_invisible_on(LIBCWD_TSD);
  memblk_info_base_ct memblk_info_dummy;
  alloc_ct const* alloc = find_memblk_info(memblk_info_dummy, true, ptr LIBCWD_COMMA_TSD);
  void const* start = NULL;
  if (!alloc)
    std::cout << ptr << " is not (part of) a dynamic allocation.\n";
  else
  {
    start = alloc->start();
    if (start != ptr)
      std::cout << ptr << "WARNING: points inside a memory allocation that starts at " << start << "\n";
    std::cout << "Added watch for freeing of allocation starting at " << start << "\n";
  }
  std::cout << std::flush;
  _private_::set_invisible_off(LIBCWD_TSD);
  --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
  return start;
}

int cwdebug_alloc(void const* ptr) __attribute__ ((unused));

/**
 * \fn int cwdebug_alloc(void const* ptr)
 * \ingroup chapter_gdb
 *
 * \brief Print information about the memory at the location \a ptr
 *
 * This function can be called from inside gdb.  It can be used
 * to figure out what/where some memory location was allocated.
 * This is especially handy in large applications where about everything
 * is dynamically allocated, like GUI applications.
 */
int cwdebug_alloc(void const* ptr)
{
  using namespace libcwd;
  LIBCWD_TSD_DECLARATION;
  ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
  _private_::set_invisible_on(LIBCWD_TSD);
  memblk_info_base_ct memblk_info;
  alloc_ct const* alloc = find_memblk_info(memblk_info, false, ptr LIBCWD_COMMA_TSD);
  if (!alloc)
    std::cout << ptr << " is not (part of) a dynamic allocation.\n";
  else
  {
    void const* start = alloc->start();
    if (start != ptr)
      std::cout << ptr << " points inside a memory allocation that starts at " << start << "\n";
    std::cout << "      start: " << start << '\n';
    std::cout << "       size: " << alloc->size() << '\n';
    type_info_ct const& type_info = alloc->type_info();
    std::cout << "       type: " << ((&type_info != &unknown_type_info_c) ?
	type_info.demangled_name() : "<No AllocTag>") << '\n';
    char const* desc = alloc->description();
    std::cout << "description: " << (desc ? desc : "<No AllocTag>") << '\n';
#if CWDEBUG_LOCATION
    std::cout << "   location: " << alloc->location() << '\n';
    char const* function_name = alloc->location().mangled_function_name();
    if (function_name != unknown_function_c)
    {
      std::cout << "in function: ";
      size_t s;
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      do
      {
	_private_::internal_string f;
	_private_::demangle_symbol(function_name, f);
	_private_::set_alloc_checking_on(LIBCWD_TSD);
	s = f.size();
	std::cout.write(f.data(), s);
	_private_::set_alloc_checking_off(LIBCWD_TSD);
      }
      while(0);
      _private_::set_alloc_checking_on(LIBCWD_TSD);
      std::cout << '\n';
    }
#endif
    struct tm* tbuf_ptr;
    struct timeval const& a_time(alloc->time());
    time_t tv_sec = a_time.tv_sec;			// On some OS, tv_sec is long.
#if LIBCWD_THREAD_SAFE
    struct tm tbuf;
    tbuf_ptr = localtime_r(&tv_sec, &tbuf);
#else
    tbuf_ptr = localtime(&tv_sec);
#endif
    char prev_fill = std::cout.fill('0');
    std::cout << "       when: ";
    std::cout.width(2);
    std::cout << tbuf_ptr->tm_hour << ':';
    std::cout.width(2);
    std::cout << tbuf_ptr->tm_min << ':';
    std::cout.width(2);
    std::cout << tbuf_ptr->tm_sec << '.';
    std::cout.width(6);
    std::cout << a_time.tv_usec << '\n';
    std::cout.fill(prev_fill);
    if (memblk_info.is_watched())
      std::cout << "This memory block is being watched for deletion.\n";
  }
  std::cout << std::flush;
  _private_::set_invisible_off(LIBCWD_TSD);
  --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
  return 0;
}

} // extern "C"

#endif // CWDEBUG_ALLOC
