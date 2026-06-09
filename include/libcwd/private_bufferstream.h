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
class BufferStream : public std::ostream
{
public:
  using char_type = char;
  using traits_type = std::char_traits<char>;
  using allocator_type = ::std::allocator<char_type>;
  using int_type = traits_type::int_type;
  using pos_type = traits_type::pos_type;
  using off_type = traits_type::off_type;
  using string_type = std::basic_string<char_type, traits_type, allocator_type>;
  using stringbuf_type = std::basic_stringbuf<char_type, traits_type, allocator_type>;

public:
  stringbuf_type* stringbuf;

public:
  explicit
  BufferStream(stringbuf_type* sb) : std::basic_ostream<char, std::char_traits<char> >(sb), stringbuf(sb) { }
  ~BufferStream() { }

  stringbuf_type* rdbuf() const { return stringbuf; }
  string_type str() const { return stringbuf->str(); }
  void str(string_type const& s) { stringbuf->str(s); }
};

} // namespace libcwd::_private_

#endif // LIBCWD_PRIVATE_BUFFERSTREAM_H
