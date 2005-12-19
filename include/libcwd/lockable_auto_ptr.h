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

/** \file libcwd/lockable_auto_ptr.h
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
#ifndef LIBCWD_PRIVATE_ASSERT_H
#include <libcwd/private_assert.h>
#endif
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

template<class X, bool array = false>	// Use array == true when `ptr' was allocated with new [].
  class lockable_auto_ptr {
    typedef X element_type;

    private:
    template<class Y, bool ARRAY> friend class lockable_auto_ptr;
    X* ptr;		// Pointer to object of type X, or NULL when not pointing to anything.
    bool locked;	// Set if this lockable_auto_ptr object is locked.
    mutable bool owner;	// Set if this lockable_auto_ptr object is the owner of the object that
    			// `ptr' points too.

  public:
    //-----------------------------------------------------------------------------------------------
    // Constructors
    //

    explicit lockable_auto_ptr(X* p = 0) : ptr(p), locked(false), owner(p) { }
	// Explicit constructor that creates a lockable_auto_ptr pointing to `p'.

    lockable_auto_ptr(lockable_auto_ptr const& r) :
        ptr(r.ptr), locked(false), owner(r.owner && !r.locked)
	{ if (!r.locked) r.owner = 0; }
	// The default copy constructor.

    template<class Y>
      lockable_auto_ptr(lockable_auto_ptr<Y, array> const& r) :
          ptr(r.ptr), locked(false), owner(r.owner && !r.locked)
	  { if (!r.locked) r.owner = 0; }
	// Constructor to copy a lockable_auto_ptr that point to an object derived from X.

    //-----------------------------------------------------------------------------------------------
    // Operators
    //

    template<class Y>
    lockable_auto_ptr& operator=(lockable_auto_ptr<Y, array> const& r);

    lockable_auto_ptr& operator=(lockable_auto_ptr const& r) { return operator= <X> (r); }
	// The default assignment operator.

    //-----------------------------------------------------------------------------------------------
    // Destructor
    //

    ~lockable_auto_ptr() { if (owner) { if (array) delete [] ptr; else delete ptr; } }

    //-----------------------------------------------------------------------------------------------
    // Accessors
    //

    X& operator*() const { return *ptr; }
      // Access the object that this `lockable_auto_ptr' points to.

    X* operator->() const { return ptr; }
      // Access the object that this `lockable_auto_ptr' points to.

    X* get() const { return ptr; }
      // Return the pointer itself.

    bool strict_owner() const { LIBCWD_ASSERT(is_owner()); return locked; }
      // Returns `true' when this object is the strict owner.
      // You should only call this when this object is the owner.

#ifdef CWDEBUG
    bool is_owner(void) const { return owner; }
      // Don't use this except for debug testing.
#endif

    //-----------------------------------------------------------------------------------------------
    // Manipulators
    //

    void reset()
    {
      bool owns = owner;
      owner = 0;
      if (owns)
      {
	if (array)
	  delete [] ptr;
	else
	  delete ptr;
      }
      ptr = NULL;
    }
      // Get rid of object, if any.

    X* release() const { LIBCWD_ASSERT(is_owner()); owner = 0; return ptr; }
      // Release this object of its ownership (the caller is now responsible for deleting it if
      // this object was the owner).  You should only call this when this object is the owner.

    void lock() { LIBCWD_ASSERT(is_owner()); locked = true; }
      // Lock the ownership.
      // You should only call this when this object is the owner.

    void unlock() { locked = false; }
      // Unlock the ownership (if any).
  };

template<class X, bool array>
  template<class Y>
    lockable_auto_ptr<X, array>&
    lockable_auto_ptr<X, array>::operator=(lockable_auto_ptr<Y, array> const& r)
    {
      if ((void*)&r != (void*)this)
      {
	if (owner) 
	{
	  if (array)
	    delete [] ptr;
	  else
	    delete ptr;
	}
	ptr = r.ptr;
	if (r.locked)
	  owner = 0;
	else
	{
	  owner = r.owner; 
	  r.owner = 0;
	}
      }
      return *this;
    }

} // namespace libcwd

#endif // LIBCWD_LOCKABLE_AUTO_PTR_H
