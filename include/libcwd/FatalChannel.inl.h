// SPDX-FileCopyrightText: 2001-2004, 2018, 2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_CLASS_FATAL_CHANNEL_INL
#define LIBCWD_CLASS_FATAL_CHANNEL_INL

#include "FatalChannel.h"
#include "control_flag.h"

namespace libcwd {

inline FatalChannel::FatalChannel(char const* label, control_flag_t maskbit)
{
  NS_initialize(label, maskbit);
}

inline control_flag_t FatalChannel::get_maskbit() const
{
  return maskbit_;
}

inline char const* FatalChannel::get_label() const
{
  return label_;
}

} // namespace libcwd

#endif // LIBCWD_CLASS_FATAL_CHANNEL_INL
