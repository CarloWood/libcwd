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
// This is version 2 of libcw's "debugmalloc", it should be a hundred times
// more surveyable then the first version ;).
//
// This file can be viewed at as an object, with static variables instead of
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
// We maintain two main data 'trees':
// 1) A big Red/Black tree (using STL's map class) which contains all
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
//   This is particular interesting because it allows us to find back the
//   start of an instance of an object, inside a method of a base class
//   of this object.
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
// at the point where you delete it).
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
// - void list_allocations_on(debug_ct& debug_object);
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

#ifdef _REENTRANT

// We got the C++ config earlier by <bits/stl_alloc.h>.

#ifdef __STL_THREADS
#include <bits/stl_threads.h>   // For interface to _STL_mutex_lock (node allocator lock)
#if defined(__STL_GTHREADS)
#include "bits/gthr.h"
#else
#error You have an unsupported configuraton of gcc. Please tell that you dont have gthreads along with the output of gcc -v to libcw@alinoe.com.
#endif // __STL_GTHREADS


#endif // __STL_THREADS
#endif // _REENTRANT
#include <iostream>
#include <iomanip>
#include "cwd_debug.h"
#include "ios_base_Init.h"
#include <libcw/cwprint.h>

// MULTI THREADING
//
// Global one-time initialization variables that are initialized before main():
// b libcw::debug::memblk_map
// b libcw::debug::WST_initialization_state
// D libcw::debug::_private_::WST_ios_base_initialized
//
// Global variables that are protected with the same rwlock as memblk_map.
// d libcw::debug::base_alloc_list
// D libcw::debug::dm_alloc_ct::current_alloc_list
// D libcw::debug::dm_alloc_ct::current_owner_node
// D libcw::debug::dm_alloc_ct::memblks
// D libcw::debug::dm_alloc_ct::memsize
//
#ifdef _REENTRANT
using libcw::debug::_private_::mutex_tct;
using libcw::debug::_private_::memblk_map_instance;
// We can't use rwlock_tct here because that leads to a dead lock.
// rwlocks have to use condition variables or semaphores and both try to get a
// (libpthread internal) self-lock that is already set by libthread when it calls
// free() in order to destroy thread specific data 1st level arrays.
#define ACQUIRE_WRITE_LOCK	mutex_tct<memblk_map_instance>::lock();		// rwlock_tct<memblk_map_instance>::wrlock();
#define RELEASE_WRITE_LOCK	mutex_tct<memblk_map_instance>::unlock();	// rwlock_tct<memblk_map_instance>::wrunlock();
#define ACQUIRE_READ_LOCK	mutex_tct<memblk_map_instance>::lock();		// rwlock_tct<memblk_map_instance>::rdlock();
#define RELEASE_READ_LOCK	mutex_tct<memblk_map_instance>::unlock();	// rwlock_tct<memblk_map_instance>::rdunlock();
#define ACQUIRE_READ2WRITE_LOCK							// rwlock_tct<memblk_map_instance>::rd2wrlock();
#define ACQUIRE_WRITE2READ_LOCK 						// rwlock_tct<memblk_map_instance>::wr2rdlock();
#else // !_REENTRANT
#define ACQUIRE_WRITE_LOCK
#define RELEASE_WRITE_LOCK
#define ACQUIRE_READ_LOCK
#define RELEASE_READ_LOCK
#define ACQUIRE_READ2WRITE_LOCK
#define ACQUIRE_WRITE2READ_LOCK
#endif // !_REENTRANT

#ifdef LIBCWD_USE_EXTERNAL_C_LINKAGE_FOR_MALLOC
#define __libcwd_malloc malloc
#define __libcwd_calloc calloc
#define __libcwd_realloc realloc
#define __libcwd_free free
#define dc_malloc dc::malloc
#ifdef LIBCWD_HAVE__LIBC_MALLOC
#define __libc_malloc _libc_malloc
#define __libc_calloc _libc_calloc
#define __libc_realloc _libc_realloc
#define __libc_free _libc_free
#endif
#else
#define __libc_malloc malloc
#define __libc_calloc calloc
#define __libc_realloc realloc
#define __libc_free free
#define dc_malloc dc::__libcwd_malloc
#endif

extern "C" void* __libc_malloc(size_t size);
extern "C" void* __libc_calloc(size_t nmemb, size_t size);
extern "C" void* __libc_realloc(void* ptr, size_t size);
extern "C" void __libc_free(void* ptr);

