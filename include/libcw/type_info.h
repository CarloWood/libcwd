// $Header$
//
// Copyright (C) 2000 - 2001, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcw/type_info.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_TYPE_INFO_H
#define LIBCW_TYPE_INFO_H

#ifndef LIBCW_PRIVATE_THREADING_H
#include <libcw/private_threading.h>
#endif
#ifndef LIBCW_TYPEINFO
#define LIBCW_TYPEINFO
#include <typeinfo>		// Needed for typeid()
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif

namespace libcw {
  namespace debug {

namespace _private_ {
  extern char const* make_label(char const* mangled_name);
}

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
  type_info_ct(void) :
      M_type_size(0), M_type_ref_size(0), M_name(NULL), M_dem_name("<unknown type>") { }
  /**
   * \brief Construct a type_info_ct object for a type (T) with encoding \a type_encoding, size \a s and size of reference \a rs.
   * \internal
   */
  type_info_ct(char const* type_encoding, size_t s, size_t rs) :
      M_type_size(s), M_type_ref_size(rs), M_name(type_encoding),
      M_dem_name(_private_::make_label(type_encoding)) { }
  //! The demangled type name.
  char const* demangled_name(void) const { return M_dem_name; }
  //! The encoded type name (as returned by <CODE>typeid(T).%name()</CODE>).
  char const* name(void) const { return M_name; }
  //! sizeof(T).
  size_t size(void) const { return M_type_size; }
  //! sizeof(*T) or 0 when T is not a pointer.
  size_t ref_size(void) const { return M_type_ref_size; }
};

namespace _private_ {

#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
  extern char const* extract_exact_name(char const* LIBCWD_COMMA_TSD_PARAM);
#else
  extern char const* extract_exact_name(char const*, char const* LIBCWD_COMMA_TSD_PARAM);
#endif

  //-------------------------------------------------------------------------------------------------
  // type_info_of

  // _private_::
  template<typename T>
    struct type_info {
      static type_info_ct const value_c;
    };

  // Specialization for general pointers.
  // _private_::
  template<typename T>
    struct type_info<T*> {
      static type_info_ct const value_c;
    };

  // Specialization for `void*'.
  // _private_::
  struct type_info<void*> {
    static type_info_ct const value_c;
  };

  // NOTE:
  // Compiler versions 2.95.x will terminate with an "Internal compiler error"
  // in the line below if you use the option '-fno-rtti'.  Either upgrade to version
  // 2.96 or higher, or don't use '-fno-rtti'.  The exact reason for the compiler
  // crash is the use of `typeid'.
  // _private_::
  template<typename T>
    type_info_ct const type_info<T>::value_c(typeid(T).name(), sizeof(T), 0);

  // _private_::
  template<typename T>
    type_info_ct const type_info<T*>::value_c(typeid(T*).name(), sizeof(T*), sizeof(T));

#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
  //-------------------------------------------------------------------------------------------------
  // sizeof_star

  // _private_::
  template<typename T>
    struct sizeof_star {
      static size_t const value_c = 0;
    };

  // _private_::
  template<typename T>
    struct sizeof_star<T*> {
      static size_t const value_c = sizeof(T);
    };

