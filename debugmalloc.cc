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

#ifdef __GNUG__
#pragma implementation
#endif
#define DEBUGMALLOC_INTERNAL
#include "libcw/sys.h"
#include "libcw/debugging_defs.h"

#ifdef DEBUGMALLOC

#include <malloc.h>
#include <cstring>
#include <string>
#include <map>
#include <iomanip.h>
#include "libcw/h.h"
#include "libcw/debug.h"
#include "libcw/lockable_auto_ptr.h"
#include "libcw/demangle.h"
#ifdef DEBUGUSEBFD
#include "libcw/bfd.h"
#endif
#ifdef DEBUG
#include "libcw/iomanip.h"
#endif

RCSTAG_CC("$Id$")

//=============================================================================
//
// This is version 2 of libcw's "debugmalloc", it should be a hundered times
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
// - Checking wether or not a call to free(2) or realloc(2) contains a valid
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
// - size_t debug_mem_size(void)
//		Returns the total ammount of allocated memory in bytes
//		(the sum all blocks shown by `list_allocations_on').
// - long debug_memblks(void)
//		Returns the total number of allocated memory blocks
//		(Those that are shown by `list_allocations_on').
// - ostream& operator<<(ostream& o, debugmalloc_report_ct)
//		Allows to write a memory allocation report to ostream `o'.
// - void list_allocations_on(debug_ct& debug_object);
//		Prints out all visible allocated memory blocks and labels.
//
// The current 'manipulator' functions are:
//
// - void debug_move_outside(debugmalloc_marker_ct* marker, void const* ptr)
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

class dm_alloc_ct;
class memblk_key_ct;
class memblk_info_ct;

using namespace libcw;

//=============================================================================
//
// static variables
//

static bool internal = false;
  // Set if allocating/freeing `memblk_ct' objects for internal use.
  // The initialisation value must be `false' because the very first
  // call to `new' should indeed be incorporated.  Note that this
  // first call we first need to do some initialisation ourselfs.

static dm_alloc_ct* base_alloc_list = NULL;
  // The base list with `dm_alloc_ct' objects.
  // Each of these objects has a list of it's own.

static enum deallocated_from_nt { from_free, from_delete, from_delete_array, error } deallocated_from = from_free;
  // Indicates if 'debugfree()' was called directly or via 'operator delete()' or 'operator delete[]()'.

static deallocated_from_nt expected_from[] = {
  from_delete,
  error,
  from_delete_array,
  error,
  from_free,
  from_free,
  error,
  error,
  error,
  from_delete,
  error
};

ostream& operator<<(ostream& os, memblk_types_ct memblk_type)
{
#ifdef DEBUG
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
      case memblk_type_marker:
	os << "memblk_type_marker";
	break;
      case memblk_type_deleted_marker:
	os << "memblk_type_deleted_marker";
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
    case memblk_type_marker:
      os << "(MARKER)  ";
      break;
    case memblk_type_deleted:
    case memblk_type_deleted_marker:
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
  }
  return os;
}

#ifdef DEBUGUSEBFD
alloc_ct::alloc_ct(void const* s, size_t sz, memblk_types_nt type, type_info_ct const& ti, void* call_addr) :
    a_start(s), a_size(sz), a_memblk_type(type), type_info_ptr(&ti), a_description(NULL)
{
  location = libcw_bfd_pc_location(call_addr); // FIXME
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

class dm_alloc_ct : public ::libcw::alloc_ct {
friend class debugmalloc_marker_ct;
friend void debug_move_outside(debugmalloc_marker_ct* marker, void const* ptr);
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
#ifdef DEBUGUSEBFD
  dm_alloc_ct(void const* s, size_t sz, memblk_types_nt f, void* call_addr) :
      alloc_ct(s, sz , f, unknown_type_info, call_addr),
#else
  dm_alloc_ct(void const* s, size_t sz, memblk_types_nt f) :
      alloc_ct(s, sz , f, unknown_type_info),
#endif
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
  dm_alloc_ct(dm_alloc_ct const& __dummy) : alloc_ct(__dummy) { DoutFatal( dc::fatal, "Calling dm_alloc_ct::dm_alloc_ct(dm_alloc_ct const&)" ); }
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
#ifdef DEBUG
  void print_description(void) const;
  void printOn(ostream& os) const;
  friend inline ostream& operator<<(ostream& os, dm_alloc_ct const& alloc) { alloc.printOn(os); return os; }
  void show_alloc_list(int depth, NAMESPACE_LIBCW_DEBUG::channel_ct const& channel) const;
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
  bool operator==(memblk_key_ct b) const { Dout( dc::warning, "Calling memblk_key_ct::operator==(" << *this << ", " << b << ")" ); return a_start == b.start(); }
#ifdef DEBUG
  void printOn(ostream& os) const;
  friend inline ostream& operator<<(ostream& os, memblk_key_ct const& memblk) { memblk.printOn(os); return os; }
#endif
#ifdef DEBUGDEBUG
  static bool selftest(void);
    // Returns true is the self test FAILED.
#endif
  memblk_key_ct(void) { Dout( dc::core, "Huh?" ); }
    // Never use this.  It's needed for the implementation of the pair<>.
};

class memblk_info_ct {
friend class dm_alloc_ct;
friend class debugmalloc_marker_ct;
friend void debug_move_outside(debugmalloc_marker_ct* marker, void const* ptr);
private:
  lockable_auto_ptr<dm_alloc_ct> a_alloc_node;
    // The associated `dm_alloc_ct' object.
    // This must be a pointer because the allocated `dm_alloc_ct' can persist
    // after this memblk_info_ct is deleted (dm_alloc_ct marked `is_deleted'),
    // when it still has allocated 'childeren' in `a_next_list' of its own.
public:
#ifdef DEBUGUSEBFD
  memblk_info_ct(void const* s, size_t sz, memblk_types_nt f, void* call_addr);
#else
  memblk_info_ct(void const* s, size_t sz, memblk_types_nt f);
#endif
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
  void change_flags(memblk_types_nt new_flag) const
      { if (has_alloc_node()) a_alloc_node.get()->change_flags(new_flag); }
  void new_list(void) const { a_alloc_node.get()->new_list(); }
  memblk_types_nt flags(void) const { return a_alloc_node.get()->memblk_type(); }
#ifdef DEBUG
  void print_description(void) const { a_alloc_node.get()->print_description(); }
  void printOn(ostream& os) const;
  friend inline ostream& operator<<(ostream& os, memblk_info_ct const& memblk) { memblk.printOn(os); return os; }
#endif
private:
  memblk_info_ct(void) { Dout( dc::core, "huh?" ); }
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

class memblk_map_initializer_ct {
public:
  memblk_map_initializer_ct(void) { init_debugmalloc(); }
};

static memblk_map_initializer_ct memblk_map_initializer;


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
      a_memblk_type == memblk_type_deleted_marker ||
      a_memblk_type == memblk_type_freed ||
      a_memblk_type == memblk_type_removed);
}

