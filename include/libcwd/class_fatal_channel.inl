#ifndef LIBCWD_CLASS_FATAL_CHANNEL_INL
#define LIBCWD_CLASS_FATAL_CHANNEL_INL

#include "class_fatal_channel.h"
#include "control_flag.h"

namespace libcwd {

inline
fatal_channel_ct::fatal_channel_ct(char const* label, control_flag_t maskbit)
{
  LIBCWD_TSD_DECLARATION;
  NS_initialize(label, maskbit LIBCWD_COMMA_TSD);
}

inline
control_flag_t
fatal_channel_ct::get_maskbit() const
{
  return WNS_maskbit;
}

inline
char const*
fatal_channel_ct::get_label() const
{
  return WNS_label;
}

} // namespace libcwd

#endif // LIBCWD_CLASS_FATAL_CHANNEL_INL
