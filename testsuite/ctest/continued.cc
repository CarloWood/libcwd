// Verify continued debug output, nested debug output, and forced cerr output under CTest.
//
// This is a compact CTest-backed port of testsuite/libcwd.tst/continued.cc. It
// keeps the same custom FOO/BAR/RUN channels and covers the same behavior groups:
// ordinary output, split nonewline/noprefix output, error_cf text, nested debug
// output, cerr_cf interleaving, continued output with interruptions before and
// after dc::continued, and nested calls while a continued line is unfinished.
// PendingStream is used as libcw_do's ostream so the test can distinguish output
// that was merely written from output that was flushed or explicitly drained.

#include "cwd_sys.h"
#include "test_support.h"
#include "pending_stream.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace libcwd {
namespace channels {
namespace dc {

channel_ct foo("FOO");
channel_ct bar("BAR");
channel_ct run("RUN");

} // namespace dc
} // namespace channels
} // namespace libcwd

namespace {

libcwd_ctest::PendingStream* captured_output;
std::stringstream captured_cerr;

// Redirects std::cerr to a string stream for the duration of the test.
//
// This lets cerr_cf output be drained deliberately into PendingStream's committed
// buffer, matching the legacy test's explicit flush_cerr() checkpoints without
// depending on the process' real stderr.
class redirect_cerr_to_stringstream_ct
{
 private:
  std::streambuf* M_original;

