#ifndef LIBCWD_CLASS_CONTINUED_CHANNEL_INL
#define LIBCWD_CLASS_CONTINUED_CHANNEL_INL

#include "class_continued_channel.h"
#include "control_flag.h"

namespace libcwd {

inline
continued_channel_ct::continued_channel_ct(control_flag_t maskbit)
{
  NS_initialize(maskbit);
}

inline
control_flag_t
continued_channel_ct::get_maskbit() const
{
  return WNS_maskbit;
}

} // namespace libcwd

#endif // LIBCWD_CLASS_CONTINUED_CHANNEL_INL
