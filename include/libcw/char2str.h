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

#ifndef LIBCW_CHAR2STR_H
#define LIBCW_CHAR2STR_H

#include <iosfwd>

RCSTAG_H(char2str, "$Id$")

namespace libcw {
  namespace debug {

class char2str {
private:
  char c;
  void print_char_to(ostream&) const;
  void print_escaped_char_to(ostream&) const;
public:
  char2str(char ci) : c(ci) { }
  friend inline ostream& operator<<(ostream& os, char2str const c2s)
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
