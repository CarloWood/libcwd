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
// This tree also contains type of allocation (new/new[]/malloc/realloc).
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
// - void list_allocations_on(debug_ct& debug_object, ooam_filter_ct const& filter)
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
#include <libcw/debug_config.h>

#if CWDEBUG_ALLOC || defined(LIBCW_DOXYGEN)

#include <cstring>
#include <string>
#include <map>

#if LIBCWD_THREAD_SAFE

// We got the C++ config earlier by <bits/stl_alloc.h>.

#ifdef __STL_THREADS
#include <bits/stl_threads.h>   // For interface to _STL_mutex_lock (node allocator lock)
#if defined(__STL_GTHREADS)
#include "bits/gthr.h"
#else
#error You have an unsupported configuraton of gcc. Please tell that you dont have gthreads along with the output of gcc -v to libcw@alinoe.com.
#endif // __STL_GTHREADS


#endif // __STL_THREADS
#endif // LIBCWD_THREAD_SAFE
#include <iostream>
#include <iomanip>
#include <sys/time.h>		// Needed for gettimeofday(2)
#include "cwd_debug.h"
#include "ios_base_Init.h"
#include <libcw/cwprint.h>

#if LIBCWD_THREAD_SAFE
#if CWDEBUG_DEBUGT
#define UNSET_TARGETHREAD __libcwd_tsd.target_thread = NULL;
#else
#define UNSET_TARGETHREAD
#endif
#include <libcw/private_mutex.inl>
using libcw::debug::_private_::mutex_tct;
using libcw::debug::_private_::mutex_ct;
using libcw::debug::_private_::rwlock_tct;
using libcw::debug::_private_::location_cache_instance;
using libcw::debug::_private_::list_allocations_instance;
// We can't use a read/write lock here because that leads to a dead lock.
// rwlocks have to use condition variables or semaphores and both try to get a
// (libpthread internal) self-lock that is already set by libthread when it calls
// free() in order to destroy thread specific data 1st level arrays.
#define ACQUIRE_WRITE_LOCK(tt)	do { __libcwd_tsd.target_thread = tt; __libcwd_tsd.target_thread->thread_mutex.lock(); } while(0)
#define RELEASE_WRITE_LOCK	do { __libcwd_tsd.target_thread->thread_mutex.unlock(); UNSET_TARGETHREAD } while(0)
#define ACQUIRE_READ_LOCK(tt)	do { __libcwd_tsd.target_thread = tt; __libcwd_tsd.target_thread->thread_mutex.lock(); } while(0)
#define RELEASE_READ_LOCK	do { __libcwd_tsd.target_thread->thread_mutex.unlock(); UNSET_TARGETHREAD } while(0)
#define ACQUIRE_READ2WRITE_LOCK
#define ACQUIRE_WRITE2READ_LOCK
// We can rwlock_tct here, because this lock is never used from free(),
// only from new, new[], malloc and realloc.
#define ACQUIRE_LC_WRITE_LOCK		rwlock_tct<location_cache_instance>::wrlock()
#define RELEASE_LC_WRITE_LOCK		rwlock_tct<location_cache_instance>::wrunlock()
#define ACQUIRE_LC_READ_LOCK		rwlock_tct<location_cache_instance>::rdlock()
#define RELEASE_LC_READ_LOCK		rwlock_tct<location_cache_instance>::rdunlock()
#define ACQUIRE_LC_READ2WRITE_LOCK	rwlock_tct<location_cache_instance>::rd2wrlock()
#define ACQUIRE_LC_WRITE2READ_LOCK	rwlock_tct<location_cache_instance>::wr2rdlock()
#else // !LIBCWD_THREAD_SAFE
#define ACQUIRE_WRITE_LOCK(tt)
#define RELEASE_WRITE_LOCK
#define ACQUIRE_READ_LOCK(tt)
#define RELEASE_READ_LOCK
#define ACQUIRE_READ2WRITE_LOCK
#define ACQUIRE_WRITE2READ_LOCK
#define ACQUIRE_LC_WRITE_LOCK
#define RELEASE_LC_WRITE_LOCK
#define ACQUIRE_LC_READ_LOCK
#define RELEASE_LC_READ_LOCK
#define ACQUIRE_LC_READ2WRITE_LOCK
#define ACQUIRE_LC_WRITE2READ_LOCK
#endif // !LIBCWD_THREAD_SAFE

#if CWDEBUG_LOCATION
#define LIBCWD_COMMA_LOCATION(x) , x
#else
#define LIBCWD_COMMA_LOCATION(x)
#endif

#ifdef LIBCW_DOXYGEN
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

#if defined(LIBCWD_HAVE__LIBC_MALLOC) || defined(LIBCWD_HAVE___LIBC_MALLOC)
#ifdef LIBCWD_HAVE__LIBC_MALLOC
#define __libc_malloc _libc_malloc
#define __libc_calloc _libc_calloc
#define __libc_realloc _libc_realloc
#define __libc_free _libc_free
#endif
#else // USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE
#ifndef HAVE_DLOPEN
#error "configure bug: macros are inconsistent"
#endif
#define __libc_malloc (*libc_malloc)
#define __libc_calloc (*libc_calloc)
#define __libc_realloc (*libc_realloc)
#define __libc_free (*libc_free)
namespace libcw {
  namespace debug {
void* malloc_bootstrap1(size_t size);
void* calloc_bootstrap1(size_t nmemb, size_t size);
  }
}
void* __libc_malloc(size_t size) = libcw::debug::malloc_bootstrap1;
void* __libc_calloc(size_t nmemb, size_t size) = libcw::debug::calloc_bootstrap1;
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

#if !USE_DLOPEN_RATHER_THAN_MACROS_KLUDGE
extern "C" void* __libc_malloc(size_t size);
extern "C" void* __libc_calloc(size_t nmemb, size_t size);
extern "C" void* __libc_realloc(void* ptr, size_t size);
extern "C" void __libc_free(void* ptr);
#endif

namespace libcw {
  namespace debug {
    
namespace _private_ {

#if LIBCWD_THREAD_SAFE && CWDEBUG_DEBUGM
extern bool WST_multi_threaded;
#endif

#ifdef LIBCWD_USE_STRSTREAM
void* internal_strstreambuf_alloc(size_t size)
{
  LIBCWD_TSD_DECLARATION
  set_alloc_checking_off(LIBCWD_TSD);
  void* ptr = __libcwd_malloc(size);
  set_alloc_checking_on(LIBCWD_TSD);
  return ptr;
}

void internal_strstreambuf_free(void* ptr)
{
  LIBCWD_TSD_DECLARATION
  set_alloc_checking_off(LIBCWD_TSD);
  __libcwd_free(ptr);
  set_alloc_checking_on(LIBCWD_TSD);
}
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
  if (M_ptr && M_ptr->decrement())
  {
    set_alloc_checking_off(LIBCWD_TSD);
    delete M_ptr;
    set_alloc_checking_on(LIBCWD_TSD);
  }
}

void smart_ptr::copy_from(smart_ptr const& ptr)
{
  if (M_ptr != ptr.M_ptr)
  {
    LIBCWD_TSD_DECLARATION
    decrement(LIBCWD_TSD);
    M_ptr = ptr.M_ptr;
    increment();
  }
}

void smart_ptr::copy_from(char const* ptr)
{
  LIBCWD_TSD_DECLARATION
  decrement(LIBCWD_TSD);
  if (ptr)
  {
    set_alloc_checking_off(LIBCWD_TSD);
    M_ptr = new refcnt_charptr_ct(ptr);
    set_alloc_checking_on(LIBCWD_TSD);
  }
  else
    M_ptr = NULL;
}

void smart_ptr::copy_from(char* ptr)
{
  LIBCWD_TSD_DECLARATION
  decrement(LIBCWD_TSD);
  if (ptr)
  {
    set_alloc_checking_off(LIBCWD_TSD);
    M_ptr = new refcnt_charptr_ct(ptr);
    set_alloc_checking_on(LIBCWD_TSD);
  }
  else
    M_ptr = NULL;
}

} // namespace _private_

extern void ST_initialize_globals(void);

  } // namespace debug
} // namespace libcw

#if CWDEBUG_DEBUGM
#define DEBUGDEBUG_DoutInternal_MARKER DEBUGDEBUG_CERR( "DoutInternal at " << __FILE__ << ':' << __LINE__ )
#define DEBUGDEBUG_DoutFatalInternal_MARKER DEBUGDEBUG_CERR( "DoutFatalInternal at " << __FILE__ << ':' << __LINE__ )
#define DEBUGDEBUG_ELSE_DoutInternal(data) else DEBUGDEBUG_CERR( "library_call == " << __libcwd_tsd.library_call << "; DoutInternal skipped for: " << data );
#define DEBUGDEBUG_ELSE_DoutFatalInternal(data) FATALDEBUGDEBUG_CERR( "library_call == " << __libcwd_tsd.library_call << "; DoutFatalInternal skipped for: " << data );
#else
#define DEBUGDEBUG_DoutInternal_MARKER
#define DEBUGDEBUG_DoutFatalInternal_MARKER
#define DEBUGDEBUG_ELSE_DoutInternal(data)
#define DEBUGDEBUG_ELSE_DoutFatalInternal(data)
#endif

