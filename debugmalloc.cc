// $Header$
//
// Copyright (C) 2000, by
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
// - malloc, calloc, realloc and free.
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
//    - those allocated in this file
//    - those allocated after a call to set_alloc_checking_off()
//      (and before the corresponding set_alloc_checking_on()).
//    - those that are removed from this tree later with a call to
//      make_invisible() (see below).
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
// This tree also contains the data related to the memory block itself,
// currently its start, size, type of allocation (new/new[]/malloc/realloc)
// and, when provided by the user by means of AllocTag(), a type_info_ct
// of the returned pointer and a description.
//
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
// are stored as a pair<memblk_key_ct const, memblk_info_ct> in the STL map.
//
// Creating the memblk_info_ct creates a related dm_alloc_ct.
//
// delete and free erase the pair<memblk_key_ct const, memblk_info_ct> from
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
// - long memblks(void)
//		Returns the total number of allocated memory blocks
//		(Those that are shown by `list_allocations_on').
// - ostream& operator<<(ostream& o, debugmalloc_report_ct)
//		Allows to write a memory allocation report to ostream `o'.
// - void list_allocations_on(debug_ct& debug_object);
//		Prints out all visible allocated memory blocks and labels.
//
// The current 'manipulator' functions are:
//
// - void move_outside(marker_ct* marker, void const* ptr)
//		Move `ptr' outside the list of `marker'.
// - void set_alloc_checking_off(void)
//		After calling this function, new allocations are invisible.
//		This means that even test_delete() etc will not find them.
//		Calls to `set_alloc_checking_off' and `set_alloc_checking_on'
//		can be nested.
// - void set_alloc_checking_on(void)
//		Cancel a call to `set_alloc_checking_off'.
// - void make_invisible(void const* ptr)
//		Makes `ptr' invisible, as if it was not allocated at all.
// - void make_all_allocations_invisible_except(void* ptr)
//		For internal use.
//

#define DEBUGMALLOC_INTERNAL
#include <libcw/sys.h>
#include <libcw/debug_config.h>

#ifdef DEBUGMALLOC

#include <cstdlib>	// Needed for malloc prototypes
#include <cstring>
#include <string>
#include <map>
#include <iomanip>
#include <libcw/no_alloc_checking_stringstream.h>
#include <libcw/debug.h>
#include <libcw/lockable_auto_ptr.h>
#include <libcw/demangle.h>
#ifdef DEBUGUSEBFD
#include <libcw/bfd.h>
#endif
#ifdef CWDEBUG
#include <libcw/iomanip.h>
#include <libcw/cwprint.h>
#endif

RCSTAG_CC("$Id$")

#ifdef CWDEBUG
extern "C" int raise (int sig);
namespace libcw { namespace debug { extern void initialize_globals(void); } }

#ifdef DEBUGDEBUG
#define DEBUGDEBUG_DoutInternal_MARKER DEBUGDEBUG_CERR( "DoutInternal at " << __FILE__ << ':' << __LINE__ )
#define DEBUGDEBUG_DoutFatalInternal_MARKER DEBUGDEBUG_CERR( "DoutFatalInternal at " << __FILE__ << ':' << __LINE__ )
#define DEBUGDEBUG_ELSE_DoutInternal(data) else DEBUGDEBUG_CERR( "library_call == " << library_call << "; DoutInternal skipped for: " << data );
#define DEBUGDEBUG_ELSE_DoutFatalInternal(data) DEBUGDEBUG_CERR( "library_call == " << library_call << "; DoutFatalInternal skipped for: " << data );
#else
#define DEBUGDEBUG_DoutInternal_MARKER
#define DEBUGDEBUG_DoutFatalInternal_MARKER
#define DEBUGDEBUG_ELSE_DoutInternal(data)
#define DEBUGDEBUG_ELSE_DoutFatalInternal(data)
#endif

#define DoutInternal( cntrl, data )			\
  do							\
  {							\
    DEBUGDEBUG_DoutInternal_MARKER;			\
    if (library_call == 0 && libcw_do._off < 0)		\
    {							\
DEBUGDEBUG_CERR( "Entering 'DoutInternal(cntrl, \"" << data << "\")'.  internal == " << internal << "; setting library_call to 1 and internal to false." ); \
      int prev_internal = internal;			\
      internal = false;					\
      ++library_call;					\
      bool on;						\
      {							\
        using namespace channels;			\
        on = (libcw_do|cntrl).on;			\
      }							\
      if (on)						\
      {							\
        libcw_do.start();				\
        (*libcw_do.current_oss) << data;		\
        libcw_do.finish();				\
      }							\
      --library_call;					\
DEBUGDEBUG_CERR( "Leaving 'DoutInternal(cntrl, \"" << data << "\")'.  internal = " << internal << "; setting library_call to " << library_call << " and internal to " << prev_internal << '.' ); \
      internal = prev_internal;				\
    }							\
    DEBUGDEBUG_ELSE_DoutInternal(data)			\
  } while(0)

#define DoutFatalInternal( cntrl, data )		\
  do							\
  {							\
    DEBUGDEBUG_DoutFatalInternal_MARKER;		\
    if (library_call < 2)				\
    {							\
DEBUGDEBUG_CERR( "Entering 'DoutFatalInternal(cntrl, \"" << data << "\")'.  internal == " << internal << "; setting library_call to " << (library_call + 1) << " and internal to false." ); \
      internal = false;					\
      ++library_call;					\
      {							\
	using namespace channels;			\
	libcw_do|cntrl;					\
      }							\
      libcw_do.start();					\
      (*libcw_do.current_oss) << data;			\
      libcw_do.fatal_finish();	/* Never returns */	\
      ASSERT( !"Bug in libcwd!" );			\
    }							\
    else						\
    {							\
      DEBUGDEBUG_ELSE_DoutFatalInternal(data)		\
      ASSERT( !"See msg above." );			\
      raise(3);						\
    }							\
  } while(0)

#else // !CWDEBUG
#define DoutInternal( cntrl, data )
#define DoutFatalInternal( cntrl, data )
#endif // !CWDEBUG

using std::string;
using std::ostream;
using std::ios;
using std::map;
using std::pair;
using std::less;
using std::setw;
using std::ends;

