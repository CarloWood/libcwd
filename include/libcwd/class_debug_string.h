// $Header$
//
// Copyright (C) 2000 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file class_debug_string.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_DEBUG_STRING_H
#define LIBCWD_CLASS_DEBUG_STRING_H

#ifndef LIBCWD_CONFIG_H
#include "libcwd/config.h"
#endif
#ifndef LIBCW_CSTDDEF
#define LIBCW_CSTDDEF
#include <cstddef>		// Needed for size_t
#endif
#ifndef LIBCW_STRING
#define LIBCW_STRING
#include <string>
#endif

namespace libcwd {

// String class for debug_ct::margin and debug_ct::marker.
// This class can not have a constructor.

struct debug_string_stack_element_ct;
struct debug_tsd_st;
class debug_ct;

/**
 * \brief A string class used for the %debug output margin and marker.
 * \ingroup group_formatting
 *
 * This type is used for the public attributes debug_ct::margin and debug_ct::marker of the %debug object class.
 */
class debug_string_ct {
  friend class debug_ct;			// Needs access to the private functions.
  friend struct debug_tsd_st;
private:
  char* M_str;					// Pointer to malloc-ed (zero terminated) string.
  size_t M_size;				// Size of string (exclusive terminating zero).
  size_t M_capacity;				// Size of allocated area (excl. terminating zero).
  size_t M_default_capacity;			// Current minimum capacity as set with `reserve'.
  static size_t const min_capacity_c = 64;	// Minimum capacity.

  size_t calculate_capacity(size_t);
  void internal_assign(char const* s, size_t l);
  void internal_append(char const* s, size_t l);
  void internal_prepend(char const* s, size_t l);
  void internal_swallow(debug_string_ct const&);

private:
  void NS_internal_init(char const* s, size_t l);
  void deinitialize();
  debug_string_ct() { }
  ~debug_string_ct();

private:
  friend struct debug_string_stack_element_ct;
  debug_string_ct(debug_string_ct const& ds);

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
struct debug_string_stack_element_ct {
public:
  debug_string_stack_element_ct* next;
  debug_string_ct debug_string;
  debug_string_stack_element_ct(debug_string_ct const& ds);
};

}  // namespace libcwd

#endif // LIBCWD_CLASS_DEBUG_STRING_H
