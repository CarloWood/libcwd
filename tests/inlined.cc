// $Header$
//
// Copyright (C) 2003, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include "debug.h"

extern int a;
extern int h29(), h30(), h40();

inline int
i(int x, int y)
{
  a += x + y;
  return a;
}

inline void
g(int x,
  int y = h29(),
  int z = i(12345, h30()))
{
  int q[4] = { 2, 3, 5, 7 };
  a += x + y + z + reinterpret_cast<ptrdiff_t>(&q);
}

void f()
{
  Dout(dc::notice, "calling g(h40()) from f: " << location_ct(&&current_location));
current_location:
  g(h40());
}
