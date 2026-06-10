#ifndef LIBCWD_CLASS_DEBUG_INL
#define LIBCWD_CLASS_DEBUG_INL

#include "class_debug.h"
#include "class_channel.h"
#include "class_fatal_channel.h"
#include "Channel.inl.h"
#include "FatalChannel.inl.h"
#include "DebugString.inl.h"
#include "core_dump.h"

#include <ostream>

namespace libcwd {

/** \addtogroup group_formatting */
/** \{ */

inline
DebugString&
DebugObject::color_on()
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(color_on);
}

inline
DebugString const&
DebugObject::color_on() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(color_on);
}

inline
DebugString&
DebugObject::color_off()
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(color_off);
}

inline
DebugString const&
DebugObject::color_off() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(color_off);
}

inline
DebugString&
DebugObject::margin()
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(margin);
}

inline
DebugString const&
DebugObject::margin() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(margin);
}

inline
DebugString&
DebugObject::marker()
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(marker);
}

inline
DebugString const&
DebugObject::marker() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(marker);
}

/**
 * \brief Set number of spaces to indent.
 */
inline
void
DebugObject::set_indent(unsigned short i)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_TSD_MEMBER(indent) = i;
}

/**
 * \brief Increment number of spaces to indent.
 */
inline
void
DebugObject::inc_indent(unsigned short i)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_TSD_MEMBER(indent) += i;
}

/**
 * \brief Decrement number of spaces to indent.
 */
inline
void
DebugObject::dec_indent(unsigned short i)
{
  LIBCWD_TSD_DECLARATION;
  int prev_indent = LIBCWD_TSD_MEMBER(indent);
  LIBCWD_TSD_MEMBER(indent) = (i > prev_indent) ? 0 : (prev_indent - i);
}

/**
 * \brief Get the current indentation.
 */
inline
unsigned short
DebugObject::get_indent() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(indent);
}

/** \} */

/** \addtogroup group_destination */
/** \{ */

/**
 * \brief Get the \c ostream device as set with set_ostream().
 */
inline
std::ostream*
DebugObject::get_ostream() const
{
  std::ostream* real_os_ptr;
  real_os_ptr = ostream_state_.read_real_os();
  return real_os_ptr;
}

inline
bool
DebugObject::has_mutex() const
{
  bool has_mutex;
  has_mutex = ostream_state_.has_mutex();
  return has_mutex;
}

inline
_private_::LockInterfaceBase*
_private_::OstreamState::replace_with(std::ostream* os, LockInterfaceBase* new_mutex)
{
  std::lock_guard<std::mutex> lock(state_mutex_);
  LockInterfaceBase* old_mutex = mutex_;
  mutex_ = new_mutex;
  real_os_ = os;
  if (old_mutex)
  {
    // LOCK ORDER: state_mutex_ -> LockInterfaceBase
    old_mutex->lock();		// Make sure all other threads left this critical area.
    old_mutex->unlock();
  }
  return old_mutex;
}

inline
void
_private_::OstreamState::set_ostream(std::ostream* os)
{
  std::lock_guard<std::mutex> lock(state_mutex_);
  LockInterfaceBase* old_mutex = mutex_;
  real_os_ = os;
  if (old_mutex)
  {
    // LOCK ORDER: state_mutex_ -> LockInterfaceBase
    old_mutex->lock();		// Make sure all other threads left this critical area.
    old_mutex->unlock();
  }
}

inline
std::ostream*
_private_::OstreamState::read_real_os() const
{
  std::lock_guard<std::mutex> lock(state_mutex_);
  return real_os_;
}

inline
bool
_private_::OstreamState::has_mutex() const
{
  std::lock_guard<std::mutex> lock(state_mutex_);
  return mutex_ != nullptr;
}

inline
std::ostream*
_private_::OstreamState::get_locked_os(std::ostream* os, LockInterfaceBase** locked_mutex_out) const
{
  std::lock_guard<std::mutex> lock(state_mutex_);
  std::ostream* locked_os = os ? os : real_os_;
  *locked_mutex_out = mutex_;
  if (mutex_)
  {
    // LOCK ORDER: state_mutex_ -> LockInterfaceBase
    mutex_->lock();
  }
  return locked_os;
}

