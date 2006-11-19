#ifndef SYSLOG_DEBUG_H
#define SYSLOG_DEBUG_H

#ifdef CWDEBUG

#ifndef DEBUGCHANNELS
#define DEBUGCHANNELS ::syslog_example::channels
#endif
#include <libcwd/debug.h>

namespace syslog_example {
  namespace channels {
    namespace dc {
      using namespace ::libcwd::channels::dc;
      using ::libcwd::channel_ct;
      extern ::libcwd::channel_ct debug_syslog;
    }
  }
}

extern ::libcwd::debug_ct syslog_do;
#define SyslogDout(cntrl, data) LibcwDout(DEBUGCHANNELS, syslog_do, cntrl|flush_cf, data)
#define SyslogDoutFatal(cntrl, data) LibcwDoutFatal(DEBUGCHANNELS, syslog_do, cntrl|flush_cf, data)

#else // !CWDEBUG

#include "nodebug.h"
#define SyslogDout(a, b)
#define SyslogDoutFatal(a, b) LibcwDoutFatal(::std, /*nothing*/, a, b)

#endif // !CWDEBUG

#endif // SYSLOG_DEBUG_H