dm_alloc_ct::~dm_alloc_ct()
{
#ifdef DEBUGDEBUG
  if (a_next_list)
    DoutFatal( dc::core, "Removing an dm_alloc_ct that still has an own list" );
  dm_alloc_ct* tmp;
  for(tmp = *my_list; tmp && tmp != this; tmp = tmp->next);
  if (!tmp)
    DoutFatal( dc::core, "Removing an dm_alloc_ct that is not part of its own list" );
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

#ifdef DEBUG

#ifdef DEBUGUSEBFD
extern "C" {
  char* cplus_demangle (char const* mangled, int options);
}
#define DMGL_PARAMS 1
#define DMGL_ANSI 2
#endif

void dm_alloc_ct::print_description(void) const
{
#ifdef DEBUGUSEBFD
  if (location.file)
  {
    char const* filename = strrchr(location.file, '/');
    if (!filename)
      filename = location.file;
    else
      ++filename;
    Dout( dc::continued, setw(20) << filename << ':' << setw(5) << setiosflags(ios::left) << location.line );
  }
  else if (location.func)
  {
    char* f = cplus_demangle(location.func, DMGL_PARAMS|DMGL_ANSI);
    Dout( dc::continued, setw(24) << f << ' ' );
    free(f);
  }
  else
    Dout( dc::continued, setw(25) << ' ' );
#endif

  if (a_memblk_type == memblk_type_marker || a_memblk_type == memblk_type_deleted_marker)
    Dout( dc::continued, "<marker>;" );
  else
  {
    set_alloc_checking_off();	/* for `buf' */
    char const* a_type = type_info_ptr->demangled_name();
    size_t s = strlen(a_type);
    ostrstream* buf = new ostrstream;
    if (s > 0 && a_type[s - 1] == '*' && type_info_ptr->ref_size() != 0)
    {
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
#if 0
      else if (a_memblk_type == memblk_type_malloc || a_memblk_type == memblk_type_realloc || a_memblk_type == memblk_type_freed)
      {
	buf->write(a_type, s - 1);
        *buf << "[]";
      }
#endif
      *buf << ends;
      Dout( dc::continued, buf->str() );
      buf->freeze(0);
    }
    else
      Dout( dc::continued, a_type );
    delete buf;
    set_alloc_checking_on();	/* for `buf' */

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

void dm_alloc_ct::show_alloc_list(int depth, const NAMESPACE_LIBCW_DEBUG::channel_ct& channel) const
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

#endif // DEBUG


//=============================================================================
//
// memblk_ct methods
//

#ifdef DEBUGUSEBFD
inline memblk_info_ct::memblk_info_ct(void const* s, size_t sz, memblk_types_nt f, void* call_addr) :
    a_alloc_node(new dm_alloc_ct(s, sz, f, call_addr)) {}
#else
inline memblk_info_ct::memblk_info_ct(void const* s, size_t sz, memblk_types_nt f) :
    a_alloc_node(new dm_alloc_ct(s, sz, f)) {}
#endif

#ifdef DEBUG

void memblk_key_ct::printOn(ostream& os) const
{
  os << "{ a_start = " << a_start << ", a_end = " << a_end << " (size = " << size() << ") }";
}

void memblk_info_ct::printOn(ostream& os) const
{
  os << "{ alloc_node = { owner = " << a_alloc_node.is_owner() << ", locked = " << a_alloc_node.strict_owner()
      << ", px = " << a_alloc_node.get() << "\n\t( = " << *a_alloc_node.get() << " ) }";
}
#endif // DEBUG

void memblk_info_ct::erase(void)
{
  dm_alloc_ct* ap = a_alloc_node.get();
#ifdef DEBUGDEBUGMALLOC
  if (ap)
    cerr << "DEBUGDEBUGMALLOC: memblk_info_ct::erase for " << ap->start() << '\n';
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
	new_flag = memblk_type_freed;
	break;
      case memblk_type_noheap:
	new_flag = memblk_type_removed;
	break;
      case memblk_type_marker:
	new_flag = memblk_type_deleted_marker;
	break;
      case memblk_type_deleted:
      case memblk_type_deleted_array:
      case memblk_type_freed:
      case memblk_type_removed:
      case memblk_type_deleted_marker:
	DoutFatal( dc::core, "Deleting a memblk_info_ct twice ?" );
    }
    ap->change_flags(new_flag);
  }
}


//=============================================================================
//
// malloc(2), calloc(2) and `new' replacements:
//

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

#  ifdef NEED_WORD_ALIGNMENT
#define SIZE_PLUS_TWELVE(s) ((((s) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1)) + 3 * sizeof(size_t))
#define SIZE_PLUS_FOUR(s) ((((s) + sizeof(size_t)) & ~(sizeof(size_t) - 1)) + sizeof(size_t))
#  else // !NEED_WORD_ALIGNMENT
#define SIZE_PLUS_TWELVE(s) ((s) + 3 * sizeof(size_t))
#define SIZE_PLUS_FOUR(s) ((s) + sizeof(size_t))
#  endif

#endif // DEBUGMAGICMALLOC

#undef CALL_ADDRESS
#ifdef DEBUGUSEBFD
// The address we called from is a bit less then the address we will return to:
#define CALL_ADDRESS , reinterpret_cast<char*>(__builtin_return_address(0)) - 1
static void* internal_debugmalloc(size_t size, memblk_types_nt flag, void* call_addr);
#else
#define CALL_ADDRESS
static void* internal_debugmalloc(size_t size, memblk_types_nt flag);
#endif

void* debugmalloc(size_t size)
{
#if defined(DEBUGDEBUGMALLOC) && defined(DEBUGDEBUG)
  if (internal)
    cerr << "DEBUGDEBUGMALLOC: Internal: Entering `debugmalloc(" << size << ")'" << endl;
#endif
  if (internal)
#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return malloc(size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  {
#  ifndef DEBUGMAGICMALLOC
    void* ptr = malloc(size);
#    ifdef DEBUGDEBUG
    internal = false;
    Dout( dc::debugmalloc|cerr_cf, "Internal malloc(" << size << ") = " << ptr );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `debugmalloc': " << ptr << endl;
#      endif
    internal = true;
#    endif
    return ptr;
#  else // DEBUGMAGICMALLOC
    void* ptr = malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      return NULL;
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
#    ifdef DEBUGDEBUG
    internal = false;
    Dout( dc::debugmalloc|cerr_cf, "Internal malloc(" << size << ") = " << static_cast<size_t*>(ptr) + 2 );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `debugmalloc': " << static_cast<size_t*>(ptr) + 2 << endl;
#      endif
    internal = true;
#    endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr) + 2;
#  endif // DEBUGMAGICMALLOC
  }
#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
#ifdef DEBUG
  internal = false;	// Reset before doing Dout()
  Dout( dc::debugmalloc|continued_cf, "malloc(" << size << ") = " );
#endif
  internal = true;
  void* ptr = internal_debugmalloc(size, memblk_type_malloc CALL_ADDRESS);
#ifdef DEBUGMAGICMALLOC
  if (ptr)
  {
    ((size_t*)ptr)[-2] = MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
  }
#endif
  internal = false;
  return ptr;
}

