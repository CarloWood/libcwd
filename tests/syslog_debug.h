#ifndef SYSLOG_DEBUG_H
#define SYSLOG_DEBUG_H

#ifndef DEBUGCHANNELS
#define DEBUGCHANNELS ::syslog_example::debug::channels
#endif
#include <libcw/debug.h>

namespace syslog_example {
  namespace debug {
    namespace channels {
      namespace dc {
	using namespace ::libcw::debug::channels::dc;
	extern ::libcw::debug::channel_ct const debug_syslog;
      }
    }
  }
}

#ifdef DEBUG
extern libcw::debug::debug_ct syslog_do;
#define SyslogDout(cntrl, data) LibcwDout(DEBUGCHANNELS, syslog_do, cntrl|flush_cf, data)
#define SyslogDout_vform(cntrl, format, vl) LibcwDout_vform(DEBUGCHANNELS, syslog_do, cntrl|flush_cf, format, vl)
#define SyslogDoutFatal(cntrl, data) LibcwDoutFatal(DEBUGCHANNELS, syslog_do, cntrl|flush_cf, data)
#else // !DEBUG
#define SysDout(a, b)
#define SysDout_vform(a, b, c)
#define SysDoutFatal(a, b) LibcwDoutFatal(::std, /*nothing*/, a, b)
#endif // !DEBUG

#endif // SYSLOG_DEBUG_H