#define DoutInternal( cntrl, data )												\
  do																\
  {																\
    DEBUGDEBUG_DoutInternal_MARKER;												\
    if (__libcwd_tsd.library_call == 0 && LIBCWD_DO_TSD_MEMBER_OFF(libcw_do) < 0)						\
    {																\
      DEBUGDEBUG_CERR( "Entering 'DoutInternal(cntrl, \"" << data << "\")'.  internal == " << __libcwd_tsd.internal << '.' );	\
      channel_set_bootstrap_st channel_set(LIBCWD_DO_TSD(libcw_do) LIBCWD_COMMA_TSD);						\
      bool on;															\
      {																\
        using namespace channels;												\
        on = (channel_set|cntrl).on;												\
      }																\
      if (on)															\
      {																\
        LIBCWD_DO_TSD(libcw_do).start(libcw_do, channel_set LIBCWD_COMMA_TSD);							\
	++ LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);											\
	_private_::no_alloc_ostream_ct no_alloc_ostream(*LIBCWD_DO_TSD_MEMBER(libcw_do, current_oss)); 				\
        no_alloc_ostream << data;												\
	-- LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);											\
        LIBCWD_DO_TSD(libcw_do).finish(libcw_do, channel_set LIBCWD_COMMA_TSD);							\
      }																\
      DEBUGDEBUG_CERR( "Leaving 'DoutInternal(cntrl, \"" << data << "\")'.  internal = " << __libcwd_tsd.internal << '.' ); 	\
    }																\
    DEBUGDEBUG_ELSE_DoutInternal(data)												\
  } while(0)

#define DoutFatalInternal( cntrl, data )											\
  do																\
  {																\
    DEBUGDEBUG_DoutFatalInternal_MARKER;											\
    if (__libcwd_tsd.library_call < 2)												\
    {																\
      DEBUGDEBUG_CERR( "Entering 'DoutFatalInternal(cntrl, \"" << data << "\")'.  internal == " <<				\
			__libcwd_tsd.internal << "; setting internal to 0." ); 							\
      __libcwd_tsd.internal = 0;												\
      channel_set_bootstrap_st channel_set(LIBCWD_DO_TSD(libcw_do) LIBCWD_COMMA_TSD);						\
      {																\
	using namespace channels;												\
	channel_set&cntrl;													\
      }																\
      LIBCWD_DO_TSD(libcw_do).start(libcw_do, channel_set LIBCWD_COMMA_TSD);							\
      ++ LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);											\
      _private_::no_alloc_ostream_ct no_alloc_ostream(*LIBCWD_DO_TSD_MEMBER(libcw_do, current_oss)); 				\
      no_alloc_ostream << data;													\
      -- LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);											\
      LIBCWD_DO_TSD(libcw_do).fatal_finish(libcw_do, channel_set LIBCWD_COMMA_TSD);	/* Never returns */			\
      LIBCWD_ASSERT( !"Bug in libcwd!" );											\
    }																\
    else															\
    {																\
      DEBUGDEBUG_ELSE_DoutFatalInternal(data)											\
      LIBCWD_ASSERT( !"See msg above." );											\
      core_dump();														\
    }																\
  } while(0)

namespace libcw {
  namespace debug {

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

#ifdef __GLIBCPP__
  //
  // The following kludge is needed because of a bug in libstdc++-v3.
  //
  bool WST_ios_base_initialized = false;			// MT-safe: this is set to true before main() is reached and
  								// never changed anymore (see `inside_ios_base_Init_Init').

  // _private_::
  bool inside_ios_base_Init_Init(void)				// Single Threaded function.
  {
    LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
    LIBCWD_ASSERT( __libcwd_tsd.internal == 0 );
#endif
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
#endif // __GLIBCPP__

#if CWDEBUG_DEBUGM
// _private_::
void set_alloc_checking_off(LIBCWD_TSD_PARAM)
{
  LIBCWD_DEBUGM_CERR( "set_alloc_checking_off called from " << __builtin_return_address(0) << ": internal == " << __libcwd_tsd.internal << "; setting it to " << __libcwd_tsd.internal + 1 << '.' );
  ++__libcwd_tsd.internal;
}

// _private_::
void set_alloc_checking_on(LIBCWD_TSD_PARAM)
{
  if (__libcwd_tsd.internal == 0)
    DoutFatal(dc::core, "Calling `set_alloc_checking_on' while ALREADY on.");
  LIBCWD_DEBUGM_CERR( "set_alloc_checking_on called from " << __builtin_return_address(0) << ": internal == " << __libcwd_tsd.internal << "; setting it to " << __libcwd_tsd.internal - 1 << '.' );
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
    case memblk_type_external:
      os.write("external  ", 10);
      break;
  }
}

#if CWDEBUG_DEBUGM
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, memblk_key_ct const& data)
{
  write(2, "<memblk_key_ct>", 15);
  return raw_write;
}
#endif

class dm_alloc_base_ct : public alloc_ct {
public:
  dm_alloc_base_ct(void const* s, size_t sz, memblk_types_nt type,
      type_info_ct const& ti, struct timeval const& t LIBCWD_COMMA_LOCATION(location_ct const* l))
    : alloc_ct(s, sz, type, ti, t LIBCWD_COMMA_LOCATION(l)) { }
  void print_description(ooam_filter_ct const& filter LIBCWD_COMMA_TSD_PARAM) const;
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
  dm_alloc_ct** my_list;	// Pointer to my list, never NULL.
  dm_alloc_ct* my_owner_node;	// Pointer to node who's a_next_list contains
  				// this object.

public:
  dm_alloc_ct(void const* s, size_t sz, memblk_types_nt f, struct timeval const& t
              LIBCWD_COMMA_TSD_PARAM LIBCWD_COMMA_LOCATION(location_ct const* l)) :		// MT-safe: write lock is set.
      dm_alloc_base_ct(s, sz , f, unknown_type_info_c, t LIBCWD_COMMA_LOCATION(l)), prev(NULL), a_next_list(NULL)
      {
#if CWDEBUG_DEBUGT
	LIBCWD_ASSERT( __libcwd_tsd.target_thread == &(*__libcwd_tsd.thread_iter) );
#endif
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
  dm_alloc_ct(dm_alloc_ct const& __dummy) : dm_alloc_base_ct(__dummy) { LIBCWD_TSD_DECLARATION DoutFatalInternal( dc::fatal, "Calling dm_alloc_ct::dm_alloc_ct(dm_alloc_ct const&)" ); }
    // No copy constructor allowed.
#endif
  ~dm_alloc_ct();
  void new_list(LIBCWD_TSD_PARAM)							// MT-safe: write lock is set.
      {
#if CWDEBUG_DEBUGT
	// A new list is always added for the current thread.
	LIBCWD_ASSERT( __libcwd_tsd.target_thread == &(*__libcwd_tsd.thread_iter) );
#endif
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
  static void descend_current_alloc_list(LIBCWD_TSD_PARAM) throw();			// MT-safe: write lock is set.
  friend inline std::ostream& operator<<(std::ostream& os, dm_alloc_ct const& alloc) { alloc.printOn(os); return os; }
  friend inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, dm_alloc_ct const& alloc) { alloc.printOn(os.M_os); return os; }
};

class dm_alloc_copy_ct : public dm_alloc_base_ct {
private:
  dm_alloc_copy_ct* next;
  dm_alloc_copy_ct* a_next_list;
public:
  dm_alloc_copy_ct(dm_alloc_ct const& alloc);
  void show_alloc_list(int depth, channel_ct const& channel, ooam_filter_ct const& filter) const;
};

dm_alloc_copy_ct::dm_alloc_copy_ct(dm_alloc_ct const& alloc) : dm_alloc_base_ct(alloc)
{
  if (alloc.next)
    next = new dm_alloc_copy_ct(*alloc.next);
  else
    next = NULL;
  if (alloc.a_next_list)
    a_next_list = new dm_alloc_copy_ct(*alloc.a_next_list);
  else
    a_next_list = NULL;
}

typedef dm_alloc_ct const const_dm_alloc_ct;

// Set `CURRENT_ALLOC_LIST' back to its parent list.
void dm_alloc_ct::descend_current_alloc_list(LIBCWD_TSD_PARAM) throw()			// MT-safe: write lock is set.
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

class memblk_key_ct {
private:
  void const* a_start;		// Start of allocated memory block
  void const* a_end;		// End of allocated memory block
public:
  memblk_key_ct(void const* s, size_t size) : a_start(s), a_end(static_cast<char const*>(s) + size) {}
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
      LIBCWD_TSD_DECLARATION
      LIBCWD_ASSERT( __libcwd_tsd.internal );
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

class memblk_info_ct {
#if CWDEBUG_MARKER
  friend class marker_ct;
  friend void move_outside(marker_ct* marker, void const* ptr);
#endif
private:
  memblk_types_nt M_memblk_type;
  lockable_auto_ptr<dm_alloc_ct> a_alloc_node;
    // The associated `dm_alloc_ct' object.
    // This must be a pointer because the allocated `dm_alloc_ct' can persist
    // after this memblk_info_ct is deleted (dm_alloc_ct marked `is_deleted'),
    // when it still has allocated 'childeren' in `a_next_list' of its own.
public:
  memblk_info_ct(void const* s, size_t sz, memblk_types_nt f, struct timeval const& t LIBCWD_COMMA_TSD_PARAM LIBCWD_COMMA_LOCATION(location_ct const* l));
  void erase(LIBCWD_TSD_PARAM);
  void lock(void) { a_alloc_node.lock(); }
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
  void change_flags(memblk_types_nt new_flag)
      { M_memblk_type = new_flag; if (has_alloc_node()) a_alloc_node.get()->change_flags(new_flag); }
  void new_list(LIBCWD_TSD_PARAM) const { a_alloc_node.get()->new_list(LIBCWD_TSD); }			// MT-safe: write lock is set.
  memblk_types_nt flags(void) const { return M_memblk_type; }
  void print_description(ooam_filter_ct const& filter LIBCWD_COMMA_TSD_PARAM) const
      { a_alloc_node.get()->print_description(filter LIBCWD_COMMA_TSD); }
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

static int WST_initialization_state;		// MT-safe: We will reach state '1' the first call to malloc.
						// We *assume* that the first call to malloc is before we reach
						// main(), or at least before threads are created.

