// $Header$
//
// Copyright (C) 2000, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCW_NO_ALLOC_CHECKING_STRSTREAM_H
#define LIBCW_NO_ALLOC_CHECKING_STRSTREAM_H

RCSTAG_H(no_alloc_checking_stringstream, "$Id$")

#ifdef LIBCW_USE_STRSTREAM
#include <strstream>
#else
#include <sstream>
#endif

namespace libcw {
  namespace debug {

#ifdef DEBUGMALLOC

#ifdef LIBCW_USE_STRSTREAM

class no_alloc_checking_stringstream : public std::strstream {
private:
  std::strstreambuf* my_sb;
public:
  no_alloc_checking_stringstream(void);
  ~no_alloc_checking_stringstream();
  std::strstreambuf* rdbuf() { return my_sb; }
  typedef streampos pos_type;
};

#else // !LIBCW_USE_STRSTREAM

class no_alloc_checking_allocator : public std::allocator<char> {
public:
  pointer allocate(size_type size, std::allocator<void>::const_pointer hint = 0);
  void deallocate(pointer p, size_type);
};

typedef std::basic_stringstream<char, std::char_traits<char>, no_alloc_checking_allocator> no_alloc_checking_stringstream;

#endif // !LIBCW_USE_STRSTREAM

#else // !DEBUGMALLOC

#ifdef LIBCW_USE_STRSTREAM
class no_alloc_checking_stringstream : public strstream {
public:
  typedef streampos pos_type;
};
#else
typedef std::stringstream no_alloc_checking_stringstream;
#endif

#endif // !DEBUGMALLOC

  }	// namespace debug
}	// namespace libcw

#endif // LIBCW_NO_ALLOC_CHECKING_STRSTREAM_H
