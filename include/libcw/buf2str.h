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

#ifndef LIBCW_BUF2STR_H
#define LIBCW_BUF2STR_H

#include <libcw/char2str.h>
#include <sys/types.h>

RCSTAG_H(buf2str, "$Id$")

namespace libcw {
  namespace debug {

namespace pu {

  //
  // buf2str
  //
  // Use as:
  //
  // ostream os;
  // os << buf2str(buf, len) << endl;
  //
  // Converts `len' characters from `buf' into all printable characters
  // by either printing itself, the octal representation or one of \a, \b, 
  // \t, \n, \f, \r, \e or \\.
  //

  // pu::
  class buf2str {

  private:
    char const* b;
    size_t l;

  public:
    buf2str(char const* buf, size_t len) : b(buf), l(len) {}

    /*
     * Note: This function is only intended for debugging purposes,
     *       it is not a good idea to use it in the final compilation.
     */
    friend inline ostream& operator<<(ostream& os, buf2str const& b2s)
    {
      register size_t len = b2s.l;
      for (char const* p1 = b2s.b; len > 0; --len, p1++)
	os << char2str(*p1);
      return os;
    }
  };

} // namespace pu

  } // namespace debug
} // namespace libcw

#endif // LIBCW_BUF2STR_H
