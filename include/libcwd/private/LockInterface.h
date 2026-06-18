// SPDX-FileCopyrightText: 2002-2005, 2018-2019, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** @file
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_LOCK_INTERFACE_H
#define LIBCWD_PRIVATE_LOCK_INTERFACE_H

#ifndef HIDE_FROM_DOXYGEN
namespace libcwd::_private_ {

class LockInterfaceBase
{
 public:
  virtual int try_lock() = 0;
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual ~LockInterfaceBase() { }
};

template <class T>
class LockInterface : public LockInterfaceBase
{
 private:
  T* mutex_;
  virtual int try_lock() { return mutex_->try_lock(); }
  virtual void lock() { mutex_->lock(); }
  virtual void unlock() { mutex_->unlock(); }

 public:
  LockInterface(T* mutex) : mutex_(mutex) { }
};

} // namespace libcwd::_private_
#endif // HIDE_FROM_DOXYGEN

#endif // LIBCWD_PRIVATE_LOCK_INTERFACE_H
