// SPDX-FileCopyrightText: 2006, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <iostream>
#include <cstring>
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
      os << '"' << buf2str(*p, std::strlen(*p)) << "\", ";
      ++p;
    }
    os << "NULL ]";
  }
};

} // namespace libcwd

#endif // ENVIRONMENT_H

