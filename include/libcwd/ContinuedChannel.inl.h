// SPDX-FileCopyrightText: 2001-2004, 2018, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_CLASS_CONTINUED_CHANNEL_INL
#define LIBCWD_CLASS_CONTINUED_CHANNEL_INL

#include "ContinuedChannel.h"
#include "control_flag.h"

namespace libcwd {

inline ContinuedChannel::ContinuedChannel(control_flag_t maskbit)
{
  NS_initialize(maskbit);
}

inline control_flag_t ContinuedChannel::get_maskbit() const
{
  return maskbit_;
}

} // namespace libcwd

#endif // LIBCWD_CLASS_CONTINUED_CHANNEL_INL