namespace libcw {
  namespace debug {
    
namespace _private_ {

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
    DoutFatal( dc::core, "Calling `set_alloc_checking_on' while ALREADY on." );
  LIBCWD_DEBUGM_CERR( "set_alloc_checking_on from " << __builtin_return_address(0) << ": internal == " << __libcwd_tsd.internal << "; setting it to " << __libcwd_tsd.internal - 1 << '.' );
  --__libcwd_tsd.internal;
}
#endif

} // namespace _private_

class dm_alloc_ct;
class memblk_key_ct;
class memblk_info_ct;

//=============================================================================
//
// static variables
//

static dm_alloc_ct* base_alloc_list = NULL;
  // The base list with `dm_alloc_ct' objects.
  // Each of these objects has a list of it's own.

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

class dm_alloc_ct : public alloc_ct {
#if CWDEBUG_MARKER
  friend class marker_ct;
  friend void move_outside(marker_ct* marker, void const* ptr);
#endif
private:
  static dm_alloc_ct** current_alloc_list;
  static dm_alloc_ct* current_owner_node;
  static size_t memsize;
  static unsigned long memblks;

  dm_alloc_ct* next;		// Next memblk in list `my_list'
  dm_alloc_ct* prev;		// Previous memblk in list `my_list'
  dm_alloc_ct* a_next_list;	// First element of my childeren (if any)
  dm_alloc_ct** my_list;	// Pointer to my list, never NULL.
  dm_alloc_ct* my_owner_node;	// Pointer to node who's a_next_list contains
  				// this object.

public:
  dm_alloc_ct(void const* s, size_t sz, memblk_types_nt f) :		// MT-safe: write lock is set.
      alloc_ct(s, sz , f, unknown_type_info_c),
      next(*dm_alloc_ct::current_alloc_list), prev(NULL), a_next_list(NULL),
      my_list(dm_alloc_ct::current_alloc_list), my_owner_node(dm_alloc_ct::current_owner_node)
      {
        *dm_alloc_ct::current_alloc_list = this;
        if (next)
          next->prev = this;
	memsize += sz;
	++memblks;
      }
    // Constructor: Add `node' at the start of `list'
#if CWDEBUG_DEBUGM
  dm_alloc_ct(dm_alloc_ct const& __dummy) : alloc_ct(__dummy) { LIBCWD_TSD_DECLARATION DoutFatalInternal( dc::fatal, "Calling dm_alloc_ct::dm_alloc_ct(dm_alloc_ct const&)" ); }
    // No copy constructor allowed.
#endif
  ~dm_alloc_ct();
  void new_list(void)							// MT-safe: write lock is set.
      { dm_alloc_ct::current_alloc_list = &a_next_list;
        dm_alloc_ct::current_owner_node = this; }
    // Start a new list in this node.
  void change_flags(memblk_types_nt new_memblk_type) { a_memblk_type = new_memblk_type; }
  void change_label(type_info_ct const& ti, lockable_auto_ptr<char, true> description) { type_info_ptr = &ti; a_description = description; }
  type_info_ct const* typeid_ptr(void) const { return type_info_ptr; }
  char const* description(void) const { return a_description.get(); }
  dm_alloc_ct const* next_node(void) const { return next; }
  dm_alloc_ct const* prev_node(void) const { return prev; }
  dm_alloc_ct const* next_list(void) const { return a_next_list; }
  void const* start(void) const { return a_start; }
  bool is_deleted(void) const;
  size_t size(void) const { return a_size; }
  static void descend_current_alloc_list(void)				// MT-safe: write lock is set.
      {
	if (dm_alloc_ct::current_owner_node)
	{
	  dm_alloc_ct::current_alloc_list = dm_alloc_ct::current_owner_node->my_list;
	  dm_alloc_ct::current_owner_node = (*dm_alloc_ct::current_alloc_list)->my_owner_node;
	}
	else
	  dm_alloc_ct::current_alloc_list = &base_alloc_list;
      }
    // Set `current_alloc_list' back to its parent list.
  static unsigned long get_memblks(void) { return memblks; }		// MT-safe: read lock is set.
  static size_t get_memsize(void) { return memsize; }			// MT-safe: read lock is set.
  void print_description(LIBCWD_TSD_PARAM) const;
  void printOn(std::ostream& os) const;
  friend inline std::ostream& operator<<(std::ostream& os, dm_alloc_ct const& alloc) { alloc.printOn(os); return os; }
  friend inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, dm_alloc_ct const& alloc) { alloc.printOn(os.M_os); return os; }
  void show_alloc_list(int depth, channel_ct const& channel) const;
};

typedef dm_alloc_ct const const_dm_alloc_ct;

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
#if CWDEBUG_DEBUGM
  static bool selftest(void);
    // Returns true is the self test FAILED.
