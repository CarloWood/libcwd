// $Header$
//
// Copyright (C) 2001, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//
 
#include <libcw/sys.h>
#ifndef LIBCW_USE_STRSTREAM
#define _GLIBCPP_FULLY_COMPLIANT_HEADERS
// This is needed to get the definitions of the template member functions of
// std::basic_stringbuf<char, std::char_traits<char>, libcw::debug::no_alloc_checking_allocator>
#include <sstream>
// and operator<<(basic_ostream<char, std::char_traits<char> >&, basic_string<char, std::char_traits<char>, libcw::debug::no_alloc_checking_al#include <ostream>
#undef _GLIBCPP_FULLY_COMPLIANT_HEADERS
#endif
#include <libcw/debug.h>
#include <libcw/no_alloc_checking_stringstream.h>

namespace libcw {
  namespace debug {

#ifdef DEBUGMALLOC

#ifdef LIBCW_USE_STRSTREAM
typedef void* pointer;
pointer no_alloc_checking_alloc(size_t size)
#else
no_alloc_checking_allocator::pointer
no_alloc_checking_allocator::allocate(no_alloc_checking_allocator::size_type size, std::allocator<void>::const_pointer hint = 0)
#endif
{
  set_alloc_checking_off();
  channels::dc::malloc.off();
  pointer ptr = (pointer)new char[size];
  channels::dc::malloc.on();
  set_alloc_checking_on();
  return ptr;
}

#ifdef LIBCW_USE_STRSTREAM
void no_alloc_checking_free(void* p)
#else
void no_alloc_checking_allocator::deallocate(no_alloc_checking_allocator::pointer p, no_alloc_checking_allocator::size_type)
#endif
{
  set_alloc_checking_off();
  channels::dc::malloc.off();
  delete [] (char*)p;
  channels::dc::malloc.on();
  set_alloc_checking_on();
}

#ifdef LIBCW_USE_STRSTREAM
no_alloc_checking_stringstream::no_alloc_checking_stringstream(void)
{
  set_alloc_checking_off();
  my_sb = new strstreambuf(no_alloc_checking_alloc, no_alloc_checking_free);
  set_alloc_checking_on();
  ios::init(my_sb);                                 // Add the real buffer
}

no_alloc_checking_stringstream::~no_alloc_checking_stringstream()
{
  set_alloc_checking_off();
  delete my_sb;
  set_alloc_checking_on();
}
#endif

#endif // DEBUGMALLOC

  } // namespace debug
} // namespace libcw

#if __GNUC__ > 2 || __GNUC_MINOR__ > 96
// Explicit instantiation
template class std::basic_stringbuf<char, std::char_traits<char>, libcw::debug::no_alloc_checking_allocator>;
template std::basic_ostream<char, std::char_traits<char> >& std::operator<< <char, std::char_traits<char>, libcw::debug::no_alloc_checking_allocator>(std::basic_ostream<char, std::char_traits<char> >&, std::basic_string<char, std::char_traits<char>, libcw::debug::no_alloc_checking_allocator> const&);
#endif

