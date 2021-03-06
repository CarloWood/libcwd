// $Header$
//
// Copyright (C) 2002 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file private_lock_interface.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_LOCK_INTERFACE_H
#define LIBCWD_PRIVATE_LOCK_INTERFACE_H

#if LIBCWD_THREAD_SAFE
namespace libcwd {
  namespace _private_ {

class lock_interface_base_ct {
public:
  virtual int try_lock() = 0;
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual ~lock_interface_base_ct() { }
};

template<class T>
  class lock_interface_tct : public lock_interface_base_ct {
    private:
      T* ptr;
      virtual int try_lock() { return ptr->try_lock(); }
      virtual void lock() { ptr->lock(); }
      virtual void unlock() { ptr->unlock(); }
    public:
      lock_interface_tct(T* mutex) : ptr(mutex) { }
  };

class pthread_lock_interface_ct : public lock_interface_base_ct {
  private:
    pthread_mutex_t* ptr;
    virtual int try_lock();
    virtual void lock();
    virtual void unlock();
  public:
    pthread_lock_interface_ct(pthread_mutex_t* mutex) : ptr(mutex) { }
};

  } // namespace _private_
}  // namespace libcwd

#endif // LIBCWD_THREAD_SAFE
#endif // LIBCWD_PRIVATE_LOCK_INTERFACE_H