  //  0: memblk_map, location_cache_map and libcwd both not initialized yet.
  //  1: memblk_map, location_cache_map and libcwd both initialized.
  // -1: memblk_map and location_cache_map initialized but libcwd not initialized yet.

//=============================================================================
//
// Location cache
//

location_ct const* location_cache(void const* addr LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
  bool found;
  location_ct* location_info;
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
  return location_info;
}

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

dm_alloc_ct::~dm_alloc_ct()						// MT-safe: write lock is set.
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( __libcwd_tsd.internal );
  if (a_next_list)
    DoutFatalInternal( dc::core, "Removing an dm_alloc_ct that still has an own list" );
  dm_alloc_ct* tmp;
  for(tmp = *my_list; tmp && tmp != this; tmp = tmp->next);
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
}

#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#if CWDEBUG_LOCATION
// Bug work around.
inline std::ostream& operator<<(std::ostream& os, smanip<int> const& smanip)
{
  return std::operator<<(os, smanip);
}

inline std::ostream& operator<<(std::ostream& os, smanip<unsigned long> const& smanip)
{
  return std::operator<<(os, smanip);
}
#endif
#endif

extern void demangle_symbol(char const* in, _private_::internal_string& out);

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

void dm_alloc_base_ct::print_description(ooam_filter_ct const& filter LIBCWD_COMMA_TSD_PARAM) const
{
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.internal && !__libcwd_tsd.library_call );
#endif
#if CWDEBUG_LOCATION
  LibcwDoutScopeBegin(channels, libcw_do, dc::continued);
  if (filter.M_flags & show_objectfile)
  {
    object_file_ct const* object_file = M_location->object_file();
    if (object_file)
      LibcwDoutStream << object_file->filename() << ':';
    else
      LibcwDoutStream << "<unknown object file>:";
  }
  if (M_location->is_known())
  {
    if (filter.M_flags & show_path)
    {
      size_t len = M_location->filepath_length();
      if (len < 20)
	LibcwDoutStream.write(twentyfive_spaces_c, 20 - len);
      M_location->print_filepath_on(LibcwDoutStream);
    }
    else
    {
      size_t len = M_location->filename_length();
      if (len < 20)
	LibcwDoutStream.write(twentyfive_spaces_c, 20 - len);
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
  else if (M_location->mangled_function_name() != unknown_function_c)
  {
    _private_::internal_string f;
    demangle_symbol(M_location->mangled_function_name(), f);
    size_t s = f.size();
    LibcwDoutStream.write(f.data(), s);
    if (s < 25)
      LibcwDoutStream.write(twentyfive_spaces_c, 25 - s);
    LibcwDoutStream.put(' ');
  }
  else
    LibcwDoutStream.write(twentyfive_spaces_c, 25);
  LibcwDoutScopeEnd;
#endif

#if CWDEBUG_MARKER
  if (a_memblk_type == memblk_type_marker || a_memblk_type == memblk_type_deleted_marker)
    DoutInternal( dc::continued, "<marker>;" );
  else
#endif

  {
    char const* a_type = type_info_ptr->demangled_name();
    size_t s = a_type ? strlen(a_type) : 0;		// Can be 0 while deleting a qualifiers_ct object in demangle3.cc
    if (s > 0 && a_type[s - 1] == '*' && type_info_ptr->ref_size() != 0)
    {
      __libcwd_tsd.internal = 1;
      _private_::internal_stringstream* buf = new _private_::internal_stringstream;
      if (a_memblk_type == memblk_type_new || a_memblk_type == memblk_type_deleted)
      {
	if (s > 1 && a_type[s - 2] == ' ')
	  buf->write(a_type, s - 2);
	else
	  buf->write(a_type, s - 1);
      }
      else /* if (a_memblk_type == memblk_type_new_array || a_memblk_type == memblk_type_deleted_array) */
      {
	buf->write(a_type, s - 1);
	// We can't use operator<<(ostream&, int) because that uses std::alloc.
	buf->put('[');
	_private_::no_alloc_print_int_to(buf, a_size / type_info_ptr->ref_size(), false);
	buf->put(']');
      }
#ifdef LIBCWD_USE_STRSTREAM
      buf->put('\0');
#endif
      DoutInternal( dc::continued, buf->str() );
#ifdef LIBCWD_USE_STRSTREAM
      buf->freeze(0);
#endif
      delete buf;
      __libcwd_tsd.internal = 0;
    }
    else
      DoutInternal( dc::continued, a_type );

    DoutInternal( dc::continued, ';' );
  }

  DoutInternal( dc::continued, " (sz = " << a_size << ") " );

  if (!a_description.is_null())
    DoutInternal( dc::continued, ' ' << a_description );
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

void dm_alloc_copy_ct::show_alloc_list(int depth, channel_ct const& channel, ooam_filter_ct const& filter) const
{
#if LIBCWD_THREAD_SAFE && CWDEBUG_DEBUG
  LIBCWD_ASSERT( _private_::is_locked(list_allocations_instance) );
#endif
  dm_alloc_copy_ct const* alloc;
  LIBCWD_TSD_DECLARATION
  for (alloc = this; alloc; alloc = alloc->next)
  {
    const_cast<location_ct*>(&alloc->location())->handle_delayed_initialization();
    object_file_ct const* object_file = alloc->location().object_file();
    if (object_file && object_file->hide_from_alloc_list())
      continue;
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
    LibcwDoutScopeBegin(channels, libcw_do, channel|nolabel_cf|continued_cf);
    for (int i = depth; i > 1; i--)
      LibcwDoutStream << "    ";
    if (filter.M_flags & show_time)
    {
      struct tm* tbuf_ptr;
#if LIBCWD_THREAD_SAFE
      struct tm tbuf;
      tbuf_ptr = localtime_r(&alloc->a_time.tv_sec, &tbuf);
#else
      tbuf_ptr = localtime(&alloc->a_time.tv_sec);
#endif
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
    alloc->print_description(filter LIBCWD_COMMA_TSD);
    Dout( dc::finish, "" );
    if (alloc->a_next_list)
      alloc->a_next_list->show_alloc_list(depth + 1, channel, filter);
  }
}

//=============================================================================
//
// memblk_ct methods
//

inline memblk_info_ct::memblk_info_ct(void const* s, size_t sz,
    memblk_types_nt f, struct timeval const& t LIBCWD_COMMA_TSD_PARAM LIBCWD_COMMA_LOCATION(location_ct const* l))
  : M_memblk_type(f), a_alloc_node(new dm_alloc_ct(s, sz, f, t LIBCWD_COMMA_TSD LIBCWD_COMMA_LOCATION(l))) { }	// MT-safe: write lock is set.

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

void memblk_info_ct::erase(LIBCWD_TSD_PARAM)
{
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( __libcwd_tsd.internal );
#endif
  dm_alloc_ct* ap = a_alloc_node.get();
#if CWDEBUG_DEBUGM
  if (ap)
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: memblk_info_ct::erase for " << ap->start() );
#endif
  if (ap && ap->next_list())
  {
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
  }
}

void memblk_info_ct::make_invisible(void)
{
#if LIBCWD_THREAD_SAFE && CWDEBUG_DEBUG
  LIBCWD_TSD_DECLARATION
  LIBCWD_ASSERT( (*__libcwd_tsd.thread_iter).thread_mutex.is_locked() ); // MT-safe: write lock is set (needed for ~dm_alloc_ct).
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

#if CWDEBUG_MAGIC

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

#define REST_OF_EXPLANATION " with alloc checking OFF (a libcwd 'internal' allocation).  Note that allocations done inside a 'Dout()' are 'internal' (which means that alloc_checking is off): don't allocate memory while writing debug output!";

char const* diagnose_magic(size_t magic_begin, size_t const* magic_end)
{
  switch(magic_begin)
  {
    case INTERNAL_MAGIC_NEW_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_NEW_END)
        return ") (while alloc checking is on) that was allocated with 'new'" REST_OF_EXPLANATION;
      break;
    case INTERNAL_MAGIC_NEW_ARRAY_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_NEW_ARRAY_END)
        return ") (while alloc checking is on) that was allocated with 'new[]'" REST_OF_EXPLANATION;
      break;
    case INTERNAL_MAGIC_MALLOC_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_MALLOC_END)
        return ") (while alloc checking is on) that was allocated with 'malloc()'" REST_OF_EXPLANATION;
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
    default:
      switch(magic_begin)
      {
        case ~INTERNAL_MAGIC_NEW_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'new' internally by libcwd.";
        case ~INTERNAL_MAGIC_NEW_ARRAY_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'new[]' internally by libcwd.";
	case ~INTERNAL_MAGIC_MALLOC_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'malloc()' internally by libcwd.";
        case ~MAGIC_NEW_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'new'.";
        case ~MAGIC_NEW_ARRAY_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'new[]'.";
        case ~MAGIC_MALLOC_BEGIN:
	  return ") which appears to be a deleted block that was originally allocated with 'malloc()'.";
      }
      return ") which seems uninitialized, or has a corrupt first magic number (buffer underrun?).";
  }
  switch (*magic_end)
  {
    case INTERNAL_MAGIC_NEW_END:
    case INTERNAL_MAGIC_NEW_ARRAY_END:
    case INTERNAL_MAGIC_MALLOC_END:
    case MAGIC_NEW_END:
    case MAGIC_NEW_ARRAY_END:
    case MAGIC_MALLOC_END:
    case ~INTERNAL_MAGIC_NEW_END:
    case ~INTERNAL_MAGIC_NEW_ARRAY_END:
    case ~INTERNAL_MAGIC_MALLOC_END:
    case ~MAGIC_NEW_END:
    case ~MAGIC_NEW_ARRAY_END:
    case ~MAGIC_MALLOC_END:
      return ") with inconsistant magic numbers!";
  }
  return ") with a corrupt second magic number (buffer overrun?)!";
}

#endif // CWDEBUG_MAGIC

//=============================================================================
//
// internal_malloc
//
// Allocs a new block of size `size' and updates the internal administration.
//
// Note: This function is called by `__libcwd_malloc', `__libcwd_calloc' and
// `operator new' which end with a call to DoutInternal( dc_malloc|continued_cf, ...)
// and should therefore end with a call to DoutInternal( dc::finish, ptr ).
//

#  ifdef LIBCWD_NEED_WORD_ALIGNMENT
#define SIZE_PLUS_TWELVE(s) ((((s) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1)) + 3 * sizeof(size_t))
#define SIZE_PLUS_FOUR(s) ((((s) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1)) + sizeof(size_t))
#  else // !LIBCWD_NEED_WORD_ALIGNMENT
#define SIZE_PLUS_TWELVE(s) ((s) + 3 * sizeof(size_t))
#define SIZE_PLUS_FOUR(s) ((s) + sizeof(size_t))
#  endif

#if CWDEBUG_LOCATION
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
static void* internal_malloc(size_t size, memblk_types_nt flag, void* call_addr LIBCWD_COMMA_TSD_PARAM, int saved_marker)
#else
static void* internal_malloc(size_t size, memblk_types_nt flag, void* call_addr LIBCWD_COMMA_TSD_PARAM)
#endif
#else // !CWDEBUG_LOCATION
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
static void* internal_malloc(size_t size, memblk_types_nt flag LIBCWD_COMMA_TSD_PARAM, int saved_marker)
#else
static void* internal_malloc(size_t size, memblk_types_nt flag LIBCWD_COMMA_TSD_PARAM)
#endif
#endif // !CWDEBUG_LOCATION
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

  register void* mptr;
#if !CWDEBUG_MAGIC
  if (!(mptr = __libc_malloc(size)))
#else
  if ((mptr = static_cast<char*>(__libc_malloc(SIZE_PLUS_TWELVE(size))) + 2 * sizeof(size_t)) == (void*)(2 * sizeof(size_t)))
#endif
  {
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
    DoutInternal( dc::finish, "NULL [" << saved_marker << ']' );
#else
    DoutInternal( dc::finish, "NULL" );
#endif
    DoutInternal( dc_malloc, "Out of memory ! this is only a pre-detection!" );
    return NULL;	// A fatal error should occur directly after this
  }

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
  memblk_info_ct* memblk_info;

  LIBCWD_DEFER_CANCEL;

  DEBUGDEBUG_CERR( "internal_malloc: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;

  // Update our administration:
  struct timeval alloc_time;
  gettimeofday(&alloc_time, 0);
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  std::pair<memblk_map_ct::iterator, bool> const&
      iter(memblk_map_write->insert(memblk_ct(memblk_key_ct(mptr, size),
	      memblk_info_ct(mptr, size, flag, alloc_time LIBCWD_COMMA_TSD LIBCWD_COMMA_LOCATION(loc)))));
  memblk_info = &(*iter.first).second;
#if CWDEBUG_DEBUGM
  error = !iter.second;
#endif
  RELEASE_WRITE_LOCK;

  DEBUGDEBUG_CERR( "internal_malloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;

  LIBCWD_RESTORE_CANCEL;

#if CWDEBUG_DEBUGM
  // MT-safe: iter points to an element that is garanteed unique to this thread (we just allocated it).
  if (error)
    DoutFatalInternal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );
#endif

  memblk_info->lock();				// Lock ownership (doesn't call malloc).

#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  DoutInternal( dc::finish, (void*)(mptr) << " [" << saved_marker << ']' );
#else
  DoutInternal( dc::finish, (void*)(mptr) );
#endif
  return mptr;
}

#if LIBCWD_THREAD_SAFE
static bool search_in_maps_of_other_threads(void const* ptr, memblk_map_ct::const_iterator& iter LIBCWD_COMMA_TSD_PARAM)
{
  using _private_::threadlist_instance;
  using _private_::threadlist_t;
  using _private_::threadlist;
  bool found = false;
  mutex_tct<threadlist_instance>::lock();
  for(threadlist_t::iterator thread_iter = threadlist->begin(); thread_iter != threadlist->end(); ++thread_iter)
  {
    if (thread_iter == __libcwd_tsd.thread_iter)
      continue;	// Already searched.
    ACQUIRE_READ_LOCK(&(*thread_iter));
    iter = target_memblk_map_read->find(memblk_key_ct(ptr, 0));
    found = (iter != target_memblk_map_read->end());
    if (found)
      break;
    RELEASE_READ_LOCK;
  }
  mutex_tct<threadlist_instance>::unlock();
  return found;
}
#endif

static ooam_filter_ct const default_ooam_filter(0);
static void internal_free(void* ptr, deallocated_from_nt from LIBCWD_COMMA_TSD_PARAM)
{
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << __LINE__ - 2 << ": " << __PRETTY_FUNCTION__ << ": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#endif
#if CWDEBUG_DEBUGM && defined(__GLIBCPP__) && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC)
  LIBCWD_ASSERT( _private_::WST_ios_base_initialized || __libcwd_tsd.internal );
#endif
  if (__libcwd_tsd.internal)
  {
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
    if (from == from_delete)
    {
      LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal `delete(" << ptr << ")' [" << ++__libcwd_tsd.marker << ']' );
    }
    else if (from == from_delete_array)
    {
      LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal `delete[](" << ptr << ")' [" << ++__libcwd_tsd.marker << ']' );
    }
    else
    {
      LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal `free(" << ptr << ")' [" << ++__libcwd_tsd.marker << ']' );
    }
#endif // CWDEBUG_DEBUGM
#if CWDEBUG_MAGIC
    if (!ptr)
      return;
    ptr = static_cast<size_t*>(ptr) - 2;
    if (from == from_delete)
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_NEW_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_NEW_END)
        DoutFatalInternal( dc::core, "internal delete: " << diagnose_from(from, true) << (static_cast<size_t*>(ptr) + 2) <<
	    diagnose_magic(((size_t*)ptr)[0], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])) - 1) );
    }
    else if (from == from_delete_array)
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_NEW_ARRAY_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_NEW_ARRAY_END)
        DoutFatalInternal( dc::core, "internal delete[]: " << diagnose_from(from, true) << (static_cast<size_t*>(ptr) + 2) <<
	    diagnose_magic(((size_t*)ptr)[0], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])) - 1) );
    }
    else
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_MALLOC_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_MALLOC_END)
        DoutFatalInternal( dc::core, "internal free: " << diagnose_from(from, true) << (static_cast<size_t*>(ptr) + 2) <<
	    diagnose_magic(((size_t*)ptr)[0], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])) - 1) );
    }
    ((size_t*)ptr)[0] ^= (size_t)-1;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] ^= (size_t)-1;
#endif // CWDEBUG_MAGIC
    __libc_free(ptr);
    return;
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
  if (!ptr)
  {
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
    DoutInternal( dc_malloc, "Trying to free NULL - ignored [" << ++__libcwd_tsd.marker << "]." );
#else
    DoutInternal( dc_malloc, "Trying to free NULL - ignored." );
#endif
    --__libcwd_tsd.inside_malloc_or_free;
    return;
  }

#if LIBCWD_THREAD_SAFE
  LIBCWD_DEFER_CANCEL_NO_BRACE;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr, 0)));
#endif
  bool found = (iter != target_memblk_map_read->end() && (*iter).first.start() == ptr);
#if LIBCWD_THREAD_SAFE
  if (!found)
  {
    RELEASE_READ_LOCK;
    found = search_in_maps_of_other_threads(ptr, iter, __libcwd_tsd) && (*iter).first.start() == ptr;
  }
#endif
  if (!found)
  {
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
#if CWDEBUG_MAGIC
    if (((size_t*)ptr)[-2] == INTERNAL_MAGIC_NEW_BEGIN ||
        ((size_t*)ptr)[-2] == INTERNAL_MAGIC_NEW_ARRAY_BEGIN ||
	((size_t*)ptr)[-2] == INTERNAL_MAGIC_MALLOC_BEGIN)
      DoutFatalInternal( dc::core, "Trying to " <<
          ((from == from_delete) ? "delete" : ((from == from_free) ? "free" : "delete[]")) <<
	  " a pointer (" << ptr << ") that appears to be internally allocated!  The magic number diagnostic gives: " <<
	  diagnose_from(from, false) << ptr << diagnose_magic(((size_t*)ptr)[-2], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])) - 1) );

#endif
    DoutFatalInternal( dc::core, "Trying to " <<
        ((from == from_delete) ? "delete" : ((from == from_free) ? "free" : "delete[]")) << " an invalid pointer (" << ptr << ')' );
  }
  else
  {
    memblk_types_nt f = (*iter).second.flags();
    bool visible = (!__libcwd_tsd.library_call && (*iter).second.has_alloc_node());
    if (visible)
    {
      DoutInternal( dc_malloc|continued_cf,
	  ((from == from_free) ? "free(" : ((from == from_delete) ? "delete " : "delete[] "))
	  << ptr << ((from == from_free) ? ") " : " ") );
      if (channels::dc_malloc.is_on())
	(*iter).second.print_description(default_ooam_filter LIBCWD_COMMA_TSD);
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
      DoutInternal( dc::continued, " [" << ++__libcwd_tsd.marker << "] " );
#else
      DoutInternal( dc::continued, ' ' );
#endif
    }
    if (expected_from[f] != from)
    {
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
#endif

    // Convert lock into a writers lock without allowing other writers to start.
    // First unlocking the reader and then locking a writer would make it possible
    // that the application would crash when two threads try to free simultaneously
    // the same memory block (instead of detecting that and exiting gracefully).
    ACQUIRE_READ2WRITE_LOCK;
    (*memblk_iter_write).second.erase(LIBCWD_TSD);	// Update flags and optional decouple
    target_memblk_map_write->erase(memblk_iter_write);		// Update administration
    RELEASE_WRITE_LOCK;

    DEBUGDEBUG_CERR( "__libcwd_free: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
    __libcwd_tsd.internal = 0;

    LIBCWD_RESTORE_CANCEL_NO_BRACE;

#if CWDEBUG_MAGIC
    if (f == memblk_type_external)
      __libc_free(ptr);
    else
    {
      if (from == from_delete)
      {
	if (((size_t*)ptr)[-2] != MAGIC_NEW_BEGIN ||
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_NEW_END)
	  DoutFatalInternal( dc::core, diagnose_from(from, false, has_alloc_node) << ptr << diagnose_magic(((size_t*)ptr)[-2], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])) - 1) );
      }
      else if (from == from_delete_array)
      {
	if (((size_t*)ptr)[-2] != MAGIC_NEW_ARRAY_BEGIN ||
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_NEW_ARRAY_END)
	  DoutFatalInternal( dc::core, diagnose_from(from, false, has_alloc_node) << ptr << diagnose_magic(((size_t*)ptr)[-2], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])) - 1) );
      }
      else
      {
	if (((size_t*)ptr)[-2] != MAGIC_MALLOC_BEGIN ||
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_MALLOC_END)
	  DoutFatalInternal( dc::core, diagnose_from(from, false, has_alloc_node) << ptr << diagnose_magic(((size_t*)ptr)[-2], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])) - 1) );
      }
      ((size_t*)ptr)[-2] ^= (size_t)-1;
      ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] ^= (size_t)-1;
      __libc_free(static_cast<size_t*>(ptr) - 2);		// Free memory block
    }
#else // !CWDEBUG_MAGIC
    __libc_free(ptr);			// Free memory block
#endif // !CWDEBUG_MAGIC

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
static char allocation_heap[128];
static void* allocation_ptrs[4];
static unsigned int allocation_counter = 0;
static char* allocation_ptr = allocation_heap;

void* malloc_bootstrap2(size_t size)
{
  assert( allocation_counter <= sizeof(allocation_ptrs) / sizeof(void*) );
  assert( allocation_ptr + size <= allocation_heap + sizeof(allocation_heap) );
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

#undef dlopen
#undef dlclose
void init_malloc_function_pointers(void)
{
  // Point functions to next phase.
  libc_malloc = malloc_bootstrap2;
  libc_calloc = calloc_bootstrap2;
  libc_realloc = realloc_bootstrap2;
  libc_free = free_bootstrap2;
#if !LIBCWD_THREAD_SAFE
  // Please mail libcwd@alinoe.com if you need to extend this.
  char const* libc_filename[] = {
    "/usr/lib/libc.so.4",		// FreeBSD 4.5
    "/lib/libc.so.1",			// Solaris 8
    NULL
  };
#else
  // Please mail libcwd@alinoe.com if you need to extend this.
  char const* libc_filename[] = {
    "/usr/lib/libc_r.so.4",		// FreeBSD 4.5
    "/lib/libc.so.1",			// Solaris 8
    NULL
  };
#endif
  // This calls malloc.
  void* handle = NULL;
  int i = 0;
  while (!handle && libc_filename[i])
  {
    handle = ::dlopen(libc_filename[i], RTLD_LAZY);
    ++i;
  }
  assert(handle);
  void* (*libc_malloc_tmp)(size_t size);
  void* (*libc_calloc_tmp)(size_t nmemb, size_t size);
  void* (*libc_realloc_tmp)(void* ptr, size_t size);
  void (*libc_free_tmp)(void* ptr);
  libc_malloc_tmp = (void* (*)(size_t))dlsym(handle, "malloc");
  assert(libc_malloc_tmp);
  libc_calloc_tmp = (void* (*)(size_t, size_t))dlsym(handle, "calloc");
  assert(libc_calloc_tmp);
  libc_realloc_tmp = (void* (*)(void*, size_t))dlsym(handle, "realloc");
  assert(libc_realloc_tmp);
  libc_free_tmp = (void (*)(void*))dlsym(handle, "free");
  assert(libc_free_tmp);
  // Hopefully this calls free again.
  ::dlclose(handle);
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

// Very first time that malloc(2) or calloc(2) are called; initialize the function pointers.

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
  UNSET_TARGETHREAD
  return memblk_map;
}

bool delete_memblk_map(void* ptr LIBCWD_COMMA_TSD_PARAM)
{
  // Already internal.  Called from thread_ct::tsd_destroyed()
  memblk_map_ct* memblk_map = reinterpret_cast<memblk_map_ct*>(ptr);
  bool deleted = false;
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  if (memblk_map->size() == 0)
  {
    delete memblk_map;
    deleted = true;
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
    LIBCWD_TSD_DECLARATION
    // This block is Single Threaded.
    if (WST_initialization_state == 0)			// Only true once.
    {
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      // MT-safe: There are no threads created yet when we get here.
      location_cache_map.MT_unsafe = new location_cache_map_ct;
#if CWDEBUG_DEBUG && LIBCWD_THREAD_SAFE
      LIBCWD_ASSERT( !_private_::WST_multi_threaded );
#endif
#if !LIBCWD_THREAD_SAFE
      // With threads, memblk_map is initialized in 'LIBCWD_TSD_DECLARATION'.
      ST_memblk_map = new memblk_map_ct;
#endif
      WST_initialization_state = -1;
      _private_::set_alloc_checking_on(LIBCWD_TSD);
    }
    if (1
#ifdef __GLIBCPP__
        // "ios_base" is always initialized for libstdc++ version 2.
	&& !_private_::WST_ios_base_initialized && !_private_::inside_ios_base_Init_Init()
#endif // __GLIBCPP__
        )
    {
      WST_initialization_state = 1;		// ST_initialize_globals() calls malloc again of course.
      int recursive_store = __libcwd_tsd.inside_malloc_or_free;
      __libcwd_tsd.inside_malloc_or_free = 0;	// Allow that (this call to malloc will not have done from STL allocator).
      libcw::debug::ST_initialize_globals();	// This doesn't belong in the malloc department at all, but malloc() happens
						// to be a function that is called _very_ early - and hence this is a good moment
						// to initialize ALL of libcwd.
      __libcwd_tsd.inside_malloc_or_free = recursive_store;
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
bool test_delete(void const* ptr)
{
  LIBCWD_TSD_DECLARATION
  bool found;
#if LIBCWD_THREAD_SAFE
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr, 0)));
#endif
  // MT: Because the expression `(*iter).first.start()' is included inside the locked
  //     area too, no core dump will occur when another thread would be deleting
  //     this allocation at the same time.  The behaviour of the application would
  //     still be undefined however because it makes it possible that this function
  //     returns false (not deleted) for a deleted memory block.
  found = (iter != target_memblk_map_read->end() && (*iter).first.start() == ptr);
#if LIBCWD_THREAD_SAFE
  RELEASE_READ_LOCK;
  if (!found)
    found = search_in_maps_of_other_threads(ptr, iter, __libcwd_tsd) && (*iter).first.start() == ptr;
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
#if CWDEBUG_DEBUG
  LIBCWD_TSD_DECLARATION
#endif
  LIBCWD_DEFER_CANCEL;
  _private_::mutex_tct<_private_::threadlist_instance>::lock();
  for(_private_::threadlist_t::iterator thread_iter = _private_::threadlist->begin(); thread_iter != _private_::threadlist->end(); ++thread_iter)
  {
    ACQUIRE_READ_LOCK(&(*thread_iter));
#endif
    memsize += MEMSIZE;
#if LIBCWD_THREAD_SAFE
    RELEASE_READ_LOCK;
  }
  _private_::mutex_tct<_private_::threadlist_instance>::unlock();
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
#if CWDEBUG_DEBUG
  LIBCWD_TSD_DECLARATION
#endif
  LIBCWD_DEFER_CANCEL;
  _private_::mutex_tct<_private_::threadlist_instance>::lock();
  for(_private_::threadlist_t::iterator thread_iter = _private_::threadlist->begin(); thread_iter != _private_::threadlist->end(); ++thread_iter)
  {
    ACQUIRE_READ_LOCK(&(*thread_iter));
#endif
    memblks += MEMBLKS;
#if LIBCWD_THREAD_SAFE
    RELEASE_READ_LOCK;
  }
  _private_::mutex_tct<_private_::threadlist_instance>::unlock();
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
#if CWDEBUG_DEBUG
  LIBCWD_TSD_DECLARATION
#endif
  LIBCWD_DEFER_CANCEL;
  _private_::mutex_tct<_private_::threadlist_instance>::lock();
  for(_private_::threadlist_t::iterator thread_iter = _private_::threadlist->begin(); thread_iter != _private_::threadlist->end(); ++thread_iter)
  {
    ACQUIRE_READ_LOCK(&(*thread_iter));
#endif
    memsize += MEMSIZE;
    memblks += MEMBLKS;
#if LIBCWD_THREAD_SAFE
    RELEASE_READ_LOCK;
  }
  _private_::mutex_tct<_private_::threadlist_instance>::unlock();
  LIBCWD_RESTORE_CANCEL;
#endif
  o << "Allocated memory: " << memsize << " bytes in " << memblks << " blocks";
  return o;
}

/**
 * \brief List all current allocations to a given %debug object using a specified format.
 * \ingroup group_overview
 *
 * With \link enable_alloc CWDEBUG_ALLOC \endlink set to 1, it is possible
 * to write the \ref group_overview "overview of allocated memory" to
 * a \ref group_debug_object "Debug Object".&nbsp;
 *
 * For example:
 *
 * \code
 * Debug(
 *   ooam_filter_ct ooam_filter(show_objectfile);
 *   std::vector<std::string> masks;
 *   masks.push_back("libc.so*");
 *   masks.push_back("libstdc++*");
 *   ooam_filter.hide_objectfiles_matching(masks);
 *   list_allocations_on(libcw_do, ooam_filter)
 * );
 * \endcode
 *
 * which would print on \link libcw::debug::libcw_do libcw_do \endlink using
 * \ref group_debug_channels "debug channel" \link libcw::debug::dc::malloc dc::malloc \endlink,
 * not showing allocations that belong to shared libraries matching "libc.so*" or "libstdc++*".
 * The remaining items would show which object file (shared library name or the executable name)
 * they belong to, because we used \c show_objectfile as flag.
 *
 * \sa group_alloc_format
 */
void list_allocations_on(debug_ct& debug_object, ooam_filter_ct const& filter)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
#endif

  dm_alloc_copy_ct* list = NULL;
  size_t memsize;
  unsigned long memblks;
  LIBCWD_DEFER_CANCEL;
#if LIBCWD_THREAD_SAFE
  __libcwd_tsd.target_thread = &(*__libcwd_tsd.thread_iter);
#endif
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memsize = MEMSIZE;
  memblks = MEMBLKS;
  if (BASE_ALLOC_LIST)
  {
    _private_::set_alloc_checking_off(LIBCWD_TSD);
    list = new dm_alloc_copy_ct(*BASE_ALLOC_LIST);
    _private_::set_alloc_checking_on(LIBCWD_TSD);
  }
  RELEASE_READ_LOCK;
  LIBCWD_RESTORE_CANCEL;
  LibcwDout( channels, debug_object, dc_malloc, "Allocated memory: " << memsize << " bytes in " << memblks << " blocks." );
  if (list)
  {
#if LIBCWD_THREAD_SAFE
    LIBCWD_DEFER_CANCEL;
    pthread_cleanup_push(reinterpret_cast<void(*)(void*)>(&mutex_tct<list_allocations_instance>::cleanup), NULL);
    mutex_tct<list_allocations_instance>::lock();
#endif
    filter.M_check_synchronization();
    list->show_alloc_list(1, channels::dc_malloc, filter);
#if LIBCWD_THREAD_SAFE
    pthread_cleanup_pop(1);
    LIBCWD_RESTORE_CANCEL;
#endif
    _private_::set_alloc_checking_off(LIBCWD_TSD);
    delete list;
    _private_::set_alloc_checking_on(LIBCWD_TSD);
  }
}

/**
 * \brief List all current allocations to a given %debug object.
 * \ingroup group_overview
 *
 * With \link enable_alloc CWDEBUG_ALLOC \endlink set to 1, it is possible
 * to write the \ref group_overview "overview of allocated memory" to
 * a \ref group_debug_object "Debug Object".&nbsp;
 * The syntax to do this is:
 *
 * \code
 * Debug( list_allocations_on(libcw_do) );	// libcw_do is the (default) debug object.
 * \endcode
 *
 * which would print on \link libcw::debug::libcw_do libcw_do \endlink using
 * \ref group_debug_channels "debug channel" \link libcw::debug::dc::malloc dc::malloc \endlink.
 *
 * Note that not passing formatting information is equivalent with,
 *
 * \code
 * Debug(
 *     ooam_filter_ct format(0);
 *     list_allocations_on(debug_object, format)
 * );
 * \endcode
 *
 * meaning that all allocations are shown without time, without path and without to which library they belong to.
 */
void list_allocations_on(debug_ct& debug_object)
{
  list_allocations_on(debug_object, default_ooam_filter);
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
void make_invisible(void const* ptr)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
#if LIBCWD_THREAD_SAFE
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr, 0)));
#endif
  bool found = (iter != target_memblk_map_read->end() && (*iter).first.start() == ptr);
#if LIBCWD_THREAD_SAFE
  if (!found)
  {
    RELEASE_READ_LOCK;
    found = search_in_maps_of_other_threads(ptr, iter, __libcwd_tsd) && (*iter).first.start() == ptr;
  }
#endif
  if (!found)
  {
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatalInternal( dc::core, "Trying to turn non-existing memory block (" << ptr << ") into an 'internal' block" );
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
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.internal );
#endif
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

// Undocumented (used in macro AllocTag)
void set_alloc_label(void const* ptr, type_info_ct const& ti, char const* description LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(ptr, 0)));
  if (iter != memblk_map_write->end() && (*iter).first.start() == ptr)
    (*iter).second.change_label(ti, description);
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;
}

