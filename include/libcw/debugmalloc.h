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

#ifndef LIBCW_DEBUGMALLOC_H
#ifdef __GNUG__
#pragma interface
#endif
#define LIBCW_DEBUGMALLOC_H

RCSTAG_H(debugmalloc, "$Id$")

#ifdef DEBUGMALLOC

#include <iosfwd>
#include <libcw/iomanip.h>
#include <libcw/lockable_auto_ptr.h>
#ifdef DEBUGUSEBFD
#include <libcw/bfd.h>
#endif

// Forward declaration
class type_info_ct;

namespace libcw {

  //===========================================================================
  //
  // Flags used to mark the type of `memblk':
  //

  enum memblk_types_nt {
    memblk_type_new,              // Allocated with `new'
    memblk_type_deleted,          // Deleted with `delete'
    memblk_type_new_array,        // Allocated with `new[]'
    memblk_type_deleted_array,    // Deleted with `delete[]'
    memblk_type_malloc,           // Allocated with `malloc'
    memblk_type_realloc,          // Reallocated with `realloc'
    memblk_type_freed,            // Freed with `free'
    memblk_type_noheap,           // Allocated in data or stack segment
    memblk_type_removed,          // No heap, corresponding `debugmalloc_newctor_ct' removed
    memblk_type_marker,           // A memory allocation "marker"
    memblk_type_deleted_marker    // A deleted memory allocation "marker"
  };

#ifdef DEBUG
  // Ostream manipulator for memblk_types_ct

  class memblk_types_manipulator_data_ct {
  private:
    bool debug_channel;
    bool amo_label;
  public:
    memblk_types_manipulator_data_ct(void) : debug_channel(false), amo_label(false) { }
    void setdebug(void) { debug_channel = true; }
    void setlabel(bool what) { amo_label = what; }
    bool is_debug_channel(void) const { return debug_channel; }
    bool is_amo_label(void) const { return amo_label; }
  };
#endif

  class memblk_types_ct {
  private:
    memblk_types_nt memblk_type;
  public:
    memblk_types_ct(memblk_types_nt mbt) : memblk_type(mbt) { }
    memblk_types_nt operator()(void) const { return memblk_type; }
#ifdef DEBUG
    typedef memblk_types_manipulator_data_ct omanip_data_ct;
    __DEFINE_OMANIP0(memblk_types_ct, setdebug)
    __DEFINE_OMANIP1_OPERATOR(memblk_types_ct, bool)
    __DEFINE_OMANIP1_FUNCTION(setlabel, bool)
#endif
  };
};

extern ostream& operator<<(ostream& os, ::libcw::memblk_types_ct memblk_type);

namespace libcw {

  inline ostream& operator<<(ostream& os, memblk_types_nt memblk_type)
  {
    return os << memblk_types_ct(memblk_type);
  }

  //===========================================================================
  //
  // The class which describes allocated memory blocks.
  //

  class alloc_ct {
  protected:
    void const* a_start;        	// Duplicate of (original) memblk_key_ct
    size_t a_size;              	// Duplicate of (original) memblk_key_ct
    memblk_types_nt a_memblk_type;	// A flag which indicates the type of allocation
    type_info_ct const* type_info_ptr;	// Type info of related object
    lockable_auto_ptr<char, true> a_description;	// A label describing this memblk
#ifdef DEBUGUSEBFD
    location_st location;		// Source file, function and line number from where the allocator was called from
#endif
  public:
#ifdef DEBUGUSEBFD
    alloc_ct(void const* s, size_t sz, memblk_types_nt type, type_info_ct const& ti, void* call_addr);
#else
    alloc_ct(void const* s, size_t sz, memblk_types_nt type, type_info_ct const& ti) :
        a_start(s), a_size(sz), a_memblk_type(type), type_info_ptr(&ti) { }
#endif
    size_t size(void) const { return a_size; }
    void const* start(void) const { return a_start; }
    memblk_types_nt memblk_type(void) const { return a_memblk_type; }
    type_info_ct const& type_info(void) const { return *type_info_ptr; }
    char const* description(void) const { return a_description.get(); }
#ifdef DEBUGUSEBFD
    location_st const& get_location(void) const { return location; }
#endif
protected:
    virtual ~alloc_ct() {}
  };
};

class dm_alloc_ct;

class debugmalloc_marker_ct {
private:
  void register_marker(char const* label);
public:
  debugmalloc_marker_ct(char const* label)
  {
    register_marker(label);
  }
  debugmalloc_marker_ct(void)
  {
    register_marker("debugmalloc_marker_ct");
  }
  ~debugmalloc_marker_ct(void);
};