namespace libcw {
  namespace debug {

namespace _internal_ {
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
  // free memory.  Writing debug output or directly to cerr also causes a
  // lot of calls to operator new and operator new[].  But in this case
  // we have no control from where these allocations are done, or from
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
  // and when creating a location_ct (which is a 'library call' of libcwd).
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
  bool internal = false;
  int library_call = 0;
} // namespace _internal_

using _internal_::internal;
using _internal_::library_call;

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

static enum deallocated_from_nt { from_free, from_delete, from_delete_array, error } deallocated_from = from_free;
  // Indicates if '__libcwd_free()' was called directly or via 'operator delete()' or 'operator delete[]()'.

static deallocated_from_nt expected_from[] = {
  from_delete,
  error,
  from_delete_array,
  error,
  from_free,
  from_free,
  error,
  error,
  from_delete,
  error,
  error,
  from_free
};

ostream& operator<<(ostream& os, memblk_types_ct memblk_type)
{
#ifdef CWDEBUG
  memblk_types_ct::omanip_data_ct& manip_data(get_omanip_data<memblk_types_ct>(os));
  if (manip_data.is_debug_channel() && !manip_data.is_amo_label())
  {
    switch(memblk_type())
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
      case memblk_type_noheap:
	os << "memblk_type_noheap";
	break;
      case memblk_type_removed:
	os << "memblk_type_removed";
	break;
#ifdef DEBUGMARKER
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
#endif
  switch (memblk_type())
  {
    case memblk_type_new:
      os << "          ";
      break;
    case memblk_type_new_array:
      os << "new[]     ";
      break;
    case memblk_type_malloc:
      os << "malloc    ";
      break;
    case memblk_type_realloc:
      os << "realloc   ";
      break;
#ifdef DEBUGMARKER
    case memblk_type_marker:
      os << "(MARKER)  ";
      break;
    case memblk_type_deleted_marker:
#endif
    case memblk_type_deleted:
    case memblk_type_deleted_array:
      os << "(deleted) ";
      break;
    case memblk_type_freed:
      os << "(freed)   ";
      break;
    case memblk_type_noheap:
      os << "(NO HEAP) ";
      break;
    case memblk_type_removed:
      os << "(No heap) ";
      break;
    case memblk_type_external:
      os << "external  ";
      break;
  }
  return os;
}

#ifdef DEBUGDEBUG
_internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, memblk_key_ct const& data)
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
#ifdef DEBUGMARKER
  friend class marker_ct;
  friend void libcw_debug_move_outside(marker_ct* marker, void const* ptr);
#endif
private:
  static dm_alloc_ct** current_alloc_list;
  static dm_alloc_ct* current_owner_node;
  static size_t mem_size;
  static unsigned long memblks;

  dm_alloc_ct* next;		// Next memblk in list `my_list'
  dm_alloc_ct* prev;		// Previous memblk in list `my_list'
  dm_alloc_ct* a_next_list;	// First element of my childeren (if any)
  dm_alloc_ct** my_list;	// Pointer to my list, never NULL.
  dm_alloc_ct* my_owner_node;	// Pointer to node who's a_next_list contains
  				// this object.

public:
  dm_alloc_ct(void const* s, size_t sz, memblk_types_nt f) :
      alloc_ct(s, sz , f, unknown_type_info),
      next(*dm_alloc_ct::current_alloc_list), prev(NULL), a_next_list(NULL),
      my_list(dm_alloc_ct::current_alloc_list), my_owner_node(dm_alloc_ct::current_owner_node)
      {
        *dm_alloc_ct::current_alloc_list = this;
        if (next)
          next->prev = this;
	mem_size += sz;
	++memblks;
      }
    // Constructor: Add `node' at the start of `list'
#ifdef DEBUGDEBUG
  dm_alloc_ct(dm_alloc_ct const& __dummy) : alloc_ct(__dummy) { DoutFatalInternal( dc::fatal, "Calling dm_alloc_ct::dm_alloc_ct(dm_alloc_ct const&)" ); }
    // No copy constructor allowed.
#endif
  ~dm_alloc_ct();
  void new_list(void)
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
  static void descend_current_alloc_list(void)
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
  static unsigned long get_memblks(void) { return memblks; }
  static size_t get_mem_size(void) { return mem_size; }
#ifdef CWDEBUG
  void print_description(void) const;
  void printOn(ostream& os) const;
  friend inline ostream& operator<<(ostream& os, dm_alloc_ct const& alloc) { alloc.printOn(os); return os; }
  void show_alloc_list(int depth, channel_ct const& channel) const;
#endif
};

//=============================================================================
//
// classes memblk_key_ct and memblk_info_ct
//
// The object memblk_ct (pair<memblk_key_ct const, memblk_info_ct) that is
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
  bool operator==(memblk_key_ct b) const { ASSERT( internal ); DoutInternal( dc::warning, "Calling memblk_key_ct::operator==(" << *this << ", " << b << ")" ); return a_start == b.start(); }
#ifdef CWDEBUG
  void printOn(ostream& os) const;
  friend inline ostream& operator<<(ostream& os, memblk_key_ct const& memblk) { memblk.printOn(os); return os; }
#endif
#ifdef DEBUGDEBUG
  static bool selftest(void);
    // Returns true is the self test FAILED.
#endif
  memblk_key_ct(void) { DoutFatalInternal( dc::core, "Huh?" ); }
    // Never use this.  It's needed for the implementation of the pair<>.
};

class memblk_info_ct {
  friend class dm_alloc_ct;
#ifdef DEBUGMARKER
  friend class marker_ct;
  friend void libcw_debug_move_outside(marker_ct* marker, void const* ptr);
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
  void erase(void);
  void lock(void) { a_alloc_node.lock(); }
  void make_invisible(void) { a_alloc_node.reset(); }
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
  void new_list(void) const { a_alloc_node.get()->new_list(); }
  memblk_types_nt flags(void) const { return M_memblk_type; }
#ifdef CWDEBUG
  void print_description(void) const { a_alloc_node.get()->print_description(); }
  void printOn(ostream& os) const;
  friend inline ostream& operator<<(ostream& os, memblk_info_ct const& memblk) { memblk.printOn(os); return os; }
#endif
private:
  memblk_info_ct(void) { DoutFatalInternal( dc::core, "huh?" ); }
    // Never use this.  It's needed for the implementation of the pair<>.
};

typedef pair<memblk_key_ct const, memblk_info_ct> memblk_ct;
  // The value type of the map (which is a
  // map<memblk_key_ct, memblk_info_ct, less<memblk_key_ct> >::value_type)

//=============================================================================
//
// The memblk map:
//

typedef map<memblk_key_ct, memblk_info_ct, less<memblk_key_ct> > memblk_map_ct;
  // The map containing all `memblk_ct' objects.

static memblk_map_ct* memblk_map;
  // The `map' implementation calls `new' when initializing a new `map',
  // therefore this must be a pointer. Or below:

//=============================================================================
//
// dm_alloc_ct methods
//

dm_alloc_ct** dm_alloc_ct::current_alloc_list = &base_alloc_list;
  // The current list to which newly allocated memory blocks are added.

dm_alloc_ct* dm_alloc_ct::current_owner_node = NULL;
  // If the current_alloc_list != &base_alloc_list, then this variable
  // points to the dm_alloc_ct node who owns the current list.

size_t dm_alloc_ct::mem_size = 0;
  // Total number of allocated bytes (excluding internal allocations!)

unsigned long dm_alloc_ct::memblks = 0;
  // Total number of allocated blocks (excluding internal allocations!)

inline bool dm_alloc_ct::is_deleted(void) const
{
  return (a_memblk_type == memblk_type_deleted ||
#ifdef DEBUGMARKER
      a_memblk_type == memblk_type_deleted_marker ||
#endif
      a_memblk_type == memblk_type_freed ||
      a_memblk_type == memblk_type_removed);
}

