// Verify flushing and continued-line debug output under CTest.
//
// This ports testsuite/libcwd.tst/flush.cc by exercising a custom GENERATE
// channel, flush_cf, continued_cf, and finish. The legacy test wrote
// "<sleeping>" directly to stdout between debug writes; this CTest version writes
// the same marker to the debug output stream so the complete sequence can be
// compared deterministically in memory.

#include "cwd_sys.h"
#include "test_support.h"
#include "pending_stream.h"

#include <cstdlib>
#include <ostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace libcwd {
namespace channels {
namespace dc {

channel_ct generate("GENERATE");

} // namespace dc
} // namespace channels
} // namespace libcwd

namespace {

// Emit the table-generation messages used to test pending flush output.
//
// Inserts synthetic sleep markers directly into `output` to mark to moment where the thread is sleeping.
// The function writes two lines to dc::generate after each.
void generate_tables(libcwd_ctest::PendingStream& output)
{
  output.direct_out() << "<sleeping1>";
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  Dout(dc::generate, "Inside generate_tables()");
  output.direct_out() << "<sleeping2>";
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  Dout(dc::generate, "Leaving generate_tables()");
}

} // namespace

int main()
{
  Debug(main_reached());

  libcwd_ctest::PendingStream captured;
  Debug(libcw_do.set_ostream(&captured));
  ForAllDebugChannels(if (!debugChannel.is_on()) debugChannel.on(););
  Debug(libcw_do.on());

  //Debug( dc::generate.off() );
  Dout( dc::notice|flush_cf|continued_cf, "Generating tables part1... " );
  generate_tables(captured);
  Dout(dc::continued, "part2... ");     // Should still be flushed (before <sleeping1> is printed).
  Dout(dc::notice, "Not flushed");
  generate_tables(captured);
  Dout( dc::finish, "done" );

  captured << std::endl;

  Dout( dc::notice|continued_cf, "Generating tables part3... " );
  generate_tables(captured);
  Dout( dc::continued, "part4... " );
  Dout(dc::notice, "Not flushed");
  generate_tables(captured);
  Dout( dc::finish, "done" );

  Debug(libcw_do.off());

  char const* expected[] = {
    "NOTICE  : Generating tables part1... <sleeping1><sleeping2><unfinished>",
    "GENERATE:     Inside generate_tables()",
    "GENERATE:     Leaving generate_tables()",
    "NOTICE  : <continued> part2... <sleeping1><sleeping2><unfinished>",
    "NOTICE  :     Not flushed",
    "GENERATE:     Inside generate_tables()",
    "GENERATE:     Leaving generate_tables()",
    "NOTICE  : <continued> done",
    "",
    "<sleeping1><sleeping2><sleeping1><sleeping2>NOTICE  : Generating tables part3... <unfinished>",
    "GENERATE:     Inside generate_tables()",
    "GENERATE:     Leaving generate_tables()",
    "NOTICE  : <continued> part4... <unfinished>",
    "NOTICE  :     Not flushed",
    "GENERATE:     Inside generate_tables()",
    "GENERATE:     Leaving generate_tables()",
    "NOTICE  : <continued> done",
    nullptr
  };

  captured.rdbuf()->pubsync();
  std::cout << "Captured output:\n" << captured.output_str() << std::endl;
  return libcwd_ctest::matches_expected_output(captured.input(), expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