void* debugcalloc(size_t nmemb, size_t size)
{
#if defined(DEBUGDEBUGMALLOC) && defined(DEBUGDEBUG)
  if (internal)
    cerr << "DEBUGDEBUGMALLOC: Internal: Entering `debugcalloc(" << nmemb << ", " << size << ")'" << endl;
#endif
  if (internal)
#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return calloc(nmemb, size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  {
#  ifndef DEBUGMAGICMALLOC
    void* ptr = calloc(nmemb, size);
#    ifdef DEBUGDEBUG
    internal = false;
    Dout( dc::debugmalloc|cerr_cf, "Internal debugcalloc(" << nmemb << ", " << size << ") = " << ptr );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `debugcalloc': " << ptr << endl;
#      endif
    internal = true;
#    endif
    return ptr;
#  else // DEBUGMAGICMALLOC
    void* ptr = malloc(SIZE_PLUS_TWELVE(nmemb * size));
    if (!ptr)
      return NULL;
    memset(static_cast<void*>(static_cast<size_t*>(ptr) + 2), 0, nmemb * size);
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_MALLOC_BEGIN;
    ((size_t*)ptr)[1] = nmemb * size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(nmemb * size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
#    ifdef DEBUGDEBUG
    internal = false;
    Dout( dc::debugmalloc|cerr_cf, "Internal calloc(" << nmemb << ", " << size << ") = " << static_cast<size_t*>(ptr) + 2 );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `debugcalloc': " << static_cast<size_t*>(ptr) + 2 << endl;
#      endif
    internal = true;
#    endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr) + 2;
#  endif // DEBUGMAGICMALLOC
  }
#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
#ifdef DEBUG
  internal =false;	// Reset before doing Dout()
  Dout( dc::debugmalloc|continued_cf, "calloc(" << nmemb << ", " << size << ") = " );
#endif
  internal = true;
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
  internal = false;
  return ptr;
}