void set_alloc_label(void const* ptr, type_info_ct const& ti, _private_::smart_ptr description LIBCWD_COMMA_TSD_PARAM)
{
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(ptr, 0)));
  if (iter != memblk_map_write->end() && (*iter).first.start() == ptr)
    (*iter).second.change_label(ti, description);
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;
}

#undef CALL_ADDRESS
#if CWDEBUG_LOCATION
#define CALL_ADDRESS , reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset
#else
#define CALL_ADDRESS
#endif
#undef SAVEDMARKER
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
#define SAVEDMARKER , saved_marker
#else
#define SAVEDMARKER
#endif

#if CWDEBUG_MARKER
void marker_ct::register_marker(char const* label)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
#endif
  Dout( dc_malloc, "New libcw::debug::marker_ct at " << this );
  bool error = false;
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(this, 0)));
  memblk_info_ct& info((*iter).second);
  if (iter == memblk_map_write->end() || (*iter).first.start() != this || info.flags() != memblk_type_new)
    error = true;
  else
  {
    info.change_label(type_info_of(this), label);
    info.change_flags(memblk_type_marker);
    info.new_list(LIBCWD_TSD);					// MT: needs write lock set.
  }
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;
  if (error)
    DoutFatal( dc::core, "Use 'new' for libcw::debug::marker_ct" );
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  Debug( list_allocations_on(libcw_do) );
#endif
}

