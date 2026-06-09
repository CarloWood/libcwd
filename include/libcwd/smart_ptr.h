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
  int reference_count_;	// Number of smart_ptr objects pointing to this stub.
  char* ptr_;			// Pointer to the actual character string, allocated with new char[...].
public:
  RefCountedCharPtr(char* ptr) : reference_count_(1), ptr_(ptr) { }
  void increment() { ++reference_count_; }
  bool decrement()
  {
    if (ptr_ && --reference_count_ == 0)
    {
      delete [] ptr_;
      ptr_ = NULL;
      return true;
    }
    return false;
  }
  char* get() const { return ptr_; }
  int reference_count() const { return reference_count_; }
};

class smart_ptr {
private:
  void* ptr_;
  bool string_literal_;

public:
  // Default constructor and destructor.
  smart_ptr() : ptr_(NULL), string_literal_(true) { }
  ~smart_ptr() { if (!string_literal_) reinterpret_cast<RefCountedCharPtr*>(ptr_)->decrement(); }

  // Copy constructor.
  smart_ptr(smart_ptr const& ptr) : ptr_(NULL), string_literal_(true) { copy_from(ptr); }

  // Other constructors.
  smart_ptr(char const* ptr) : string_literal_(true) { copy_from(ptr); }
  smart_ptr(char* ptr) : string_literal_(true) { copy_from(ptr); }

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
  bool is_null() const { return ptr_ == NULL; }
  char const* get() const { return string_literal_ ? reinterpret_cast<char*>(ptr_) : reinterpret_cast<RefCountedCharPtr*>(ptr_)->get(); }

protected:
  // Helper methods.
  void copy_from(smart_ptr const& ptr);
  void copy_from(char const* ptr);
  void copy_from(char* ptr);

private:
  // Implementation.
  void increment() { if (!string_literal_) reinterpret_cast<RefCountedCharPtr*>(ptr_)->increment(); }
  void decrement(LIBCWD_TSD_PARAM);
};

} // namespace libcwd::_private_

#endif // LIBCWD_SMART_PTR_H
