// $Header$
//
// Copyright (C) 2000 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCW_CLASS_FATAL_CHANNEL_INL
#define LIBCW_CLASS_FATAL_CHANNEL_INL

#ifndef LIBCW_CLASS_FATAL_CHANNEL_H
#include <libcwd/class_fatal_channel.h>
#endif
#ifndef LIBCW_CONTROL_FLAG_H
#include <libcwd/control_flag.h>
#endif

namespace libcw {
  namespace debug {

__inline__
fatal_channel_ct::fatal_channel_ct(char const* label, control_flag_t maskbit)
{
  NS_initialize(label, maskbit);
}

__inline__
control_flag_t
fatal_channel_ct::get_maskbit(void) const
{ 
  return WNS_maskbit;
}

__inline__
char const*
fatal_channel_ct::get_label(void) const
{ 
  return WNS_label;
}

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_FATAL_CHANNEL_INL
