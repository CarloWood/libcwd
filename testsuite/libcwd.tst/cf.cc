#include <libcw/sys.h>
#include <errno.h>
#include <libcw/debug.h>

int main(void)
{
#ifdef LIBCW_USE_STRSTREAM
  std::ostrstream buf;
#else
  std::ostringstream buf;
#endif

  Debug( check_configuration() );
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );

  Debug( libcw_do.set_margin("MARGIN") );
  Debug( libcw_do.set_marker("MARKER") );
  Debug( libcw_do.set_indent(3) );

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

  Debug( libcw_do.set_ostream(&buf) );
  Dout(dc::notice, "This is written to buf");
  Dout(dc::notice|cerr_cf, "cerr_cf");
  buf.rdbuf()->pubseekoff(0, std::ios_base::end);

  exit(0);
}
