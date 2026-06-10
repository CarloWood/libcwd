#pragma once

#ifndef LIBCWD_CLASS_ALWAYS_CHANNEL_INL
#define LIBCWD_CLASS_ALWAYS_CHANNEL_INL

#include "AlwaysChannel.h"
#include "control_flag.h"

namespace libcwd {

inline ChannelSet& ChannelSetBootstrap::operator|(AlwaysChannel const& adc)
{
  mask = 0;
  label = adc.label;
  on = true;
  return *reinterpret_cast<ChannelSet*>(this);
}

} // namespace libcwd

#endif // LIBCWD_CLASS_ALWAYS_CHANNEL_INL