void* operator new(size_t size)
{
#if defined(DEBUGDEBUGMALLOC) && defined(DEBUGDEBUG)
  if (internal)
    cerr << "DEBUGDEBUGMALLOC: Internal: Entering `operator new', size = " << size << endl;
#endif
  if (internal)
#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return malloc(size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  {
#  ifndef DEBUGMAGICMALLOC
    void* ptr = malloc(size);
    if (!ptr)
      DoutFatal( dc::core, "Out of memory in `operator new'" );
#    ifdef DEBUGDEBUG
    internal = false;
    Dout( dc::debugmalloc|cerr_cf, "Internal operator new(" << size << ") = " << ptr );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `operator new': " << ptr << endl;
#      endif
    internal = true;
#    endif // DEBUGDEBUG
    return ptr;
#  else // DEBUGMAGICMALLOC
    void* ptr = malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      DoutFatal( dc::core, "Out of memory in `operator new'" );
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_NEW_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_NEW_END;
#    ifdef DEBUGDEBUG
    internal = false;
    Dout( dc::debugmalloc|cerr_cf, "Internal operator new(" << size << ") = " << static_cast<size_t*>(ptr) + 2 );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `operator new': " << static_cast<size_t*>(ptr) + 2 << endl;
#      endif
    internal = true;
#    endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr) + 2;
#  endif // DEBUGMAGICMALLOC
  }
#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
#ifdef DEBUG
  internal = false;	// Reset before doing Dout()
  Dout( dc::debugmalloc|continued_cf, "operator new (size = " << size << ") = " );
#endif
  internal = true;
  void* ptr = internal_debugmalloc(size, memblk_type_new CALL_ADDRESS);
  if (!ptr)
    DoutFatal( dc::core, "Out of memory in `operator new'" );
#ifdef DEBUGMAGICMALLOC
  else
  {
    ((size_t*)ptr)[-2] = MAGIC_NEW_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_NEW_END;
  }
#endif
  internal = false;
  return ptr;
}

void* operator new[](size_t size)
{
#if defined(DEBUGDEBUGMALLOC) && defined(DEBUGDEBUG)
  if (internal)
    cerr << "DEBUGDEBUGMALLOC: Internal: Entering `operator new[]', size = " << size << endl;
#endif
  if (internal)
#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return malloc(size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  {
#  ifndef DEBUGMAGICMALLOC
    void* ptr = malloc(size);
    if (!ptr)
      DoutFatal( dc::core, "Out of memory in `operator new[]'" );
#    ifdef DEBUGDEBUG
    internal = false;
    Dout( dc::debugmalloc|cerr_cf, "Internal operator new[](" << size << ") = " << ptr );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `operator new[]': " << ptr << endl;
#      endif
    internal = true;
#    endif // DEBUGDEBUG
    return ptr;
#  else // DEBUGMAGICMALLOC
    void* ptr = malloc(SIZE_PLUS_TWELVE(size));
    if (!ptr)
      DoutFatal( dc::core, "Out of memory in `operator new[]'" );
    ((size_t*)ptr)[0] = INTERNAL_MAGIC_NEW_ARRAY_BEGIN;
    ((size_t*)ptr)[1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_NEW_ARRAY_END;
#    ifdef DEBUGDEBUG
    internal = false;
    Dout( dc::debugmalloc|cerr_cf, "Internal operator new[](" << size << ") = " << static_cast<size_t*>(ptr) + 2 );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `operator new[]': " << static_cast<size_t*>(ptr) + 2 << endl;
#      endif
    internal = true;
#    endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr) + 2;
#  endif // DEBUGMAGICMALLOC
  }
#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
#ifdef DEBUG
  internal = false;	// Reset before doing Dout()
  Dout( dc::debugmalloc|continued_cf, "operator new[] (size = " << size << ") = " );
#endif
  internal = true;
  void* ptr = internal_debugmalloc(size, memblk_type_new_array CALL_ADDRESS);
  if (!ptr)
    DoutFatal( dc::core, "Out of memory in `operator new[]'" );
#ifdef DEBUGMAGICMALLOC
  else
  {
    ((size_t*)ptr)[-2] = MAGIC_NEW_ARRAY_BEGIN;
    ((size_t*)ptr)[-1] = size;
    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_NEW_ARRAY_END;
  }
#endif
  internal = false;
  return ptr;
}

//=============================================================================
//
// `delete' replacement:
//

void debugfree(void* ptr);

void operator delete(void* ptr)
{
  deallocated_from = from_delete;
  debugfree(ptr);
}

void operator delete[](void* ptr)
{
  deallocated_from = from_delete_array;
  debugfree(ptr);			// Note that the standard demands that we call free(), and not delete().
  					// This forces everyone to overload both, operator delete() and operator delete[]()
					// and not only operator delete().
}

//=============================================================================
//
// internal_debugmalloc
//
// Allocs a new block of size `size' and updates the internal administration.
// `internal' is already set.
//
// Note: This function is called by `debugmalloc', `debugcalloc' and
// `operator new' which end with a call to Dout( dc::debugmalloc|continued_cf, ...)
// and should therefore end with a call to Dout( dc::finish, ptr ).
//

#ifdef DEBUGUSEBFD
static void* internal_debugmalloc(size_t size, memblk_types_nt flag, void* call_addr)
#else
static void* internal_debugmalloc(size_t size, memblk_types_nt flag)
#endif
{
  register void* mptr;
#ifndef DEBUGMAGICMALLOC
  if (!(mptr = malloc(size)))
#else
  if ((mptr = static_cast<char*>(malloc(SIZE_PLUS_TWELVE(size))) + 2 * sizeof(size_t)) == (void*)(2 * sizeof(size_t)))
#endif
  {
    internal = false;			// Reset before doing Dout()
    Dout( dc::finish, "NULL" );
    Dout( dc::debugmalloc, "Out of memory ! this is only a pre-detection!" );
    return NULL;	// A fatal error should occur directly after this
  }

  // Update our administration:
  pair<memblk_map_ct::iterator, bool> const& i(memblk_map->insert(
      memblk_ct(memblk_key_ct(mptr, size),
      memblk_info_ct(mptr, size, flag
#ifdef DEBUGUSEBFD
      , call_addr
#endif
      ))));
  internal = false;			// Reset before doing Dout()
  if (!i.second)
    DoutFatal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );
  (*i.first).second.lock();		// Lock ownership.

  Dout( dc::finish, (void*)(mptr) );
  return mptr;
}

