// SPDX-FileCopyrightText: 2006, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "libcwd/buf2str.h"

#include <cstring>
#include <iostream>

namespace libcwd {

class Environment
{
 private:
  char const* const* envp_;

 public:
  Environment(char const* const envp[]) : envp_(envp) { }
  void print_on(std::ostream& os) const
  {
    os << "[ ";
    char const* const* p = envp_;
    while (*p)
    {
      os << '"' << buf2str(*p, std::strlen(*p)) << "\", ";
      ++p;
    }
    os << "NULL ]";
  }
};

} // namespace libcwd

#endif // ENVIRONMENT_H
