// $Header$
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

/** \file libcwd/type_info.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_TYPE_INFO_H
#define LIBCWD_TYPE_INFO_H

#ifndef LIBCWD_PRIVATE_THREADING_H
#include <libcwd/private_threading.h>
#endif
#ifndef LIBCW_TYPEINFO
#define LIBCW_TYPEINFO
#include <typeinfo>		// Needed for typeid()
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif

namespace libcwd {

namespace _private_ {
  extern char const* make_label(char const* mangled_name);
} // namespace _private_

/**
 * \brief Class that holds type information for debugging purposes.&nbsp;
 * Returned by type_info_of().
 * \ingroup group_type_info
 */
class type_info_ct {
protected:
  size_t M_type_size;			//!< sizeof(T).
  size_t M_type_ref_size;		//!< sizeof(*T) or 0 when T is not a pointer.
  char const* M_name;			//!< Encoded type of T (as returned by typeid(T).name()).
  char const* M_dem_name;		//!< Demangled type name of T.
public:
  /**
   * \brief Default constructor.
   * \internal
   */
  type_info_ct() { }
  /**
   * \brief Constructor used for unknown_type_info_c.
   * \internal
   */
  type_info_ct(int) : M_type_size(0), M_type_ref_size(0), M_name(NULL), M_dem_name("<unknown type>") { }
  /**
   * \brief Construct a type_info_ct object for a type (T) with encoding \a type_encoding, size \a s and size of reference \a rs.
   * \internal
   */
  void init(char const* type_encoding, size_t s, size_t rs)
  {
    M_type_size = s;
    M_type_ref_size = rs;
    M_name = type_encoding;
    M_dem_name = _private_::make_label(type_encoding);
  }
  //! The demangled type name.
  char const* demangled_name() const { return M_dem_name; }
  //! The encoded type name (as returned by <CODE>typeid(T).%name()</CODE>).
  char const* name() const { return M_name; }
  //! sizeof(T).
  size_t size() const { return M_type_size; }
  //! sizeof(*T) or 0 when T is not a pointer.
  size_t ref_size() const { return M_type_ref_size; }
};

namespace _private_ {

  extern char const* extract_exact_name(char const*, char const* LIBCWD_COMMA_TSD_PARAM);

  //-------------------------------------------------------------------------------------------------
  // type_info_of

  // _private_::
  template<typename T>
    struct type_info {
    private:
      static type_info_ct S_value;
      static bool S_initialized;
    public:
      static type_info_ct const& value();
    };

  // Specialization for general pointers.
  // _private_::
  template<typename T>
    struct type_info<T*> {
    private:
      static type_info_ct S_value;
      static bool S_initialized;
    public:
      static type_info_ct const& value();
    };

  // Specialization for `void*'.
  // _private_::
  template<>
    struct type_info<void*> {
    private:
      static type_info_ct S_value;
      static bool S_initialized;
    public:
      static type_info_ct const& value();
    };

  // _private_::
  template<typename T>
    type_info_ct type_info<T>::S_value;

  // _private_::
  template<typename T>
    bool type_info<T>::S_initialized;

  // _private_::
  template<typename T>
    type_info_ct const& type_info<T>::value()
    {
      if (!S_initialized)
      {
	S_value.init(typeid(T).name(), sizeof(T), 0);
	S_initialized = true;
      }
      return S_value;
    }

  // _private_::
  template<typename T>
    type_info_ct type_info<T*>::S_value;

  // _private_::
  template<typename T>
    bool type_info<T*>::S_initialized;

  // _private_::
  template<typename T>
    type_info_ct const& type_info<T*>::value()
    {
      if (!S_initialized)
      {
	S_value.init(typeid(T*).name(), sizeof(T*), sizeof(T));
	S_initialized = true;
      }
      return S_value;
    }

} // namespace _private_

} // namespace libcwd

//---------------------------------------------------------------------------------------------------
// libcwd_type_info_exact

template<typename T>
  struct libcwd_type_info_exact {
  private:
    static ::libcwd::type_info_ct S_value;
    static bool S_initialized;
  public:
    static ::libcwd::type_info_ct const& value();
  };

// Specialization for general pointers.
template<typename T>
  struct libcwd_type_info_exact<T*> {
  private:
    static ::libcwd::type_info_ct S_value;
    static bool S_initialized;
  public:
    static ::libcwd::type_info_ct const& value();
  };

// Specialization for `void*'.
template<>
  struct libcwd_type_info_exact<void*> {
  private:
    static ::libcwd::type_info_ct S_value;
    static bool S_initialized;
  public:
    static ::libcwd::type_info_ct const& value();
  };

template<typename T>
  ::libcwd::type_info_ct libcwd_type_info_exact<T>::S_value;

template<typename T>
  bool libcwd_type_info_exact<T>::S_initialized;

template<typename T>
  ::libcwd::type_info_ct const& libcwd_type_info_exact<T>::value()
  {
    if (!S_initialized)
    {
      S_value.init(::libcwd::_private_::extract_exact_name(typeid(libcwd_type_info_exact<T>).name(), typeid(T).name() LIBCWD_COMMA_TSD_INSTANCE), sizeof(T), 0);
      S_initialized = true;
    }
    return S_value;
  }

template<typename T>
  ::libcwd::type_info_ct libcwd_type_info_exact<T*>::S_value;

template<typename T>
  bool libcwd_type_info_exact<T*>::S_initialized;

template<typename T>
  ::libcwd::type_info_ct const& libcwd_type_info_exact<T*>::value()
  {
    if (!S_initialized)
    {
      S_value.init(::libcwd::_private_::extract_exact_name(typeid(libcwd_type_info_exact<T*>).name(), typeid(T*).name() LIBCWD_COMMA_TSD_INSTANCE), sizeof(T*), sizeof(T));
      S_initialized = true;
    }
    return S_value;
  }

namespace libcwd {

/** \addtogroup group_type_info */
/** \{ */

#ifndef LIBCWD_DOXYGEN
// Prototype of `type_info_of'.
template<typename T>
  inline
  type_info_ct const&
  type_info_of(T const&);
#endif

/**
 * \brief Get type information of a given class or type.
 *
 * This specialization allows to specify a type without an object
 * (for example by calling: <CODE>type_info_of<int const>()</CODE>).
 *
 * As it doesn't ignore top-level qualifiers it is best suited to print for example template parameters.&nbsp;
 * For example,
 * \code
 * template<typename T>
 *   void Foo::func(T const&)
 *   {
 *     Dout(dc::notice, "Calling Foo::func(" << type_info_of<T const&>().demangled_name() << ')');
 *   }
 * \endcode
 */
template<typename T>
  inline
  type_info_ct const&
  type_info_of()
  {
    return ::libcwd_type_info_exact<T>::value();
  }

/**
 * \brief Get type information of a given class \em instance.
 *
 * This template is used by passing an object to it, top level CV-qualifiers (and a possible reference)
 * are ignored in the same way as does \c typeid() (see 5.2.8 Type identification of the ISO C++ standard).
 */
template<typename T>
  inline
  type_info_ct const&
  type_info_of(T const&)		// If we don't use a reference, this would _still_ cause the copy constructor to be called.
					// Besides, using `const&' doesn't harm the result as typeid() always ignores the top-level
					// CV-qualifiers anyway (see C++ standard ISO+IEC+14882, 5.2.8 point 5).
  {
    return _private_::type_info<T>::value();
  }

extern type_info_ct const unknown_type_info_c;

/** \} */

} // namespace libcwd

#endif // LIBCWD_TYPE_INFO_H