void init_debugmalloc(void)
{
  if (!memblk_map)
  {
    set_alloc_checking_off();
    memblk_map = new memblk_map_ct;
    set_alloc_checking_on();
  }
}

//=============================================================================
//
// debugfree, replacement for free(2)
//
// frees a block and updates the internal administration.
//

void debugfree(void* ptr)
{
  deallocated_from_nt from = deallocated_from;
  deallocated_from = from_free;
  if (internal)
  {
#ifdef DEBUGDEBUG
    internal = false;			// Reset before doing Dout()
    if (from == from_delete)
    {
      Dout( dc::debugmalloc|cerr_cf, "Internal delete(" << ptr << ')' );
#  ifdef DEBUGDEBUGMALLOC
      cerr << "DEBUGDEBUGMALLOC: Internal `delete(" << ptr << ")'" << endl;
#  endif
    }
    else if (from == from_delete_array)
    {
      Dout( dc::debugmalloc|cerr_cf, "Internal delete[](" << ptr << ')' );
#  ifdef DEBUGDEBUGMALLOC
      cerr << "DEBUGDEBUGMALLOC: Internal `delete[](" << ptr << ")'" << endl;
#  endif
    }
    else
    {
      Dout( dc::debugmalloc|cerr_cf, "Internal free(" << ptr << ')' );
#  ifdef DEBUGDEBUGMALLOC
      cerr << "DEBUGDEBUGMALLOC: Internal `free(" << ptr << ")'" << endl;
#  endif
    }
    internal = true;		// Restore
#endif // DEBUGDEBUG
#ifdef DEBUGMAGICMALLOC
    if (!ptr)
      return;
    ptr = static_cast<size_t*>(ptr) - 2;
    if (from == from_delete)
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_NEW_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_NEW_END)
      {
        internal = false;	// Reset before doing Dout()
        DoutFatal( dc::core, "internal delete: magic number corrupt!" );
      }
    }
    else if (from == from_delete_array)
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_NEW_ARRAY_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_NEW_ARRAY_END)
      {
        internal = false;	// Reset before doing Dout()
        DoutFatal( dc::core, "internal delete[]: magic number corrupt!" );
      }
    }
    else
    {
      if (((size_t*)ptr)[0] != INTERNAL_MAGIC_MALLOC_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_MALLOC_END)
      {
        internal = false;	// Reset before doing Dout()
        DoutFatal( dc::core, "internal free: magic number corrupt!" );
      }
    }
