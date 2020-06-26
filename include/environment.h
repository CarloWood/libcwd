// $Header$
//
// Copyright (C) 2006, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <iostream>
#include "libcwd/buf2str.h"

namespace libcwd {

class environment_ct {
private:
  char const* const* __envp;
public:
  environment_ct(char const* const envp[]) : __envp(envp) { }
  void print_on(std::ostream& os) const
  {
    os << "[ ";
    char const* const* p = __envp;
    while(*p)
    {
      os << '"' << buf2str(*p, strlen(*p)) << "\", ";
      ++p;
    }
    os << "NULL ]";
  }
};

} // namespace libcwd

#endif // ENVIRONMENT_H

