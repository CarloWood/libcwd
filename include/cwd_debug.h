// Generated automatically from sys.ho.in by configure.
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

#ifndef DEBUG_H
#define DEBUG_H

#include <libcw/debug.h>
#include <libcw/private_internal_string.h>
#include <iostream>

extern "C" size_t strlen(const char *s) throw();
#ifdef DEBUGDEBUG
extern "C" ssize_t write(int fd, const void *buf, size_t count) throw();
#endif

namespace libcw {
  namespace debug {

namespace _private_ {

extern void no_alloc_print_int_to(std::ostream* os, unsigned long val, bool hexadecimal);

//----------------------------------------------------------------------------------------------
// struct no_alloc_ostream_ct
//
// A fake ostream that is used in DoutInternal and LIBCWD_Dout in order to write
// to an ostream without allocating memory through std::__default_allocator<true, 0>
// which could lead to a dead lock.

struct no_alloc_ostream_ct {
  std::ostream& M_os;
  no_alloc_ostream_ct(std::ostream& os) : M_os(os) { }
};

} // namespace _private_

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, char const* data)
{
  os.M_os.write(data, strlen(data));
  return os;
}

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, libcw::debug::_private_::internal_string const& data)
{
  os.M_os.write(data.data(), data.size());
  return os;
}

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, char data)
{
  os.M_os.put(data);
  return os;
}

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, int data)
{
  _private_::no_alloc_print_int_to(&os.M_os, data, false);
  return os;
}

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, unsigned int data)
{
  _private_::no_alloc_print_int_to(&os.M_os, data, false);
  return os;
}

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, long data)
{
  _private_:: no_alloc_print_int_to(&os.M_os, data, false);
  return os;
}

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, unsigned long data)
{
  _private_::no_alloc_print_int_to(&os.M_os, data, false);
  return os;
}

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, void const* data)
{
  _private_::no_alloc_print_int_to(&os.M_os, reinterpret_cast<unsigned long>(data), true);
  return os;
}

#define LIBCWD_Dout( cntrl, data )				\
  do								\
  {								\
    if (libcw_do._off < 0)					\
    {								\
      bool on;							\
      {								\
        using namespace channels;				\
	on = (libcw_do|cntrl).on;				\
      }								\
      if (on)							\
      {								\
	libcw_do.start(LIBCWD_TSD);				\
	_private_::no_alloc_ostream_ct no_alloc_ostream(*libcw_do.current_oss);	\
	no_alloc_ostream << data;				\
	libcw_do.finish(LIBCWD_TSD);				\
      }								\
    }								\
  } while(0)

#define LIBCWD_DoutFatal( cntrl, data )			\
  do							\
  {							\
    {							\
      using namespace dc_namespace;			\
      libcw_do&cntrl;					\
    }							\
    libcw_do.start(LIBCWD_TSD);				\
    _private_::no_alloc_ostream_ct no_alloc_ostream(*libcw_do.current_oss);	\
    no_alloc_ostream << data;				\
    libcw_do.fatal_finish(LIBCWD_TSD);			\
  } while(0)

#ifdef DEBUGDEBUG
namespace _private_ {
  static class non_allocating_fake_ostream_using_write_ct { } const raw_write = { };
}

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, char const* data)
{
  write(2, data, strlen(data));
  return raw_write;
}

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, void const* data)
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

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, bool data)
{
  if (data)
    write(2, "true", 4);
  else
    write(2, "false", 5);
  return raw_write;
}

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, char data)
{
  char c[1];
  c[0] = data;
  write(2, c, 1);
  return raw_write;
}

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, unsigned long data)
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

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, long data)
{
  if (data < 0)
  {
    write(2, "-", 1);
    data = -data;
  }
  return operator<<(raw_write, (unsigned long)data);
}

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, int data)
{
  return operator<<(raw_write, (long)data);
}

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, unsigned int data)
{
  return operator<<(raw_write, static_cast<unsigned long>(data));
}

inline _private_::non_allocating_fake_ostream_using_write_ct const& operator<<(_private_::non_allocating_fake_ostream_using_write_ct const& raw_write, libcw::debug::_private_::internal_string const& data)
{
  write(2, data.data(), data.size());
  return raw_write;
}

#endif // DEBUGDEBUG

  }  // namespace debug
} // namespace libcw

#ifdef DEBUGDEBUG
// The difference between DEBUGDEBUG_CERR and FATALDEBUGDEBUG_CERR is that the latter is not suppressed
// when --disable-libcwd-debug-output is used because a fatal error occured anyway, so this can't
// disturb the testsuite.
#define FATALDEBUGDEBUG_CERR(x)									\
    do {											\
      if (1/*::libcw::debug::_private_::WST_ios_base_initialized FIXME: uncomment again*/) {			\
	::write(2, "DEBUGDEBUG: ", 12);								\
	LIBCWD_TSD_DECLARATION									\
	/*  __libcwd_lcwc means library_call write counter.  Used to avoid the 'scope of for changed' warning. */ \
	for (int __libcwd_lcwc = 0; __libcwd_lcwc < __libcwd_tsd.library_call; ++__libcwd_lcwc)	\
	  ::write(2, "    ", 4);								\
	::libcw::debug::_private_::raw_write << x << '\n';					\
      }												\
    } while(0)
#else // !DEBUGDEBUG
#define FATALDEBUGDEBUG_CERR(x)
#endif // !DEBUGDEBUG

#ifdef DEBUGDEBUGOUTPUT
#define DEBUGDEBUG_CERR(x) FATALDEBUGDEBUG_CERR(x)
#else // !DEBUGDEBUGOUTPUT
#define DEBUGDEBUG_CERR(x)
#endif // !DEBUGDEBUGOUTPUT

#endif // DEBUG_H
