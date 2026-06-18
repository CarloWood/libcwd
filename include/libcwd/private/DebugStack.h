// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_DEBUG_STACK_H
#define LIBCWD_PRIVATE_DEBUG_STACK_H

#include "libcwd/config.h"

#include <cstddef> // Needed for size_t

#ifndef HIDE_FROM_DOXYGEN
namespace libcwd::_private_ {

// Stack implementation that doesn't have a constructor.
// The size of 64 should be MORE then enough.

template <typename T> // T must be a builtin type.
struct DebugStack
{
 public:
  static constexpr int max_size = 64;

 private:
  T stack_[max_size];
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
#endif // HIDE_FROM_DOXYGEN

#endif // LIBCWD_PRIVATE_DEBUG_STACK_H
