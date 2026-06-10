// SPDX-FileCopyrightText: 2001-2004, 2018, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef PRIVATE_DEBUG_STACK_INL
#define PRIVATE_DEBUG_STACK_INL

#include "libcwd/core_dump.h"
#include "libcwd/LIBCWD_ASSERT.h"
#include "libcwd/private/DebugStack.h"

#include <cstddef> // Needed for size_t

namespace libcwd::_private_ {

// Stack implementation that doesn't have a constructor.
// The size of 64 should be MORE then enough.

template <typename T>
inline void DebugStack<T>::init()
{
  current_ = stack_ - 1;
  end_ = stack_ + max_size - 1;
}

template <typename T>
inline void DebugStack<T>::push(T ptr)
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT(end_ != NULL);
#endif
  if (current_ == end_)
    core_dump(); // This is really not normal, if you core here you probably did something wrong.
                 // Doing a back trace in gdb should reveal an `infinite' debug output re-entrance loop.
                 // This means that while printing debug output you call a function that makes
                 // your program return to the same line, starting to print out that debug output
                 // again. Try to break this loop somehow.
  *++current_ = ptr;
}

extern void print_pop_error();

template <typename T>
inline void DebugStack<T>::pop()
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT(end_ != NULL);
#endif
  if (current_ == stack_ - 1)
    print_pop_error();
  --current_;
}

template <typename T>
inline T DebugStack<T>::top() const
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT(end_ != NULL);
#endif
  return *current_;
}

template <typename T>
inline size_t DebugStack<T>::size() const
{
#if CWDEBUG_DEBUG
  LIBCWD_ASSERT(end_ != NULL);
#endif
  return current_ - (stack_ - 1);
}

} // namespace libcwd::_private_

#endif // PRIVATE_DEBUG_STACK_INL