dm_alloc_ct::~dm_alloc_ct()
{
#ifdef DEBUGDEBUG
  ASSERT( internal );
  if (a_next_list)
    DoutFatalInternal( dc::core, "Removing an dm_alloc_ct that still has an own list" );
  dm_alloc_ct* tmp;
  for(tmp = *my_list; tmp && tmp != this; tmp = tmp->next);
  if (!tmp)
    DoutFatalInternal( dc::core, "Removing an dm_alloc_ct that is not part of its own list" );
#endif
  mem_size -= size();
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

#ifdef CWDEBUG

void dm_alloc_ct::print_description(void) const
{
#ifdef DEBUGDEBUG
  ASSERT( !internal && !library_call );
#endif
#ifdef DEBUGUSEBFD
  if (M_location.is_known())
    Dout(dc::continued, setw(20) << cwprint_using(M_location, &location_ct::print_filename_on) <<
        ':' << setw(5) << setiosflags(std::ios_base::left) << M_location.line());
  else if (M_location.mangled_function_name() != unknown_function_c)
  {
    string f;
    demangle_symbol(M_location.mangled_function_name(), f);
    if (f.size() < 25)
      f.append(25 - f.size(), ' ');
    Dout( dc::continued, f << ' ' );
  }
  else
    Dout( dc::continued, setw(25) << ' ' );
#endif

#ifdef DEBUGMARKER
  if (a_memblk_type == memblk_type_marker || a_memblk_type == memblk_type_deleted_marker)
    Dout( dc::continued, "<marker>;" );
  else
#endif

  {
    char const* a_type = type_info_ptr->demangled_name();
    size_t s = a_type ? strlen(a_type) : 0;		// Can be 0 while deleting a qualifiers_ct object in demangle3.cc
    if (s > 0 && a_type[s - 1] == '*' && type_info_ptr->ref_size() != 0)
    {
      set_alloc_checking_off();	/* for `buf' */
      no_alloc_checking_stringstream* buf = new no_alloc_checking_stringstream;
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
	*buf << '[' << (a_size / type_info_ptr->ref_size()) << ']';
      }
      *buf << ends;
      set_alloc_checking_on();
      Dout( dc::continued, buf->str() );
      set_alloc_checking_off();
#ifdef LIBCW_USE_STRSTREAM
      buf->freeze(0);
#endif
      delete buf;
      set_alloc_checking_on();	/* for `buf' */
    }
    else
      Dout( dc::continued, a_type );

    Dout( dc::continued, ';' );
  }

  if (a_memblk_type != memblk_type_noheap && a_memblk_type != memblk_type_removed)
    Dout( dc::continued, " (sz = " << a_size << ") " );

  if (a_description.get())
    Dout( dc::continued, ' ' << a_description.get() );
}

void dm_alloc_ct::printOn(ostream& os) const
{
  os << "{ start = " << a_start << ", size = " << a_size << ", a_memblk_type = " << a_memblk_type << ",\n\ttype = \"" << type_info_ptr->demangled_name() << "\", description = \"" << (a_description.get() ? a_description.get() : "NULL") <<
      "\", next = " << (void*)next << ", prev = " << (void*)prev <<
      ",\n\tnext_list = " << (void*)a_next_list << ", my_list = " << (void*)my_list << "\n\t( = " << (void*)*my_list << " ) }";
}

void dm_alloc_ct::show_alloc_list(int depth, const channel_ct& channel) const
{
  dm_alloc_ct const* alloc;
  Dout( channel|noprefix_cf|nonewline_cf, memblk_types_ct::setlabel(true) );
  for (alloc = this; alloc; alloc = alloc->next)
  {
    Dout( channel|nolabel_cf|continued_cf, "" ); // Only prints prefix
    for (int i = depth; i > 1; i--)
      Dout( dc::continued, "    " );
    Dout( dc::continued, alloc->memblk_type() << alloc->a_start << ' ' ); // Prints label and start
    alloc->print_description();
    Dout( dc::finish, "" );
    if (alloc->a_next_list)
      alloc->a_next_list->show_alloc_list(depth + 1, channel);
  }
  Dout( channel|noprefix_cf|nonewline_cf, memblk_types_ct::setlabel(false) );
}

#endif // CWDEBUG

//=============================================================================
//
// memblk_ct methods
//

inline memblk_info_ct::memblk_info_ct(void const* s, size_t sz, memblk_types_nt f) :
    M_memblk_type(f), a_alloc_node(new dm_alloc_ct(s, sz, f)) {}

#ifdef CWDEBUG

void memblk_key_ct::printOn(ostream& os) const
{
  os << "{ a_start = " << a_start << ", a_end = " << a_end << " (size = " << size() << ") }";
}

void memblk_info_ct::printOn(ostream& os) const
{
  os << "{ alloc_node = { owner = " << a_alloc_node.is_owner() << ", locked = " << a_alloc_node.strict_owner()
      << ", px = " << a_alloc_node.get() << "\n\t( = " << *a_alloc_node.get() << " ) }";
}
#endif // CWDEBUG

void memblk_info_ct::erase(void)
{
#ifdef DEBUGDEBUG
  ASSERT( internal );
#endif
  dm_alloc_ct* ap = a_alloc_node.get();
#ifdef DEBUGDEBUGMALLOC
  if (ap)
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: memblk_info_ct::erase for " << ap->start() );
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
      case memblk_type_noheap:
	new_flag = memblk_type_removed;
	break;
#ifdef DEBUGMARKER
      case memblk_type_marker:
	new_flag = memblk_type_deleted_marker;
	break;
      case memblk_type_deleted_marker:
#endif
      case memblk_type_deleted:
      case memblk_type_deleted_array:
      case memblk_type_freed:
      case memblk_type_removed:
	DoutFatalInternal( dc::core, "Deleting a memblk_info_ct twice ?" );
    }
    ap->change_flags(new_flag);
  }
}

//=============================================================================
//
// internal_debugmalloc
//
// Allocs a new block of size `size' and updates the internal administration.
//
// Note: This function is called by `__libcwd_malloc', `__libcwd_calloc' and
// `operator new' which end with a call to DoutInternal( dc::__libcwd_malloc|continued_cf, ...)
// and should therefore end with a call to DoutInternal( dc::finish, ptr ).
//

#  ifdef NEED_WORD_ALIGNMENT
#define SIZE_PLUS_TWELVE(s) ((((s) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1)) + 3 * sizeof(size_t))
#define SIZE_PLUS_FOUR(s) ((((s) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1)) + sizeof(size_t))
#  else // !NEED_WORD_ALIGNMENT
#define SIZE_PLUS_TWELVE(s) ((s) + 3 * sizeof(size_t))
#define SIZE_PLUS_FOUR(s) ((s) + sizeof(size_t))
#  endif

