#include "sys.h"
#include <errno.h>
#include <libcw/debug.h>
#ifdef LIBCWD_USE_STRSTREAM
#include <strstream>
#else
#include <sstream>
#endif

libcw::debug::debug_ct local_debug_object;
#define MyDout(cntrl, data) LibcwDout(DEBUGCHANNELS, local_debug_object, cntrl, data)

#ifdef THREADTEST
pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

MAIN_FUNCTION
{ PREFIX_CODE
#ifdef LIBCWD_USE_STRSTREAM
  std::ostrstream buf;
#elif !CWDEBUG_ALLOC
  std::ostringstream buf;
#else
#if __GNUC__ < 3
  ::std::basic_stringstream<char, string_char_traits<char>, ::libcw::debug::_private_::userspace_allocator> buf;
#else
  ::std::basic_stringstream<char, ::std::char_traits<char>, ::libcw::debug::_private_::userspace_allocator> buf;
#endif
#endif

  Debug( check_configuration() );
  Debug( local_debug_object.on() );
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );

  Debug( libcw_do.set_margin("MARGIN") );
  Debug( libcw_do.set_marker("MARKER") );
  Debug( libcw_do.set_indent(3) );
  Debug( local_debug_object.set_margin("MARGIN") );
  Debug( local_debug_object.set_marker("MARKER") );
  Debug( local_debug_object.set_indent(3) );

  Dout(dc::notice|nonewline_cf, "x");
  Dout(dc::notice|nonewline_cf, "y");
  Dout(dc::notice, "z<newline>");

  Dout(dc::notice, "<no flags>");
  Dout(dc::notice|noprefix_cf, "noprefix_cf");
  Dout(dc::notice|nolabel_cf, "nolabel_cf");
  Dout(dc::notice|blank_margin_cf, "blank_margin_cf");
  Dout(dc::notice|blank_label_cf, "blank_label_cf");
  Dout(dc::notice|blank_marker_cf, "blank_marker_cf");

  Dout(dc::notice|noprefix_cf|nonewline_cf, "a");
  Dout(dc::notice|nolabel_cf|nonewline_cf, "b");
  Dout(dc::notice|blank_margin_cf|nonewline_cf, "c");
  Dout(dc::notice|blank_label_cf|nonewline_cf, "d");
  Dout(dc::notice|blank_marker_cf|nonewline_cf, "e");
  Dout(dc::notice, "f");

  Dout(dc::notice|nolabel_cf|noprefix_cf, "nolabel_cf|noprefix_cf");
  Dout(dc::notice|blank_margin_cf|noprefix_cf, "blank_margin_cf|noprefix_cf");
  Dout(dc::notice|blank_label_cf|noprefix_cf, "blank_label_cf|noprefix_cf");
  Dout(dc::notice|blank_marker_cf|noprefix_cf, "blank_marker_cf|noprefix_cf");

  Dout(dc::notice|blank_margin_cf|nolabel_cf, "blank_margin_cf|nolabel_cf");
  Dout(dc::notice|blank_label_cf|nolabel_cf, "blank_label_cf|nolabel_cf");
  Dout(dc::notice|blank_marker_cf|nolabel_cf, "blank_marker_cf|nolabel_cf");

  Dout(dc::notice|blank_label_cf|blank_margin_cf, "blank_label_cf|blank_margin_cf");
  Dout(dc::notice|blank_marker_cf|blank_margin_cf, "blank_marker_cf|blank_margin_cf");

  Dout(dc::notice|blank_marker_cf|blank_label_cf, "blank_marker_cf|blank_label_cf");

  errno = EAGAIN;
  Dout(dc::notice|error_cf, "error_cf");

  Debug( local_debug_object.set_ostream(&buf COMMA_THREADED(&buf_mutex)) );
  MyDout(dc::notice, "This is written to buf");
  MyDout(dc::notice|cerr_cf, "cerr_cf");
#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
  buf.rdbuf()->pubseekoff(0, std::ios::end);
#else
  buf.rdbuf()->pubseekoff(0, std::ios_base::end);
#endif

  EXIT(0);
}