#endif // DEBUGMAGICMALLOC
    free(ptr);
    return;
  }
  if (!ptr)
  {
    Dout( dc::debugmalloc, "Trying to free NULL - ignored." );
    return;
  }
  internal = true;

  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));

  if (i == memblk_map->end() || (*i).first.start() != ptr)
  {
    internal = false;		// Reset before doing Dout()
    DoutFatal( dc::core, "Trying to "
	<< ((from == from_delete) ? "delete" : ((from == from_free) ? "free" : "delete[]")) << " an invalid pointer (" << ptr << ')' );
  }
  else
  {
    bool visible = (*i).second.has_alloc_node();
    if (visible)
    {
#ifdef DEBUG
      internal = false;		// Reset before doing Dout()
      Dout( dc::debugmalloc|continued_cf,
          ((from == from_free) ? "free(" : ((from == from_delete) ? "delete " : "delete[] "))
	  << ptr << ((from == from_free) ? ") " : " ") );
      (*i).second.print_description();
      Dout( dc::continued, ' ' );
      internal = true;		// Restore
#endif // DEBUG
      if (expected_from[(*i).second.flags()] != from)
      {
        memblk_types_nt f = (*i).second.flags();
	if (from == from_delete)
	{
	  if (f == memblk_type_malloc)
	    DoutFatal( dc::core, "You are `delete'-ing a block that was allocated with `malloc()' ! Use `free()' instead." );
	  else if (f == memblk_type_realloc)
	    DoutFatal( dc::core, "You are `delete'-ing a block that was returned by `realloc()' ! Use `free()' instead." );
	  else if (f == memblk_type_new_array)
	    DoutFatal( dc::core, "You are `delete'-ing a block that was allocated with `new[]' ! Use `delete[]' instead." );
        }
	else if (from == from_delete)
	{
	  if (f == memblk_type_malloc)
	    DoutFatal( dc::core, "You are `delete[]'-ing a block that was allocated with `malloc()' ! Use `free()' instead." );
	  else if (f == memblk_type_realloc)
	    DoutFatal( dc::core, "You are `delete[]'-ing a block that was returned by `realloc()' ! Use `free()' instead." );
          else if (f == memblk_type_new)
	    DoutFatal( dc::core, "You are `delete[]'-ing a block that was allocated with `new' ! Use `delete' instead." );
        }
	else if (from == from_free)
	{
	  if (f == memblk_type_new)
	    DoutFatal( dc::core, "You are `free()'-ing a block that was allocated with `new'. Use `delete' instead." );
	  else if (f == memblk_type_new_array)
	    DoutFatal( dc::core, "You are `free()'-ing a block that was allocated with `new[]'. Use `delete[]' instead." );
	}
	DoutFatal( dc::core, "Huh? Bug in libcw" );
      }
    }

    (*i).second.erase();	// Update flags and optional decouple
    memblk_map->erase(i);	// Update administration
#ifndef DEBUGMAGICMALLOC
    free(ptr);			// Free memory block
#else // DEBUGMAGICMALLOC
    if (from == from_delete)
    {
      if (((size_t*)ptr)[-2] != MAGIC_NEW_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_NEW_END)
      {
        internal = false;	// Reset before doing Dout()
	if (((size_t*)ptr)[-2] == MAGIC_MALLOC_BEGIN &&
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_MALLOC_END)
	  DoutFatal( dc::core, "You are `delete'-ing a block that was allocated with `malloc()' ! Use `free()' instead." );
	if (((size_t*)ptr)[-2] == MAGIC_NEW_ARRAY_BEGIN &&
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_ARRAY_END)
	  DoutFatal( dc::core, "You are `delete'-ing a block that was allocated with `new[]' ! Use `delete[]' instead." );
        DoutFatal( dc::core, "delete: magic number corrupt!" );
      }
    }
    else if (from == from_delete_array)
    {
      if (((size_t*)ptr)[-2] != MAGIC_NEW_ARRAY_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_NEW_ARRAY_END)
      {
        internal = false;	// Reset before doing Dout()
	if (((size_t*)ptr)[-2] == MAGIC_MALLOC_BEGIN &&
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_MALLOC_END)
	  DoutFatal( dc::core, "You are `delete[]'-ing a block that was allocated with `malloc()' ! Use `free()' instead." );
	if (((size_t*)ptr)[-2] == MAGIC_NEW_BEGIN &&
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_END)
	  DoutFatal( dc::core, "You are `delete[]'-ing a block that was allocated with `new' ! Use `delete' instead." );
        DoutFatal( dc::core, "delete[]: magic number corrupt!" );
      }
    }
    else
    {
      if (((size_t*)ptr)[-2] != MAGIC_MALLOC_BEGIN ||
          ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_MALLOC_END)
      {
        internal = false;	// Reset before doing Dout()
	if (((size_t*)ptr)[-2] == MAGIC_NEW_BEGIN &&
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_END)
          DoutFatal( dc::core, "You are `free()'-ing a block that was allocated with `new'. Use `delete' instead." );
	if (((size_t*)ptr)[-2] == MAGIC_NEW_ARRAY_BEGIN &&
	    ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_ARRAY_END)
          DoutFatal( dc::core, "You are `free()'-ing a block that was allocated with `new[]'. Use `delete[]' instead." );
        DoutFatal( dc::core, "free: magic number corrupt!" );
      }
    }
    free(static_cast<size_t*>(ptr) - 2);		// Free memory block
#endif // DEBUGMAGICMALLOC

#ifdef DEBUG
    if (visible)
    {
      internal = false;			// Reset before doing Dout()
#if 0 // Leaks are now detected in libcw_exit()
      if (dm_alloc_ct::get_memblks() == 0)
	Dout( dc::finish, "(No blocks left)" );
      else
      {
        if (dm_alloc_ct::get_memblks() == 1)
	  Dout( dc::finish, "(1 block left)" );
	else if (dm_alloc_ct::get_memblks() < 10) // To catch small leaks
	  Dout( dc::finish, '(' << dm_alloc_ct::get_memblks() << " blocks left)" );
	Debug( list_allocations_on(libcw_do) );
      }
      else
#endif
      Dout( dc::finish, "" );
    }
#endif // DEBUG

  }
  internal = false;
}


//=============================================================================
//
// debugrealloc, replacement for realloc(2)
//
// reallocates a block and updates the internal administration.
//

void* debugrealloc(void* ptr, size_t size)
{
#if defined(DEBUGDEBUGMALLOC) && defined(DEBUGDEBUG)
  if (internal)
    cerr << "DEBUGDEBUGMALLOC: Internal: Entering `debugrealloc(" << ptr << ", " << size << ")'" << endl;
#endif
  if (internal)
#if !defined(DEBUGDEBUG) && !defined(DEBUGMAGICMALLOC)
    return realloc(ptr, size);
#else // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)
  {
#  ifndef DEBUGMAGICMALLOC
    void* ptr1 = realloc(ptr, size);
#    ifdef DEBUGDEBUG
    internal = false;	// Reset before doing Dout()
    Dout( dc::debugmalloc|cerr_cf, "Internal realloc(" << ptr << ", " << size << ") = " << ptr1 );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `debugrealloc': " << ptr1 << endl;
#      endif
    internal = true;	// Restore
#    endif // DEBUGDEBUG
    return ptr1;
#  else // DEBUGMAGICMALLOC
    ptr = static_cast<size_t*>(ptr) - 2;
    if (((size_t*)ptr)[0] != INTERNAL_MAGIC_MALLOC_BEGIN ||
	((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_TWELVE(((size_t*)ptr)[1])))[-1] != INTERNAL_MAGIC_MALLOC_END)
    {
      internal = false;
      DoutFatal( dc::core, "internal realloc: magic number corrupt!" );
    }
    void* ptr1 = realloc(ptr, SIZE_PLUS_TWELVE(size));
    ((size_t*)ptr1)[1] = size;
    ((size_t*)(static_cast<char*>(ptr1) + SIZE_PLUS_TWELVE(size)))[-1] = INTERNAL_MAGIC_MALLOC_END;
#    ifdef DEBUGDEBUG
    internal = false;	// Reset before doing Dout()
    Dout( dc::debugmalloc|cerr_cf, "Internal realloc(" << static_cast<size_t*>(ptr) + 2 << ", " << size << ") = " << static_cast<size_t*>(ptr1) + 2 );
#      ifdef DEBUGDEBUGMALLOC
    cerr << "DEBUGDEBUGMALLOC: Internal: Leaving `debugrealloc': " << static_cast<size_t*>(ptr1) + 2 << endl;
#      endif
    internal = true;	// Restore
#    endif // DEBUGDEBUG
    return static_cast<size_t*>(ptr1) + 2;
#  endif // DEBUGMAGICMALLOC
  }
#endif // defined(DEBUGDEBUG) || defined(DEBUGMAGICMALLOC)

#ifdef DEBUG
  internal = false;	// Reset before doing Dout()
  Dout( dc::debugmalloc|continued_cf, "realloc(" << ptr << ", " << size << ") = " );
#endif
  internal = true;

  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));

  if (i == memblk_map->end() || (*i).first.start() != ptr)
  {
    internal = false;	// Reset before doing Dout()
    Dout( dc::finish, "" );
    DoutFatal( dc::core, "Trying to realloc() an invalid pointer" );
  }

  register void* mptr;