inline
bool
_private_::OstreamState::try_lock_os(std::ostream* os, std::ostream** locked_os_out,
                                         LockInterfaceBase** locked_mutex_out) const
{
  std::lock_guard<std::mutex> lock(state_mutex_);
  // LOCK ORDER: state_mutex_ -> LockInterfaceBase
  if (mutex_ && mutex_->try_lock())
    return false;
  *locked_os_out = os ? os : real_os_;
  *locked_mutex_out = mutex_;
  return true;
}

inline
void
_private_::OstreamState::write_color_off_newline(std::ostream* os, char const* color_off,
                                                     std::size_t color_off_size) const
{
  std::lock_guard<std::mutex> lock(state_mutex_);
  std::ostream* target_os = os ? os : real_os_;
  if (color_off_size > 0)
    target_os->write(color_off, color_off_size);
  target_os->put('\n');
}

/** \} */

/**
 * \brief Constructor
 *
 * A %debug object must be global.
 *
 * \sa group_debug_object
 * \sa chapter_custom_do
 */
inline
DebugObject::DebugObject()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUG
  if (!NS_init(LIBCWD_TSD))
    core_dump();
#else
  [[maybe_unused]] bool success = NS_init(LIBCWD_TSD);
#endif
}

/**
 * \brief Turn this %debug object off.
 */
inline
void
DebugObject::off()
{
  LIBCWD_TSD_DECLARATION;
  ++LIBCWD_TSD_MEMBER_OFF;
}

/**
 * \brief Cancel last call to off().
 *
 * Calls to off() and on() has to be done in pairs (first off() then on()).
 * These pairs can be nested.
 *
 * <b>Example:</b>
 *
 * \code
 * int i = 0;
 * Debug( libcw_do.off() );
 * Dout( dc::notice, "Adding one to " << i++ );
 * Debug( libcw_do.on() );
 * Dout( dc::notice, "i == " << i );
 * \endcode
 *
 * Outputs:
 *
 * <PRE class="example-output">NOTICE : i == 0
 * </PRE>
 *
 * Note that the statement <CODE>i++</CODE> was never executed.
 */
inline
void
DebugObject::on()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGOUTPUT
  if (LIBCWD_TSD_MEMBER(first_time) && LIBCWD_TSD_MEMBER_OFF == -1)
    LIBCWD_TSD_MEMBER(first_time) = false;
  else
    --LIBCWD_TSD_MEMBER_OFF;
#else
  --LIBCWD_TSD_MEMBER_OFF;
#endif
}

inline
bool
DebugObject::is_on(LIBCWD_TSD_PARAM) const
{
  return __libcwd_tsd.debug_object_off_array[index_] == -1;
}

/**
 * \brief Turn always-flush for this %debug object on.
 */
inline
void
DebugObject::always_flush_on()
{
  ++always_flush_;
}

/**
 * \brief Cancel last call to always_flush_on().
 *
 * Calls to always_flush_on() and always_flush_off() has to be done in pairs (first on() then off()).
 * These pairs can be nested.
 *
 * <b>Example:</b>
 *
 * \code
 * Debug( libcw_do.always_flush_on() );
 * Dout( dc::notice, "This is flushed.");
 * Debug( libcw_do.always_flush_off() );
 * \endcode
 */
inline
void
DebugObject::always_flush_off()
{
#if CWDEBUG_DEBUG
  if (always_flush_ <= 0)
    core_dump();
#endif
  --always_flush_;
}

inline
bool
DebugObject::always_flush_is_on() const
{
  return always_flush_ > 0;   // 0 means off.
}

inline
ChannelSet&
ChannelSetBootstrap::operator|(Channel const& dc)
{
  mask = 0;
  label = dc.get_label();
  on = dc.is_on();
  return *reinterpret_cast<ChannelSet*>(this);
}

inline
ChannelSet&
FatalChannelSetBootstrap::operator|(FatalChannel const& fdc)
{
  mask = fdc.get_maskbit();
  label = fdc.get_label();
  on = true;
  return *reinterpret_cast<ChannelSet*>(this);
}

} // namespace libcwd

#endif // LIBCWD_CLASS_DEBUG_INL
