// Verify that location_ct reports source-level locations for PCs that belong to an inlined function body.
//
// The noinline verifier below captures its caller's return address while it is called from an always-inline
// function. With -O2 this creates a concrete DW_TAG_inlined_subroutine range inside outer_call_site(), and the
// captured PC should resolve to the inline function's original body line and function name rather than the
// enclosing physical symbol.

#include "cwd_sys.h"
#include "test_support.h"
#include <libcwd/demangle.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace foo {

int volatile inlined_side_effect_sink;

// Check the return address of the call from inlined_body_probe against the expected source line.
//
// expected_line is the line containing the call inside the inline function. The function has no intended side
// effects except diagnostic output on failure; it returns true only when the file, line and function name all
// describe the original inline body.
[[gnu::noinline]] bool verify_inlined_body_location(unsigned int expected_line)
{
  libcwd::location_ct location((char*)__builtin_return_address(0) + libcwd::builtin_return_address_offset);
  Dout(dc::notice, "verify_inlined_body_location(" << expected_line << "): called from " << location);

  std::string demangled_name;
  libcwd::demangle_symbol(location.mangled_function_name(), demangled_name);

  bool ok = true;
  if (!location.is_known())
  {
    std::cerr << "inlined body location is unknown\n";
    ok = false;
  }
  else if (location.file() != "location_inlined_function_body.cc" || location.line() != expected_line)
  {
    std::cerr << "inlined body location mismatch: got " << location.file() << ':' << location.line()
              << ", expected location_inlined_function_body.cc:" << expected_line << '\n';
    ok = false;
  }

  if (demangled_name.find("inlined_body_probe") == std::string::npos &&
      std::string(location.mangled_function_name()).find("inlined_body_probe") == std::string::npos)
  {
    std::cerr << "inlined body function mismatch: got mangled `" << location.mangled_function_name() << "', demangled `"
              << demangled_name << "', expected `*inlined_body_probe*'.\n";
    ok = false;
  }

  return ok;
}

// Produce an inlined body that contains an address-observable call to verify_inlined_body_location.
//
// value is folded into a volatile side effect after the verifier returns so the optimizer keeps the call in the
// middle of the inline body. The return value lets the caller fold this check into the process exit status.
[[gnu::always_inline]] inline bool inlined_body_probe(int value)
{
  bool const location_ok = verify_inlined_body_location(__LINE__);
  inlined_side_effect_sink += value;
  return location_ok && value == 12345;
}

// Call inlined_body_probe from a stable physical function.
//
// The noinline attribute keeps a distinct enclosing subprogram around the inlined body so a failed inline-aware
// lookup reports this function instead of inlined_body_probe.
[[gnu::noinline]] bool outer_call_site()
{
  return inlined_body_probe(12345);
}

} // namespace foo

int main()
{
  Debug(main_reached());
  Debug(dc::notice.on(); libcw_do.on());
  return foo::outer_call_site() ? EXIT_SUCCESS : EXIT_FAILURE;
}