#ifdef DEBUGUSEBFD
static void* internal_debugmalloc(size_t size, memblk_types_nt flag, void* call_addr)
#else
static void* internal_debugmalloc(size_t size, memblk_types_nt flag)
#endif
{
  ASSERT( !internal );

  register void* mptr;
#ifndef DEBUGMAGICMALLOC
  if (!(mptr = malloc(size)))
#else
  if ((mptr = static_cast<char*>(malloc(SIZE_PLUS_TWELVE(size))) + 2 * sizeof(size_t)) == (void*)(2 * sizeof(size_t)))
#endif
  {
    DoutInternal( dc::finish, "NULL" );
    DoutInternal( dc::__libcwd_malloc, "Out of memory ! this is only a pre-detection!" );
    return NULL;	// A fatal error should occur directly after this
  }

  if (!memblk_map)
  {
    internal = true;
    memblk_map = new memblk_map_ct;
#ifdef CWDEBUG
    libcw::debug::initialize_globals();	// This doesn't belong in the malloc department at all, but malloc() happens
    					// to be a function that is called _very_ early - and hence this is a good moment
					// to initialize ALL of libcwd.
#endif
    internal = false;
  }

#ifdef DEBUGUSEBFD
  if (library_call++)
    libcw_do._off++;	// Otherwise debug output will be generated from bfd.cc (location_ct)
  location_ct loc(call_addr);
  if (--library_call)
    libcw_do._off--;
#endif

  DEBUGDEBUG_CERR( "internal_debugmalloc: internal == " << internal << "; setting it to true." );
  internal = true;

  // Update our administration:
  pair<memblk_map_ct::iterator, bool> const& i(memblk_map->insert(memblk_ct(memblk_key_ct(mptr, size), memblk_info_ct(mptr, size, flag))));

  DEBUGDEBUG_CERR( "internal_debugmalloc: internal == " << internal << "; setting it to false." );
  internal = false;
  
  if (!i.second)
    DoutFatalInternal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );

  memblk_info_ct& memblk_info((*i.first).second);
  memblk_info.lock();		// Lock ownership (doesn't call malloc).
#ifdef DEBUGUSEBFD
  memblk_info.get_alloc_node()->location_reference().move(loc);
#endif

  DoutInternal( dc::finish, (void*)(mptr) );
  return mptr;
}

void init_debugmalloc(void)
{
  if (!memblk_map)
  {
    set_alloc_checking_off();
    memblk_map = new memblk_map_ct;
#ifdef CWDEBUG
    libcw::debug::initialize_globals();	// This doesn't belong in the malloc department at all, but malloc() happens
    					// to be a function that is called _very_ early - and hence this is a good moment
					// to initialize ALL of libcwd.
#endif
    set_alloc_checking_on();
  }
}

//=============================================================================
//
// 'Accessor' functions
//

bool test_delete(void const* ptr)
{
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  return (i == memblk_map->end() || (*i).first.start() != ptr);
}

size_t mem_size(void)
{
  return dm_alloc_ct::get_mem_size();
}

long memblks(void)
{
  return dm_alloc_ct::get_memblks();
}

#ifdef CWDEBUG

ostream& operator<<(ostream& o, debugmalloc_report_ct)
{
  size_t mem_size = dm_alloc_ct::get_mem_size();
  unsigned long memblks = dm_alloc_ct::get_memblks();
  o << "Allocated memory: " << mem_size << " bytes in " << memblks << " blocks.";
  return o;
}

void list_allocations_on(debug_ct& debug_object)
{
#ifdef DEBUGDEBUG
  ASSERT( !internal && !library_call );
#endif

#if 0
  // Print out the entire `map':
  set_alloc_checking_off();
  memblk_map_ct const* memblk_map_copy = new memblk_map_ct(*memblk_map);
  set_alloc_checking_on();
  LibcwDout( channels, debug_object, dc::__libcwd_malloc, "map:" );
  int cnt = 0;
  for(memblk_map_ct::const_iterator i(memblk_map_copy->begin()); i != memblk_map_copy->end(); ++i)
    LibcwDout( channels, debug_object, dc::__libcwd_malloc|nolabel_cf, << ++cnt << ":\t(*i).first = " << (*i).first << '\n' << "\t(*i).second = " << (*i).second );
  set_alloc_checking_off();
  delete memblk_map_copy;
  set_alloc_checking_on();
#endif

  LibcwDout( channels, debug_object, dc::__libcwd_malloc, "Allocated memory: " << dm_alloc_ct::get_mem_size() << " bytes in " << dm_alloc_ct::get_memblks() << " blocks." );
  if (base_alloc_list)
    base_alloc_list->show_alloc_list(1, channels::dc::__libcwd_malloc);
}

//=============================================================================
//
// 'Manipulator' functions
//

void make_invisible(void const* ptr)
{
#ifdef DEBUGDEBUG
  if (internal)
    DoutFatalInternal( dc::core, "What do you think you're doing?" );
#endif
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i == memblk_map->end() || (*i).first.start() != ptr)
    DoutFatal( dc::core, "Trying to turn non-existing memory block (" << ptr << ") into an 'internal' block" );
  DEBUGDEBUG_CERR( "make_invisible: internal == " << internal << "; setting it to true." );
  internal = true;
  (*i).second.make_invisible();
  DEBUGDEBUG_CERR( "make_invisible: internal == " << internal << "; setting it to false." );
  internal = false;
}

void make_all_allocations_invisible_except(void const* ptr)
{
  for (dm_alloc_ct const* alloc = base_alloc_list; alloc;)
  {
    dm_alloc_ct const* next = alloc->next_node();
    if (alloc->start() != ptr)
      make_invisible(alloc->start());
    alloc = next;
  }
}

#endif // CWDEBUG

static unsigned int alloc_checking_off_counter = 0;
static bool prev_internal;

void set_alloc_checking_off(void)
{
  if (alloc_checking_off_counter++ == 0)
    prev_internal = internal;
  DEBUGDEBUG_CERR( "set_alloc_checking_off called from " << __builtin_return_address(0) << ": internal == " << internal << "; setting it to true." );
  internal = true;
}

void set_alloc_checking_on(void)
{
#ifdef CWDEBUG
  if (alloc_checking_off_counter == 0)
    DoutFatal( dc::core, "Calling `set_alloc_checking_on' while ALREADY on." );
#endif
  if (--alloc_checking_off_counter == 0)
  {
    DEBUGDEBUG_CERR( "set_alloc_checking_on from " << __builtin_return_address(0) << ": internal == " << internal << "; setting it to " << prev_internal << "." );
    internal = prev_internal;
  }
}

// Undocumented (used in macro AllocTag)
void set_alloc_label(void const* ptr, type_info_ct const& ti, char const* description)
{
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i != memblk_map->end() && (*i).first.start() == ptr)
    (*i).second.change_label(ti, description);
}

void set_alloc_label(void const* ptr, type_info_ct const& ti, lockable_auto_ptr<char, true> description)
{
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i != memblk_map->end() && (*i).first.start() == ptr)
    (*i).second.change_label(ti, description);
}

#undef CALL_ADDRESS
#ifdef DEBUGUSEBFD
#define CALL_ADDRESS , reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset
static void* internal_debugmalloc(size_t size, memblk_types_nt flag, void* call_addr);
#else
#define CALL_ADDRESS
static void* internal_debugmalloc(size_t size, memblk_types_nt flag);
#endif

