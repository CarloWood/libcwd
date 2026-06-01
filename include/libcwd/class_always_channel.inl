#ifndef LIBCWD_CLASS_ALWAYS_CHANNEL_INL
#define LIBCWD_CLASS_ALWAYS_CHANNEL_INL

#include "class_always_channel.h"
#include "control_flag.h"

namespace libcwd {

inline
channel_set_st&
channel_set_bootstrap_st::operator|(always_channel_ct const& adc)
{
  mask = 0;
  label = adc.label;
  on = true;
  return *reinterpret_cast<channel_set_st*>(this);
}

} // namespace libcwd

#endif // LIBCWD_CLASS_ALWAYS_CHANNEL_INL