class debugmalloc_newctor_ct {
private:
  dm_alloc_ct* no_heap_alloc_node;
public:
  debugmalloc_newctor_ct(void* object_ptr, type_info_ct const& label);
  ~debugmalloc_newctor_ct(void);
};

class debugmalloc_report_ct { };
debugmalloc_report_ct const malloc_report = { }; // Dummy

// Accessors:
extern ::libcw::alloc_ct const* find_alloc(void const* ptr);
extern bool test_delete(void const* ptr);
extern size_t debug_mem_size(void);
extern long debug_memblks(void);
extern ostream& operator<<(ostream& o, debugmalloc_report_ct);

// Manipulators:
extern void debug_move_outside(debugmalloc_marker_ct*, void const* ptr);
extern void set_alloc_checking_off(void);
extern void set_alloc_checking_on(void);
extern void make_invisible(void const* ptr);
extern void make_all_allocations_invisible_except(void const* ptr);

// Undocumented (used inside AllocTag, AllocTag_dynamic_description, AllocTag1 and AllocTag2):
extern void set_alloc_label(void const* ptr, type_info_ct const& ti, char const* description);			// For static descriptions
extern void set_alloc_label(void const* ptr, type_info_ct const& ti, lockable_auto_ptr<char, true> description);// For descriptions
														// allocated with new[]
// Undocumented (libcw `internal' function)
extern void init_debugmalloc(void);

#  ifdef DEBUG

#    ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#    endif
    extern void list_allocations_on(debug_ct& debug_object);
#    ifndef DEBUGNONAMESPACE
  };
};
#    endif

#    define AllocTag1(p) set_alloc_label(p, type_info_of(p), (char const*)NULL)
#    define AllocTag2(p, desc) set_alloc_label(p, type_info_of(p), const_cast<char const*>(desc))
#    define AllocTag(p, x) \
	do { \
	  static char const* desc; \
	  if (!desc) { \
	    set_alloc_checking_off(); \
	    if (1) \
	    { \
	      NAMESPACE_LIBCW_DEBUG::no_alloc_checking_ostrstream buf; \
	      set_alloc_checking_on(); \
	      buf << x << ends; \
	      set_alloc_checking_off(); \
	      desc = buf.str(); /* Implicit buf.freeze(1) */ \
	    } \
	    set_alloc_checking_on(); \
	  } \
	  set_alloc_label(p, type_info_of(p), desc); /* Don't care about the memory leak though */ \
	} while(0)
#    define AllocTag_dynamic_description(p, x) \
	do { \
	  char* desc; \
	  set_alloc_checking_off(); \
	  if (1) \
	  { \
	    NAMESPACE_LIBCW_DEBUG::no_alloc_checking_ostrstream buf; \
	    set_alloc_checking_on(); \
	    buf << x << ends; \
	    set_alloc_checking_off(); \
	    desc = buf.str(); /* Implicit buf.freeze(1) */ \
	  } \
	  set_alloc_checking_on(); \
	  set_alloc_label(p, type_info_of(p), lockable_auto_ptr<char, true>(desc)); \
	} while(0)

template<typename TYPE>
inline TYPE* __libcw_allocCatcher(TYPE* new_ptr) {
  AllocTag1(new_ptr);
  return new_ptr;
};
     
#    define New(x) __libcw_allocCatcher(new x)

#include <libcw/type_info.h>

#  else // !DEBUG

#    define AllocTag(p, x)
#    define AllocTag_dynamic_description(p, x)
#    define AllocTag1(p)
#    define AllocTag2(p, desc)
#    define New(x) new x

#  endif // !DEBUG

#else // !DEBUGMALLOC

#  define AllocTag(p, x)
#  define AllocTag_dynamic_description(p, x)
#  define AllocTag1(p)
#  define AllocTag2(p, desc)
#  define New(x) new x

#endif // !DEBUGMALLOC

#ifndef DEBUGMALLOC_INTERNAL
#  ifdef DEBUGMALLOC
#    ifdef DEBUGMALLOCMYSBRK
#      define rcalloc(x, y) debugcalloc(x, y)
#      define rmalloc(x) debugmalloc(x)
#      define rfree(x) debugfree(x)
#      define rrealloc(x, y) debugrealloc(x, y)
#    else
#      define calloc(x, y) debugcalloc(x, y)
#      define malloc(x) debugmalloc(x)
#      define free(x) debugfree(x)
#      define realloc(x, y) debugrealloc(x, y)
#    endif
extern void* debugcalloc(size_t nmemb, size_t size);
extern void* debugmalloc(size_t size);
extern void debugfree(void* ptr);
extern void* debugrealloc(void* ptr, size_t size);
#  endif /* DEBUGMALLOC */
#endif /* !DEBUG_INTERNAL */

#endif // LIBCW_DEBUGMALLOC_H
