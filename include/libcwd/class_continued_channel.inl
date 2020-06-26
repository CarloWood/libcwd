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

#ifndef LIBCWD_CLASS_CONTINUED_CHANNEL_INL
#define LIBCWD_CLASS_CONTINUED_CHANNEL_INL

#ifndef LIBCWD_CLASS_CONTINUED_CHANNEL_H
#include "class_continued_channel.h"
#endif
#ifndef LIBCWD_CONTROL_FLAG_H
#include "control_flag.h"
#endif

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
