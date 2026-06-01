#ifndef LIBCWD_CLASS_CHANNEL_SET_INL
#define LIBCWD_CLASS_CHANNEL_SET_INL

#ifndef LIBCWD_CLASS_CHANNEL_SET_H
#include "class_channel_set.h"
#endif
#ifndef LIBCWD_CLASS_CHANNEL_H
#include "class_channel.h"
#endif
#ifndef LIBCWD_CLASS_FATAL_CHANNEL_H
#include "class_fatal_channel.h"
#endif
#ifndef LIBCWD_CONTROL_FLAG_H
#include "control_flag.h"
#endif
#ifndef LIBCWD_CLASS_CHANNEL_INL
#include "class_channel.inl"
#endif
#ifndef LIBCWD_CLASS_FATAL_CHANNEL_INL
#include "class_fatal_channel.inl"
#endif

namespace libcwd {

inline
continued_channel_set_st&
continued_channel_set_st::operator|(control_flag_t cf)
{
  mask |= cf;
  return *this;
}

inline
channel_set_st&
channel_set_st::operator|(control_flag_t cf)
{
  mask |= cf;
  return *this;
}

inline
channel_set_st&
channel_set_st::operator|(channel_ct const& dc)
{
  if (!on)
  {
    label = dc.get_label();
    on = dc.is_on();
  }
  return *this;
}

inline
channel_set_st&
channel_set_st::operator|(fatal_channel_ct const& fdc)
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

