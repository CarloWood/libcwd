// Generated automatically from sys.h.in by configure.
// $Header$
//
// Copyright (C) 2001 - 2003, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef RAW_WRITE_INL
#define RAW_WRITE_INL

#if CWDEBUG_DEBUG

#ifndef LIBCWD_PRIVATE_INTERNAL_STRING_H
#include <libcwd/private_internal_string.h>
#endif

namespace libcw {
  namespace debug {

__inline__
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, char const* data)
{
  write(2, data, strlen(data));
  return raw_write;
}

__inline__
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, void const* data)
{
  size_t dat = (size_t)data;
  write(2, "0x", 2);
  char c[11];
  char* p = &c[11];
  do
  {
    int d = (dat % 16);
    *--p = ((d < 10) ? '0' : ('a' - 10)) + d;
    dat /= 16;
  }
  while(dat > 0);
  write(2, p, &c[11] - p);
  return raw_write;
}

__inline__ _private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, bool data)
{
  if (data)
    write(2, "true", 4);
  else
    write(2, "false", 5);
  return raw_write;
}

__inline__ _private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, char data)
{
  char c[1];
  c[0] = data;
  write(2, c, 1);
  return raw_write;
}

__inline__ _private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, unsigned long data)
{
  char c[11];
  char* p = &c[11];
  do
  {
    *--p = '0' + (data % 10);
    data /= 10;
  }
  while(data > 0);
  write(2, p, &c[11] - p);
  return raw_write;
}

__inline__
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, long data)
{
  if (data < 0)
  {
    write(2, "-", 1);
    data = -data;
  }
  return operator<<(raw_write, (unsigned long)data);
}

__inline__ _private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, int data)
{
  return operator<<(raw_write, (long)data);
}

__inline__
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, unsigned int data)
{
  return operator<<(raw_write, static_cast<unsigned long>(data));
}

__inline__
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, libcw::debug::_private_::internal_string const& data)
{
  write(2, data.data(), data.size());
  return raw_write;
}

  }  // namespace debug
} // namespace libcw
#endif // CWDEBUG_DEBUG

#endif // RAW_WRITE_INL