/**
 * \brief Destructor.
 */
marker_ct::~marker_ct()
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
#endif

  dm_alloc_copy_ct* list = NULL;
  _private_::smart_ptr description;

  LIBCWD_DEFER_CANCEL_NO_BRACE;
#if LIBCWD_THREAD_SAFE
  __libcwd_tsd.target_thread = &(*__libcwd_tsd.thread_iter);
#endif
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(this, 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != this)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatal( dc::core, "Trying to delete an invalid marker" );
  }

  description = (*iter).second.description();		// This won't call malloc.

  if ((*iter).second.a_alloc_node.get()->next_list())
  {
    _private_::set_alloc_checking_off(LIBCWD_TSD);
    list = new dm_alloc_copy_ct(*(*iter).second.a_alloc_node.get()->next_list());
    _private_::set_alloc_checking_on(LIBCWD_TSD);
  }

  // Set `CURRENT_ALLOC_LIST' one list back
  if (*CURRENT_ALLOC_LIST != (*iter).second.a_alloc_node.get()->next_list())
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    Dout( dc_malloc, "Removing libcw::debug::marker_ct at " << this << " (" << description.get() << ')' );
    DoutFatal( dc::core, "Deleting a marker must be done in the same \"scope\" as where it was allocated; for example, "
	"you cannot allocate marker A, then allocate marker B and then delete marker A before deleting first marker B." );
  }
  ACQUIRE_READ2WRITE_LOCK;
  dm_alloc_ct::descend_current_alloc_list(LIBCWD_TSD);		// MT: needs write lock.
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL_NO_BRACE;

  Dout( dc_malloc, "Removing libcw::debug::marker_ct at " << this << " (" << description.get() << ')' );
  if (list)
  {
#if LIBCWD_THREAD_SAFE
    LIBCWD_DEFER_CANCEL;
    pthread_cleanup_push(reinterpret_cast<void(*)(void*)>(&mutex_tct<list_allocations_instance>::cleanup), NULL);
    mutex_tct<list_allocations_instance>::lock();
#endif
    default_ooam_filter.M_check_synchronization();
    libcw_do.push_margin();
    libcw_do.margin().append("  * ", 4);
    Dout( dc::warning, "Memory leak detected!" );
    list->show_alloc_list(1, channels::dc::warning, default_ooam_filter);
    libcw_do.pop_margin();
#if LIBCWD_THREAD_SAFE
    pthread_cleanup_pop(1);
    LIBCWD_RESTORE_CANCEL;
#endif
    _private_::set_alloc_checking_off(LIBCWD_TSD);
    delete list;
    _private_::set_alloc_checking_on(LIBCWD_TSD);
  }
}