  // _private_::
  struct sizeof_star<void*> {
    static size_t const value_c = 0;
  };
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw

//---------------------------------------------------------------------------------------------------
// libcwd_type_info_exact

template<typename T>
  struct libcwd_type_info_exact {
    static ::libcw::debug::type_info_ct const value_c;
  };

// Specialization for general pointers.
template<typename T>
  struct libcwd_type_info_exact<T*> {
    static ::libcw::debug::type_info_ct const value_c;
  };

// Specialization for `void*'.
struct libcwd_type_info_exact<void*> {
  static ::libcw::debug::type_info_ct const value_c;
};

#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
template<typename T>
  ::libcw::debug::type_info_ct const libcwd_type_info_exact<T>::value_c(::libcw::debug::_private_::extract_exact_name(typeid(libcwd_type_info_exact<T>).name() LIBCWD_COMMA_TSD_INSTANCE), sizeof(T), 0);

template<typename T>
  ::libcw::debug::type_info_ct const libcwd_type_info_exact<T*>::value_c(::libcw::debug::_private_::extract_exact_name(typeid(libcwd_type_info_exact<T*>).name() LIBCWD_COMMA_TSD_INSTANCE), sizeof(T*), sizeof(T));
#else
template<typename T>
  ::libcw::debug::type_info_ct const libcwd_type_info_exact<T>::value_c(::libcw::debug::_private_::extract_exact_name(typeid(libcwd_type_info_exact<T>).name(), typeid(T).name() LIBCWD_COMMA_TSD_INSTANCE), sizeof(T), 0);

template<typename T>
  ::libcw::debug::type_info_ct const libcwd_type_info_exact<T*>::value_c(::libcw::debug::_private_::extract_exact_name(typeid(libcwd_type_info_exact<T*>).name(), typeid(T*).name() LIBCWD_COMMA_TSD_INSTANCE), sizeof(T*), sizeof(T));
#endif

namespace libcw {
  namespace debug {

/** \addtogroup group_type_info */
/** \{ */

// Prototype of `type_info_of'.
template<typename T>
  __inline__
  type_info_ct const&
  type_info_of(T const&
#ifdef LIBCW_DOXYGEN
      instance
#endif
      );

// This is really only necessary for GNU g++ version 2, otherwise
// libcw::debug::type_info<>::value_c could be used directly.
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
  __inline__
  type_info_ct const&
  type_info_of(void)
  {
#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
    // In early versions of g++, typeid is broken and doesn't work on a template parameter type.
    // We have to use the following hack.
    if (::libcwd_type_info_exact<T>::value_c.size() == 0)		// Not initialized already?
    {
#ifdef LIBCWD_THREAD_SAFE
      volatile static bool spin_lock = false;
      _private_::mutex_tct<_private_::type_info_of_instance>::lock();
      while(spin_lock);
      spin_lock = true;
      _private_::mutex_tct<_private_::type_info_of_instance>::unlock();
      if (::libcwd_type_info_exact<T>::value_c.size() == 0)		// Recheck now that we acquired the lock.
#endif
      {
        LIBCWD_TSD_DECLARATION
	new (const_cast<type_info_ct*>(&::libcwd_type_info_exact<T>::value_c))	// MT: const_cast is safe: we are locked.
	    type_info_ct(_private_::extract_exact_name(typeid(::libcwd_type_info_exact<T>).name() LIBCWD_COMMA_TSD), sizeof(T), _private_::sizeof_star<T>::value_c);	// In place initialize the static type_info_ct object.
      }
#ifdef LIBCWD_THREAD_SAFE
      spin_lock = false;
#endif
    }
#endif // __GNUC__ == 2 && __GNUC_MINOR__ < 97
    return ::libcwd_type_info_exact<T>::value_c;
  }

// We could have used type_info_of<typeof(obj)>(), but typeof(obj) doesn't
// work when obj has a template parameter as type (not supported in 2.95.3 and
// broken in 3.0; see also http://gcc.gnu.org/cgi-bin/gnatsweb.pl?cmd=view&pr=2703&database=gcc).
/**
 * \brief Get type information of a given class \em instance.
 *
 * This template is used by passing an object to it, top level CV-qualifiers (and a possible reference)
 * are ignored in the same way as does \c typeid() (see 5.2.8 Type identification of the ISO C++ standard).
 */ 
template<typename T>
  __inline__
  type_info_ct const&
  type_info_of(T const&)		// If we don't use a reference, this would _still_ cause the copy constructor to be called.
  					// Besides, using `const&' doesn't harm the result as typeid() always ignores the top-level
					// CV-qualifiers anyway (see C++ standard ISO+IEC+14882, 5.2.8 point 5).
  {
#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
    return type_info_of<T>();
#else
    return _private_::type_info<T>::value_c;
#endif
  }

extern type_info_ct const unknown_type_info_c;

/** \} */

  } // namespace debug
} // namespace libcw

#endif // LIBCW_TYPE_INFO_H
