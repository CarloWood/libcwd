// $Header$
//
// Copyright (C) 2000 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcw/char2str.h
 *
 * \brief Definition of utility class \link libcw::debug::char2str char2str \endlink.
 *
 * This header file provides the declaration and definition of utility class
 * \link libcw::debug::char2str char2str \endlink.
 */

#ifndef LIBCW_CHAR2STR_H
#define LIBCW_CHAR2STR_H

#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>
#endif

namespace libcw {
  namespace debug {

/**
 * \class char2str char2str.h libcw/char2str.h
 * \ingroup group_special
 *
 * \brief Print a \c char to a %debug ostream, escaping non-printable characters as needed.
 *
 * Prints the character \a c (see example below) to an ostream, converting it into a printable
 * sequence when needed using the octal representation or one of \c \a, \c \b, \c \t, \c \n,
 * \c \f, \c \r, \c \e or \c \\.
 *
 * \sa libcw::debug::buf2str
 *
 * <b>Example:</b>
 *
 * \code
 * char c = '\f';
 *
 * Dout(dc::notice, "The variable c contains: '" << char2str(c) << '\'');
 * \endcode
 */

class char2str {
private:
  char c;						//!< The character to be printed.

private:
  void print_char_to(std::ostream&) const;
  void print_escaped_char_to(std::ostream&) const;

public:
  //! Construct a \c char2str object with attribute \a ci.
  char2str(char ci) : c(ci) { }

  /**
   * \brief Write the character represented by \a c2s to the \c ostream \a os,
   * escaping it when it is a non-printable character.
   */
  friend __inline__ std::ostream& operator<<(std::ostream& os, char2str const c2s)
  {
    if ((c2s.c > 31 && c2s.c != 92 && c2s.c != 127) || (unsigned char)c2s.c > 159)
      c2s.print_char_to(os);
    else
      c2s.print_escaped_char_to(os);
    return os;
  }
};

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CHAR2STR_H
