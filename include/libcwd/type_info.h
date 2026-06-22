// SPDX-FileCopyrightText: 2000-2005, 2007, 2018-2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_TYPE_INFO_H
#define LIBCWD_TYPE_INFO_H

#include <cstddef> // Needed for size_t
#include <typeinfo> // Needed for typeid()

namespace libcwd {
namespace _private_ {

extern char const* make_label(char const* mangled_name);

template <typename T>
struct size_of_completed
{
  static constexpr size_t size = sizeof(T);
};

struct size_of_not_completed
{
  static constexpr size_t size = 0;
};

template <typename T>
struct sizeof_ref
{
  template <typename U>
  static size_of_completed<T> test(int (*)[sizeof(U)]);

  template <typename>
  static size_of_not_completed test(...);

  static constexpr size_t value = decltype(test<T>(nullptr))::size;
};

template <typename T>
size_t sizeof_ref_v = sizeof_ref<T>::value;

} // namespace _private_

/**
 * \brief Class that holds type information for debugging purposes.
 * Returned by type_info_of().
 * \ingroup group_type_info
 */
class TypeInfo
{
 protected:
  size_t type_size_; //!< sizeof(T).
  size_t dereferenced_type_size_; //!< sizeof(*T) or 0 when T is not a pointer (or a pointer to an incomplete type).
  char const* name_; //!< Encoded type of T (as returned by typeid(T).name()).
  char const* demangled_name_; //!< Demangled type name of T.
 public:
  /**
   * \brief Default constructor.
   * \internal
   */
  TypeInfo() { }
  /**
   * \brief Constructor used for unknown_type_info_c.
   * \internal
   */
  TypeInfo(int) : type_size_(0), dereferenced_type_size_(0), name_(NULL), demangled_name_("<unknown type>") { }
  /**
   * \brief Construct a TypeInfo object for a type (T) with encoding \a type_encoding, size \a s and size of reference
   * \a rs.
   * \internal
   */
  void init(char const* type_encoding, size_t s, size_t rs)
  {
    type_size_ = s;
    dereferenced_type_size_ = rs;
    name_ = type_encoding;
    demangled_name_ = _private_::make_label(type_encoding);
  }
  //! The demangled type name.
  char const* demangled_name() const { return demangled_name_; }
  //! The encoded type name (as returned by <CODE>typeid(T).%name()</CODE>).
  char const* name() const { return name_; }
  //! sizeof(T).
  size_t size() const { return type_size_; }
  //! sizeof(*T) or 0 when T is not a pointer (or a pointer to an incomplete type).
  size_t ref_size() const { return dereferenced_type_size_; }
};

namespace _private_ {

extern char const* extract_exact_name(char const*, char const*, LIBCWD_TSD_PARAM);

//-------------------------------------------------------------------------------------------------
// type_info_of

// _private_::
template <typename T>
struct type_info
{
 private:
  static TypeInfo s_value_;
  static bool s_initialized_;

 public:
  static TypeInfo const& value();
};

// Specialization for general pointers.
// _private_::
template <typename T>
struct type_info<T*>
{
 private:
  static TypeInfo s_value_;
  static bool s_initialized_;

 public:
  static TypeInfo const& value();
};

// Specialization for `void*'.
// _private_::
template <>
struct type_info<void*>
{
 private:
  static TypeInfo s_value_;
  static bool s_initialized_;

 public:
  static TypeInfo const& value();
};

// _private_::
template <typename T>
TypeInfo type_info<T>::s_value_;

// _private_::
template <typename T>
bool type_info<T>::s_initialized_;

// _private_::
template <typename T>
TypeInfo const& type_info<T>::value()
{
  if (!s_initialized_)
  {
    s_value_.init(typeid(T).name(), sizeof(T), 0);
    s_initialized_ = true;
  }
  return s_value_;
}

// _private_::
template <typename T>
TypeInfo type_info<T*>::s_value_;

// _private_::
template <typename T>
bool type_info<T*>::s_initialized_;

// _private_::
template <typename T>
TypeInfo const& type_info<T*>::value()
{
  if (!s_initialized_)
  {
    s_value_.init(typeid(T*).name(), sizeof(T*), sizeof_ref_v<T>);
    s_initialized_ = true;
  }
  return s_value_;
}

} // namespace _private_
} // namespace libcwd

