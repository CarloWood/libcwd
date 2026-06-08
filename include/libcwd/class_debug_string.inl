#ifndef LIBCWD_CLASS_DEBUG_STRING_INL
#define LIBCWD_CLASS_DEBUG_STRING_INL

#include "class_debug_string.h"
#include <cstddef>		// Needed for size_t

namespace libcwd {

/**
 * \brief Copy constructor.
 */
inline
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
inline
void
debug_string_ct::assign(char const* str, size_t len)
{
  LIBCWD_TSD_DECLARATION;
  internal_assign(str, len);
}

/**
 * \brief Append \a str with size \a len to the string.
 */
inline
void
debug_string_ct::append(char const* str, size_t len)
{
  LIBCWD_TSD_DECLARATION;
  internal_append(str, len);
}

/**
 * \brief Prepend \a str with size \a len to the string.
 */
inline
void
debug_string_ct::prepend(char const* str, size_t len)
{
  LIBCWD_TSD_DECLARATION;
  internal_prepend(str, len);
}

/**
 * \brief The size of the string.
 */
inline
size_t
debug_string_ct::size() const
{
  return M_size;
}

/**
 * \brief The capacity of the string.
 */
inline
size_t
debug_string_ct::capacity() const
{
  return M_capacity;
}

/**
 * \brief A zero terminated char const pointer.
 */
inline
char const*
debug_string_ct::c_str() const
{
  return M_str;
}

/**
 * \brief Assign \a str to the string.
 */
inline
void
debug_string_ct::assign(std::string const& str)
{
  assign(str.data(), str.size());
}

/**
 * \brief Append \a str to the string.
 */
inline
void
debug_string_ct::append(std::string const& str)
{
  append(str.data(), str.size());
}

/**
 * \brief Prepend \a str to the string.
 */
inline
void
debug_string_ct::prepend(std::string const& str)
{
  prepend(str.data(), str.size());
}

inline
debug_string_stack_element_ct::debug_string_stack_element_ct(debug_string_ct const& ds) : debug_string(ds)
{
}

} // namespace libcwd

#endif // LIBCWD_CLASS_DEBUG_STRING_INL