#endif
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
  memblk_info_ct(void const* s, size_t sz, memblk_types_nt f);
  void erase(LIBCWD_TSD_PARAM);
  void lock(void) { a_alloc_node.lock(); }
  void make_invisible(void) { a_alloc_node.reset(); }			// MT-safe: write lock is set (needed for ~dm_alloc_ct).
  bool has_alloc_node(void) const { return a_alloc_node.get(); }
  alloc_ct* get_alloc_node(void) const
      { if (has_alloc_node()) return a_alloc_node.get(); return NULL; }
  type_info_ct const* typeid_ptr(void) const { return a_alloc_node.get()->typeid_ptr(); }
  char const* description(void) const { return a_alloc_node.get()->description(); }
  void change_label(type_info_ct const& ti, lockable_auto_ptr<char, true> description) const
      { if (has_alloc_node()) a_alloc_node.get()->change_label(ti, description); }
  void change_label(type_info_ct const& ti, char const* description) const
      { lockable_auto_ptr<char, true> desc(const_cast<char*>(description));
        /* Make sure we won't delete it: */ if (description) desc.release(); change_label(ti, desc); }
  void change_flags(memblk_types_nt new_flag)
      { M_memblk_type = new_flag; if (has_alloc_node()) a_alloc_node.get()->change_flags(new_flag); }
  void new_list(void) const { a_alloc_node.get()->new_list(); }			// MT-safe: write lock is set.
  memblk_types_nt flags(void) const { return M_memblk_type; }
  void print_description(LIBCWD_TSD_PARAM) const { a_alloc_node.get()->print_description(LIBCWD_TSD); }
  void printOn(std::ostream& os) const;
  friend inline std::ostream& operator<<(std::ostream& os, memblk_info_ct const& memblk) { memblk.printOn(os); return os; }
private:
  memblk_info_ct(void) { }
    // Never use this.  It's needed for the implementation of the std::pair<>.
};

typedef std::pair<memblk_key_ct const, memblk_info_ct> memblk_ct;
  // The value type of the map (memblk_map_ct::value_type).

//=============================================================================
//
// The memblk map:
//

typedef std::map<memblk_key_ct, memblk_info_ct, std::less<memblk_key_ct>, _private_::memblk_map_allocator> memblk_map_ct;
  // The map containing all `memblk_ct' objects.

union memblk_map_t {
#ifdef _REENTRANT
  // See http://www.cuj.com/experts/1902/alexandr.htm?topic=experts
  memblk_map_ct const volatile* MT_unsafe;
#else
  memblk_map_ct const* MT_unsafe;
#endif
  memblk_map_ct* write;		// Should only be used after an ACQUIRE_WRITE_LOCK and before the corresponding RELEASE_WRITE_LOCK.
  memblk_map_ct const* read;	// Should only be used after an ACQUIRE_READ_LOCK and before the corresponding RELEASE_READ_LOCK.
};

static memblk_map_t memblk_map;		// MT-safe: initialized before WST_initialization_state == 1.

// The `map' implementation calls `new' when initializing a new `map',
// therefore this must be a pointer.

#define memblk_map_write (memblk_map.write)
#define memblk_map_read  (memblk_map.read)
#define iter_write reinterpret_cast<memblk_map_ct::iterator&>(const_cast<memblk_map_ct::const_iterator&>(iter))

static int WST_initialization_state;		// MT-safe: We will reach state '1' the first call to malloc.
						// We *assume* that the first call to malloc is before we reach
						// main(), or at least before threads are created.
  //  0: memblk_map and libcwd both not initialized yet.
  //  1: memblk_map and libcwd both initialized.
  // -1: memblk_map initialized but libcwd not initialized yet.

//=============================================================================
//
// dm_alloc_ct methods
//

dm_alloc_ct** dm_alloc_ct::current_alloc_list = &base_alloc_list;
  // The current list to which newly allocated memory blocks are added.

dm_alloc_ct* dm_alloc_ct::current_owner_node = NULL;
  // If the current_alloc_list != &base_alloc_list, then this variable
  // points to the dm_alloc_ct node who owns the current list.

size_t dm_alloc_ct::memsize = 0;
  // Total number of allocated bytes (excluding internal allocations!)

unsigned long dm_alloc_ct::memblks = 0;
  // Total number of allocated blocks (excluding internal allocations!)

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
#if CWDEBUG_DEBUGM
  {
    LIBCWD_TSD_DECLARATION
    LIBCWD_ASSERT( __libcwd_tsd.internal );
    if (a_next_list)
      DoutFatalInternal( dc::core, "Removing an dm_alloc_ct that still has an own list" );
    dm_alloc_ct* tmp;
    for(tmp = *my_list; tmp && tmp != this; tmp = tmp->next);
    if (!tmp)
      DoutFatalInternal( dc::core, "Removing an dm_alloc_ct that is not part of its own list" );
  }
#endif
  memsize -= size();
  --memblks;
  if (dm_alloc_ct::current_alloc_list == &a_next_list)
    descend_current_alloc_list();
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

#if CWDEBUG_LOCATION
struct dm_location_ct : public location_ct {
  void print_on(std::ostream& os) const;
  friend _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, dm_location_ct const& data);
#if CWDEBUG_DEBUGOUTPUT
  friend _private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& os, dm_location_ct const& data);
#endif
};
#endif // CWDEBUG_LOCATION

static char const* const twentyfive_spaces_c = "                         ";

