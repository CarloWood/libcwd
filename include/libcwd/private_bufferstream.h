// SPDX-FileCopyrightText: 2003-2005, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file libcwd/private_bufferstream.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_PRIVATE_BUFFERSTREAM_H
#define LIBCWD_PRIVATE_BUFFERSTREAM_H

#include "libcwd/config.h"

namespace libcwd::_private_ {

// This is a pseudo stringstream that allows the stringbuf to be changed on
// the fly.
class bufferstream_ct : public std::ostream
{
public:
  typedef char char_type;
  typedef std::char_traits<char> traits_type;
  typedef ::std::allocator<char_type> allocator_type;
  typedef traits_type::int_type int_type;
  typedef traits_type::pos_type pos_type;
  typedef traits_type::off_type off_type;
  typedef std::basic_string<char_type, traits_type, allocator_type> string_type;
  typedef std::basic_stringbuf<char_type, traits_type, allocator_type> stringbuf_type;

public:
  stringbuf_type* M_stringbuf;

public:
  explicit
  bufferstream_ct(stringbuf_type* sb) : std::basic_ostream<char, std::char_traits<char> >(sb), M_stringbuf(sb) { }
  ~bufferstream_ct() { }

  stringbuf_type* rdbuf() const { return M_stringbuf; }
  string_type str() const { return M_stringbuf->str(); }
  void str(string_type const& s) { M_stringbuf->str(s); }
};

} // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_BUFFERSTREAM_H
