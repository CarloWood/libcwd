// $Header$
//
// Copyright (C) 2001 - 2004, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef CWD_DEBUG_H
#define CWD_DEBUG_H

#ifndef RAW_WRITE_H
#include <raw_write.h>
#endif
#ifndef RAW_WRITE_INL
#include <raw_write.inl>
#endif
#ifndef LIBCW_IOSTREAM
#define LIBCW_IOSTREAM
#include <iostream>
#endif
#ifndef LIBCWD_PRIVATE_INTERNAL_STRING_H
#include <libcwd/private_internal_string.h>
#endif
#include <libcwd/debug.h>

extern "C" size_t strlen(const char *s) throw();

namespace libcwd {

#if CWDEBUG_ALLOC
namespace _private_ {

extern void no_alloc_print_int_to(std::ostream* os, unsigned long val, bool hexadecimal);

//----------------------------------------------------------------------------------------------
// struct no_alloc_ostream_ct
//
// A fake ostream that is used in DoutInternal and LIBCWD_Dout in order to write
// to an ostream without allocating memory through std::__default_allocator<true, 0>
// which could lead to a deadlock.

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

inline _private_::no_alloc_ostream_ct& operator<<(_private_::no_alloc_ostream_ct& os, libcwd::_private_::internal_string const& data)
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

#define LIBCWD_WRITE_TO_CURRENT_OSS(data) \
	_private_::no_alloc_ostream_ct no_alloc_ostream(*LIBCWD_DO_TSD_MEMBER(libcw_do, current_bufferstream)); \
	no_alloc_ostream << data

#else // !CWDEBUG_ALLOC

#define LIBCWD_WRITE_TO_CURRENT_OSS(data) \
	(*LIBCWD_DO_TSD_MEMBER(libcw_do, current_bufferstream)) << data

#endif // !CWDEBUG_ALLOC

#define LIBCWD_Dout( cntrl, data )									\
  do													\
  {													\
    if (LIBCWD_DO_TSD_MEMBER_OFF(libcw_do) < 0)								\
    {													\
      bool on;												\
      channel_set_bootstrap_st channel_set(LIBCWD_DO_TSD(libcw_do) LIBCWD_COMMA_TSD);			\
      {													\
        using namespace channels;									\
	on = (channel_set|cntrl).on;									\
      }													\
      if (on)												\
      {													\
	LIBCWD_DO_TSD(libcw_do).start(libcw_do, channel_set LIBCWD_COMMA_TSD);				\
	LIBCWD_WRITE_TO_CURRENT_OSS(data);								\
	LIBCWD_DO_TSD(libcw_do).finish(libcw_do, channel_set LIBCWD_COMMA_TSD);				\
      }													\
    }													\
  } while(0)

#define LIBCWD_DoutFatal( cntrl, data )									\
  do													\
  {													\
    channel_set_bootstrap_st channel_set(LIBCWD_DO_TSD(libcw_do) LIBCWD_COMMA_TSD);			\
    {													\
      using namespace dc_namespace;									\
      channel_set&cntrl;										\
    }													\
    LIBCWD_DO_TSD(libcw_do).start(libcw_do, channel_set LIBCWD_COMMA_TSD);				\
    LIBCWD_WRITE_TO_CURRENT_OSS(data);									\
    LIBCWD_DO_TSD(libcw_do).fatal_finish(libcw_do, channel_set LIBCWD_COMMA_TSD);			\
  } while(0)

} // namespace libcwd

#endif // CWD_DEBUG_H
