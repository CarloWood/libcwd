// $Header$
//
// Copyright (C) 2002 - 2004, by
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
#include <match.h>

namespace libcwd {
  namespace _private_ {

bool match(char const* mask, size_t masklen, char const* name)
{
  char const* m = mask;
  char const* n = name;

  for(;;)
  {
    if (*n == 0)
    {
      while(masklen-- > 0)
	if (*m++ != '*')
	  return false;
      return true;
    }
    if (*m == '*')
      break;
    if (*n++ != *m++)
      return false;
    --masklen;
  }
  while(--masklen > 0 && *++m == '*');
  if (masklen == 0)
    return true;
  while(*n != *m || !match(m, masklen, n))
    if (*n++ == 0)
      return false;
  return true;
}

  } // namespace _private_
} // namespace libcwd