/**
 * \brief Move memory allocation pointed to by \a ptr outside \a marker.
 * \ingroup group_markers
 */
void move_outside(marker_ct* marker, void const* ptr)
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION
  LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
#endif

  LIBCWD_DEFER_CANCEL_NO_BRACE;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(ptr, 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != ptr)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DoutFatal( dc::core, "Trying to move non-existing memory block (" << ptr << ") outside memory leak test marker" );
  }
  memblk_map_ct::const_iterator const& iter2(memblk_map_read->find(memblk_key_ct(marker, 0)));
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
  Dout( dc::warning, "Memory block at " << ptr << " is already outside the marker at " << (void*)marker << " (" << marker_alloc_node->type_info_ptr->demangled_name() << ") area!" );
  RELEASE_READ_LOCK;
  LIBCWD_RESTORE_CANCEL_NO_BRACE;
}
#endif // CWDEBUG_MARKER

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
 * libcw::debug::alloc_ct const* alloc = libcw::debug::find_alloc(&buf[10]);
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
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION
  LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
#endif

  alloc_ct const* res;
#if LIBCWD_THREAD_SAFE
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_READ_LOCK(&(*__libcwd_tsd.thread_iter));
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr, 0)));
#endif
  bool found = (iter != target_memblk_map_read->end());
#if LIBCWD_THREAD_SAFE
  if (!found)
  {
    RELEASE_READ_LOCK;
    found = search_in_maps_of_other_threads(ptr, iter, __libcwd_tsd);
  }
