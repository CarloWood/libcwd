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

namespace _internal_ {
  extern char const* make_label(char const* mangled_name);
}

class type_info_ct {
protected:
  size_t M_type_size;				// sizeof(T)
  size_t M_type_ref_size;			// sizeof(*T) or 0 when T is not a pointer
  char const* M_name;				// Encoded type of T (as returned by typeid(T).name()).
  char const* M_dem_name;			// Demangled type name of T
public:
  type_info_ct(void) :
      M_type_size(0), M_type_ref_size(0), M_name(NULL), M_dem_name("<unknown type>") {}
  type_info_ct(char const* type_encoding, size_t s, size_t rs) :
      M_type_size(s), M_type_ref_size(rs), M_name(type_encoding), M_dem_name(_internal_::make_label(type_encoding)) {}
  char const* demangled_name(void) const { return M_dem_name; }
  char const* name(void) const { return M_name; }
  size_t size(void) const { return M_type_size; }
  size_t ref_size(void) const { return M_type_ref_size; }
};

namespace _internal_ {

  extern char const* extract_exact_name(char const*);

  //------------------------------------------------------------------------------------
  // type_info_of

  // _internal_::
  template<typename T>
    struct type_info {
      static type_info_ct const value;
    };

  // Specialization for general pointers.
  // _internal_::
  template<typename T>
    struct type_info<T*> {
      static type_info_ct const value;
    };

  // Specialization for `void*'.
  // _internal_::
  struct type_info<void*> {
    static type_info_ct const value;
  };

  // _internal_::
  template<typename T>
    type_info_ct const type_info<T>::value(typeid(T).name(), sizeof(T), 0);

  // _internal_::
  template<typename T>
    type_info_ct const type_info<T*>::value(typeid(T*).name(), sizeof(T*), sizeof(T));

  //------------------------------------------------------------------------------------
  // type_info_exact

  // _internal_::
  template<typename T>
    struct type_info_exact {
      static type_info_ct const value;
    };

  // Specialization for general pointers.
  // _internal_::
  template<typename T>
    struct type_info_exact<T*> {
      static type_info_ct const value;
    };

  // Specialization for `void*'.
  // _internal_::
  struct type_info_exact<void*> {
    static type_info_ct const value;
  };

  // _internal_::
  template<typename T>
    type_info_ct const type_info_exact<T>::value(extract_exact_name(typeid(type_info_exact<T>).name()), sizeof(T), 0);

  // _internal_::
  template<typename T>
    type_info_ct const type_info_exact<T*>::value(extract_exact_name(typeid(type_info_exact<T*>).name()), sizeof(T*), sizeof(T));

#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
  //------------------------------------------------------------------------------------
  // sizeof_star

  // _internal_::
  template<typename T>
    struct sizeof_star {
      static size_t const value = 0;
    };

  // _internal_::
  template<typename T>
    struct sizeof_star<T*> {
      static const size_t value = sizeof(T);
    };

  // _internal_::
  struct sizeof_star<void*> {
    static size_t const value = 0;
  };
#endif

} // namespace _internal_

// Prototype of `type_info_of'.
template<typename T>
  __inline type_info_ct const& type_info_of(T const&);

// Specialization that allows to specify a type without an object.
// This is really only necessary for GNU g++ version 2, otherwise
// libcw::debug::type_info<>:value could be used directly.
//
template<class T>
  __inline type_info_ct const&
  type_info_of(void)
  {
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
    // In early versions of g++, typeid is broken and doesn't work on a template parameter type.
    // We have to use the following hack.
    if (_internal_::type_info_exact<T>::value.size() == 0)		// Not initialized already?
    {
      // T* tp;                                  			// Create pointer to object.
      new (const_cast<type_info_ct*>(&_internal_::type_info_exact<T>::value))
          type_info_ct(_internal_::extract_exact_name(typeid(this).name()), sizeof(T), _internal_::sizeof_star<T>::value);	// In place initialize the static type_info_ct object.
    }
#endif
    return _internal_::type_info_exact<T>::value;
  }

// We could/should have used type_info_of<typeof(obj)>(), but typeof(obj) doesn't
// work when obj has a template parameter as type (not supported in 2.95.3 and
// broken in 3.0; see also http://gcc.gnu.org/cgi-bin/gnatsweb.pl?cmd=view&pr=2703&database=gcc).
template<typename T>
  __inline type_info_ct const&
  type_info_of(T const&)		// If we don't use a reference, this would _still_ cause the copy constructor to be called.
  					// Besides, using `const&' doesn't harm the result as typeid() always ignores the top-level
					// CV-qualifiers anyway (see C++ standard ISO+IEC+14882, 5.2.8 point 5).
  {
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
    return type_info_of<T>();
#else
    return _internal_::type_info<T>::value;
#endif
  }

extern type_info_ct unknown_type_info;

  } // namespace debug
} // namespace libcw

#endif // LIBCW_TYPE_INFO_H