//---------------------------------------------------------------------------------------------------
// libcwd_type_info_exact

template <typename T>
struct libcwd_type_info_exact
{
 private:
  static ::libcwd::TypeInfo s_value_;
  static bool s_initialized_;

 public:
  static ::libcwd::TypeInfo const& value();
};

// Specialization for general pointers.
template <typename T>
struct libcwd_type_info_exact<T*>
{
 private:
  static ::libcwd::TypeInfo s_value_;
  static bool s_initialized_;

 public:
  static ::libcwd::TypeInfo const& value();
};

// Specialization for `void*'.
template <>
struct libcwd_type_info_exact<void*>
{
 private:
  static ::libcwd::TypeInfo s_value_;
  static bool s_initialized_;

 public:
  static ::libcwd::TypeInfo const& value();
};

template <typename T>
::libcwd::TypeInfo libcwd_type_info_exact<T>::s_value_;

template <typename T>
bool libcwd_type_info_exact<T>::s_initialized_;

template <typename T>
::libcwd::TypeInfo const& libcwd_type_info_exact<T>::value()
{
  if (!s_initialized_)
  {
    s_value_.init(::libcwd::_private_::extract_exact_name(typeid(libcwd_type_info_exact<T>).name(),
                                                          typeid(T).name(), LIBCWD_TSD_INSTANCE),
                  sizeof(T), 0);
    s_initialized_ = true;
  }
  return s_value_;
}

template <typename T>
::libcwd::TypeInfo libcwd_type_info_exact<T*>::s_value_;

template <typename T>
bool libcwd_type_info_exact<T*>::s_initialized_;

template <typename T>
::libcwd::TypeInfo const& libcwd_type_info_exact<T*>::value()
{
  if (!s_initialized_)
  {
    s_value_.init(::libcwd::_private_::extract_exact_name(typeid(libcwd_type_info_exact<T*>).name(),
                                                          typeid(T*).name(), LIBCWD_TSD_INSTANCE),
                  sizeof(T*), ::libcwd::_private_::sizeof_ref_v<T>);
    s_initialized_ = true;
  }
  return s_value_;
}

namespace libcwd {

/** \addtogroup group_type_info */
/** \{ */

#ifndef LIBCWD_DOXYGEN
// Prototype of `type_info_of'.
template <typename T>
inline TypeInfo const& type_info_of(T const&);
#endif

/**
 * \brief Get type information of a given class or type.
 *
 * This specialization allows to specify a type without an object
 * (for example by calling: <CODE>type_info_of<int const>()</CODE>).
 *
 * As it doesn't ignore top-level qualifiers it is best suited to print for example template parameters.
 * For example,
 * ```cpp
 * template<typename T>
 * void Foo::func(T const&)
 * {
 *   Dout(dc::notice, "Calling Foo::func(" << type_info_of<T const&>().demangled_name() << ')');
 * }
 * ```
 */
template <typename T>
inline TypeInfo const& type_info_of()
{
  return ::libcwd_type_info_exact<T>::value();
}

/**
 * \brief Get type information of a given class \em instance.
 *
 * This template is used by passing an object to it, top level CV-qualifiers (and a possible reference)
 * are ignored in the same way as does \c typeid() (see 5.2.8 Type identification of the ISO C++ standard).
 */
template <typename T>
inline TypeInfo const& type_info_of(
    T const&) // If we don't use a reference, this would _still_ cause the copy constructor to be called.
              // Besides, using `const&' doesn't harm the result as typeid() always ignores the top-level
              // CV-qualifiers anyway (see C++ standard ISO+IEC+14882, 5.2.8 point 5).
{
  return _private_::type_info<T>::value();
}

extern TypeInfo const unknown_type_info_c;

/** \} */

} // namespace libcwd

#endif // LIBCWD_TYPE_INFO_H