#if CWDEBUG_LOCATION
_private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, dm_location_ct const& data)
{
  size_t len = strlen(data.M_filename);
  if (len < 20)
    os.M_os.write(twentyfive_spaces_c, 20 - len);
  os.M_os.write(data.M_filename, len);
  os.M_os.put(':');
  _private_::no_alloc_print_int_to(&os.M_os, data.M_line, false);
  int l = data.M_line;
  int cnt = 0;
  while(l < 10000)
  {
    ++cnt;
    l *= 10;
  }
  os.M_os.write(twentyfive_spaces_c, cnt);
  return os;
}

#if CWDEBUG_DEBUGOUTPUT
_private_::raw_write_nt const& operator<<(_private_::raw_write_nt const& raw_write, dm_location_ct const& data)
{
  size_t len = strlen(data.M_filename);
  if (len < 20)
    write(2, twentyfive_spaces_c, 20 - len);
  write(2, data.M_filename, len);
  write(2, ":", 1);
  (void)operator<<(raw_write, data.M_line);
  int l = data.M_line;
  int cnt = 0;
  while(l < 10000)
  {
    ++cnt;
    l *= 10;
  }
  write(2, twentyfive_spaces_c, cnt);
  return raw_write;
}
#endif // CWDEBUG_DEBUGOUTPUT
#endif // CWDEBUG_LOCATION

void dm_alloc_ct::print_description(LIBCWD_TSD_PARAM) const
{
#if CWDEBUG_DEBUGM
  LIBCWD_ASSERT( !__libcwd_tsd.internal && !__libcwd_tsd.library_call );
#endif
#if CWDEBUG_LOCATION
  if (M_location.is_known())
    DoutInternal(dc::continued, *static_cast<dm_location_ct const*>(&M_location));
  else if (M_location.mangled_function_name() != unknown_function_c)
  {
    __libcwd_tsd.internal = 1;
    {
      _private_::internal_string f;
      demangle_symbol(M_location.mangled_function_name(), f);
      if (f.size() < 25)
	f.append(25 - f.size(), ' ');
      DoutInternal( dc::continued, f << ' ' );
    }
    __libcwd_tsd.internal = 0;
  }
  else
    DoutInternal( dc::continued, twentyfive_spaces_c );
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

  if (a_description.get())
    DoutInternal( dc::continued, ' ' << a_description.get() );
}

void dm_alloc_ct::printOn(std::ostream& os) const
{
  _private_::no_alloc_ostream_ct no_alloc_ostream(os); 
  no_alloc_ostream << "{ start = " << a_start << ", size = " << a_size << ", a_memblk_type = " << a_memblk_type << ",\n\ttype = \"" << type_info_ptr->demangled_name() << "\", description = \"" << (a_description.get() ? a_description.get() : "NULL") <<
      "\", next = " << (void*)next << ", prev = " << (void*)prev <<
      ",\n\tnext_list = " << (void*)a_next_list << ", my_list = " << (void*)my_list << "\n\t( = " << (void*)*my_list << " ) }";
}

void dm_alloc_ct::show_alloc_list(int depth, channel_ct const& channel) const
{
  dm_alloc_ct const* alloc;
  LIBCWD_TSD_DECLARATION
  for (alloc = this; alloc; alloc = alloc->next)
  {
    Dout( channel|nolabel_cf|continued_cf, "" ); // Only prints prefix
    for (int i = depth; i > 1; i--)
      Dout( dc::continued, "    " );
    // Print label and start.
    Dout( dc::continued, cwprint(memblk_types_label_ct(alloc->memblk_type())) << alloc->a_start << ' ' );
    alloc->print_description(LIBCWD_TSD);
    Dout( dc::finish, "" );
    if (alloc->a_next_list)
      alloc->a_next_list->show_alloc_list(depth + 1, channel);
  }
}

//=============================================================================
//
// memblk_ct methods
//

inline memblk_info_ct::memblk_info_ct(void const* s, size_t sz, memblk_types_nt f) :	// MT-safe: write lock is set.
    M_memblk_type(f), a_alloc_node(new dm_alloc_ct(s, sz, f)) {}

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
  location_ct loc(call_addr LIBCWD_COMMA_TSD);
  if (--__libcwd_tsd.library_call)
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
#endif

#if CWDEBUG_DEBUGM
  bool error;
