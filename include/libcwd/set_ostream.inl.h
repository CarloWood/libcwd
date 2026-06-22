// SPDX-FileCopyrightText: 2002-2004, 2018-2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_SET_OSTREAM_INL
#define LIBCWD_SET_OSTREAM_INL

#include "private/LockInterface.h"

namespace libcwd {

/**
 * \brief Set output device and provide external lock.
 * \ingroup group_destination
 *
 * Assign a new \c ostream to this %debug object.
 * The \c ostream will only be written to after obtaining the lock
 * that is passed as second argument.  Each \c ostream needs to have
 * a unique lock. If the application also writes directly
 * to the same \c ostream then use the same lock.
 *
 * **Example:**
 *
 * ```cpp
 * MyLock lock;
 *
 * // Uses MyLock::lock(), MyLock::try_lock() and MyLock::unlock().
 * Debug( libcw_do.set_ostream(&std::cerr, &lock) );
 *
 * lock.lock();
 * std::cerr << "The application uses cerr too\n";
 * lock.unlock();
 * ```
 */
template <class T>
void DebugObject::set_ostream(std::ostream* os, T* mutex)
{
  _private_::LockInterfaceBase* new_mutex = new _private_::LockInterface<T>(mutex);
  _private_::LockInterfaceBase* old_mutex;
  old_mutex = ostream_state_.replace_with(os, new_mutex);
  // Delete old_mutex after unlocking in order to avoid a dead lock in case the delete causes debug output.
  if (old_mutex)
    delete old_mutex;
}

} // namespace libcwd

#endif // LIBCWD_SET_OSTREAM_INL