#ifndef DEBUGMAGICMALLOC
  if (!(mptr = realloc(ptr, size)))
#else
  if (((size_t*)ptr)[-2] != MAGIC_MALLOC_BEGIN ||
      ((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] != MAGIC_MALLOC_END)
  {
    internal = false;	// Reset before doing Dout()
    if (((size_t*)ptr)[-2] == MAGIC_NEW_BEGIN &&
	((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_END)
      DoutFatal( dc::core, "You can't realloc() a block that was allocated with `new'!" );
    if (((size_t*)ptr)[-2] == MAGIC_NEW_ARRAY_BEGIN &&
	((size_t*)(static_cast<char*>(ptr) + SIZE_PLUS_FOUR(((size_t*)ptr)[-1])))[-1] == MAGIC_NEW_ARRAY_END)
      DoutFatal( dc::core, "You can't realloc() a block that was allocated with `new[]'!" );
    DoutFatal( dc::core, "realloc: magic number corrupt!" );
  }
  if ((mptr = static_cast<char*>(realloc(static_cast<size_t*>(ptr) - 2, SIZE_PLUS_TWELVE(size))) + 2 * sizeof(size_t)) == (void*)(2 * sizeof(size_t)))
#endif
  {
    internal = false;
    Dout( dc::finish, "NULL" );
    Dout( dc::debugmalloc, "Out of memory! This is only a pre-detection!" );
    return NULL; // A fatal error should occur directly after this
  }
#ifdef DEBUGMAGICMALLOC
  ((size_t*)mptr)[-1] = size;
  ((size_t*)(static_cast<char*>(mptr) + SIZE_PLUS_FOUR(size)))[-1] = MAGIC_MALLOC_END;
#endif

  // Update administration
  // Note that the way this is done assumes that memory blocks allocated
  // with malloc, calloc or realloc do NOT have a next_list ! Only
  // memory blocks allocated with `new' can have a next_list.
  type_info_ct const* t = (*i).second.typeid_ptr();
  char const* d = (*i).second.description();
  memblk_map->erase(i);
  pair<memblk_map_ct::iterator, bool> const&
      j(memblk_map->insert(memblk_ct(memblk_key_ct(mptr, size),
      memblk_info_ct(mptr, size, memblk_type_realloc CALL_ADDRESS))));
  if (!j.second)
  {
    internal = false;	// Reset before doing Dout()
    DoutFatal( dc::core, "memblk_map corrupt: Newly allocated block collides with existing memblk!" );
  }
  (*(j.first)).second.change_label(*t, d);

  internal = false;

  Dout( dc::finish, (void*)(mptr) );
  return mptr;
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

size_t debug_mem_size(void)
{
  return dm_alloc_ct::get_mem_size();
}

long debug_memblks(void)
{
  return dm_alloc_ct::get_memblks();
}

#ifdef DEBUG

ostream& operator<<(ostream& o, debugmalloc_report_ct)
{
  size_t mem_size = dm_alloc_ct::get_mem_size();
  unsigned long memblks = dm_alloc_ct::get_memblks();
  o << "Allocated memory: " << mem_size << " bytes in " << memblks << " blocks.";
  return o;
}

#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif

    void list_allocations_on(debug_ct& debug_object)
    {
#if 0
      // Print out the entire `map':
      LibcwDout( NAMESPACE_LIBCW_DEBUG::channels, debug_object, dc::debugmalloc, "map:" );
      int cnt = 0;
      for(memblk_map_ct::const_iterator i(memblk_map->begin()); i != memblk_map->end(); ++i)
	LibcwDout( NAMESPACE_LIBCW_DEBUG::channels, debug_object, dc::debugmalloc|nolabel_cf, << ++cnt << ":\t(*i).first = " << (*i).first << '\n' << "\t(*i).second = " << (*i).second );
#endif

      LibcwDout( NAMESPACE_LIBCW_DEBUG::channels, debug_object, dc::debugmalloc, "Allocated memory: " << dm_alloc_ct::get_mem_size() << " bytes in " << dm_alloc_ct::get_memblks() << " blocks." );
      if (base_alloc_list)
	base_alloc_list->show_alloc_list(1, NAMESPACE_LIBCW_DEBUG::channels::dc::debugmalloc);
    }

#ifndef DEBUGNONAMESPACE
  };
};
#endif

// Undocumented (used from gdb while debugging libcw)
void list_allocations_on_cerr(void)
{
  Dout( dc::warning|cerr_cf, "Allocated memory: " << dm_alloc_ct::get_mem_size() << " bytes in " << dm_alloc_ct::get_memblks() << " blocks.");
  if (base_alloc_list)
    base_alloc_list->show_alloc_list(1, NAMESPACE_LIBCW_DEBUG::channels::dc::debugmalloc);
}


//=============================================================================
//
// 'Manipulator' functions
//

void make_invisible(void const* ptr)
{
#ifdef DEBUGDEBUG
  if (internal)
  {
    internal = false;
    DoutFatal( dc::core, "What do you think you're doing?" );
  }
#endif
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i == memblk_map->end() || (*i).first.start() != ptr)
    DoutFatal( dc::core, "Trying to turn non-existing memory block (" << ptr << ") into an 'internal' block" );
  internal = true;
  (*i).second.make_invisible();
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

#endif // DEBUG

static unsigned int alloc_checking_off_counter = 0;
static bool prev_internal;

void set_alloc_checking_off(void)
{
  if (alloc_checking_off_counter++ == 0)
    prev_internal = internal;
  internal = true;
}

void set_alloc_checking_on(void)
{
#ifdef DEBUG
  if (alloc_checking_off_counter == 0)
    DoutFatal( dc::core, "Calling `set_alloc_checking_on' while ALREADY on." );
#endif
  if (--alloc_checking_off_counter == 0)
    internal = prev_internal;
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

debugmalloc_newctor_ct::debugmalloc_newctor_ct(void* ptr, type_info_ct const& ti) : no_heap_alloc_node(NULL)
{
#ifdef DEBUGDEBUG
  Dout( dc::debugmalloc, "New debugmalloc_newctor_ct at " << this << " from object " << ti.name() << " (" << ptr << ")" );
#endif
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(ptr, 0)));
  if (i != memblk_map->end())
  {
    (*i).second.change_label(ti, NULL);
    (*i).second.new_list();
  }
  else
  {
    internal = true;
    no_heap_alloc_node = new dm_alloc_ct(ptr, 0, memblk_type_noheap CALL_ADDRESS);
    internal = false;
    no_heap_alloc_node->new_list();
  }
#ifdef DEBUGDEBUG
  Debug( list_allocations_on(libcw_do) );
#endif
}

