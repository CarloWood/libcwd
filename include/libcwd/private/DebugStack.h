// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_debug_stack.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_DEBUG_STACK_H
#define LIBCWD_PRIVATE_DEBUG_STACK_H

#include "libcwd/config.h"

#include <cstddef> // Needed for size_t

namespace libcwd::_private_ {

// Stack implementation that doesn't have a constructor.
// The size of 64 should be MORE then enough.

template <typename T> // T must be a builtin type.
struct DebugStack
{
 private:
  T stack_[64];
  T* current_;
  T* end_;

 public:
  void init();
  void push(T ptr);
  void pop();
  T top() const;
  size_t size() const;
};

} // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_DEBUG_STACK_H
