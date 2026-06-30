// Verify libcwd control flags for prefixes, labels, markers, newlines, errors, and cerr routing.
//
// This is the CTest-backed form of testsuite/libcwd.tst/cf.cc. It writes the
// same sequence of control-flag combinations to a captured debug stream and
// checks the exact visible output, including the line formed by several
// nonewline_cf writes and the cerr_cf line emitted by a separate debug object.

#include "cwd_sys.h"
#include "test_support.h"

#include <cerrno>
#include <cstdlib>
#include <sstream>

libcwd::DebugObject local_debug_object;
#define MyDout(cntrl, ...) LibcwDout(LIBCWD_DEBUGCHANNELS, local_debug_object, cntrl, __VA_ARGS__)

int main()
{
  libcwd_ctest::PendingStream captured(libcwd::libcw_do);
  std::ostringstream hidden_local_output;

  Debug(main_reached());
  Debug(local_debug_object.set_ostream(&hidden_local_output));
  Debug(local_debug_object.on());
  Debug(libcw_do.on());
  Debug(dc::notice.on());

  Debug(libcw_do.margin().assign("MARGIN", 6));
  Debug(libcw_do.marker().assign("MARKER", 6));
  Debug(libcw_do.set_indent(3));
  Debug(local_debug_object.margin().assign("MARGIN", 6));
  Debug(local_debug_object.marker().assign("MARKER", 6));
  Debug(local_debug_object.set_indent(3));

  {
    libcwd_ctest::RedirectCerr redirect(captured);

    Dout(dc::notice | nonewline_cf, "x");
    Dout(dc::notice | nonewline_cf, "y");
    Dout(dc::notice, "z<newline>");

    Dout(dc::notice, "<no flags>");
    Dout(dc::notice | noprefix_cf, "noprefix_cf");
    Dout(dc::notice | nolabel_cf, "nolabel_cf");
    Dout(dc::notice | blank_margin_cf, "blank_margin_cf");
    Dout(dc::notice | blank_label_cf, "blank_label_cf");
    Dout(dc::notice | blank_marker_cf, "blank_marker_cf");

    Dout(dc::notice | noprefix_cf | nonewline_cf, "a");
    Dout(dc::notice | nolabel_cf | nonewline_cf, "b");
    Dout(dc::notice | blank_margin_cf | nonewline_cf, "c");
    Dout(dc::notice | blank_label_cf | nonewline_cf, "d");
    Dout(dc::notice | blank_marker_cf | nonewline_cf, "e");
    Dout(dc::notice, "f");

    Dout(dc::notice | nolabel_cf | noprefix_cf, "nolabel_cf|noprefix_cf");
    Dout(dc::notice | blank_margin_cf | noprefix_cf, "blank_margin_cf|noprefix_cf");
    Dout(dc::notice | blank_label_cf | noprefix_cf, "blank_label_cf|noprefix_cf");
    Dout(dc::notice | blank_marker_cf | noprefix_cf, "blank_marker_cf|noprefix_cf");

    Dout(dc::notice | blank_margin_cf | nolabel_cf, "blank_margin_cf|nolabel_cf");
    Dout(dc::notice | blank_label_cf | nolabel_cf, "blank_label_cf|nolabel_cf");
    Dout(dc::notice | blank_marker_cf | nolabel_cf, "blank_marker_cf|nolabel_cf");

    Dout(dc::notice | blank_label_cf | blank_margin_cf, "blank_label_cf|blank_margin_cf");
    Dout(dc::notice | blank_marker_cf | blank_margin_cf, "blank_marker_cf|blank_margin_cf");
    Dout(dc::notice | blank_marker_cf | blank_label_cf, "blank_marker_cf|blank_label_cf");

    errno = EAGAIN;
    Dout(dc::notice | error_cf, "error_cf");

    MyDout(dc::notice, "This is written to buf");
    MyDout(dc::notice | cerr_cf, "cerr_cf");
  }

  captured.sync();

  char const* expected[] = {
      "MARGINNOTICE  MARKER   xMARGINNOTICE  MARKER   yMARGINNOTICE  MARKER   z<newline>",
      "MARGINNOTICE  MARKER   <no flags>",
      "noprefix_cf",
      "MARGINnolabel_cf",
      "      NOTICE  MARKER   blank_margin_cf",
      "MARGIN        MARKER   blank_label_cf",
      "MARGINNOTICE           blank_marker_cf",
      "aMARGINb      NOTICE  MARKER   cMARGIN        MARKER   dMARGINNOTICE           eMARGINNOTICE  MARKER   f",
      "nolabel_cf|noprefix_cf",
      "blank_margin_cf|noprefix_cf",
      "blank_label_cf|noprefix_cf",
      "blank_marker_cf|noprefix_cf",
      "      blank_margin_cf|nolabel_cf",
      "MARGINblank_label_cf|nolabel_cf",
      "MARGINblank_marker_cf|nolabel_cf",
      "              MARKER   blank_label_cf|blank_margin_cf",
      "      NOTICE           blank_marker_cf|blank_margin_cf",
      "MARGIN                 blank_marker_cf|blank_label_cf",
      "MARGINNOTICE  MARKER   error_cf: EAGAIN (Resource temporarily unavailable)",
      "MARGINNOTICE  MARKER   cerr_cf",
      nullptr};

  return libcwd_ctest::matches_expected_output(captured.direct_istream(), expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
