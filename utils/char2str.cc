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

#include <libcw/sys.h>
#include <libcw/h.h>
#include <libcw/char2str.h>
#include <iostream>

RCSTAG_CC("$Id$")

char const c2s_tab[7] = { 'a', 'b', 't', 'n', 'v', 'f', 'r' };

void char2str::print_char_to(ostream& os) const
{
  os.put(c);
}

void char2str::print_escaped_char_to(ostream& os) const
{
  os.put('\\');
  if (c > 6 && c < 14)
  {
    os.put(c2s_tab[c - 7]);
    return;
  }
  else if (c == '\e')
  {
    os.put('e');
    return;
  }
  else if (c == '\\')
  {
    os.put('\\');
    return;
  }
  short old_fill = os.fill('0');
  ios::fmtflags old_flgs = os.flags();
  os.width(3);
  os << oct << (int)((unsigned char)c);
  os.setf(old_flgs);
  os.fill(old_fill);
}
