#ifndef SYSLOG_DEBUG_H
#define SYSLOG_DEBUG_H

#ifdef CWDEBUG

#ifndef DEBUGCHANNELS
#define DEBUGCHANNELS ::syslog_example::debug::channels
#endif
#include <libcwd/debug.h>

namespace syslog_example {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace ::libcw::debug::channels::dc;
	extern ::libcw::debug::channel_ct debug_syslog;
      }
    }
  }
}

extern libcw::debug::debug_ct syslog_do;
#define SyslogDout(cntrl, data) LibcwDout(DEBUGCHANNELS, syslog_do, cntrl|flush_cf, data)
#define SyslogDoutFatal(cntrl, data) LibcwDoutFatal(DEBUGCHANNELS, syslog_do, cntrl|flush_cf, data)

#else // !CWDEBUG

#include "nodebug.h"
#define SysDout(a, b)
#define SysDoutFatal(a, b) LibcwDoutFatal(::std, /*nothing*/, a, b)

#endif // !CWDEBUG

#endif // SYSLOG_DEBUG_H
