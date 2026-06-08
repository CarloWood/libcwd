// SPDX-FileCopyrightText: 2002-2005, 2018, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/smart_ptr.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_SMART_PTR_H
#define LIBCWD_SMART_PTR_H

#include "private_struct_TSD.h"

namespace libcwd::_private_ {

class RefCountedCharPtr {
private:
  int M_reference_count;	// Number of smart_ptr objects pointing to this stub.
  char* M_ptr;			// Pointer to the actual character string, allocated with new char[...].
public:
  RefCountedCharPtr(char* ptr) : M_reference_count(1), M_ptr(ptr) { }
  void increment() { ++M_reference_count; }
  bool decrement()
  {
    if (M_ptr && --M_reference_count == 0)
    {
      delete [] M_ptr;
      M_ptr = NULL;
      return true;
    }
    return false;
  }
  char* get() const { return M_ptr; }
  int reference_count() const { return M_reference_count; }
};

class smart_ptr {
private:
  void* M_ptr;
  bool M_string_literal;

public:
  // Default constructor and destructor.
  smart_ptr() : M_ptr(NULL), M_string_literal(true) { }
  ~smart_ptr() { if (!M_string_literal) reinterpret_cast<RefCountedCharPtr*>(M_ptr)->decrement(); }

  // Copy constructor.
  smart_ptr(smart_ptr const& ptr) : M_ptr(NULL), M_string_literal(true) { copy_from(ptr); }

  // Other constructors.
  smart_ptr(char const* ptr) : M_string_literal(true) { copy_from(ptr); }
  smart_ptr(char* ptr) : M_string_literal(true) { copy_from(ptr); }

public:
  // Assignment operators.
  smart_ptr& operator=(smart_ptr const& ptr) { copy_from(ptr); return *this; }
  smart_ptr& operator=(char const* ptr) { copy_from(ptr); return *this; }
  smart_ptr& operator=(char* ptr) { copy_from(ptr); return *this; }

public:
  // Casting operator.
  operator char const* () const { return get(); }

  // Comparison Operators.
  bool operator==(smart_ptr const& ptr) const { return get() == ptr.get(); }
  bool operator==(char const* ptr) const { return get() == ptr; }
  bool operator!=(smart_ptr const& ptr) const { return get() != ptr.get(); }
  bool operator!=(char const* ptr) const { return get() != ptr; }

public:
  bool is_null() const { return M_ptr == NULL; }
  char const* get() const { return M_string_literal ? reinterpret_cast<char*>(M_ptr) : reinterpret_cast<RefCountedCharPtr*>(M_ptr)->get(); }

protected:
  // Helper methods.
  void copy_from(smart_ptr const& ptr);
  void copy_from(char const* ptr);
  void copy_from(char* ptr);

private:
  // Implementation.
  void increment() { if (!M_string_literal) reinterpret_cast<RefCountedCharPtr*>(M_ptr)->increment(); }
  void decrement(LIBCWD_TSD_PARAM);
};

} // namespace libcwd::_private_

#endif // LIBCWD_SMART_PTR_H
