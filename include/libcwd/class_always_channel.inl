// $Header$
//
// Copyright (C) 2002 - 2004, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_CLASS_ALWAYS_CHANNEL_INL
#define LIBCWD_CLASS_ALWAYS_CHANNEL_INL

#ifndef LIBCWD_CLASS_ALWAYS_CHANNEL_H
#include <libcwd/class_always_channel.h>
#endif
#ifndef LIBCWD_CONTROL_FLAG_H
#include <libcwd/control_flag.h>
#endif

namespace libcwd {

__inline__
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
