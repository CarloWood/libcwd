// SPDX-FileCopyrightText: 2000-2004, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file class_debug_string.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_DEBUG_STRING_H
#define LIBCWD_CLASS_DEBUG_STRING_H

#include "libcwd/config.h"
#include <cstddef>		// Needed for size_t
#include <string>

namespace libcwd {

// String class for DebugObject::margin and DebugObject::marker.
// This class can not have a constructor.

struct DebugStringStackElement;
struct DebugObject_ThreadSpecificData;
class DebugObject;

/**
 * \brief A string class used for the %debug output margin and marker.
 * \ingroup group_formatting
 *
 * This type is used for the public attributes DebugObject::margin and DebugObject::marker of the %debug object class.
 */
class DebugString {
  friend class DebugObject;			// Needs access to the private functions.
  friend struct DebugObject_ThreadSpecificData;
private:
  char* M_str;					// Pointer to malloc-ed (zero terminated) string.
  size_t M_size;				// Size of string (exclusive terminating zero).
  size_t M_capacity;				// Size of allocated area (excl. terminating zero).
  size_t M_default_capacity;			// Current minimum capacity as set with `reserve'.
  static constexpr size_t min_capacity = 64;	// Minimum capacity.

  size_t calculate_capacity(size_t);
  void internal_assign(char const* s, size_t l);
  void internal_append(char const* s, size_t l);
  void internal_prepend(char const* s, size_t l);
  void internal_swallow(DebugString const&);

private:
  void NS_internal_init(char const* s, size_t l);
  void deinitialize();
  DebugString() { }
  ~DebugString();

private:
  friend struct DebugStringStackElement;
  DebugString(DebugString const& ds);

public:
  size_t size() const;
  size_t capacity() const;
  void reserve(size_t);
  char const* c_str() const;
  void assign(char const* str, size_t len);
  void append(char const* str, size_t len);
  void prepend(char const* str, size_t len);
  void assign(std::string const& str);
  void append(std::string const& str);
  void prepend(std::string const& str);
};

// Used for the margin and marker stacks.
struct DebugStringStackElement {
public:
  DebugStringStackElement* next;
  DebugString debug_string;
  DebugStringStackElement(DebugString const& ds);
};

}  // namespace libcwd

#endif // LIBCWD_CLASS_DEBUG_STRING_H
