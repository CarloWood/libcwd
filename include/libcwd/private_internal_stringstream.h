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

/** \file libcwd/private_internal_stringstream.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_PRIVATE_INTERNAL_STRINGSTREAM_H
#define LIBCW_PRIVATE_INTERNAL_STRINGSTREAM_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcwd/config.h>
#endif
#ifndef LIBCW_PRIVATE_ALLOCATOR_H
#include <libcwd/private_allocator.h>
#endif
#if CWDEBUG_DEBUGM && !defined(LIBCW_STRUCT_TSD_H)
#include <libcwd/private_struct_TSD.h>
#endif
#ifndef LIBCW_SSTREAM
#define LIBCW_SSTREAM
#ifdef LIBCWD_USE_STRSTREAM
#include <strstream>
#else
#include <sstream>
#endif
#endif

namespace libcw {
  namespace debug {
    namespace _private_ {

#ifdef LIBCWD_USE_STRSTREAM
#if !CWDEBUG_ALLOC
typedef std::strstream auto_internal_stringstream;
#else // CWDEBUG_ALLOC
extern void* auto_internal_strstreambuf_alloc(size_t);
extern void auto_internal_strstreambuf_free(void*);

class auto_internal_strstreambase : virtual public ios {
protected:
  strstreambuf M_my_sb;
public:
  strstreambuf* rdbuf(void) { return &M_my_sb; }
protected:
  auto_internal_strstreambase(void) :
      M_my_sb(::libcw::debug::_private_::auto_internal_strstreambuf_alloc,
	      ::libcw::debug::_private_::auto_internal_strstreambuf_free)
      { init(&M_my_sb); }
};

class auto_internal_stringstream : public auto_internal_strstreambase, public iostream {
public:
#if CWDEBUG_DEBUGM
  auto_internal_stringstream(void) { LIBCWD_TSD_DECLARATION; LIBCWD_ASSERT( !__libcwd_tsd.internal ); }
  ~auto_internal_stringstream(void) { LIBCWD_TSD_DECLARATION; LIBCWD_ASSERT( !__libcwd_tsd.internal ); }
#endif
  _IO_ssize_t pcount(void) { return ((strstreambuf*)_strbuf)->pcount(); }
  char* str(void) { return ((strstreambuf*)_strbuf)->str(); }
  void freeze(int n = 1) { ((strstreambuf*)_strbuf)->freeze(n); }
  int frozen(void) { return ((strstreambuf*)_strbuf)->frozen(); }
};
#endif // CWDEBUG_ALLOC
#else // !LIBCWD_USE_STRSTREAM
#if CWDEBUG_ALLOC
typedef ::std::basic_stringstream<char, ::std::char_traits<char>, ::libcw::debug::_private_::auto_internal_allocator> auto_internal_stringstream;
#else
typedef ::std::stringstream auto_internal_stringstream;
#endif
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#endif // LIBCW_PRIVATE_INTERNAL_STRINGSTREAM_H