debugmalloc_newctor_ct::debugmalloc_newctor_ct(void* ptr, type_info_ct const& ti) : no_heap_alloc_node(NULL)
{
#ifdef DEBUGDEBUG
  ASSERT( !internal && !library_call );
  Dout( dc::__libcwd_malloc, "New debugmalloc_newctor_ct at " << this << " from object " << ti.name() << " (" << ptr << ")" );
#endif
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i != memblk_map->end())
  {
    (*i).second.change_label(ti, NULL);
    (*i).second.new_list();
  }
  else
  {
    DEBUGDEBUG_CERR( "debugmalloc_newctor_ct: internal == " << internal << "; setting it to true." );
    internal = true;
    no_heap_alloc_node = new dm_alloc_ct(ptr, 0, memblk_type_noheap);
    DEBUGDEBUG_CERR( "debugmalloc_newctor_ct: internal == " << internal << "; setting it to false." );
    internal = false;
#ifdef DEBUGUSEBFD
    location_ct loc(location_ct(reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset));
    no_heap_alloc_node->location_reference().move(loc);
#endif
    no_heap_alloc_node->new_list();
  }
#ifdef DEBUGDEBUG
  Debug( list_allocations_on(libcw_do) );
#endif
}

debugmalloc_newctor_ct::~debugmalloc_newctor_ct(void)
{
#ifdef DEBUGDEBUG
  ASSERT( !internal && !library_call );
  Dout( dc::__libcwd_malloc, "Removing debugmalloc_newctor_ct at " << (void*)this );
  Debug( list_allocations_on(libcw_do) );
#endif
  // Set `current_alloc_list' one list back
  dm_alloc_ct::descend_current_alloc_list();
  if (no_heap_alloc_node)
  {
    if (no_heap_alloc_node->next_list())
      no_heap_alloc_node->change_flags(memblk_type_removed);
    else
    {
      DEBUGDEBUG_CERR( "~debugmalloc_newctor_ct: internal == " << internal << "; setting it to true." );
      internal = true;
      delete no_heap_alloc_node;
      DEBUGDEBUG_CERR( "~debugmalloc_newctor_ct: internal == " << internal << "; setting it to false." );
      internal = false;
    }
  }
}

#ifdef DEBUGMARKER
void marker_ct::register_marker(char const* label)
{
#ifdef DEBUGDEBUG
  ASSERT( !internal && !library_call );
#endif
  Dout( dc::__libcwd_malloc, "New libcw::debug::marker_ct at " << this );
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(this, 0)));
  memblk_info_ct &info((*i).second);
  if (i == memblk_map->end() || (*i).first.start() != this || info.flags() != memblk_type_new)
    DoutFatal( dc::core, "Use 'new' for libcw::debug::marker_ct" );

  info.change_label(type_info_of(this), label);
  info.change_flags(memblk_type_marker);
  info.new_list();

#ifdef DEBUGDEBUG
  Debug( list_allocations_on(libcw_do) );
#endif
}

marker_ct::~marker_ct(void)
{
#ifdef DEBUGDEBUG
  ASSERT( !internal && !library_call );
#endif

  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(this, 0)));
  if (i == memblk_map->end() || (*i).first.start() != this)
    DoutFatal( dc::core, "Trying to delete an invalid marker" );

#ifdef CWDEBUG
  Dout( dc::__libcwd_malloc, "Removing libcw::debug::marker_ct at " << this << " (" << (*i).second.description() << ')' );

  if ((*i).second.a_alloc_node.get()->next_list())
  {
    string margin = libcw_do.get_margin();
    Debug( libcw_do.set_margin(margin + "  * ") );
    Dout( dc::warning, "Memory leak detected!" );
    (*i).second.a_alloc_node.get()->next_list()->show_alloc_list(1, channels::dc::warning);
    Debug( libcw_do.set_margin(margin) );
  }
#endif

  // Set `current_alloc_list' one list back
  if (*dm_alloc_ct::current_alloc_list != (*i).second.a_alloc_node.get()->next_list())
    DoutFatal( dc::core, "Deleting a marker must be done in the same \"scope\" as where it was allocated" );
  dm_alloc_ct::descend_current_alloc_list();
}

void libcw_debug_move_outside(marker_ct* marker, void const* ptr)
{
#ifdef DEBUGDEBUG
  ASSERT( !internal && !library_call );
#endif

  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i == memblk_map->end() || (*i).first.start() != ptr)
    DoutFatal( dc::core, "Trying to move non-existing memory block (" << ptr << ") outside memory leak test marker" );
  memblk_map_ct::iterator const& j(memblk_map->find(memblk_key_ct(marker, 0)));
  if (j == memblk_map->end() || (*j).first.start() != marker)
    DoutFatal( dc::core, "No such marker: " << (void*)marker );
  dm_alloc_ct* alloc_node = (*i).second.a_alloc_node.get();
  if (!alloc_node)
    DoutFatal( dc::core, "Trying to move an invisible memory block outside memory leak test marker" );
  dm_alloc_ct* marker_alloc_node = (*j).second.a_alloc_node.get();
  if (!marker_alloc_node || marker_alloc_node->a_memblk_type != memblk_type_marker)
    DoutFatal( dc::core, "That is not a marker: " << (void*)marker );

  // Look if it is in the right list
  for(dm_alloc_ct* node = alloc_node; node;)
  {
    node = node->my_owner_node;
    if (node == marker_alloc_node)
    {
      // Correct.
      // Delink it:
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
      return;
    }
  }
  Dout( dc::warning, "Memory block at " << ptr << " is already outside the marker at " << (void*)marker << " (" << marker_alloc_node->type_info_ptr->demangled_name() << ") area!" );
}
#endif // DEBUGMARKER

alloc_ct const* find_alloc(void const* ptr)
{ 
#ifdef DEBUGDEBUG
  ASSERT( !internal && !library_call );
#endif

  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  return (i == memblk_map->end()) ? NULL : (*i).second.get_alloc_node();
}

//=============================================================================
//
// Self tests:
//

