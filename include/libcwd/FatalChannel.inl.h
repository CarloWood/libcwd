#pragma once

#ifndef LIBCWD_CLASS_FATAL_CHANNEL_INL
#define LIBCWD_CLASS_FATAL_CHANNEL_INL

#include "FatalChannel.h"
#include "control_flag.h"

namespace libcwd {

inline FatalChannel::FatalChannel(char const* label, control_flag_t maskbit)
{
  LIBCWD_TSD_DECLARATION;
  NS_initialize(label, maskbit, LIBCWD_TSD);
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
