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

namespace libcw {
  namespace debug {

#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
    namespace _internal_ {

      template<typename T>
        struct sizeof_star {
	  static size_t const value = 0;
	};

      template<typename T>
        struct sizeof_star<T*> {
	  static const size_t value = sizeof(T);
	};

      struct sizeof_star<void*> {
	static size_t const value = 0;
      };

    }
#endif

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

template<typename T>
  struct type_info {
    static type_info_ct const value;
  };

// Specialization for general pointers.
template<typename T>
  struct type_info<T*> {
    static type_info_ct const value;
  };
 
// Specialization for `void*'.
struct type_info<void*> {
  static type_info_ct const value;
};

extern type_info_ct unknown_type_info;

template<typename T>
  type_info_ct const type_info<T>::value(typeid(T), sizeof(T), 0);

template<typename T>
  type_info_ct const type_info<T*>::value(typeid(T*), sizeof(T*), sizeof(T));

// Prototype
template<typename T>
  __inline type_info_ct const& type_info_of(T);

// Specialization that allows to specify a type without an object.
// This is really only necessary for GNU g++ version 2, otherwise
// libcw::debug::type_info<>:value could be used directly.
//
// You also need to use this for constants since passing an object
// to a function (the function type_info(T) below) strips the 'const'
// from a parameter.
//
template<class T>
  __inline type_info_ct const&
  type_info_of(void)
  {
#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
    // In early versions of g++, typeid is broken and doesn't work on a template parameter type.
    // We have to use the following hack.
    if (type_info<T>::value.size() == 0)		// Not initialized already?
    {
      class type_info_with_name : public std::type_info {	// Nuke the 'protected' qualifier of this constructor.
      public:
	type_info_with_name(char const* name) : std::type_info(name) { }
      };

      T* tp;                                  			// Create pointer to object.
      static type_info_with_name ti(typeid(tp).name() + 1);	// Strip leading 'P' in the mangled name to get rid of the 'pointer' again.
      new (const_cast<type_info_ct*>(&type_info<T>::value))
          type_info_ct(ti, sizeof(T), _internal_::sizeof_star<T>::value);	// In place initialize the static type_info_ct object.
    }
#endif
    return type_info<T>::value;
  }

// We could/should have used type_info_of<typeof(obj)>(), but typeof(obj) doesn't
// work when obj has a template parameter as type (not supported in 2.95.3 and
// broken in 3.0; see also http://gcc.gnu.org/cgi-bin/gnatsweb.pl?cmd=view&pr=2703&database=gcc).
template<typename T>
  __inline type_info_ct const&
  type_info_of(T)
  {
#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
    return type_info_of<T>();
#else
    return type_info<T>::value;
#endif
  }

  } // namespace debug
} // namespace libcw

#endif // LIBCW_TYPE_INFO_H
