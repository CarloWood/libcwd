// SPDX-FileCopyrightText: 2001-2004, 2017, 2019-2020, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef CWD_DEBUG_H
#define CWD_DEBUG_H

#include "raw_write.h"
#include "libcwd/config.h"

#include <iostream>

extern "C" size_t strlen(char const* s) throw();

namespace libcwd {

#define LIBCWD_WRITE_TO_CURRENT_OSS(data) (*LIBCWD_DO_TSD_MEMBER(libcw_do, current_bufferstream)) << data

#define LIBCWD_Dout(cntrl, data)                                                 \
  do                                                                             \
  {                                                                              \
    if (LIBCWD_DO_TSD_MEMBER_OFF(libcw_do) < 0)                                  \
    {                                                                            \
      bool on;                                                                   \
      ChannelSetBootstrap channel_set(LIBCWD_DO_TSD(libcw_do) LIBCWD_COMMA_TSD); \
      {                                                                          \
        using namespace channels;                                                \
        on = (channel_set | cntrl).on;                                           \
      }                                                                          \
      if (on)                                                                    \
      {                                                                          \
        LIBCWD_DO_TSD(libcw_do).start(libcw_do, channel_set LIBCWD_COMMA_TSD);   \
        LIBCWD_WRITE_TO_CURRENT_OSS(data);                                       \
        LIBCWD_DO_TSD(libcw_do).finish(libcw_do, channel_set LIBCWD_COMMA_TSD);  \
      }                                                                          \
    }                                                                            \
  } while (0)

#define LIBCWD_DoutFatal(cntrl, data)                                             \
  do                                                                              \
  {                                                                               \
    ChannelSetBootstrap channel_set(LIBCWD_DO_TSD(libcw_do) LIBCWD_COMMA_TSD);    \
    {                                                                             \
      using namespace dc_namespace;                                               \
      channel_set & cntrl;                                                        \
    }                                                                             \
    LIBCWD_DO_TSD(libcw_do).start(libcw_do, channel_set LIBCWD_COMMA_TSD);        \
    LIBCWD_WRITE_TO_CURRENT_OSS(data);                                            \
    LIBCWD_DO_TSD(libcw_do).fatal_finish(libcw_do, channel_set LIBCWD_COMMA_TSD); \
  } while (0)

} // namespace libcwd

#endif // CWD_DEBUG_H
