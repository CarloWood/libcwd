/**
 * ai-utils -- C++ Core utilities
 *
 * @file
 * @brief Definition of class AIRefCount.
 *
 * @Copyright (C) 2017  Carlo Wood.
 *
 * pub   dsa3072/C155A4EEE4E527A2 2018-08-16 Carlo Wood (CarloWood on Libera) <carlo@alinoe.com>
 * fingerprint: 8020 B266 6305 EE2F D53E  6827 C155 A4EE E4E5 27A2
 *
 * This file is part of ai-utils.
 *
 * ai-utils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ai-utils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ai-utils.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

//#include "utils/FuzzyBool.h"
#include <atomic>
#include <boost/intrusive_ptr.hpp>
#include "debug.h"

// Usage:
//
// class MyClass : public AIRefCount
// {
// };
//
// int main()
// {
//   boost::intrusive_ptr<MyClass> my_class = new MyClass;      // m_count = 1
//   {
//     MyClass* p = my_class.get();
//     boost::intrusive_ptr<MyClass> ip(p);                     // m_count = 2
//   }                                                          // m_count = 1
//   my_class->print();
// }                                                            // m_count = 0 --> destruction of MyClass.
//
//
// Note 1
//
// If one wishes to make sure that an object is (temporarily) not deleted,
// but cannot use a boost::intrusive_ptr<MyClass> because there is no scope
// where it can be created and destructed, then do NOT call
// intrusive_ptr_add_ref(ptr) / intrusive_ptr_release(ptr)
// as a pair to achieve that.
//
// Instead, use ptr->inhibit_deletion() / ptr->allow_deletion() as a pair.
//
//
// Note 2
//
// An object that is derived from AIRefCount should only ever be created on the
// heap, aka by a call to new. If the resulting pointer is not passed a
// boost::intrusive_ptr<MyClass> then the reference counter m_count remains zero.
// This means that most of the API cannot be used anymore.
//
// The only member function that is still guaranteed to work is the debug
// function is_destructed().
//
// On the other hand; it is allowed to "fake" a boost::intrusive_ptr<MyClass>
// by simply calling intrusive_ptr_add_ref() after creation. In that case
// everything works as expected, except that the object can be 'released'
// by doing a call to intrusive_ptr_release() (once!).
//
// In other words, the following is allowed:
//
//   MyClass* ptr = new MyClass;        // m_count = 0
//   intrusive_ptr_add_ref(ptr);        // m_count = 1
//
// And then later, somewhere else, do:
//
//   intrusive_ptr_release(ptr);        // Cancel initial call to intrusive_ptr_add_ref.
//                                      // This *might* cause ptr to be destructed immediately.

// Forward declare intrusive_ptr_add_ref and intrusive_ptr_release because
// that gives a much clearer compile error on g++ when the class used in
// a boost::intrusive_ptr was only forward declared (and not fully defined)
// at the point the boost::intrusive_ptr is being used.
class AIRefCount;
inline void intrusive_ptr_add_ref(AIRefCount const* ptr);
inline void intrusive_ptr_release(AIRefCount const* ptr);

class AIRefCount
{
 private:
#if CW_DEBUG
  static constexpr int s_deleted = -0x6de1e7ed;                 // A negative magic number.
#endif
  mutable std::atomic<int> m_count;

 public:
  friend void intrusive_ptr_add_ref(AIRefCount const* ptr)
  {
    ptr->m_count.fetch_add(1, std::memory_order_relaxed);
  }

  friend void intrusive_ptr_release(AIRefCount const* ptr)
  {
    if (ptr->m_count.fetch_sub(1, std::memory_order_release) == 1)
    {
      std::atomic_thread_fence(std::memory_order_acquire);
      DEBUG_ONLY(ptr->m_count = s_deleted);
      delete ptr;
    }
  }

  // Increment m_count; returns the previous value (mainly for debugging purposes).
  int inhibit_deletion(DEBUG_ONLY(bool can_cause_immediate_allow_deletion = true)) const
  {
    int prev_count = m_count.fetch_add(1, std::memory_order_relaxed);
    // Because m_count is overwritten with s_deleted upon destruction when CWDEBUG is defined, a reference count of
    // zero means that this object is probably still being constructed and wasn't passed to a boost::intrusive_ptr _yet_.
    // If under those circumstances an intrusive_ptr_add_ref/intrusive_ptr_release pair,
    // that is merely intended to temporarily inhibit deletion, could inadvertently delete
    // the object while it is still being constructed (at least, before we even got
    // the chance to pass it to the first boost::intrusive_ptr).
    // This test can help in detecting that it was not a good idea to call inhibit_deletion
    // if that can result in a subsequent call to allow_deletion() before that first
    // boost::intrusive_ptr is created. If that is not possible then you can pass
    // false to this function.
    //
    // Normally, when this assert fires you should find the function that was
    // called from the constructor of the derived class that caused inhibit_deletion
    // to be called, and delay that call to after you have a boost::intrusive_ptr.
    //
    // For example,
    //
    // struct A : public AIRefCount {
    //   void f() { inhibit_deletion(); }
    //   A() { f(); }
    // };
    //
    // boost::intrusive_ptr<A> p = new A;
    //
    // Should become,
    //
    // struct A : public AIRefCount {
    //   void f() { inhibit_deletion(); }
    //   A() { }
    // };
    //
    // boost::intrusive_ptr<A> p = new A;
    // p->f();
    //
    // If you are using the evio submodule and this object is derived from evio::FileDescriptor
    // then you probably created a device without assigning it to a boost::intrusive_ptr at all
    // (e.g: MyDevice my_device;) Don't do that. Devices should be created like:
    //
    // auto my_device = evio::create<MyDevice>(/* device constructor arguments */);
    //
    // where MyDevice is something like (derived from) File, Socket etc (InputDevice and/or OutputDevice).
    ASSERT(!can_cause_immediate_allow_deletion || prev_count > 0);
    return prev_count;
  }

  // Balance a call to inhibit_deletion(). Decrements m_count; returns the previous value (mainly for debugging purposes).
  // If defer_delete is true however, then the object must be deleted by the caller iff the returned value is 1.
  int allow_deletion(bool defer_delete = false, int count = 1) const
  {
    // Do not call this function with a count of 0.
    ASSERT(count > 0);
    int prev_count = m_count.fetch_sub(count, std::memory_order_release);
    // Paranoia check. This should never fail.
    ASSERT(count <= prev_count);
    if (prev_count == count)
    {
      std::atomic_thread_fence(std::memory_order_acquire);
      if (!defer_delete)
      {
        DEBUG_ONLY(m_count = s_deleted);
        delete this;
      }
    }
    return prev_count;
  }

 private:
  // You should use inhibit_deletion / allow_deletion.
  void intrusive_ptr_add_ref(AIRefCount const*);
  void intrusive_ptr_release(AIRefCount const*);

 protected:
  AIRefCount() : m_count(0) { }
  AIRefCount(AIRefCount const&) : m_count(0) { }
  virtual ~AIRefCount() { }
  AIRefCount& operator=(AIRefCount const&) { return *this; }
  void swap(AIRefCount&) { }

 public:
  // Returns true if there is only one reference to this object left.
  // If this function returns true it is therefore guaranteed to stay true,
  // but if it returns false it might become true shortly afterwards.
//  utils::FuzzyBool unique() const { return std::atomic_load_explicit(&m_count, std::memory_order_relaxed) == 1 ? fuzzy::True : fuzzy::WasFalse; }

#if CW_DEBUG
  // Pretty unreliable, but sometimes useful.
  bool is_destructed() const { int cnt = std::atomic_load_explicit(&m_count, std::memory_order_relaxed); return cnt < 0; }
  // Used when deferred deleting an object.
  void mark_deleted() const { m_count = s_deleted; }
#endif
#ifdef CWDEBUG
  // Purely for debug code. The returned value suffers from race conditions and is not stable.
  int read_count_racy() const
  {
    return m_count.load(std::memory_order_relaxed);
  }
#endif
};
