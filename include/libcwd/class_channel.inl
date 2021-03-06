// $Header$
//
// Copyright (C) 2000 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_CLASS_CHANNEL_INL
#define LIBCWD_CLASS_CHANNEL_INL

#ifndef LIBCWD_CLASS_CHANNEL_H
#include "class_channel.h"
#endif
#ifndef LIBCWD_PRIVATE_THREADING_H
#include "private_threading.h"
#endif

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

#if LIBCWD_THREAD_SAFE
inline
bool
channel_ct::is_on(LIBCWD_TSD_PARAM) const
{
  return (__libcwd_tsd.off_cnt_array[WNS_index] < 0);
}
#endif


/**
 * \brief Returns `true' if the channel is active.
 */
inline
bool
channel_ct::is_on() const
{
#if LIBCWD_THREAD_SAFE
  LIBCWD_TSD_DECLARATION;
  return is_on(LIBCWD_TSD);
#else
  return (off_cnt < 0);
#endif
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

