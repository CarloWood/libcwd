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

#include "libcwd/config.h"
#ifndef RAW_WRITE_H
#include "raw_write.h"
#endif
#ifndef RAW_WRITE_INL
#include "raw_write.inl"
#endif
#ifndef LIBCW_IOSTREAM
#define LIBCW_IOSTREAM
#include <iostream>
#endif

extern "C" size_t strlen(const char *s) throw();

namespace libcwd {

#define LIBCWD_WRITE_TO_CURRENT_OSS(data) \
	(*LIBCWD_DO_TSD_MEMBER(libcw_do, current_bufferstream)) << data


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