#endif
  memblk_info_ct* memblk_info;

  LIBCWD_DEFER_CANCEL

  DEBUGDEBUG_CERR( "internal_malloc: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;

  // Update our administration:
  ACQUIRE_WRITE_LOCK
  std::pair<memblk_map_ct::iterator, bool> const&
      iter(memblk_map_write->insert(memblk_ct(memblk_key_ct(mptr, size), memblk_info_ct(mptr, size, flag))));
  memblk_info = &(*iter.first).second;
#if CWDEBUG_DEBUGM
  error = !iter.second;
#endif
  RELEASE_WRITE_LOCK

  DEBUGDEBUG_CERR( "internal_malloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;

  LIBCWD_RESTORE_CANCEL

#if CWDEBUG_DEBUGM
  // MT-safe: iter points to an element that is garanteed unique to this thread (we just allocated it).
  if (error)
    DoutFatalInternal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );
#endif

  memblk_info->lock();				// Lock ownership (doesn't call malloc).
#if CWDEBUG_LOCATION
  memblk_info->get_alloc_node()->location_reference().move(loc);
#endif

#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  DoutInternal( dc::finish, (void*)(mptr) << " [" << saved_marker << ']' );
#else
  DoutInternal( dc::finish, (void*)(mptr) );
#endif
  return mptr;
}

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

  LIBCWD_DEFER_CANCEL_NO_BRACE
  ACQUIRE_READ_LOCK
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(ptr, 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != ptr)
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
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
	(*iter).second.print_description(LIBCWD_TSD);
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
      DoutFatalInternal( dc::core, "Huh? Bug in libcw" );
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
    ACQUIRE_READ2WRITE_LOCK
    (*iter_write).second.erase(LIBCWD_TSD);	// Update flags and optional decouple
    memblk_map_write->erase(iter_write);	// Update administration
    RELEASE_WRITE_LOCK

    DEBUGDEBUG_CERR( "__libcwd_free: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
    __libcwd_tsd.internal = 0;

    LIBCWD_RESTORE_CANCEL_NO_BRACE

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

void init_debugmalloc(void)
{
  if (WST_initialization_state <= 0)
  {
    LIBCWD_TSD_DECLARATION
    // This block is Single Threaded.
    if (WST_initialization_state == 0)			// Only true once.
    {
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      memblk_map.MT_unsafe = new memblk_map_ct;		// MT-safe: There are no threads created yet when we get here.
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
      __libcwd_tsd.inside_malloc_or_free = 0;		// Allow that (this call to malloc will not have done from STL allocator).
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
  bool res;
  LIBCWD_DEFER_CANCEL
  ACQUIRE_READ_LOCK
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(ptr, 0)));
  // MT: Because the expression `(*iter).first.start()' is included inside the locked
  //     area too, no core dump will occur when another thread would be deleting
  //     this allocation at the same time.  The behaviour of the application would
  //     still be undefined however because it makes it possible that this function
  //     returns false (not deleted) for a deleted memory block.
  res = (iter == memblk_map_read->end() || (*iter).first.start() != ptr);
  RELEASE_READ_LOCK
  LIBCWD_RESTORE_CANCEL
  return res;
}

/**
 * \brief Returns the total number of allocated bytes.
 * \ingroup book_allocations
 */
size_t mem_size(void)
{
  size_t memsize;
  LIBCWD_DEFER_CANCEL
  ACQUIRE_READ_LOCK
  memsize = const_dm_alloc_ct::get_memsize();
  RELEASE_READ_LOCK
  LIBCWD_RESTORE_CANCEL
  return memsize;
}

/**
 * \brief Returns the total number of allocated memory blocks.
 * \ingroup book_allocations
 */
unsigned long mem_blocks(void)
{
  unsigned long memblks;
  LIBCWD_DEFER_CANCEL
  ACQUIRE_READ_LOCK
  memblks = const_dm_alloc_ct::get_memblks();
  RELEASE_READ_LOCK
  LIBCWD_RESTORE_CANCEL
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
  size_t memsize;
  unsigned long memblks;
  LIBCWD_DEFER_CANCEL
  ACQUIRE_READ_LOCK
  memsize = const_dm_alloc_ct::get_memsize();
  memblks = const_dm_alloc_ct::get_memblks();
  RELEASE_READ_LOCK
  LIBCWD_RESTORE_CANCEL
  o << "Allocated memory: " << memsize << " bytes in " << memblks << " blocks";
  return o;
}

/**
 * \brief List all current allocations to a given %debug object.
 * \ingroup group_overview
 *
 * With both, \link enable_alloc CWDEBUG_ALLOC \endlink and
 * \link enable_debug CWDEBUG_DEBUG \endlink set to 1, it is possible
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
 */
void list_allocations_on(debug_ct& debug_object)
{
#if CWDEBUG_DEBUGM
  {
    LIBCWD_TSD_DECLARATION
    LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
  }
#endif

  if (0)
  {
    LIBCWD_TSD_DECLARATION
    // Print out the entire `map':
    memblk_map_ct const* memblk_map_copy;
    LIBCWD_DEFER_CANCEL
    __libcwd_tsd.internal = 1;
    ACQUIRE_READ_LOCK
    memblk_map_copy = new memblk_map_ct(*memblk_map_read);
    __libcwd_tsd.internal = 0;
    LibcwDout( channels, debug_object, dc_malloc, "map:" );
    int cnt = 0;
    for(memblk_map_ct::const_iterator iter(memblk_map_copy->begin()); iter != memblk_map_copy->end(); ++iter)
      LibcwDout( channels, debug_object, dc_malloc|nolabel_cf, ++cnt << ":\t(*iter).first = " << (*iter).first << '\n' << "\t(*iter).second = " << (*iter).second );
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL
    __libcwd_tsd.internal = 1;
    delete memblk_map_copy;
    __libcwd_tsd.internal = 0;
  }

  LIBCWD_DEFER_CLEANUP_PUSH(&mutex_tct<memblk_map_instance>::cleanup /* &rwlock_tct<memblk_map_instance>::cleanup */, NULL);
  ACQUIRE_READ_LOCK
  LibcwDout( channels, debug_object, dc_malloc, "Allocated memory: " << const_dm_alloc_ct::get_memsize() << " bytes in " << const_dm_alloc_ct::get_memblks() << " blocks." );
  if (base_alloc_list)
    const_cast<dm_alloc_ct const*>(base_alloc_list)->show_alloc_list(1, channels::dc_malloc);
  RELEASE_READ_LOCK
  LIBCWD_CLEANUP_POP_RESTORE(false);
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
  LIBCWD_DEFER_CANCEL
  ACQUIRE_READ_LOCK
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(ptr, 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != ptr)
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
    DoutFatalInternal( dc::core, "Trying to turn non-existing memory block (" << ptr << ") into an 'internal' block" );
  }
  DEBUGDEBUG_CERR( "make_invisible: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;
  ACQUIRE_READ2WRITE_LOCK
  (*iter_write).second.make_invisible();
  RELEASE_WRITE_LOCK
  DEBUGDEBUG_CERR( "make_invisible: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  LIBCWD_RESTORE_CANCEL
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
  LIBCWD_DEFER_CANCEL
  ACQUIRE_WRITE_LOCK
  for (memblk_map_ct::iterator iter(memblk_map_write->begin()); iter != memblk_map_write->end(); ++iter)
    if ((*iter).second.has_alloc_node() && (*iter).first.start() != ptr)
    {
      DEBUGDEBUG_CERR( "make_all_allocations_invisible_except: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
      __libcwd_tsd.internal = 1;
      (*iter).second.make_invisible();
      DEBUGDEBUG_CERR( "make_all_allocations_invisible_except: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
      __libcwd_tsd.internal = 0;
    }
  RELEASE_WRITE_LOCK
  LIBCWD_RESTORE_CANCEL
}

// Undocumented (used in macro AllocTag)
void set_alloc_label(void const* ptr, type_info_ct const& ti, char const* description)
{
  LIBCWD_DEFER_CANCEL
  ACQUIRE_WRITE_LOCK
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(ptr, 0)));
  if (iter != memblk_map_write->end() && (*iter).first.start() == ptr)
    (*iter).second.change_label(ti, description);
  RELEASE_WRITE_LOCK
  LIBCWD_RESTORE_CANCEL
}

void set_alloc_label(void const* ptr, type_info_ct const& ti, lockable_auto_ptr<char, true> description)
{
  LIBCWD_DEFER_CANCEL
  ACQUIRE_WRITE_LOCK
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(ptr, 0)));
  if (iter != memblk_map_write->end() && (*iter).first.start() == ptr)
    (*iter).second.change_label(ti, description);
  RELEASE_WRITE_LOCK
  LIBCWD_RESTORE_CANCEL
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
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION
  LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
#endif
  Dout( dc_malloc, "New libcw::debug::marker_ct at " << this );
  bool error = false;
  LIBCWD_DEFER_CANCEL
  ACQUIRE_WRITE_LOCK
  memblk_map_ct::iterator const& iter(memblk_map_write->find(memblk_key_ct(this, 0)));
  memblk_info_ct& info((*iter).second);
  if (iter == memblk_map_write->end() || (*iter).first.start() != this || info.flags() != memblk_type_new)
    error = true;
  else
  {
    info.change_label(type_info_of(this), label);
    info.change_flags(memblk_type_marker);
    info.new_list();					// MT: needs write lock set.
  }
  RELEASE_WRITE_LOCK
  LIBCWD_RESTORE_CANCEL
  if (error)
    DoutFatal( dc::core, "Use 'new' for libcw::debug::marker_ct" );
#if CWDEBUG_DEBUGM && CWDEBUG_DEBUGOUTPUT
  Debug( list_allocations_on(libcw_do) );
#endif
}

