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

#ifndef LIBCW_TYPE_INFO_H
#define LIBCW_TYPE_INFO_H

RCSTAG_H(type_info, "$Id$")

#include <typeinfo>		// Needed for typeid()
#include <cstddef>		// Needed for size_t
#include <cstring>		// Needed for strncpy()

namespace libcw {
  namespace debug {

extern char const* make_label(char const* mangled_name);

class type_info_ct {
protected:
  std::type_info const* a_type_info;	// Pointer to type_info of T
  size_t type_size;			// sizeof(T)
  size_t type_ref_size;			// sizeof(*T) or 0 when T is not a pointer
  char const* dem_name;			// Demangled type name of T
public:
  type_info_ct(void) :
      a_type_info(0), type_size(0), type_ref_size(0), dem_name("<unknown type>") {}
  type_info_ct(std::type_info const& ti, size_t s, size_t rs) :
      a_type_info(&ti), type_size(s), type_ref_size(rs), dem_name(make_label(ti.name())) {}
  char const* demangled_name(void) const { return dem_name; }
  char const* name(void) const { return a_type_info->name(); }
  size_t size(void) const { return type_size; }
  size_t ref_size(void) const { return type_ref_size; }
};

namespace _internal_ {

  //
  // Note on the overload trick:
  // Because we always pass `1' as second parameter, and conversion to
  // `char' has a low priority, calling the template function with
  // `T const*' as first parameter has the highest priority now
  // (in the case that `obj' is really a pointer).
  //
  // Note on the const_cast:
  // We must use a reference as parameter, even when using __inline,
  // because without optimization constructors and destructors for temporaries
  // are called and the destructor could be private for instance.
  // When passing a reference, we must use a `const' too to avoid a warning
  // and that will cause all types to look like consts.
  // Therefore we simply get rid of all const-ness (looks better).

  // _internal_::
  template<typename T>
    type_info_ct const& type_info_of(T const* obj, int)
    {
      static type_info_ct const type_info_singleton(typeid(const_cast<T*>(obj)), sizeof(T*), sizeof(T));
      return type_info_singleton;
    }

  // Specialization for `void*'.
  // _internal_::
  extern type_info_ct const& type_info_of(void const* obj, int);

  // This one is called when `obj' is not a pointer of some type.
  // _internal_::
  template<typename T>
    type_info_ct const& type_info_of(T const& obj, char)
    {
      static type_info_ct const type_info_singleton(typeid(const_cast<T&>(obj)), sizeof(T), 0);
      return type_info_singleton;
    }

  // _internal_::
  class type_info : public std::type_info {
  public:
    type_info(char const* name) : std::type_info(name) { }
  };

  // _internal_::
  template<typename T>
    type_info_ct const&
    type_info_of_type(T const* const*, int)
    {
      T const* tp;
      return type_info_of(tp, 1);
    }

  // _internal_::
  template<typename T>
    type_info_ct const&
    type_info_of_type(T const**, int)
    {
      T* tp;
      return type_info_of(tp, 1);
    }

  // _internal_::
  template<typename T>
    type_info_ct const&
    type_info_of_type(T const*, char)
    {
      static type_info_ct const* type_info_singleton = 0;
      if (!type_info_singleton)
      {
#ifdef DEBUGMALLOC
	set_alloc_checking_off();
#endif
	T* tp;					// Create pointer to object
	char const* tp_name = typeid(tp).name();
	size_t len = strlen(tp_name);
	char* t_name = new char[len];
	strncpy(t_name, tp_name + 1, len - 1);	// Strip off the leading `P'
	t_name[len - 1] = 0;
	type_info ti(t_name);
	type_info_singleton = new type_info_ct(ti, sizeof(T), 0);
#ifdef DEBUGMALLOC
	set_alloc_checking_on();
#endif
      }
      return *type_info_singleton;
    }

} // namespace _internal_

template<class T>
  __inline type_info_ct const&
  type_info_of(T const& obj)
  {
    return _internal_::type_info_of(obj, 1);
  }

// For types without objects.
// This function makes assumptions about how names are mangled.
template<class T>
  __inline type_info_ct const&
  type_info_of(void)
  {
    return _internal_::type_info_of_type(reinterpret_cast<T*>(0), 1);
  }

extern type_info_ct unknown_type_info;

  } // namespace debug
} // namespace libcw

#endif // LIBCW_TYPE_INFO_H
