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
#define LIBCW_DEBUGMALLOC_H

RCSTAG_H(debugmalloc, "$Id$")

#ifndef LIBCW_DEBUG_H
#error "Don't include <libcw/debugmalloc.h> directly, include the appropriate \"debug.h\" instead."
#endif

#ifdef DEBUGMALLOC

#include <libcw/iomanip.h>
#include <libcw/lockable_auto_ptr.h>
#include <libcw/no_alloc_checking_stringstream.h>
#ifdef DEBUGUSEBFD
#include <libcw/bfd.h>
#endif

namespace libcw {
  namespace debug {

namespace _internal_ {
#if 0 //def DEBUGDEBUG
  extern bool ios_base_initialized; 
#endif
  extern bool internal;
  extern int library_call;

  static class Desperation { } const raw_write = { };
} // namespace _internal_

// Forward declaration
class type_info_ct;

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
#ifdef DEBUGMARKER
  memblk_type_marker,           // A memory allocation "marker"
  memblk_type_deleted_marker,   // A deleted memory allocation "marker"
#endif
  memblk_type_removed           // No heap, corresponding `debugmalloc_newctor_ct' removed
};

#ifdef CWDEBUG
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
#ifdef CWDEBUG
  typedef memblk_types_manipulator_data_ct omanip_data_ct;
  __DEFINE_OMANIP0(memblk_types_ct, setdebug)
  __DEFINE_OMANIP1_OPERATOR(memblk_types_ct, bool)
  __DEFINE_OMANIP1_FUNCTION(setlabel, bool)
#endif
};

extern std::ostream& operator<<(std::ostream& os, memblk_types_ct);

inline std::ostream& operator<<(std::ostream& os, memblk_types_nt memblk_type)
{
  return os << memblk_types_ct(memblk_type);
}

//===========================================================================
//
// The class which describes allocated memory blocks.
//

class alloc_ct {
protected:
  void const* a_start;        			// Duplicate of (original) memblk_key_ct
  size_t a_size;              			// Duplicate of (original) memblk_key_ct
  memblk_types_nt a_memblk_type;		// A flag which indicates the type of allocation
  type_info_ct const* type_info_ptr;		// Type info of related object
  lockable_auto_ptr<char, true> a_description;	// A label describing this memblk
#ifdef DEBUGUSEBFD
  location_ct M_location;			// Source file, function and line number from where the allocator was called from
#endif
public:
  alloc_ct(void const* s, size_t sz, memblk_types_nt type, type_info_ct const& ti) :
      a_start(s), a_size(sz), a_memblk_type(type), type_info_ptr(&ti) { }
  size_t size(void) const { return a_size; }
  void const* start(void) const { return a_start; }
  memblk_types_nt memblk_type(void) const { return a_memblk_type; }
  type_info_ct const& type_info(void) const { return *type_info_ptr; }
  char const* description(void) const { return a_description.get(); }
#ifdef DEBUGUSEBFD
  location_ct& location_reference(void) { return M_location; }
  location_ct const& location(void) const { return M_location; }
#endif
protected:
  virtual ~alloc_ct() {}
};

class dm_alloc_ct;

#ifdef DEBUGMARKER
class marker_ct {
private:
  void register_marker(char const* label);
public:
  marker_ct(char const* label)
  {
    register_marker(label);
  }
  marker_ct(void)
  {
    register_marker("An allocation marker");
  }
  ~marker_ct(void);
};
#endif

class debugmalloc_newctor_ct {
private:
  dm_alloc_ct* no_heap_alloc_node;
public:
  debugmalloc_newctor_ct(void* object_ptr, type_info_ct const& label);
  ~debugmalloc_newctor_ct(void);
};

class debugmalloc_report_ct {
  friend std::ostream& operator<<(std::ostream& o, debugmalloc_report_ct);
};
debugmalloc_report_ct const malloc_report = { }; // Dummy

// Accessors:
extern alloc_ct const* find_alloc(void const* ptr);
extern bool test_delete(void const* ptr);
extern size_t mem_size(void);
extern long memblks(void);

// Manipulators:
extern void set_alloc_checking_off(void);
extern void set_alloc_checking_on(void);
extern void make_invisible(void const* ptr);
extern void make_all_allocations_invisible_except(void const* ptr);

#ifdef DEBUGMARKER
extern void libcw_debug_move_outside(marker_ct*, void const* ptr);
inline void move_outside(marker_ct* marker, void const* ptr)
{
  libcw_debug_move_outside(marker, ptr);
}
#endif

// Undocumented (used inside AllocTag, AllocTag_dynamic_description, AllocTag1 and AllocTag2):
extern void set_alloc_label(void const* ptr, type_info_ct const& ti, char const* description); // For static descriptions
extern void set_alloc_label(void const* ptr,
    type_info_ct const& ti, lockable_auto_ptr<char, true> description);	   // For dynamic descriptions
																		    // allocated with new[]
// Undocumented (libcw `internal' function)
extern void init_debugmalloc(void);

  } // namespace debug
} // namespace libcw

#ifdef CWDEBUG

#define AllocTag1(p) ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), (char const*)NULL)
#define AllocTag2(p, desc) ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), const_cast<char const*>(desc))
#define AllocTag(p, x) \
    do { \
      static char* desc; \
      if (!desc) { \
	::libcw::debug::set_alloc_checking_off(); \
	if (1) \
	{ \
	  ::libcw::debug::no_alloc_checking_stringstream buf; \
	  ::libcw::debug::set_alloc_checking_on(); \
	  buf << x << ::std::ends; \
	  ::libcw::debug::set_alloc_checking_off(); \
	  size_t size = buf.rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out); \
	  desc = new char [size]; /* This is never deleted anymore */ \
	  buf.rdbuf()->sgetn(desc, size); \
	} \
	::libcw::debug::set_alloc_checking_on(); \
      } \
      ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), desc); \
    } while(0)
