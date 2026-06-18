// SPDX-FileCopyrightText: 2000-2005, 2018, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_LOCKABLE_AUTO_PTR_H
#define LIBCWD_LOCKABLE_AUTO_PTR_H

// This header file is really part of libcw, and must be allowed to be used
// without that libcwd installed or used.
#ifndef CWDEBUG
#ifndef LIBCWD_ASSERT
#define LIBCWD_ASSERT(x)
#endif
#else // CWDEBUG
#include "LIBCWD_ASSERT.h"
#endif // CWDEBUG

namespace libcwd {

//===================================================================================================
// class lockable_auto_ptr
//
// An 'auto_ptr' with lockable ownership.
//
// When the `lockable_auto_ptr' is not locked it behaves the same as
// an 'auto_ptr': Ownership of the object it points to is transfered
// when the `lockable_auto_ptr' is copied or assigned to another
// `lockable_auto_ptr'.  The object it points to is deleted when the
// `lockable_auto_ptr' that owns it is destructed.
//
// When the `lockable_auto_ptr' is locked, then the ownership is
// not transfered, but stays on the same (locked) `lockable_auto_ptr'.
//

template <class X, bool array = false> // Use array == true when `ptr' was allocated with new [].
class lockable_auto_ptr
{
  using element_type = X;

 private:
  template <class Y, bool ARRAY>
  friend class lockable_auto_ptr;
  X* ptr_; // Pointer to object of type X, or NULL when not pointing to anything.
  bool locked_; // Set if this lockable_auto_ptr object is locked.
  mutable bool owner_; // Set if this lockable_auto_ptr object is the owner of the object that
                       // `ptr_' points too.

 public:
  //-----------------------------------------------------------------------------------------------
  // Constructors
  //

  explicit lockable_auto_ptr(X* p = 0) : ptr_(p), locked_(false), owner_(p) { }
  // Explicit constructor that creates a lockable_auto_ptr pointing to `p'.

  lockable_auto_ptr(lockable_auto_ptr const& r) : ptr_(r.ptr_), locked_(false), owner_(r.owner_ && !r.locked_)
  {
    if (!r.locked_)
      r.owner_ = 0;
  }
  // The default copy constructor.

  template <class Y>
  lockable_auto_ptr(lockable_auto_ptr<Y, array> const& r) : ptr_(r.ptr_), locked_(false), owner_(r.owner_ && !r.locked_)
  {
    if (!r.locked_)
      r.owner_ = 0;
  }
  // Constructor to copy a lockable_auto_ptr that point to an object derived from X.

  //-----------------------------------------------------------------------------------------------
  // Operators
  //

  template <class Y>
  lockable_auto_ptr& operator=(lockable_auto_ptr<Y, array> const& r);

  lockable_auto_ptr& operator=(lockable_auto_ptr const& r) { return operator= <X>(r); }
  // The default assignment operator.

  //-----------------------------------------------------------------------------------------------
  // Destructor
  //

  ~lockable_auto_ptr()
  {
    if (owner_)
    {
      if (array)
        delete[] ptr_;
      else
        delete ptr_;
    }
  }

  //-----------------------------------------------------------------------------------------------
  // Accessors
  //

  X& operator*() const { return *ptr_; }
  // Access the object that this `lockable_auto_ptr' points to.

  X* operator->() const { return ptr_; }
  // Access the object that this `lockable_auto_ptr' points to.

  X* get() const { return ptr_; }
  // Return the pointer itself.

  bool strict_owner() const
  {
    LIBCWD_ASSERT(is_owner());
    return locked_;
  }
  // Returns `true' when this object is the strict owner.
  // You should only call this when this object is the owner.

#ifdef CWDEBUG
  bool is_owner() const { return owner_; }
  // Don't use this except for debug testing.
#endif

  //-----------------------------------------------------------------------------------------------
  // Manipulators
  //

  void reset()
  {
    bool owns = owner_;
    owner_ = 0;
    if (owns)
    {
      if (array)
        delete[] ptr_;
      else
        delete ptr_;
    }
    ptr_ = NULL;
  }
  // Get rid of object, if any.

  X* release() const
  {
    LIBCWD_ASSERT(is_owner());
    owner_ = 0;
    return ptr_;
  }
  // Release this object of its ownership (the caller is now responsible for deleting it if
  // this object was the owner).  You should only call this when this object is the owner.

  void lock()
  {
    LIBCWD_ASSERT(is_owner());
    locked_ = true;
  }
  // Lock the ownership.
  // You should only call this when this object is the owner.

  void unlock() { locked_ = false; }
  // Unlock the ownership (if any).
};

template <class X, bool array>
template <class Y>
lockable_auto_ptr<X, array>& lockable_auto_ptr<X, array>::operator=(lockable_auto_ptr<Y, array> const& r)
{
  if ((void*)&r != (void*)this)
  {
    if (owner_)
    {
      if (array)
        delete[] ptr_;
      else
        delete ptr_;
    }
    ptr_ = r.ptr_;
    if (r.locked_)
      owner_ = 0;
    else
    {
      owner_ = r.owner_;
      r.owner_ = 0;
    }
  }
  return *this;
}

} // namespace libcwd

#endif // LIBCWD_LOCKABLE_AUTO_PTR_H
