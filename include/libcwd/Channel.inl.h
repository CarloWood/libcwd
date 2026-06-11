// SPDX-FileCopyrightText: 2001-2004, 2018, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_CLASS_CHANNEL_INL
#define LIBCWD_CLASS_CHANNEL_INL

#include "Channel.h"

namespace libcwd {

/**
 * \brief Construct a new %debug channel with name \a label.
 *
 * A newly created channel is off by default (except \ref libcwd::channels::dc::warning "dc::warning").&nbsp;
 * All channel objects must be global objects.
 *
 * \sa \ref chapter_custom_debug_h
 */
inline Channel::Channel(char const* label, bool add_to_channel_list)
{
  LIBCWD_TSD_DECLARATION;
  NS_initialize(label, LIBCWD_TSD, add_to_channel_list);
}

inline bool Channel::is_on(LIBCWD_TSD_PARAM) const
{
  return (__libcwd_tsd.off_cnt_array[index_] < 0);
}

/**
 * \brief Returns `true` if the channel is active.
 */
inline bool Channel::is_on() const
{
  LIBCWD_TSD_DECLARATION;
  return is_on(LIBCWD_TSD);
}

/**
 * \brief Pointer to the label of the %debug channel.
 */
inline char const* Channel::get_label() const
{
  return label_;
}

} // namespace libcwd

#endif // LIBCWD_CLASS_CHANNEL_INL
