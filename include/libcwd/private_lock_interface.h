// SPDX-FileCopyrightText: 2002-2005, 2018-2019, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file private_lock_interface.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_LOCK_INTERFACE_H
#define LIBCWD_PRIVATE_LOCK_INTERFACE_H

namespace libcwd::_private_ {

class LockInterfaceBase {
public:
  virtual int try_lock() = 0;
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual ~LockInterfaceBase() { }
};

template<class T>
  class lock_interface_tct : public LockInterfaceBase {
    private:
      T* ptr;
      virtual int try_lock() { return ptr->try_lock(); }
      virtual void lock() { ptr->lock(); }
      virtual void unlock() { ptr->unlock(); }
    public:
      lock_interface_tct(T* mutex) : ptr(mutex) { }
  };

}  // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_LOCK_INTERFACE_H