#endif
  if (!found)
  {
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    return NULL;
  }
  res = (*iter).second.get_alloc_node();
  RELEASE_READ_LOCK;
  LIBCWD_RESTORE_CANCEL;
  return res;
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
void register_external_allocation(void const* mptr, size_t size)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
#endif
#if CWDEBUG_DEBUGM && defined(__GLIBCPP__)
  LIBCWD_ASSERT( _private_::WST_ios_base_initialized );
#endif
  if (__libcwd_tsd.internal)
    DoutFatalInternal( dc::core, "Calling register_external_allocation while `internal' is non-zero!  "
                                 "You can't use RegisterExternalAlloc() inside a Dout() et. al. "
				 "(or whenever alloc_checking is off)." );
  ++__libcwd_tsd.inside_malloc_or_free;
  DoutInternal( dc_malloc, "register_external_allocation(" << (void*)mptr << ", " << size << ')' );

  if (WST_initialization_state == 0)		// Only true once.
  {
    __libcwd_tsd.internal = 1;
    location_cache_map.MT_unsafe = new location_cache_map_ct;
#if CWDEBUG_DEBUG && LIBCWD_THREAD_SAFE
    LIBCWD_ASSERT( !_private_::WST_multi_threaded );
#endif
    // The memblk_map of the second and later threads are initialized in 'LIBCWD_TSD_DECLARATION' (by calling new_memblk_map()).
    __libcwd_tsd.memblk_map = new memblk_map_ct;
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
  struct timeval alloc_time;
  gettimeofday(&alloc_time, 0);
  std::pair<memblk_map_ct::iterator, bool> iter;
  LIBCWD_DEFER_CANCEL;
  ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));	// MT: memblk_info_ct() needs wrlock.
  memblk_ct memblk(memblk_key_ct(mptr, size), memblk_info_ct(mptr, size, memblk_type_external, alloc_time LIBCWD_COMMA_LOCATION(loc)));
  iter = memblk_map_write->insert(memblk);
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL;

  DEBUGDEBUG_CERR( "register_external_allocation: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  
  // MT-safe: iter points to an element that is garanteed unique to this thread (we just allocated it).
  if (!iter.second)
    DoutFatalInternal( dc::core, "register_external_allocation: externally (supposedly newly) allocated block collides with *existing* memblk!  Are you sure this memory block was externally allocated, or did you call RegisterExternalAlloc twice for the same pointer?" );

  memblk_info_ct& memblk_info((*iter.first).second);
  memblk_info.lock();		// Lock ownership (doesn't call malloc).
  --__libcwd_tsd.inside_malloc_or_free;
}
#endif // !LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC

  } // namespace debug
} // namespace libcw

using namespace ::libcw::debug;

extern "C" {

//=============================================================================
//
// __libcwd_free, replacement for free(3)
//
// frees a block and updates the internal administration.
//

void __libcwd_free(void* ptr)
{
  LIBCWD_TSD_DECLARATION
  internal_free(ptr, from_free LIBCWD_COMMA_TSD);
}

//=============================================================================
//
// malloc(3) and calloc(3) replacements:
//

#if (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && LIBCWD_THREAD_SAFE && CWDEBUG_DEBUG
#define UNLOCK if (locked) _private_::allocator_unlock();
#else
#define UNLOCK
#endif

void* __libcwd_malloc(size_t size)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    FATALDEBUGDEBUG_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << __LINE__ - 2 << ": " << __PRETTY_FUNCTION__ << ": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
#if CWDEBUG_DEBUGM && defined(__GLIBCPP__) && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC)
  LIBCWD_ASSERT( _private_::WST_ios_base_initialized || __libcwd_tsd.internal );
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `__libcwd_malloc(" << size << ")' [" << saved_marker << ']' );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_malloc(size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

#if !CWDEBUG_MAGIC
    void* ptr = __libc_malloc(size);
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_malloc': " << ptr << " [" << saved_marker << ']' );
    return ptr;
#else // CWDEBUG_MAGIC
    void* ptr = __libc_malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      return NULL;
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_malloc': " << static_cast<size_t*>(ptr) + 2 << " [" << saved_marker << ']' );
    return static_cast<size_t*>(ptr) + 2;
#endif // CWDEBUG_MAGIC

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  DoutInternal( dc_malloc|continued_cf, "malloc(" << size << ") = [" << saved_marker << ']' );
#else
  DoutInternal( dc_malloc|continued_cf, "malloc(" << size << ") = " );
#endif
  void* ptr = internal_malloc(size, memblk_type_malloc CALL_ADDRESS LIBCWD_COMMA_TSD SAVEDMARKER);
#if CWDEBUG_MAGIC
  if (ptr)
  {
    ((size_t*)ptr)[-2] = MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ptr;
}

void* __libcwd_calloc(size_t nmemb, size_t size)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << __LINE__ - 2 << ": " << __PRETTY_FUNCTION__ << ": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
#if CWDEBUG_DEBUGM && defined(__GLIBCPP__) && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC)
  LIBCWD_ASSERT( _private_::WST_ios_base_initialized || __libcwd_tsd.internal );
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `__libcwd_calloc(" << nmemb << ", " << size << ")' [" << saved_marker << ']' );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_calloc(nmemb, size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

#if !CWDEBUG_MAGIC
    void* ptr = __libc_calloc(nmemb, size);
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_calloc': " << ptr << " [" << saved_marker << ']' );
    return ptr;
#else // CWDEBUG_MAGIC
    void* ptr = __libc_malloc(SIZE_PLUS_TWELVE(nmemb * size));
    if (!ptr)
      return NULL;
    std::memset(static_cast<void*>(static_cast<size_t*>(ptr) + 2), 0, nmemb * size);
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[1] = nmemb * size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(nmemb * size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_calloc': " << static_cast<size_t*>(ptr) + 2 << " [" << saved_marker << ']' );
    return static_cast<size_t*>(ptr) + 2;
#endif // CWDEBUG_MAGIC

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  DoutInternal( dc_malloc|continued_cf, "calloc(" << nmemb << ", " << size << ") = [" << saved_marker << ']' );
#else
  DoutInternal( dc_malloc|continued_cf, "calloc(" << nmemb << ", " << size << ") = " );
#endif
  void* ptr;
  size *= nmemb;
  if ((ptr = internal_malloc(size, memblk_type_malloc CALL_ADDRESS LIBCWD_COMMA_TSD SAVEDMARKER)))
    std::memset(ptr, 0, size);
#if CWDEBUG_MAGIC
  if (ptr)
  {
    ((size_t*)ptr)[-2] = MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ptr;
}

//=============================================================================
//
// __libcwd_realloc, replacement for realloc(3)
//
// reallocates a block and updates the internal administration.
//

void* __libcwd_realloc(void* ptr, size_t size)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << __LINE__ - 2 << ": " << __PRETTY_FUNCTION__ << ": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
#if CWDEBUG_DEBUGM && defined(__GLIBCPP__) && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC)
  LIBCWD_ASSERT( _private_::WST_ios_base_initialized || __libcwd_tsd.internal );
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `__libcwd_realloc(" << ptr << ", " << size << ")' [" << saved_marker << ']' );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_realloc(ptr, size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

#if !CWDEBUG_MAGIC
    void* ptr1 = __libc_realloc(ptr, size);
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_realloc': " << ptr1 << " [" << saved_marker << ']' );
    return ptr1;
#else // CWDEBUG_MAGIC
    void* ptr1;
    if (ptr)
    {
      ptr = static_cast<size_t*>(ptr) - 2;
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_MALLOC_BEGIN ||
	  ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_MALLOC_END)
	DoutFatalInternal( dc::core, "internal realloc: magic number corrupt!" );
      if (size == 0)
      {
        __libc_free(ptr);
	LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_realloc': NULL [" << saved_marker << ']' );
	return NULL;
      }
      ptr1 = __libc_realloc(ptr, SIZE_PLUS_TWELVE(size));
    }
    else
      ptr1 = __libc_malloc(SIZE_PLUS_TWELVE(size));
    ((size_t*)ptr1)[0] = INTERNAL_MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr1)[1] = size;
    ((size_t*)(static_cast<char*>(ptr1) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `__libcwd_realloc': " << static_cast<size_t*>(ptr1) + 2 << " [" << saved_marker << ']' );
    return static_cast<size_t*>(ptr1) + 2;
#endif // CWDEBUG_MAGIC

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  DoutInternal( dc_malloc|continued_cf, "realloc(" << ptr << ", " << size << ") = [" << saved_marker << ']' );
#else
  DoutInternal( dc_malloc|continued_cf, "realloc(" << ptr << ", " << size << ") = " );