debugmalloc_newctor_ct::~debugmalloc_newctor_ct(void)
{
#ifdef DEBUGDEBUG
  Dout( dc::debugmalloc, "Removing debugmalloc_newctor_ct at " << (void*)this );
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
      internal = true;
      delete no_heap_alloc_node;
      internal = false;
    }
  }
}

void debugmalloc_marker_ct::register_marker(char const* label)
{
  Dout( dc::debugmalloc, "New debugmalloc_marker_ct at " << this );
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(this, 0)));
  memblk_info_ct &info((*i).second);
  if (i == memblk_map->end() || (*i).first.start() != this || info.flags() != memblk_type_new)
    DoutFatal( dc::core, "Use 'new' for debugmalloc_marker_ct" );

  info.change_label(type_info_of(this), label);
  info.change_flags(memblk_type_marker);
  info.new_list();

#ifdef DEBUGDEBUG
  Debug( list_allocations_on(libcw_do) );
#endif
}

debugmalloc_marker_ct::~debugmalloc_marker_ct(void)
{
  memblk_map_ct::iterator const& i(memblk_map->find(memblk_key_ct(this, 0)));
  if (i == memblk_map->end() || (*i).first.start() != this)
    DoutFatal( dc::core, "Trying to delete an invalid marker" );

#ifdef DEBUG
  Dout( dc::debugmalloc, "Removing debugmalloc_marker_ct at " << this << " (" << (*i).second.description() << ')' );

  if ((*i).second.a_alloc_node.get()->next_list())
  {
    string margin = ::libcw::debug::libcw_do.get_margin();
    Debug( libcw_do.set_margin(margin + "  * ") );
    Dout( dc::warning, "Memory leak detected!" );
    (*i).second.a_alloc_node.get()->next_list()->show_alloc_list(1, NAMESPACE_LIBCW_DEBUG::channels::dc::warning);
    Debug( libcw_do.set_margin(margin) );
  }
#endif

  // Set `current_alloc_list' one list back
  if (*dm_alloc_ct::current_alloc_list != (*i).second.a_alloc_node.get()->next_list())
    DoutFatal( dc::core, "Deleting a marker must be done in the same \"scope\" as where it was allocated" );
  dm_alloc_ct::descend_current_alloc_list();
}

void debug_move_outside(debugmalloc_marker_ct* marker, void const* ptr)
{
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

alloc_ct const* find_alloc(void const* ptr)
{ 
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
	      memblk_info_ct((void*)start1, end1 - start1, memblk_type_malloc CALL_ADDRESS)) ));
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
                      memblk_info_ct((void*)start1, end1 - start1, memblk_type_malloc CALL_ADDRESS)) ));
                  if (!p1.second)
                    return true;
                  if ((*p1.first).first.start() != (void*)start1 || (*p1.first).first.end() != (void*)end1)
                    return true;
                  bool overlap = !((start1 < start2 && end1 <= start2) || (start2 < start1 && end2 <= start1));
                  pair<memblk_map_ct::iterator, bool> const& p2(map2.insert(
                      memblk_ct(memblk_key_ct((void*)start2, end2 - start2),
                      memblk_info_ct((void*)start2, end2 - start2, memblk_type_malloc CALL_ADDRESS)) ));
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

#endif /* DEBUGDEBUG */
#endif /* DEBUGMALLOC */
