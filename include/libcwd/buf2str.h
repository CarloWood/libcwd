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

/** \file libcwd/buf2str.h
 *
 * \brief Definition of utility class buf2str.
 *
 * This header file provides the declaration and definition of utility class
 * \link libcwd::buf2str buf2str \endlink.
 */

#ifndef LIBCWD_BUF2STR_H
#define LIBCWD_BUF2STR_H

#ifndef LIBCWD_CHAR2STR_H
#include "char2str.h"
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif
#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>
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
 * <b>Example:</b>
 *
 * \code
 * char const* buf = "\e[31m;Hello\e[0m;\n";
 * size_t size = strlen(buf);
 *
 * Dout(dc::notice, "The buffer contains: \"" << buf2str(buf, size) << '"');
 * \endcode
 */

class buf2str {
private:
  char const* M_buf;	//!< Pointer to the start of the buffer.
  size_t M_size;	//!< The size of the buffer.

public:
  //! Construct \c buf2str object with attributes \a buf and \a size.
  buf2str(char const* buf, size_t size) : M_buf(buf), M_size(size) { }

  /**
   * \brief Write the contents of the buffer represented by \a __buf2str
   * to the \c ostream \a os, escaping non-printable characters.
   */
  friend
  inline
  std::ostream&
  operator<<(std::ostream& os, buf2str const& __buf2str)
  {
    size_t size = __buf2str.M_size;
    for (char const* p1 = __buf2str.M_buf; size > 0; --size, ++p1)
      os << char2str(*p1);
    return os;
  }
};

} // namespace libcwd

#endif // LIBCWD_BUF2STR_H
