// $Header$
//
// Copyright (C) 2000 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCW_CLASS_DEBUG_STRING_INL
#define LIBCW_CLASS_DEBUG_STRING_INL

#ifndef LIBCW_CLASS_DEBUG_STRING_H
#include <libcwd/class_debug_string.h>
#endif
#ifndef LIBCW_THREADING_H
#include <libcwd/private_threading.h>
#endif
#ifndef LIBCW_PRIVATE_SET_ALLOC_CHECKING_H
#include <libcwd/private_set_alloc_checking.h>
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif

namespace libcw {
  namespace debug {

/**
 * \brief Copy constructor.
 */
__inline__
debug_string_ct::debug_string_ct(debug_string_ct const& ds)
{
  NS_internal_init(ds.M_str, ds.M_size);
  if (M_capacity < ds.M_capacity)
    reserve(ds.M_capacity);
  M_default_capacity = ds.M_default_capacity;
}

/**
 * \brief Assign \a str with size \a len to the string.
 */
__inline__
void
debug_string_ct::assign(char const* str, size_t len)
{
  LIBCWD_TSD_DECLARATION;
  _private_::set_alloc_checking_off(LIBCWD_TSD);
  internal_assign(str, len);
  _private_::set_alloc_checking_on(LIBCWD_TSD);
}

/**
 * \brief Append \a str with size \a len to the string.
 */
__inline__
void
debug_string_ct::append(char const* str, size_t len) 
{
  LIBCWD_TSD_DECLARATION;
  _private_::set_alloc_checking_off(LIBCWD_TSD);
  internal_append(str, len);
  _private_::set_alloc_checking_on(LIBCWD_TSD);
}

/**
 * \brief Prepend \a str with size \a len to the string.
 */
__inline__
void
debug_string_ct::prepend(char const* str, size_t len)
{
  LIBCWD_TSD_DECLARATION;
  _private_::set_alloc_checking_off(LIBCWD_TSD);
  internal_prepend(str, len);
  _private_::set_alloc_checking_on(LIBCWD_TSD);
}

/**
 * \brief The size of the string.
 */
__inline__
size_t
debug_string_ct::size(void) const
{
  return M_size;
}

/**
 * \brief The capacity of the string.
 */
__inline__
size_t
debug_string_ct::capacity(void) const
{
  return M_capacity;
}

/**
 * \brief A zero terminated char const pointer.
 */
__inline__
char const*
debug_string_ct::c_str(void) const
{
  return M_str;
}

/**
 * \brief Assign \s to the string.
 */
__inline__
void
debug_string_ct::assign(std::string const& s)
{
  assign(s.data(), s.size());
}

/**
 * \brief Append \s to the string.
 */
__inline__
void
debug_string_ct::append(std::string const& s)
{
  append(s.data(), s.size());
}

/**
 * \brief Prepend \s to the string.
 */
__inline__
void
debug_string_ct::prepend(std::string const& s)
{
  prepend(s.data(), s.size());
}

__inline__
debug_string_stack_element_ct::debug_string_stack_element_ct(debug_string_ct const& ds) : debug_string(ds)
{
}

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_DEBUG_STRING_INL