#define AllocTag_dynamic_description(p, x) \
    do { \
      char* desc; \
      ::libcw::debug::set_alloc_checking_off(); \
      if (1) \
      { \
	::libcw::debug::no_alloc_checking_stringstream buf; \
	::libcw::debug::set_alloc_checking_on(); \
	buf << x << ::std::ends; \
	::libcw::debug::set_alloc_checking_off(); \
	size_t size = buf.rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out); \
	desc = new char [size]; \
	buf.rdbuf()->sgetn(desc, size); \
      } \
      ::libcw::debug::set_alloc_checking_on(); \
      ::libcw::debug::set_alloc_label(p, ::libcw::debug::type_info_of(p), ::libcw::lockable_auto_ptr<char, true>(desc)); \
    } while(0)

template<typename TYPE>
inline TYPE* __libcwd_allocCatcher(TYPE* new_ptr) {
  AllocTag1(new_ptr);
  return new_ptr;
};
     
#define NEW(x) __libcwd_allocCatcher(new x)

#include <libcw/type_info.h>

#else // !CWDEBUG

#define AllocTag(p, x)
#define AllocTag_dynamic_description(p, x)
#define AllocTag1(p)
#define AllocTag2(p, desc)
#define NEW(x) new x

#endif // !CWDEBUG

#else // !DEBUGMALLOC

#define AllocTag(p, x)
#define AllocTag_dynamic_description(p, x)
#define AllocTag1(p)
#define AllocTag2(p, desc)
#define NEW(x) new x

namespace libcw {
  namespace debug {

inline void make_invisible(void const*) { }
inline void make_all_allocations_invisible_except(void const*) { }

  } // namespace debug
} // namespace libcw

#endif // !DEBUGMALLOC

#ifdef DEBUGDEBUG
extern "C" ssize_t write(int fd, const void *buf, size_t count);

namespace libcw {
  namespace debug {

inline _internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, char const* data)
{
  write(2, data, strlen(data));
  return raw_write;
}

inline _internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, void const* data)
{
  size_t dat = (size_t)data;
  write(2, "0x", 2);
  char c[11];
  char* p = &c[11];
  do
  {
    int d = (dat % 16);
    *--p = ((d < 10) ? '0' : ('a' - 10)) + d;
    dat /= 16;
  }
  while(dat > 0);
  write(2, p, &c[11] - p);
  return raw_write;
}

inline _internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, bool data)
{
  if (data)
    write(2, "true", 4);
  else
    write(2, "false", 5);
  return raw_write;
}

inline _internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, char data)
{
  char c[1];
  c[0] = data;
  write(2, c, 1);
  return raw_write;
}

inline _internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, unsigned long data)
{
  char c[11];
  char* p = &c[11];
  do
  {
    *--p = '0' + (data % 10);
    data /= 10;
  }
  while(data > 0);
  write(2, p, &c[11] - p);
  return raw_write;
}

inline _internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, long data)
{
  if (data < 0)
  {
    write(2, "-", 1);
    data = -data;
  }
  return operator<<(raw_write, (unsigned long)data);
}

inline _internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, int data)
{
  return operator<<(raw_write, (long)data);
}

inline _internal_::Desperation const& operator<<(_internal_::Desperation const& raw_write, size_t data)
{
  return operator<<(raw_write, static_cast<unsigned long>(data));
}

  }  // namespace debug
} // namespace libcw

  #define DEBUGDEBUG_CERR(x)							\
      do {									\
        write(2, "DEBUGDEBUG: ", 12);						\
	for (int i = 0; i < ::libcw::debug::_internal_::library_call; ++i) 	\
	  write(2, "    ", 4);							\
	::libcw::debug::_internal_::raw_write << x << '\n';			\
      } while(0)
#else // !DEBUGDEBUG
  #define DEBUGDEBUG_CERR(x)
#endif // !DEBUGDEBUG

#ifdef DEBUGDEBUGMALLOC
  #define DEBUGDEBUGMALLOC_CERR(x) DEBUGDEBUG_CERR(x)
#else
  #define DEBUGDEBUGMALLOC_CERR(x)
#endif

#ifdef CWDEBUG

namespace libcw {
  namespace debug {

#ifdef DEBUGMALLOC
extern void list_allocations_on(debug_ct& debug_object);
#else // !DEBUGMALLOC
inline void list_allocations_on(debug_ct&) { }
#endif // !DEBUGMALLOC

  } // namespace debug
} // namespace libcw

#endif // CWDEBUG

#ifndef DEBUGMALLOC_INTERNAL
#ifdef DEBUGMALLOC

extern void* __libcwd_calloc(size_t nmemb, size_t size);
extern void* __libcwd_malloc(size_t size);
extern void __libcwd_free(void* ptr);
extern void* __libcwd_realloc(void* ptr, size_t size);

#define calloc __libcwd_calloc
#define malloc __libcwd_malloc
#define free __libcwd_free
#define realloc __libcwd_realloc

#endif // DEBUGMALLOC
#endif // !DEBUG_INTERNAL

#endif // LIBCW_DEBUGMALLOC_H
