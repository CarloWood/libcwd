// $Header$
//
// Copyright (C) 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

/** \file libcwd/private_bufferstream.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCWD_PRIVATE_BUFFERSTREAM_H
#define LIBCWD_PRIVATE_BUFFERSTREAM_H

#ifndef LIBCWD_CONFIG_H
#include <libcwd/config.h>
#endif

namespace libcwd {
  namespace _private_ {

    // This is a pseudo stringstream with auto_internal_allocator
    // that allows to change the stringbuf on the fly.
    class bufferstream_ct : public std::ostream
    {
    public:
      typedef char char_type;
      typedef std::char_traits<char> traits_type;
#if CWDEBUG_ALLOC
      typedef auto_internal_allocator allocator_type;
#else
      typedef ::std::allocator<char_type> allocator_type;
#endif
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

      stringbuf_type* rdbuf(void) const { return M_stringbuf; }
      string_type str(void) const { return M_stringbuf->str(); }
      void str(string_type const& s) { M_stringbuf->str(s); }
    };

  } // namespace _private_
} // namespace libcwd

#endif // LIBCWD_PRIVATE_BUFFERSTREAM_H