/**
 * \brief Destructor.
 */
marker_ct::~marker_ct(void)
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION
  LIBCWD_ASSERT( !__libcwd_tsd.inside_malloc_or_free && !__libcwd_tsd.internal );
#endif

  LIBCWD_DEFER_CANCEL_NO_BRACE
  ACQUIRE_READ_LOCK
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(this, 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != this)
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
    DoutFatal( dc::core, "Trying to delete an invalid marker" );
  }

  Dout( dc_malloc, "Removing libcw::debug::marker_ct at " << this << " (" << (*iter).second.description() << ')' );

  if ((*iter).second.a_alloc_node.get()->next_list())
  {
    libcw_do.push_margin();
    libcw_do.margin().append("  * ", 4);
    Dout( dc::warning, "Memory leak detected!" );
    (*iter).second.a_alloc_node.get()->next_list()->show_alloc_list(1, channels::dc::warning);
    libcw_do.pop_margin();
  }

  // Set `current_alloc_list' one list back
  if (*const_dm_alloc_ct::current_alloc_list != (*iter).second.a_alloc_node.get()->next_list())
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
    DoutFatal( dc::core, "Deleting a marker must be done in the same \"scope\" as where it was allocated" );
  }
  ACQUIRE_READ2WRITE_LOCK
  dm_alloc_ct::descend_current_alloc_list();			// MT: needs write lock.
  RELEASE_WRITE_LOCK
  LIBCWD_RESTORE_CANCEL_NO_BRACE
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

  LIBCWD_DEFER_CANCEL_NO_BRACE
  ACQUIRE_READ_LOCK
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(ptr, 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != ptr)
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
    DoutFatal( dc::core, "Trying to move non-existing memory block (" << ptr << ") outside memory leak test marker" );
  }
  memblk_map_ct::const_iterator const& iter2(memblk_map_read->find(memblk_key_ct(marker, 0)));
  if (iter2 == memblk_map_read->end() || (*iter2).first.start() != marker)
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
    DoutFatal( dc::core, "No such marker: " << (void*)marker );
  }
  dm_alloc_ct* alloc_node = (*iter).second.a_alloc_node.get();
  if (!alloc_node)
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
    DoutFatal( dc::core, "Trying to move an invisible memory block outside memory leak test marker" );
  }
  dm_alloc_ct* marker_alloc_node = (*iter2).second.a_alloc_node.get();
  if (!marker_alloc_node || marker_alloc_node->a_memblk_type != memblk_type_marker)
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
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
      ACQUIRE_READ2WRITE_LOCK
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
      RELEASE_WRITE_LOCK
      LIBCWD_RESTORE_CANCEL_NO_BRACE
      return;
    }
  }
  Dout( dc::warning, "Memory block at " << ptr << " is already outside the marker at " << (void*)marker << " (" << marker_alloc_node->type_info_ptr->demangled_name() << ") area!" );
  RELEASE_READ_LOCK
  LIBCWD_RESTORE_CANCEL_NO_BRACE
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
  LIBCWD_DEFER_CANCEL
  ACQUIRE_READ_LOCK
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(ptr, 0)));
  res = (iter == memblk_map_read->end()) ? NULL : (*iter).second.get_alloc_node();
  RELEASE_READ_LOCK
  LIBCWD_RESTORE_CANCEL
  return res;
}

