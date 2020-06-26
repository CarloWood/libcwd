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

#ifndef LIBCWD_CLASS_FATAL_CHANNEL_INL
#define LIBCWD_CLASS_FATAL_CHANNEL_INL

#ifndef LIBCWD_CLASS_FATAL_CHANNEL_H
#include "class_fatal_channel.h"
#endif
#ifndef LIBCWD_CONTROL_FLAG_H
#include "control_flag.h"
#endif

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
