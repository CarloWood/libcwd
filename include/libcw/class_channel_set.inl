// $Header$
//
// Copyright (C) 2000 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCW_CLASS_CHANNEL_SET_INL
#define LIBCW_CLASS_CHANNEL_SET_INL

#ifndef LIBCW_CLASS_CHANNEL_SET_H
#include <libcw/class_channel_set.h>
#endif
#ifndef LIBCW_CLASS_CHANNEL_H
#include <libcw/class_channel.h>
#endif
#ifndef LIBCW_CLASS_FATAL_CHANNEL_H
#include <libcw/class_fatal_channel.h>
#endif
#ifndef LIBCW_CONTROL_FLAG_H
#include <libcw/control_flag.h>
#endif
#ifndef LIBCW_CLASS_CHANNEL_INL
#include <libcw/class_channel.inl>
#endif
#ifndef LIBCW_CLASS_FATAL_CHANNEL_INL
#include <libcw/class_fatal_channel.inl>
#endif

namespace libcw {
  namespace debug {

__inline__
continued_channel_set_st&
continued_channel_set_st::operator|(control_flag_t cf)
{
  mask |= cf;
  return *this;
}

__inline__
channel_set_st&
channel_set_st::operator|(control_flag_t cf)
{
  mask |= cf;
  return *this;
}

__inline__
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

__inline__
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

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_CHANNEL_SET_INL