//=============================================================================
//
// Self tests:
//

#if CWDEBUG_DEBUGM

bool memblk_key_ct::selftest(void)
{
  init_debugmalloc();

  memblk_map_ct map1;
  unsigned int cnt = 0;

  long const interval_start = 100;
  long const interval_end = 200;

  for (long start_or_end_start1 = interval_start;; start_or_end_start1 = interval_end)
  {
    for (int start_offset1 = -1; start_offset1 <= 1; ++start_offset1)
    {
      long start1 = start_or_end_start1 + start_offset1;
      for (long start_or_end_end1 = start_or_end_start1;; start_or_end_end1 = interval_end)
      {
	for (int end_offset1 = -1; end_offset1 <= 1; ++end_offset1)
	{
	  long end1 = start_or_end_end1 + end_offset1;
	  if (end1 < start1)
	    continue;
	  std::pair<memblk_map_ct::iterator, bool> const& p3(map1.insert(
	      memblk_ct(memblk_key_ct((void*)start1, end1 - start1),
	      memblk_info_ct((void*)start1, end1 - start1, memblk_type_malloc)) ));
	  if (p3.second)
	    ++cnt;
	  for (long start_or_end_start2 = interval_start;; start_or_end_start2 = interval_end)
	  {
	    for (int start_offset2 = -1; start_offset2 <= 1; ++start_offset2)
	    {
	      long start2 = start_or_end_start2 + start_offset2;
	      for (long start_or_end_end2 = start_or_end_start2;; start_or_end_end2 = interval_end)
	      {
		for (int end_offset2 = -1; end_offset2 <= 1; ++end_offset2)
		{
		  long end2 = start_or_end_end2 + end_offset2;
		  if (end2 < start2)
		    continue;
                  memblk_map_ct map2;
		  std::pair<memblk_map_ct::iterator, bool> const& p1(map2.insert(
                      memblk_ct(memblk_key_ct((void*)start1, end1 - start1),
                      memblk_info_ct((void*)start1, end1 - start1, memblk_type_malloc)) ));
                  if (!p1.second)
                    return true;
                  if ((*p1.first).first.start() != (void*)start1 || (*p1.first).first.end() != (void*)end1)
                    return true;
                  bool overlap = !((start1 < start2 && end1 <= start2) || (start2 < start1 && end2 <= start1));
		  std::pair<memblk_map_ct::iterator, bool> const& p2(map2.insert(
                      memblk_ct(memblk_key_ct((void*)start2, end2 - start2),
                      memblk_info_ct((void*)start2, end2 - start2, memblk_type_malloc)) ));
                  if (overlap && p2.second || (!overlap && !p2.second))
                    return true;
		  if (((*p2.first).first.start() < (void*)start2 && (*p2.first).first.end() <= (void*)start2) ||
		      ((void*)start2 < (*p2.first).first.start() && (void*)end2 <= (*p2.first).first.start()))
		    return true;
		}
		if (start_or_end_end2 == interval_end)
		  break;
	      }
	    }
	    if (start_or_end_start2 == interval_end)
	      break;
	  }
	}
	if (start_or_end_end1 == interval_end)
	  break;
      }
    }
    if (start_or_end_start1 == interval_end)
      break;
  }
  if (cnt != map1.size())
    return true;
  return false;
}

