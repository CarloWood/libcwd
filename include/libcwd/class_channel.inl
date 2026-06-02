#ifndef LIBCWD_CLASS_CHANNEL_INL
#define LIBCWD_CLASS_CHANNEL_INL

#include "class_channel.h"
#include "private_threading.h"

namespace libcwd {

/**
 * \brief Construct a new %debug channel with name \a label.
 *
 * A newly created channel is off by default (except \ref libcwd::channels::dc::warning "dc::warning").&nbsp;
 * All channel objects must be global objects.
 *
 * \sa \ref chapter_custom_debug_h
 */
inline
channel_ct::channel_ct(char const* label, bool add_to_channel_list)
{
  LIBCWD_TSD_DECLARATION;
  NS_initialize(label LIBCWD_COMMA_TSD, add_to_channel_list);
}

inline
bool
channel_ct::is_on(LIBCWD_TSD_PARAM) const
{
  return (__libcwd_tsd.off_cnt_array[WNS_index] < 0);
}

/**
 * \brief Returns `true' if the channel is active.
 */
inline
bool
channel_ct::is_on() const
{
  LIBCWD_TSD_DECLARATION;
  return is_on(LIBCWD_TSD);
}

/**
 * \brief Pointer to the label of the %debug channel.
 */
inline
char const*
channel_ct::get_label() const
{
  return WNS_label;
}

} // namespace libcwd

#endif // LIBCWD_CLASS_CHANNEL_INL

