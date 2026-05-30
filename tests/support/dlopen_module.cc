// Shared module used by the CTest-backed dlopen fixture.
//
// The exported global_test_symbol(bool) keeps the legacy C++ symbol name that
// testsuite/libcwd.tst/dlopen.cc resolved with dlsym. Instead of allocating
// memory merely to make malloc debugging report a source location, this module
// reports the call sites directly through dc::notice and verifies that libcwd can
// resolve those return addresses while the module is loaded with dlopen.

#include "cwd_sys.h"
#include "test_support.h"

#include <cstring>
#include <string>

namespace {

// Report and validate the call site of this helper's caller.
//
// scenario names the logical branch that is being exercised, expected_line is
// the source line containing the call to this helper, and expected_function is
// the function name that should own that source location. The function
// writes one deterministic notice line and returns false after writing a notice
// diagnostic when any location field differs from the expectation.
[[gnu::noinline]] bool report_call_site(char const* scenario, unsigned int expected_line, char const* expected_function)
{
  char const* addr = static_cast<char const*>(__builtin_return_address(0)) + libcwd::builtin_return_address_offset;
  libcwd::location_ct const location(addr);

  Dout(dc::notice, scenario << ": " << location << " (expected line: " << expected_line << ")");

  bool success = true;
  if (!location.is_known())
  {
    Dout(dc::notice, scenario << ": location is unknown");
    return false;
  }
  if (location.file() != "dlopen_module.cc")
  {
    Dout(dc::notice, scenario << ": file was " << location.file() << ", expected dlopen_module.cc");
    success = false;
  }
  if (location.line() != expected_line)
  {
    Dout(dc::notice, scenario << ": line was " << location.line() << ", expected " << expected_line);
    success = false;
  }
  if (std::strcmp(location.mangled_function_name(), expected_function) != 0)
  {
    Dout(dc::notice,
         scenario << ": function was " << location.mangled_function_name() << ", expected " << expected_function);
    success = false;
  }
  return success;
}

// Exercise a source location inside a file-local function in the dlopen module.
//
// Returns true only when the helper reports that the line belongs to this static
// function. Marking the function noinline keeps the return address stable even
// if the surrounding test build gains optimization flags later.
[[gnu::noinline]] static bool static_test_symbol()
{
  return report_call_site("static_test_symbol", __LINE__, "static_test_symbol");
}

} // namespace

// Exercise a source location inside an exported C++ function in the dlopen module.
//
// do_static selects the branch to test: false reports the call site in this
// exported function, while true delegates to static_test_symbol(). The return
// value tells the dlopen fixture whether the module-side location checks passed.
[[gnu::noinline]] bool global_test_symbol(bool do_static)
{
  if (do_static)
    return static_test_symbol();
  return report_call_site("global_test_symbol(false)", __LINE__, "_Z18global_test_symbolb");
}
