// SPDX-FileCopyrightText: 2000-2004, 2007, 2018, 2020, 2025-2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 *
 * \brief Definition of utility class buf2str.
 *
 * This header file provides the declaration and definition of utility class
 * \link libcwd::buf2str buf2str \endlink.
 */

#ifndef LIBCWD_BUF2STR_H
#define LIBCWD_BUF2STR_H

#include "char2str.h"

#include <cstddef> // Needed for size_t
#include <iosfwd>
#if __cpp_concepts >= 201907L
#include <concepts>
#endif

namespace libcwd {

/**
 * \class buf2str buf2str.h libcwd/buf2str.h
 * \ingroup group_special
 *
 * \brief Print a (\c char) buffer with a given size to a %debug ostream, escaping non-printable
 * characters.
 *
 * Converts \a size characters from character buffer pointed to by \a buf into all printable
 * characters by either printing the character itself, the octal representation or one of
 * \c \\a, \c \\b, \c \\t, \c \\n, \c \\f, \c \\r, \c \\e or \c \\\\.
 *
 * \sa libcwd::char2str
 *
 * **Example:**
 *
 * ```cpp
 * char const* buf = "\e[31m;Hello\e[0m;\n";
 * size_t size = strlen(buf);
 *
 * Dout(dc::notice, "The buffer contains: \"" << buf2str(buf, size) << '"');
 * ```
 */

class buf2str
{
 private:
  char const* buf_; //!< Pointer to the start of the buffer.
  size_t size_; //!< The size of the buffer.

 public:
  //! Construct \c buf2str object with attributes \a buf and \a size.
  buf2str(char const* buf, size_t size) : buf_(buf), size_(size) { }

#if __cpp_concepts >= 201907L
  //! Construct \c buf2str object from an object that has data() and size() member functions.
  template<typename T>
  requires requires(T const& t) {
    { t.data() } -> std::convertible_to<char const*>;
    { t.size() } -> std::convertible_to<size_t>;
  }
  buf2str(T const& view) : buf_(view.data()), size_(view.size())
  {
  }
#endif

  /**
   * \brief Write the contents of the buffer represented by \a __buf2str
   * to the \c ostream \a os, escaping non-printable characters.
   */
  friend inline std::ostream& operator<<(std::ostream& os, buf2str const& __buf2str)
  {
    size_t size = __buf2str.size_;
    for (char const* p1 = __buf2str.buf_; size > 0; --size, ++p1)
      os << char2str(*p1);
    return os;
  }
};

} // namespace libcwd

#endif // LIBCWD_BUF2STR_H
