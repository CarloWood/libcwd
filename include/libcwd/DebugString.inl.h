#pragma once

#ifndef LIBCWD_CLASS_DEBUG_STRING_INL
#define LIBCWD_CLASS_DEBUG_STRING_INL

#include "DebugString.h"

#include <cstddef> // Needed for size_t

namespace libcwd {

/**
 * \brief Copy constructor.
 */
inline DebugString::DebugString(DebugString const& ds)
{
  NS_internal_init(ds.str_, ds.size_);
  if (capacity_ < ds.capacity_)
    reserve(ds.capacity_);
  default_capacity_ = ds.default_capacity_;
}

/**
 * \brief Assign \a str with size \a len to the string.
 */
inline void DebugString::assign(char const* str, size_t len)
{
  LIBCWD_TSD_DECLARATION;
  internal_assign(str, len);
}

/**
 * \brief Append \a str with size \a len to the string.
 */
inline void DebugString::append(char const* str, size_t len)
{
  LIBCWD_TSD_DECLARATION;
  internal_append(str, len);
}

/**
 * \brief Prepend \a str with size \a len to the string.
 */
inline void DebugString::prepend(char const* str, size_t len)
{
  LIBCWD_TSD_DECLARATION;
  internal_prepend(str, len);
}

/**
 * \brief The size of the string.
 */
inline size_t DebugString::size() const
{
  return size_;
}

/**
 * \brief The capacity of the string.
 */
inline size_t DebugString::capacity() const
{
  return capacity_;
}

/**
 * \brief A zero terminated char const pointer.
 */
inline char const* DebugString::c_str() const
{
  return str_;
}

/**
 * \brief Assign \a str to the string.
 */
inline void DebugString::assign(std::string const& str)
{
  assign(str.data(), str.size());
}

/**
 * \brief Append \a str to the string.
 */
inline void DebugString::append(std::string const& str)
{
  append(str.data(), str.size());
}

/**
 * \brief Prepend \a str to the string.
 */
inline void DebugString::prepend(std::string const& str)
{
  prepend(str.data(), str.size());
}

inline DebugStringStackElement::DebugStringStackElement(DebugString const& ds) : debug_string(ds)
{
}

} // namespace libcwd

#endif // LIBCWD_CLASS_DEBUG_STRING_INL