 public:
  redirect_cerr_to_stringstream_ct() : M_original(std::cerr.rdbuf(captured_cerr.rdbuf())) { }
  redirect_cerr_to_stringstream_ct(redirect_cerr_to_stringstream_ct const&) = delete;
  redirect_cerr_to_stringstream_ct& operator=(redirect_cerr_to_stringstream_ct const&) = delete;
  ~redirect_cerr_to_stringstream_ct() { std::cerr.rdbuf(M_original); }
};

// Commit pending stdout-like debug output before a forced-cerr debug line.
//
// The legacy test flushed cout before printing selected cerr_cf lines so that
// stderr output appeared at a deterministic point in the combined transcript.
void flush_cout()
{
  captured_output->flush();
}

// Append captured cerr_cf output directly to the committed output buffer.
//
// The contents of captured_cerr are cleared after each drain, so repeated calls
// append only the newly forced-to-cerr debug lines.
void flush_cerr()
{
  captured_output->direct_out() << captured_cerr.str();
  captured_cerr.str({});
  captured_cerr.clear();
}

// Emit one nested FOO line, optionally through cerr_cf and/or with error_cf.
//
// Returns the literal string consumed by the surrounding output expression. When
// with_error is true, errno is set to zero to preserve the legacy "error 0"
// case; when to_cerr is true, output is explicitly drained into PendingStream's
// committed buffer before returning.
char const* nested_foo(bool with_error, bool to_cerr)
{
  if (with_error)
  {
    errno = 0;
    if (to_cerr)
    {
      flush_cout();
      Dout(dc::foo|cerr_cf|error_cf, "CERR: Inside `nested_foo()'");
      flush_cerr();
    }
    else
      Dout(dc::foo|error_cf, "Inside `nested_foo()'");
  }
  else
  {
    if (to_cerr)
    {
      flush_cout();
      Dout(dc::foo|cerr_cf, "CERR: Inside `nested_foo()'");
      flush_cerr();
    }
    else
      Dout(dc::foo, "Inside `nested_foo()'");
  }
  return "Foo";
}

// Emit a nested BAR block that calls nested_foo from inside one BAR line.
//
// The boolean parameters mirror the legacy helper: bar_with_error controls
// error_cf on the BAR lines, bar_to_cerr routes the BAR lines through cerr_cf,
// and foo_with_error/foo_to_cerr are passed to nested_foo. The return value is
// consumed by the surrounding NOTICE line.
char const* nested_bar(bool bar_with_error, bool bar_to_cerr, bool foo_with_error, bool foo_to_cerr)
{
  if (bar_to_cerr)
  {
    flush_cout();
    if (bar_with_error)
    {
      errno = EINVAL;
      Dout(dc::bar|cerr_cf|error_cf, "CERR: Entering `nested_bar()'");
      flush_cerr();
      flush_cout();
      errno = EINVAL;
      Dout(dc::bar|cerr_cf|error_cf, "CERR: `nested_foo(" << foo_with_error << ", " << foo_to_cerr
          << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it.");
      flush_cerr();
      flush_cout();
      errno = EINVAL;
      Dout(dc::bar|cerr_cf|error_cf, "CERR: Leaving `nested_bar()'");
      flush_cerr();
    }
    else
    {
      Dout(dc::bar|cerr_cf, "CERR: Entering `nested_bar()'");
      flush_cerr();
      flush_cout();
      Dout(dc::bar|cerr_cf, "CERR: `nested_foo(" << foo_with_error << ", " << foo_to_cerr
          << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it.");
      flush_cerr();
      flush_cout();
      Dout(dc::bar|cerr_cf, "CERR: Leaving `nested_bar()'");
      flush_cerr();
    }
  }
  else if (bar_with_error)
  {
    errno = EINVAL;
    Dout(dc::bar|error_cf, "Entering `nested_bar()'");
    errno = EINVAL;
    Dout(dc::bar|error_cf, "`nested_foo(" << foo_with_error << ", " << foo_to_cerr
        << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it.");
    errno = EINVAL;
    Dout(dc::bar|error_cf, "Leaving `nested_bar()'");
  }
  else
  {
    Dout(dc::bar, "Entering `nested_bar()'");
    Dout(dc::bar, "`nested_foo(" << foo_with_error << ", " << foo_to_cerr
        << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it.");
    Dout(dc::bar, "Leaving `nested_bar()'");
  }
  return "Bar";
}

// Exercise recursive continued output while nested FOO lines interrupt it.
//
// Each level starts a continued RUN line, emits a normal FOO interruption, then
// either recurses or emits a leaf marker before finishing the RUN line. This is a
// smaller deterministic analogue of the legacy continued_func() deep test.
void recursive_continued(unsigned int depth)
{
  Dout(dc::run|continued_cf, "level " << depth << " begin ");
  Dout(dc::foo, "interrupt level " << depth);
  if (depth > 0)
    recursive_continued(depth - 1);
  else
    Dout(dc::foo, "leaf");
  Dout(dc::finish, "end " << depth);
}

// Run the continued-output scenario and leave all final pending data committed.
//
// The function has no filesystem side effects. It mutates libcwd's channel/debug
// object state in the same way as other CTest fixtures and writes the captured
// transcript to captured.
void run_continued_scenario(libcwd_ctest::PendingStream& captured)
{
  captured_output = &captured;
  redirect_cerr_to_stringstream_ct redirect_cerr;

  Debug(main_reached());
  ForAllDebugChannels(if (debugChannel.is_on()) debugChannel.off(););
  Debug(dc::notice.on());
  Debug(dc::foo.on());
  Debug(dc::bar.on());
  Debug(dc::run.on());
  Debug(libcw_do.set_ostream(&captured));
  Debug(libcw_do.on());

  Dout(dc::notice, "This is a single line");
  Dout(dc::notice|nonewline_cf, "This is ");
  Dout(dc::notice|noprefix_cf, "a single line");
  errno = 0;
  Dout(dc::notice|error_cf, "This is a single line with an error message behind it");
  flush_cout();
  errno = 0;
  Dout(dc::notice|error_cf|cerr_cf, "CERR: This is a single line with an error message behind it written to cerr");
  flush_cerr();

  for (int i = 0; i < 4; ++i)
  {
    bool const with_error = (i & 2) != 0;
    bool const to_cerr = (i & 1) != 0;
    Dout(dc::notice, "`nested_foo(" << with_error << ", " << to_cerr
        << ")' returns the string \"" << nested_foo(with_error, to_cerr) << "\" when I call it.");
  }

  Dout(dc::notice, "`nested_bar(0, 0, 0, 0)' returns the string \"" << nested_bar(false, false, false, false) << "\" when I call it.");
  Dout(dc::notice, "`nested_bar(0, 1, 0, 1)' returns the string \"" << nested_bar(false, true, false, true) << "\" when I call it.");
  Dout(dc::notice, "`nested_bar(1, 0, 1, 0)' returns the string \"" << nested_bar(true, false, true, false) << "\" when I call it.");

  Dout(dc::run|continued_cf, "Hello ");
  Dout(dc::finish, "World");
  Dout(dc::run|continued_cf, "Libcwd ");
  Dout(dc::continued, "is an awesome ");
  Dout(dc::finish, "library!");
  Dout(dc::run|continued_cf, "Libcwd ");
  Dout(dc::foo, "Single interruption before.");
  Dout(dc::continued, "is an awesome ");
  Dout(dc::finish, "library!");
  Dout(dc::run|continued_cf, "Libcwd ");
  Dout(dc::continued, "is an awesome ");
  Dout(dc::foo, "Single interruption after.");
  Dout(dc::finish, "library!");
  Dout(dc::run|continued_cf, "Libcwd ");
  Dout(dc::foo, "Single interruption before and");
  Dout(dc::continued, "is an awesome ");
  Dout(dc::foo, "a single interruption after.");
  Dout(dc::finish, "library!");

  Dout(dc::run|continued_cf, "Libcwd ");
  Dout(dc::notice, "`nested_foo(0, 0)' returns the string \"" << nested_foo(false, false) << "\" when I call it.");
  Dout(dc::continued, "is an awesome ");
  Dout(dc::notice, "`nested_foo(0, 1)' returns the string \"" << nested_foo(false, true) << "\" when I call it.");
  Dout(dc::finish, "library!");

  recursive_continued(2);

  Debug(libcw_do.off());
  captured.flush();
}

// Compare captured output against the compact continued-output transcript.
//
// Lines containing strerror(0) are regular expressions because platforms spell
// that diagnostic differently; EINVAL lines are literal on the supported Linux
// test environment.
bool output_matches_expected(libcwd_ctest::PendingStream& captured)
{
  char const* expected[] = {
    "NOTICE  : This is a single line",
    "NOTICE  : This is a single line",
    "^NOTICE  : This is a single line with an error message behind it: 0 \\((Success|Error 0|Undefined error: 0|Unknown error: 0)\\)$",
    "^NOTICE  : CERR: This is a single line with an error message behind it written to cerr: 0 \\((Success|Error 0|Undefined error: 0|Unknown error: 0)\\)$",
    "FOO     :     Inside `nested_foo()'",
    "NOTICE  : `nested_foo(0, 0)' returns the string \"Foo\" when I call it.",
    "FOO     :     CERR: Inside `nested_foo()'",
    "NOTICE  : `nested_foo(0, 1)' returns the string \"Foo\" when I call it.",
    "^FOO     :     Inside `nested_foo\\(\\)': 0 \\((Success|Error 0|Undefined error: 0|Unknown error: 0)\\)$",
    "NOTICE  : `nested_foo(1, 0)' returns the string \"Foo\" when I call it.",
    "^FOO     :     CERR: Inside `nested_foo\\(\\)': 0 \\((Success|Error 0|Undefined error: 0|Unknown error: 0)\\)$",
    "NOTICE  : `nested_foo(1, 1)' returns the string \"Foo\" when I call it.",
    "BAR     :     Entering `nested_bar()'",
    "FOO     :         Inside `nested_foo()'",
    "BAR     :     `nested_foo(0, 0)' returns the string \"Foo\" when I call it.",
    "BAR     :     Leaving `nested_bar()'",
    "NOTICE  : `nested_bar(0, 0, 0, 0)' returns the string \"Bar\" when I call it.",
    "BAR     :     CERR: Entering `nested_bar()'",
    "FOO     :         CERR: Inside `nested_foo()'",
    "BAR     :     CERR: `nested_foo(0, 1)' returns the string \"Foo\" when I call it.",
    "BAR     :     CERR: Leaving `nested_bar()'",
    "NOTICE  : `nested_bar(0, 1, 0, 1)' returns the string \"Bar\" when I call it.",
    "BAR     :     Entering `nested_bar()': EINVAL (Invalid argument)",
    "^FOO     :         Inside `nested_foo\\(\\)': 0 \\((Success|Error 0|Undefined error: 0|Unknown error: 0)\\)$",
    "BAR     :     `nested_foo(1, 0)' returns the string \"Foo\" when I call it.: EINVAL (Invalid argument)",
    "BAR     :     Leaving `nested_bar()': EINVAL (Invalid argument)",
    "NOTICE  : `nested_bar(1, 0, 1, 0)' returns the string \"Bar\" when I call it.",
    "RUN     : Hello World",
    "RUN     : Libcwd is an awesome library!",
    "RUN     : Libcwd <unfinished>",
    "FOO     :     Single interruption before.",
    "RUN     : <continued> is an awesome library!",
    "RUN     : Libcwd is an awesome <unfinished>",
    "FOO     :     Single interruption after.",
    "RUN     : <continued> library!",
    "RUN     : Libcwd <unfinished>",
    "FOO     :     Single interruption before and",
    "RUN     : <continued> is an awesome <unfinished>",
    "FOO     :     a single interruption after.",
    "RUN     : <continued> library!",
    "RUN     : Libcwd <unfinished>",
    "FOO     :         Inside `nested_foo()'",
    "NOTICE  :     `nested_foo(0, 0)' returns the string \"Foo\" when I call it.",
    "RUN     : <continued> is an awesome <unfinished>",
    "FOO     :         CERR: Inside `nested_foo()'",
    "NOTICE  :     `nested_foo(0, 1)' returns the string \"Foo\" when I call it.",
    "RUN     : <continued> library!",
    "RUN     : level 2 begin <unfinished>",
    "FOO     :     interrupt level 2",
    "RUN     :     level 1 begin <unfinished>",
    "FOO     :         interrupt level 1",
    "RUN     :         level 0 begin <unfinished>",
    "FOO     :             interrupt level 0",
    "FOO     :             leaf",
    "RUN     :         <continued> end 0",
    "RUN     :     <continued> end 1",
    "RUN     : <continued> end 2",
    nullptr
  };

  return libcwd_ctest::matches_expected_output(captured.input(), expected);
}

} // namespace

int main()
{
  libcwd_ctest::PendingStream captured;
  run_continued_scenario(captured);
  return output_matches_expected(captured) ? EXIT_SUCCESS : EXIT_FAILURE;
}
