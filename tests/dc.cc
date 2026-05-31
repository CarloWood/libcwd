// Verify debug-channel state changes, custom channels, and multi-channel output.
//
// This is a CTest-backed form of testsuite/libcwd.tst/dc.cc. It defines the
// legacy WARP channel in the test application's debug namespace, captures the
// default debug object's output, toggles all registered channels with
// ForAllDebugChannels, and compares the exact channel listing and message output
// requested by the ported test.

#include "cwd_sys.h"

#define NAMESPACE_DEBUG dc_ctest::debug
#include "PendingStream.h"
#include "test_support.h"

#include <cstdlib>
#include <sstream>

// Define the application-owned custom channel used by this test. The channel is
// registered globally so ForAllDebugChannels and list_channels_on() include it
// together with libcwd's built-in channels.
NAMESPACE_DEBUG_CHANNELS_START
channel_ct warp("WARP");
NAMESPACE_DEBUG_CHANNELS_END

int main()
{
  Debug(main_reached());

  libcwd_ctest::PendingStream captured;
  Debug(libcw_do.set_ostream(&captured));
  Debug(libcw_do.on());
  Debug(dc::warp.on());
  Debug(dc::notice.on());

  Dout(dc::notice, "Debug channel Test.");
  Dout(dc::warp, "Custom channel Test.");

  ForAllDebugChannels(if (debugChannel.is_on()) debugChannel.off());
  Debug(list_channels_on(libcw_do));
  ForAllDebugChannels(while (!debugChannel.is_on()) debugChannel.on());
  Debug(list_channels_on(libcw_do));

  Dout(dc::elfutils, "elfutils Testing");
  Dout(dc::debug, "debug Testing");
  Dout(dc::notice, "notice Testing");
  Dout(dc::system, "system Testing");
  Dout(dc::warning, "warning Testing");

  Dout(dc::notice | dc::system | dc::warning, "Hello World");
  Debug(dc::notice.off());
  Dout(dc::notice | dc::system | dc::warning, "Hello World");
  Debug(dc::system.off());
  Dout(dc::notice | dc::system | dc::warning, "Hello World");
  Debug(dc::warning.off());
  Dout(dc::notice | dc::system | dc::warning, "Hello World (not)");

  captured.rdbuf()->pubsync();
  std::cout << "Captured output:\n" << captured.output_str() << std::endl;

  char const* expected[] = {"NOTICE  : Debug channel Test.",
                            "WARP    : Custom channel Test.",
                            "DEBUG   : Disabled",
                            "ELFUTILS: Disabled",
                            "NOTICE  : Disabled",
                            "SYSTEM  : Disabled",
                            "WARNING : Disabled",
                            "WARP    : Disabled",
                            "DEBUG   : Enabled",
                            "ELFUTILS: Enabled",
                            "NOTICE  : Enabled",
                            "SYSTEM  : Enabled",
                            "WARNING : Enabled",
                            "WARP    : Enabled",
                            "ELFUTILS: elfutils Testing",
                            "DEBUG   : debug Testing",
                            "NOTICE  : notice Testing",
                            "SYSTEM  : system Testing",
                            "WARNING : warning Testing",
                            "NOTICE  : Hello World",
                            "SYSTEM  : Hello World",
                            "WARNING : Hello World",
                            nullptr};

  return libcwd_ctest::matches_expected_output(captured.input(), expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
