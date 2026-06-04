#ifndef LIBCWD_SET_OSTREAM_INL
#define LIBCWD_SET_OSTREAM_INL

#include "private_lock_interface.h"
#include "private_threading.h"

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
    _private_::lock_interface_base_ct* new_mutex = new _private_::lock_interface_tct<T>(mutex);
    _private_::lock_interface_base_ct* old_mutex;
    LIBCWD_DEFER_CANCEL;
    {
      _private_::ostream_state_ts::wat ostream_state_w(ostream_state_);
      old_mutex = ostream_state_w->mutex;
      ostream_state_w->mutex = new_mutex;
      private_set_ostream(*ostream_state_w, os);
      if (old_mutex)
      {
        old_mutex->lock();		// Make sure all other threads left this critical area.
        old_mutex->unlock();
      }
    }
    LIBCWD_RESTORE_CANCEL;
    // Delete old_mutex after unlocking in order to avoid a dead lock in case the delete causes debug output.
    if (old_mutex)
      delete old_mutex;
  }

}  // namespace libcwd

#endif // LIBCWD_SET_OSTREAM_INL