#ifdef DEBUGDEBUG

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
	  pair<memblk_map_ct::iterator, bool> const& p3(map1.insert(
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
                  pair<memblk_map_ct::iterator, bool> const& p1(map2.insert(
                      memblk_ct(memblk_key_ct((void*)start1, end1 - start1),
                      memblk_info_ct((void*)start1, end1 - start1, memblk_type_malloc)) ));
                  if (!p1.second)
                    return true;
                  if ((*p1.first).first.start() != (void*)start1 || (*p1.first).first.end() != (void*)end1)
                    return true;
                  bool overlap = !((start1 < start2 && end1 <= start2) || (start2 < start1 && end2 <= start1));
                  pair<memblk_map_ct::iterator, bool> const& p2(map2.insert(
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

#endif // DEBUGDEBUG

#ifdef DEBUGMAGICMALLOC

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

char const* diagnose_from(deallocated_from_nt from, bool visible = true)
{
  switch(from)
  {
    case from_free:
      return visible ? "You are 'free()'-ing a pointer (" : "You are 'free()'-ing an invisible memory block (at ";
    case from_delete:
      return visible ? "You are 'delete'-ing a pointer (" : "You are 'delete'-ing an invisible memory block (at ";
    case from_delete_array:
      return visible ? "You are 'delete[]'-ing a pointer (" : "You are 'delete[]'-ing an invisible memory block (at ";
    case error:
      break;
  }
  return "Huh? Bug in libcwd!";
}

char const* diagnose_magic(size_t magic_begin, size_t const* magic_end)
{
  switch(magic_begin)
  {
    case INTERNAL_MAGIC_NEW_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_NEW_END)
        return ") that was allocated with 'new' internally by libcwd.  This might be a bug in libcwd.";
      break;
    case INTERNAL_MAGIC_NEW_ARRAY_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_NEW_ARRAY_END)
        return ") that was allocated with 'new[]' internally by libcwd.  This might be a bug in libcwd.";
      break;
    case INTERNAL_MAGIC_MALLOC_BEGIN:
      if (*magic_end == INTERNAL_MAGIC_MALLOC_END)
        return ") that was allocated with 'malloc()' internally by libcwd.  This might be a bug in libcwd.";
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

#endif // DEBUGMAGICMALLOC

void register_external_allocation(void const* mptr, size_t size)
{
#if defined(DEBUGDEBUG) && defined(DEBUGUSEBFD) && defined(__GLIBCPP__)
  ASSERT( _internal_::ios_base_initialized );
#endif
  if (internal)
    DoutFatalInternal( dc::core, "Calling register_external_allocation while `internal' is true!  "
                                 "You can't use RegisterExternalAlloc() inside a set_alloc_checking_off() ... "
				 "set_alloc_checking_on() set, or inside a Dout() et. al." );

  DoutInternal( dc::__libcwd_malloc, "register_external_allocation(" << (void*)mptr << ", " << size << ')' );

  if (!memblk_map)
  {
    internal = true;
    memblk_map = new memblk_map_ct;
#ifdef CWDEBUG
    libcw::debug::initialize_globals();	// This doesn't belong in the malloc department at all, but malloc() happens
    					// to be a function that is called _very_ early - and hence this is a good moment
					// to initialize ALL of libcwd.
#endif
    internal = false;
  }

#ifdef DEBUGUSEBFD
  if (library_call++)
    libcw_do._off++;	// Otherwise debug output will be generated from bfd.cc (location_ct)
  location_ct loc(reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset);
  if (--library_call)
    libcw_do._off--;
#endif

  DEBUGDEBUG_CERR( "register_external_allocation: internal == " << internal << "; setting it to true." );
  internal = true;

  // Update our administration:
  pair<memblk_map_ct::iterator, bool> const& i(memblk_map->insert(memblk_ct(memblk_key_ct(mptr, size), memblk_info_ct(mptr, size, memblk_type_external))));

  DEBUGDEBUG_CERR( "register_external_allocation: internal == " << internal << "; setting it to false." );
  internal = false;
  
  if (!i.second)
    DoutFatalInternal( dc::core, "register_external_allocation: externally (supposedly newly) allocated block collides with *existing* memblk!  Are you sure this memory block was externally allocated, or did you call RegisterExternalAlloc twice for the same pointer?" );

  memblk_info_ct& memblk_info((*i.first).second);
  memblk_info.lock();		// Lock ownership (doesn't call malloc).
#ifdef DEBUGUSEBFD
  memblk_info.get_alloc_node()->location_reference().move(loc);
#endif
}

  } // namespace debug
} // namespace libcw

using namespace ::libcw::debug;

//=============================================================================
//
// malloc(3) and calloc(3) replacements:
//

void* __libcwd_malloc(size_t size)
{
#if defined(DEBUGDEBUG) && defined(DEBUGUSEBFD) && defined(__GLIBCPP__)
  ASSERT( _internal_::ios_base_initialized );
#endif
  if (internal)
  {
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Entering `__libcwd_malloc(" << size << ")'" );

#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return malloc(size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)

  #ifndef DEBUGMAGICMALLOC
    void* ptr = malloc(size);
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal malloc(" << size << ") = " << ptr );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `__libcwd_malloc': " << ptr );
    #endif // DEBUGDEBUG
    return ptr;
  #else // DEBUGMAGICMALLOC
    void* ptr = malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      return NULL;
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal malloc(" << size << ") = " << static_cast<size_t*>(ptr) + 2 );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `__libcwd_malloc': " << static_cast<size_t*>(ptr) + 2 );
    #endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr) + 2;
  #endif // DEBUGMAGICMALLOC

#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  } // internal

  DoutInternal( dc::__libcwd_malloc|continued_cf, "malloc(" << size << ") = " );
  void* ptr = internal_debugmalloc(size, memblk_type_malloc CALL_ADDRESS);

#ifdef DEBUGMAGICMALLOC
  if (ptr)
  {
    ((size_t*)ptr)[-2] = MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
  }
#endif

  return ptr;
}

void* __libcwd_calloc(size_t nmemb, size_t size)
{
#if defined(DEBUGDEBUG) && defined(DEBUGUSEBFD) && defined(__GLIBCPP__)
  ASSERT( _internal_::ios_base_initialized );
#endif
  if (internal)
  {
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Entering `__libcwd_calloc(" << nmemb << ", " << size << ")'" );

#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return calloc(nmemb, size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)

  #ifndef DEBUGMAGICMALLOC
    void* ptr = calloc(nmemb, size);
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal calloc(" << nmemb << ", " << size << ") = " << ptr );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `__libcwd_calloc': " << ptr );
    #endif
    return ptr;
  #else // DEBUGMAGICMALLOC
    void* ptr = malloc(SIZE_PLUS_TWELVE(nmemb * size));
    if (!ptr)
      return NULL;
    memset(static_cast<void*>(static_cast<size_t*>(ptr) + 2), 0, nmemb * size);
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[1] = nmemb * size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(nmemb * size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal calloc(" << nmemb << ", " << size << ") = " << static_cast<size_t*>(ptr) + 2 );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `__libcwd_calloc': " << static_cast<size_t*>(ptr) + 2 );
    #endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr) + 2;
  #endif // DEBUGMAGICMALLOC

#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  } // internal

  DoutInternal( dc::__libcwd_malloc|continued_cf, "calloc(" << nmemb << ", " << size << ") = " );
  void* ptr;
  size *= nmemb;
  if ((ptr = internal_debugmalloc(size, memblk_type_malloc CALL_ADDRESS)))
    memset(ptr, 0, size);

#ifdef DEBUGMAGICMALLOC
  if (ptr)
  {
    ((size_t*)ptr)[-2] = MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
  }
#endif

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
#if defined(DEBUGDEBUG) && defined(DEBUGUSEBFD) && defined(__GLIBCPP__)
  ASSERT( _internal_::ios_base_initialized );
#endif
  if (internal)
  {
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Entering `__libcwd_realloc(" << ptr << ", " << size << ")'" );

#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return realloc(ptr, size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)

  #ifndef DEBUGMAGICMALLOC
    void* ptr1 = realloc(ptr, size);
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal realloc(" << ptr << ", " << size << ") = " << ptr1 );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `__libcwd_realloc': " << ptr1 );
    #endif // DEBUGDEBUG
    return ptr1;
  #else // DEBUGMAGICMALLOC
    ptr = static_cast<size_t*>(ptr) - 2;
    if (((size_t*)ptr)[0] != INTERNAL_MAGIC_MALLOC_BEGIN ||
	((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_MALLOC_END)
      DoutFatalInternal( dc::core, "internal realloc: magic number corrupt!" );
    void* ptr1 = realloc(ptr, SIZE_PLUS_TWELVE(size));
    ((size_t*)ptr1)[1] = size;
    ((size_t*)(static_cast<char*>(ptr1) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal realloc(" << static_cast<size_t*>(ptr) + 2 << ", " << size << ") = " << static_cast<size_t*>(ptr1) + 2 );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `__libcwd_realloc': " << static_cast<size_t*>(ptr1) + 2 );
    #endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr1) + 2;
  #endif // DEBUGMAGICMALLOC

#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  } // internal

  DoutInternal( dc::__libcwd_malloc|continued_cf, "realloc(" << ptr << ", " << size << ") = " );

  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << internal << "; setting it to true." );
  internal = true;
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i == memblk_map->end() || (*i).first.start() != ptr)
  {
    DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << internal << "; setting it to false." );
    internal = false;
    DoutInternal( dc::finish, "" );
    DoutFatalInternal( dc::core, "Trying to realloc() an invalid pointer (" << ptr << ')' );
  }
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << internal << "; setting it to false." );
  internal = false;

  register void* mptr;

#ifndef DEBUGMAGICMALLOC
  if (!(mptr = realloc(ptr, size)))
#else
  if (((size_t*)ptr)[-2] != MAGIC_MALLOC_BEGIN ||
      ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_MALLOC_END)
  {
    if (((size_t*)ptr)[-2] == MAGIC_NEW_BEGIN &&
	((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `new'!" );
    if (((size_t*)ptr)[-2] == MAGIC_NEW_ARRAY_BEGIN &&
	((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_ARRAY_END)
      DoutFatalInternal( dc::core, "You can't realloc() a block that was allocated with `new[]'!" );
    DoutFatalInternal( dc::core, "realloc: magic number corrupt!" );
  }
  if ((mptr = static_cast<char*>(realloc(static_cast<size_t*>(ptr) - 2, SIZE_PLUS_TWELVE(size))) + 2 * sizeof(size_t)) == (void*)(2 * sizeof(size_t)))
#endif
  {
    DoutInternal( dc::finish, "NULL" );
    DoutInternal( dc::__libcwd_malloc, "Out of memory! This is only a pre-detection!" );
    return NULL; // A fatal error should occur directly after this
  }
#ifdef DEBUGMAGICMALLOC
  ((size_t*)mptr)[-1] = size;
  ((size_t*)(static_cast<char*>(mptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
#endif

#ifdef DEBUGUSEBFD
  if (library_call++)
    libcw_do._off++;    // Otherwise debug output will be generated from bfd.cc (location_ct)
  location_ct loc(reinterpret_cast<char*>(__builtin_return_address(0)) + builtin_return_address_offset);
  if (--library_call)
    libcw_do._off--;
#endif

  // Update administration
  // Note that the way this is done assumes that memory blocks allocated
  // with malloc, calloc or realloc do NOT have a next_list ! Only
  // memory blocks allocated with `new' can have a next_list.
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << internal << "; setting it to true." );
  internal = true;
  type_info_ct const* t = (*i).second.typeid_ptr();
  char const* d = (*i).second.description();
  memblk_map->erase(i);
  pair<memblk_map_ct::iterator, bool> const&
      j(memblk_map->insert(memblk_ct(memblk_key_ct(mptr, size),
      memblk_info_ct(mptr, size, memblk_type_realloc))));
  if (!j.second)
  {
    DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << internal << "; setting it to false." );
    internal = false;
    DoutFatalInternal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );
  }
  memblk_info_ct& memblk_info((*(j.first)).second);
  memblk_info.change_label(*t, d);
  DEBUGDEBUG_CERR( "__libcwd_realloc: internal == " << internal << "; setting it to false." );
  internal = false;
#ifdef DEBUGUSEBFD
  memblk_info.get_alloc_node()->location_reference().move(loc);
#endif

  DoutInternal( dc::finish, (void*)(mptr) );
  return mptr;
}

//=============================================================================
//
// __libcwd_free, replacement for free(3)
//
// frees a block and updates the internal administration.
//

void __libcwd_free(void* ptr)
{
#if defined(DEBUGDEBUG) && defined(DEBUGUSEBFD) && defined(__GLIBCPP__)
  ASSERT( _internal_::ios_base_initialized );
#endif
  deallocated_from_nt from = deallocated_from;
  deallocated_from = from_free;
  if (internal)
  {
#ifdef DEBUGDEBUG
    if (from == from_delete)
    {
      DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal delete(" << ptr << ')' );
      DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal `delete(" << ptr << ")'" );
    }
    else if (from == from_delete_array)
    {
      DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal delete[](" << ptr << ')' );
      DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal `delete[](" << ptr << ")'" );
    }
    else
    {
      DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal free(" << ptr << ')' );
      DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal `free(" << ptr << ")'" );
    }
#endif // DEBUGDEBUG
#ifdef DEBUGMAGICMALLOC
    if (!ptr)
      return;
    ptr = static_cast<size_t*>(ptr) - 2;
    if (from == from_delete)
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_NEW_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_NEW_END)
        DoutFatalInternal( dc::core, "internal delete: " << diagnose_from(from) << (static_cast<size_t*>(ptr) + 2) <<
	    diagnose_magic(((size_t*)ptr)[0], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])) - 1) );
    }
    else if (from == from_delete_array)
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_NEW_ARRAY_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_NEW_ARRAY_END)
        DoutFatalInternal( dc::core, "internal delete[]: " << diagnose_from(from) << (static_cast<size_t*>(ptr) + 2) <<
	    diagnose_magic(((size_t*)ptr)[0], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])) - 1) );
    }
    else
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_MALLOC_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_MALLOC_END)
        DoutFatalInternal( dc::core, "internal free: " << diagnose_from(from) << (static_cast<size_t*>(ptr) + 2) <<
	    diagnose_magic(((size_t*)ptr)[0], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])) - 1) );
    }
    ((size_t*)ptr)[0] ^= (size_t)-1;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] ^= (size_t)-1;
#endif // DEBUGMAGICMALLOC
    free(ptr);
    return;
  } // internal

  if (!ptr)
  {
    DoutInternal( dc::__libcwd_malloc, "Trying to free NULL - ignored." );
    return;
  }

  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i == memblk_map->end() || (*i).first.start() != ptr)
  {
#ifdef DEBUGMAGICMALLOC
    if (((size_t*)ptr)[-2] == INTERNAL_MAGIC_NEW_BEGIN ||
        ((size_t*)ptr)[-2] == INTERNAL_MAGIC_NEW_ARRAY_BEGIN ||
	((size_t*)ptr)[-2] == INTERNAL_MAGIC_MALLOC_BEGIN)
      DoutFatalInternal( dc::core, "Trying to " <<
          ((from == from_delete) ? "delete" : ((from == from_free) ? "free" : "delete[]")) <<
	  " a pointer (" << ptr << ") that appears to be internally allocated!  This might be a bug in libcwd.  The magic number diagnostic gives: " <<
	  diagnose_from(from) << ptr << diagnose_magic(((size_t*)ptr)[-2], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])) - 1) );

#endif
    DoutFatalInternal( dc::core, "Trying to " <<
        ((from == from_delete) ? "delete" : ((from == from_free) ? "free" : "delete[]")) << " an invalid pointer (" << ptr << ')' );
  }
  else
  {
    memblk_types_nt f = (*i).second.flags();
#ifdef CWDEBUG
    bool visible = (!library_call && (*i).second.has_alloc_node());
    if (visible)
    {
      DoutInternal( dc::__libcwd_malloc|continued_cf,
	  ((from == from_free) ? "free(" : ((from == from_delete) ? "delete " : "delete[] "))
	  << ptr << ((from == from_free) ? ") " : " ") );
      (*i).second.print_description();
      DoutInternal( dc::continued, ' ' );
    }
#endif // CWDEBUG
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

    DEBUGDEBUG_CERR( "__libcwd_free: internal == " << internal << "; setting it to true." );
    internal = true;
    (*i).second.erase();	// Update flags and optional decouple
    memblk_map->erase(i);	// Update administration
    DEBUGDEBUG_CERR( "__libcwd_free: internal == " << internal << "; setting it to false." );
    internal = false;

#ifdef DEBUGMAGICMALLOC
    if (f == memblk_type_external)
      free(ptr);
    else
    {
      if (from == from_delete)
      {
	if (((size_t*)ptr)[-2] != MAGIC_NEW_BEGIN ||
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_NEW_END)
	  DoutFatalInternal( dc::core, diagnose_from(from, (*i).second.has_alloc_node()) << ptr << diagnose_magic(((size_t*)ptr)[-2], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])) - 1) );
      }
      else if (from == from_delete_array)
      {
	if (((size_t*)ptr)[-2] != MAGIC_NEW_ARRAY_BEGIN ||
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_NEW_ARRAY_END)
	  DoutFatalInternal( dc::core, diagnose_from(from, (*i).second.has_alloc_node()) << ptr << diagnose_magic(((size_t*)ptr)[-2], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])) - 1) );
      }
      else
      {
	if (((size_t*)ptr)[-2] != MAGIC_MALLOC_BEGIN ||
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_MALLOC_END)
	  DoutFatalInternal( dc::core, diagnose_from(from, (*i).second.has_alloc_node()) << ptr << diagnose_magic(((size_t*)ptr)[-2], (size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])) - 1) );
      }
      ((size_t*)ptr)[-2] ^= (size_t)-1;
      ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] ^= (size_t)-1;
      free(static_cast<size_t*>(ptr) - 2);		// Free memory block
    }
#else // !DEBUGMAGICMALLOC
    free(ptr);			// Free memory block
#endif // !DEBUGMAGICMALLOC

#ifdef CWDEBUG
    if (visible)
      DoutInternal( dc::finish, "" );
#endif // CWDEBUG

  }
}

//=============================================================================
//
// operator `new' and `new []' replacements.
//

void* operator new(size_t size)
{
  if (internal)
  {
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Entering `operator new', size = " << size );

#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return malloc(size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)

  #ifndef DEBUGMAGICMALLOC
    void* ptr = malloc(size);
    if (!ptr)
      DoutFatalInternal( dc::core, "Out of memory in `operator new'" );
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal operator new(" << size << ") = " << ptr );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `operator new': " << ptr );
    #endif // DEBUGDEBUG
    return ptr;
  #else // DEBUGMAGICMALLOC
    void* ptr = malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      DoutFatalInternal( dc::core, "Out of memory in `operator new'" );
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_NEW_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_NEW_END;
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal operator new(" << size << ") = " << static_cast<size_t*>(ptr) + 2 );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `operator new': " << static_cast<size_t*>(ptr) + 2 );
    #endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr) + 2;
  #endif // DEBUGMAGICMALLOC

#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  } // internal

  DoutInternal( dc::__libcwd_malloc|continued_cf, "operator new (size = " << size << ") = " );
  void* ptr = internal_debugmalloc(size, memblk_type_new CALL_ADDRESS);
  if (!ptr)
    DoutFatalInternal( dc::core, "Out of memory in `operator new'" );
#ifdef DEBUGMAGICMALLOC
  else
  {
    ((size_t*)ptr)[-2] = MAGIC_NEW_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_NEW_END;
  }
#endif
  return ptr;
}

void* operator new[](size_t size)
{
  if (internal)
  {
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Entering `operator new[]', size = " << size );

#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return malloc(size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)

  #ifndef DEBUGMAGICMALLOC
    void* ptr = malloc(size);
    if (!ptr)
      DoutFatalInternal( dc::core, "Out of memory in `operator new[]'" );
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal operator new[](" << size << ") = " << ptr );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `operator new[]': " << ptr );
    #endif // DEBUGDEBUG
    return ptr;
  #else // DEBUGMAGICMALLOC
    void* ptr = malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      DoutFatalInternal( dc::core, "Out of memory in `operator new[]'" );
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_NEW_ARRAY_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_NEW_ARRAY_END;
    #ifdef DEBUGDEBUG
    DoutInternal( dc::__libcwd_malloc|cerr_cf, "Internal operator new[](" << size << ") = " << static_cast<size_t*>(ptr) + 2 );
    DEBUGDEBUGMALLOC_CERR( "DEBUGDEBUGMALLOC: Internal: Leaving `operator new[]': " << static_cast<size_t*>(ptr) + 2 );
    #endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr) + 2;
  #endif // DEBUGMAGICMALLOC

#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  } // internal

  DoutInternal( dc::__libcwd_malloc|continued_cf, "operator new[] (size = " << size << ") = " );
  void* ptr = internal_debugmalloc(size, memblk_type_new_array CALL_ADDRESS);
  if (!ptr)
    DoutFatalInternal( dc::core, "Out of memory in `operator new[]'" );
#ifdef DEBUGMAGICMALLOC
  else
  {
    ((size_t*)ptr)[-2] = MAGIC_NEW_ARRAY_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_NEW_ARRAY_END;
  }
#endif
  return ptr;
}

//=============================================================================
//
// operator `delete' and `delete []' replacements.
//

void operator delete(void* ptr)
{
#if defined(DEBUGDEBUG) && defined(DEBUGUSEBFD) && defined(__GLIBCPP__)
  ASSERT( _internal_::ios_base_initialized );
#endif
  deallocated_from = from_delete;
  __libcwd_free(ptr);
}

void operator delete[](void* ptr)
{
#if defined(DEBUGDEBUG) && defined(DEBUGUSEBFD) && defined(__GLIBCPP__)
  ASSERT( _internal_::ios_base_initialized );
#endif
  deallocated_from = from_delete_array;
  __libcwd_free(ptr);			// Note that the standard demands that we call free(), and not delete().
  					// This forces everyone to overload both, operator delete() and operator delete[]()
					// and not only operator delete().
}

#endif /* DEBUGMALLOC */
