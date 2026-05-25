// Verify location_ct lookup from code that runs before main().
//
// A global object's constructor enables libcwd output, emits a continued notice
// line, resolves the address of a local label with location_ct, and then finishes
// the notice line.  main() marks libcwd initialization complete and compares the
// captured output line-by-line so regressions in pre-main location lookup are
// reported by CTest.

#include "cwd_sys.h"
#include "test_support.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace {

std::stringstream captured_cout;
std::stringstream captured_debug;

class redirect_cout_ct
{
 private:
  std::streambuf* original_;

 public:
  explicit redirect_cout_ct(std::ostream& output) : original_(std::cout.rdbuf(output.rdbuf())) { }
  redirect_cout_ct(redirect_cout_ct const&) = delete;
  redirect_cout_ct& operator=(redirect_cout_ct const&) = delete;
  ~redirect_cout_ct() { std::cout.rdbuf(original_); }
};

redirect_cout_ct redirect_cout(captured_cout);

class GlobalObject
{
 public:
  // Constructing this object exercises the pre-main location path.  It turns on
  // the minimum debug channels needed for the following Dout calls, redirects the
  // debug object to captured_debug, and records the location of current_location.
  // The label address is in the constructor body and should resolve to this file
  // and the line containing the label.
  GlobalObject();
};

GlobalObject::GlobalObject()
{
  LIBCWD_TSD_DECLARATION;
  Debug(
    libcw_do.set_ostream(&captured_debug);
    if (!dc::notice.is_on())
      dc::notice.on();
    if (!libcw_do.is_on(LIBCWD_TSD))
      libcw_do.on()
  );

  std::cout << "Calling GlobalObject::GlobalObject()\n";
  Dout(dc::notice|continued_cf, "Hello World");
current_location:
  Dout(dc::always, "We are now at " << location_ct(&&current_location));
  Dout(dc::finish|error_cf, "Hello World");
}

GlobalObject global_object;

} // namespace

int main()
{
  Debug(main_reached());
  ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
  Debug(libcw_do.on());

  std::cout << "Successful\n";

  char const* expected_cout[] = {
    "Calling GlobalObject::GlobalObject()",
    "Successful",
    nullptr
  };

  char const* expected_debug[] = {
    "NOTICE  : Hello World<unfinished>",
    ">>>>>>>>:     We are now at location_from_global_object:<unknown function>",
    "NOTICE  : <continued> Hello World: 0 (Success)",
    nullptr
  };

  bool const cout_ok = libcwd_ctest::matches_expected_output(captured_cout, expected_cout);
  bool const debug_ok = libcwd_ctest::matches_expected_output(captured_debug, expected_debug);
  return cout_ok && debug_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
