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

#ifndef LIBCWD_SET_OSTREAM_INL
#define LIBCWD_SET_OSTREAM_INL

#ifndef LIBCWD_PRIVATE_LOCK_INTERFACE_H
#include "private_lock_interface.h"
#endif
#ifndef LIBCWD_PRIVATE_THREADING_H
#include "private_threading.h"
#endif

#if LIBCWD_THREAD_SAFE || defined(LIBCWD_DOXYGEN)
namespace libcwd {

/**
 * \brief Set output device and provide external lock.
 * \ingroup group_destination
 *
 * Assign a new \c ostream to this %debug object.&nbsp;
 * The \c ostream will only be written to after obtaining the lock
 * that is passed as second argument.  Each \c ostream needs to have
 * a unique lock.&nbsp; If the application also writes directly
 * to the same \c ostream then use the same lock.
 *
 * <b>Example:</b>
 *
 * \code
 * MyLock lock;
 *
 * // Uses MyLock::lock(), MyLock::try_lock() and MyLock::unlock().
 * Debug( libcw_do.set_ostream(&std::cerr, &lock) );
 *
 * lock.lock();
 * std::cerr << "The application uses cerr too\n";
 * lock.unlock();
 * \endcode
 */
template<class T>
  void debug_ct::set_ostream(std::ostream* os, T* mutex)
  {
    LIBCWD_TSD_DECLARATION;
    _private_::set_alloc_checking_off(LIBCWD_TSD);
    _private_::lock_interface_base_ct* new_mutex = new _private_::lock_interface_tct<T>(mutex);
    _private_::set_alloc_checking_on(LIBCWD_TSD);
    LIBCWD_DEFER_CANCEL;
    _private_::mutex_tct<_private_::set_ostream_instance>::lock();
    _private_::lock_interface_base_ct* old_mutex = M_mutex;
    if (old_mutex)
      old_mutex->lock();		// Make sure all other threads left this critical area.
    M_mutex = new_mutex;
    if (old_mutex)
      old_mutex->unlock();
    private_set_ostream(os);
    _private_::mutex_tct<_private_::set_ostream_instance>::unlock();
    // Delete old_mutex after unlocking in order to avoid a dead lock in case the delete causes debug output.
    if (old_mutex)
    {
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      delete old_mutex;
      _private_::set_alloc_checking_on(LIBCWD_TSD);
    }
    LIBCWD_RESTORE_CANCEL;
  }

}  // namespace libcwd

#endif // LIBCWD_THREAD_SAFE
#endif // LIBCWD_SET_OSTREAM_INL