#endif

  if (ptr == NULL)
  {
    void* mptr = internal_malloc(size, memblk_type_realloc CALL_ADDRESS LIBCWD_COMMA_TSD SAVEDMARKER);

#if CWDEBUG_MAGIC
    if (mptr)
    {
      ((size_t*)mptr)[-2] = MAGIC_MALLOC_BEGIN;
      ((size_t*)mptr)[-1] = size;
      ((size_t*)(static_cast<char*>(mptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
    }
#endif
    --__libcwd_tsd.inside_malloc_or_free;
    return mptr;
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
  memblk_map_ct::const_iterator iter = target_memblk_map_read->find(memblk_key_ct(ptr, 0));
#else
  memblk_map_ct::const_iterator const& iter(target_memblk_map_read->find(memblk_key_ct(ptr, 0)));
#endif
  bool found = (iter != target_memblk_map_read->end() && (*iter).first.start() == ptr);
#if LIBCWD_THREAD_SAFE
  _private_::thread_ct* other_target_thread;
  if (!found)
  {
    RELEASE_READ_LOCK;
    found = search_in_maps_of_other_threads(ptr, iter, __libcwd_tsd) && (*iter).first.start() == ptr;
    other_target_thread = __libcwd_tsd.target_thread;
  }
  else
    other_target_thread = NULL;
#endif
  if (!found)
  {
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
    __libcwd_tsd.internal = 0;
    DoutInternal( dc::finish, "" );
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
    DoutFatalInternal( dc::core, "Trying to realloc() an invalid pointer (" << ptr << ") [" << saved_marker << ']' );
#else
    DoutFatalInternal( dc::core, "Trying to realloc() an invalid pointer (" << ptr << ')' );
#endif
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
    internal_free(ptr, from_free LIBCWD_COMMA_TSD);
    ++__libcwd_tsd.inside_malloc_or_free;
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
    DoutInternal( dc::finish, "NULL [" << saved_marker << ']' );
#else
    DoutInternal( dc::finish, "NULL" );
#endif
    --__libcwd_tsd.inside_malloc_or_free;
    return NULL;
  }

  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;

  register void* mptr;

#if !CWDEBUG_MAGIC
  if (!(mptr = __libc_realloc(ptr, size)))
#else
  if (((size_t*)ptr)[-2] != MAGIC_MALLOC_BEGIN ||
      ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_MALLOC_END)
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    if (((size_t*)ptr)[-2] == MAGIC_NEW_BEGIN &&
	((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `new'!" );
    if (((size_t*)ptr)[-2] == MAGIC_NEW_ARRAY_BEGIN &&
	((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_ARRAY_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `new[]'!" );
    DoutFatalInternal( dc::core, "realloc: magic number corrupt!" );
  }
  if ((mptr = static_cast<char*>(__libc_realloc(static_cast<size_t*>(ptr) - 2, SIZE_PLUS_TWELVE(size))) + 2 * sizeof(size_t)) == (void*)(2 * sizeof(size_t)))
#endif
  {
    RELEASE_READ_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
    DoutInternal( dc::finish, "NULL [" << saved_marker << ']' );
#else
    DoutInternal( dc::finish, "NULL" );
#endif
    DoutInternal( dc_malloc, "Out of memory! This is only a pre-detection!" );
    --__libcwd_tsd.inside_malloc_or_free;
    return NULL; // A fatal error should occur directly after this
  }
#if CWDEBUG_MAGIC
  ((size_t*)mptr)[-1] = size;
  ((size_t*)(static_cast<char*>(mptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
#endif

  // Update administration
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;
  type_info_ct const* t = (*iter).second.typeid_ptr();
  char const* d = (*iter).second.description().get();
  struct timeval realloc_time;
  gettimeofday(&realloc_time, 0);
#if LIBCWD_THREAD_SAFE
  if (other_target_thread)
    ACQUIRE_WRITE_LOCK(&(*__libcwd_tsd.thread_iter));			// MT: memblk_info_ct() needs wrlock.
  else
    ACQUIRE_READ2WRITE_LOCK;
#endif
  memblk_ct memblk(memblk_key_ct(mptr, size),
                   memblk_info_ct(mptr, size, memblk_type_realloc, realloc_time LIBCWD_COMMA_TSD LIBCWD_COMMA_LOCATION(loc)));
  target_memblk_map_write->erase(memblk_iter_write);
#if LIBCWD_THREAD_SAFE
  if (other_target_thread)
  {
    __libcwd_tsd.target_thread = other_target_thread;
    RELEASE_READ_LOCK;
    // We still have the lock for the current thread (set 12 lines above this).
    __libcwd_tsd.target_thread = &(*__libcwd_tsd.thread_iter);
  }
#endif
  std::pair<memblk_map_ct::iterator, bool> const& iter2(memblk_map_write->insert(memblk));
  if (!iter2.second)
  {
    RELEASE_WRITE_LOCK;
    LIBCWD_RESTORE_CANCEL_NO_BRACE;
    DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
    __libcwd_tsd.internal = 0;
    DoutFatalInternal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );
  }
  memblk_info_ct& memblk_info((*(iter2.first)).second);
  memblk_info.change_label(*t, d);
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  RELEASE_WRITE_LOCK;
  LIBCWD_RESTORE_CANCEL_NO_BRACE;

#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  DoutInternal( dc::finish, (void*)(mptr) << " [" << saved_marker << ']' );
#else
  DoutInternal( dc::finish, (void*)(mptr) );
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return mptr;
}

} // extern "C"

//=============================================================================
//
// operator `new' and `new []' replacements.
//

void* operator new(size_t size)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << __LINE__ - 2 << ": " << __PRETTY_FUNCTION__ << ": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `operator new', size = " << size << " [" << saved_marker << ']' );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_malloc(size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

#if !CWDEBUG_MAGIC
    void* ptr = __libc_malloc(size);
    if (!ptr)
      DoutFatalInternal( dc::core, "Out of memory in `operator new'" );
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `operator new': " << ptr << " [" << saved_marker << ']' );
    return ptr;
#else // CWDEBUG_MAGIC
    void* ptr = __libc_malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      DoutFatalInternal( dc::core, "Out of memory in `operator new'" );
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_NEW_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_NEW_END;
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `operator new': " << static_cast<size_t*>(ptr) + 2 << " [" << saved_marker << ']' );
    return static_cast<size_t*>(ptr) + 2;
#endif // CWDEBUG_MAGIC

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  DoutInternal( dc_malloc|continued_cf, "operator new (size = " << size << ") = [" << saved_marker << ']' );
#else
  DoutInternal( dc_malloc|continued_cf, "operator new (size = " << size << ") = " );
#endif
  void* ptr = internal_malloc(size, memblk_type_new CALL_ADDRESS LIBCWD_COMMA_TSD SAVEDMARKER);
  if (!ptr)
    DoutFatalInternal( dc::core, "Out of memory in `operator new'" );
#if CWDEBUG_MAGIC
  else
  {
    ((size_t*)ptr)[-2] = MAGIC_NEW_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_NEW_END;
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ptr;
}

void* operator new[](size_t size)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << __LINE__ - 2 << ": " << __PRETTY_FUNCTION__ << ": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#if CWDEBUG_DEBUGOUTPUT
  int saved_marker = ++__libcwd_tsd.marker;
#endif
#endif
  if (__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Entering `operator new[]', size = " << size << " [" << saved_marker << ']' );

#if !CWDEBUG_DEBUGM && !CWDEBUG_MAGIC
    return __libc_malloc(size);
#else // CWDEBUG_DEBUGM || CWDEBUG_MAGIC

#if !CWDEBUG_MAGIC
    void* ptr = __libc_malloc(size);
    if (!ptr)
      DoutFatalInternal( dc::core, "Out of memory in `operator new[]'" );
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `operator new[]': " << ptr << " [" << saved_marker << ']' );
    return ptr;
#else // CWDEBUG_MAGIC
    void* ptr = __libc_malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      DoutFatalInternal( dc::core, "Out of memory in `operator new[]'" );
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_NEW_ARRAY_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_NEW_ARRAY_END;
    LIBCWD_DEBUGM_CERR( "CWDEBUG_DEBUGM: Internal: Leaving `operator new[]': " << static_cast<size_t*>(ptr) + 2 << " [" << saved_marker << ']' );
    return static_cast<size_t*>(ptr) + 2;
#endif // CWDEBUG_MAGIC

#endif // CWDEBUG_DEBUGM || CWDEBUG_MAGIC
  } // internal

  ++__libcwd_tsd.inside_malloc_or_free;
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  DoutInternal( dc_malloc|continued_cf, "operator new[] (size = " << size << ") = [" << saved_marker << ']' );
#else
  DoutInternal( dc_malloc|continued_cf, "operator new[] (size = " << size << ") = " );
#endif
  void* ptr = internal_malloc(size, memblk_type_new_array CALL_ADDRESS LIBCWD_COMMA_TSD SAVEDMARKER);
  if (!ptr)
    DoutFatalInternal( dc::core, "Out of memory in `operator new[]'" );
#if CWDEBUG_MAGIC
  else
  {
    ((size_t*)ptr)[-2] = MAGIC_NEW_ARRAY_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_NEW_ARRAY_END;
  }
#endif
  --__libcwd_tsd.inside_malloc_or_free;
  return ptr;
}

//=============================================================================
//
// operator `delete' and `delete []' replacements.
//

void operator delete(void* ptr)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << __LINE__ - 2 << ": " << __PRETTY_FUNCTION__ << ": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#endif
#if CWDEBUG_DEBUGM && defined(__GLIBCPP__) && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC)
  LIBCWD_ASSERT( _private_::WST_ios_base_initialized || __libcwd_tsd.internal );
#endif
  internal_free(ptr, from_delete LIBCWD_COMMA_TSD);
}

void operator delete[](void* ptr)
{
  LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGM
  // We can't use `assert' here, because that can call malloc.
  if (__libcwd_tsd.inside_malloc_or_free > __libcwd_tsd.library_call && !__libcwd_tsd.internal)
  {
    LIBCWD_DEBUGM_CERR("CWDEBUG_DEBUGM: debugmalloc.cc:" << __LINE__ - 2 << ": " << __PRETTY_FUNCTION__ << ": Assertion `__libcwd_tsd.inside_malloc_or_free <= __libcwd_tsd.library_call || __libcwd_tsd.internal' failed.");
    core_dump();
  }
#endif
#if CWDEBUG_DEBUGM && defined(__GLIBCPP__) && !defined(LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC)
  LIBCWD_ASSERT( _private_::WST_ios_base_initialized || __libcwd_tsd.internal );
#endif
  internal_free(ptr, from_delete_array LIBCWD_COMMA_TSD);	// Note that the standard demands that we call free(), and not delete().
  								// This forces everyone to overload both, operator delete() and operator
								// delete[]() and not only operator delete().
}

#endif // CWDEBUG_ALLOC
