#pragma once

#ifndef LIBCWD_CLASS_CHANNEL_SET_INL
#define LIBCWD_CLASS_CHANNEL_SET_INL

#include "Channel.inl.h"
#include "FatalChannel.inl.h"
#include "Channel.h"
#include "ChannelSet.h"
#include "FatalChannel.h"
#include "control_flag.h"

namespace libcwd {

inline ContinuedChannelSet& ContinuedChannelSet::operator|(control_flag_t cf)
{
  mask |= cf;
  return *this;
}

inline ChannelSet& ChannelSet::operator|(control_flag_t cf)
{
  mask |= cf;
  return *this;
}

inline ChannelSet& ChannelSet::operator|(Channel const& dc)
{
  if (!on)
  {
    label = dc.get_label();
    on = dc.is_on();
  }
  return *this;
}

inline ChannelSet& ChannelSet::operator|(FatalChannel const& fdc)
{
  mask |= fdc.get_maskbit();
  if (!on)
  {
    label = fdc.get_label();
    on = true;
  }
  return *this;
}

} // namespace libcwd

#endif // LIBCWD_CLASS_CHANNEL_SET_INL