#endif // CWDEBUG_DEBUGM

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
    memblk_map.MT_unsafe = new memblk_map_ct;
    WST_initialization_state = -1;
    __libcwd_tsd.internal = 0;
  }

#if CWDEBUG_LOCATION
  if (__libcwd_tsd.library_call++)
    ++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);	// Otherwise debug output will be generated from bfd.cc (location_ct)
  location_ct loc(reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset LIBCWD_COMMA_TSD);
  if (--__libcwd_tsd.library_call)
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
#endif

  DEBUGDEBUG_CERR( "register_external_allocation: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;

  // Update our administration:
  LIBCWD_DEFER_CANCEL
  ACQUIRE_WRITE_LOCK
  memblk_ct memblk(memblk_key_ct(mptr, size), memblk_info_ct(mptr, size, memblk_type_external));	// MT: memblk_info_ct() needs wrlock.
  std::pair<memblk_map_ct::iterator, bool> const& iter(memblk_map_write->insert(memblk));
  RELEASE_WRITE_LOCK
  LIBCWD_RESTORE_CANCEL

  DEBUGDEBUG_CERR( "register_external_allocation: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
  
  // MT-safe: iter points to an element that is garanteed unique to this thread (we just allocated it).
  if (!iter.second)
    DoutFatalInternal( dc::core, "register_external_allocation: externally (supposedly newly) allocated block collides with *existing* memblk!  Are you sure this memory block was externally allocated, or did you call RegisterExternalAlloc twice for the same pointer?" );

  memblk_info_ct& memblk_info((*iter.first).second);
  memblk_info.lock();		// Lock ownership (doesn't call malloc).
#if CWDEBUG_LOCATION
  memblk_info.get_alloc_node()->location_reference().move(loc);
#endif
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

#if (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && defined(_REENTRANT) && CWDEBUG_DEBUG
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
    memset(static_cast<void*>(static_cast<size_t*>(ptr) + 2), 0, nmemb * size);
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
    memset(ptr, 0, size);
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
  location_ct loc(reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset LIBCWD_COMMA_TSD);
  if (--__libcwd_tsd.library_call)
    --LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
#endif

  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 1." );
  __libcwd_tsd.internal = 1;

  LIBCWD_DEFER_CANCEL_NO_BRACE
  ACQUIRE_READ_LOCK
  memblk_map_ct::const_iterator const& iter(memblk_map_read->find(memblk_key_ct(ptr, 0)));
  if (iter == memblk_map_read->end() || (*iter).first.start() != ptr)
  {
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
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
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
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
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
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
    RELEASE_READ_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
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
  char const* d = (*iter).second.description();
  ACQUIRE_READ2WRITE_LOCK
  memblk_ct memblk(memblk_key_ct(mptr, size), memblk_info_ct(mptr, size, memblk_type_realloc));	// MT: memblk_info_ct() needs wrlock.
  memblk_map_write->erase(iter_write);
  std::pair<memblk_map_ct::iterator, bool> const& iter2(memblk_map_write->insert(memblk));
  if (!iter2.second)
  {
    RELEASE_WRITE_LOCK
    LIBCWD_RESTORE_CANCEL_NO_BRACE
    DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
    __libcwd_tsd.internal = 0;
    DoutFatalInternal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );
  }
  memblk_info_ct& memblk_info((*(iter2.first)).second);
  memblk_info.change_label(*t, d);
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << __libcwd_tsd.internal << "; setting it to 0." );
  __libcwd_tsd.internal = 0;
#if CWDEBUG_LOCATION
  memblk_info.get_alloc_node()->location_reference().move(loc);
#endif
  RELEASE_WRITE_LOCK
  LIBCWD_RESTORE_CANCEL_NO_BRACE

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
