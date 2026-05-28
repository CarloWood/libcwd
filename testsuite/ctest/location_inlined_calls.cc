// Test finding the source location of inlined functions for an address inside inlined code.
//
// The companion support source contains inline functions with default arguments.
// The y_default/arg2_i/arg_g callbacks below record the return-address locations observed
// while those inline/default-argument expressions execute.
//
// The intended behavior is that PCs from inlined code report the source line in the
// original inline function body, not merely the expansion site.

#include "cwd_sys.h"
#include "test_support.h"

#include <dlfcn.h>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include "cwd_debug.h"

extern bool f();
uintptr_t executable_load_base();

int a = -12345;
uintptr_t base = executable_load_base();

int y_default(int expected_line)
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  libcwd::location_ct const y_default_loc(addr);
  Dout(dc::always, "y_default() was called from 0x" << std::hex << ((ptrdiff_t)addr - base)  << " --> " << y_default_loc << " (expected: " << std::dec << expected_line << ")");
  return y_default_loc.line() == expected_line;
}

int arg2_i(int expected_line)
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  libcwd::location_ct const arg2_i_loc(addr);
  Dout(dc::always, "arg2_i() was called from 0x" << std::hex << ((ptrdiff_t)addr - base) << " --> " << arg2_i_loc << " (expected: " << std::dec << expected_line << ")");
  return arg2_i_loc.line() == expected_line;
}

int arg_g1(int expected_line)
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  libcwd::location_ct const arg_g1_loc(addr);
  Dout(dc::always, "arg_g1() was called from 0x" << std::hex << ((ptrdiff_t)addr - base) << " --> " << arg_g1_loc << " (expected: " << std::dec << expected_line << ")");
  return arg_g1_loc.line() == expected_line;
}

int arg_g2(int expected_line)
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  libcwd::location_ct const arg_g2_loc(addr);
  Dout(dc::always, "arg_g2() was called from 0x" << std::hex << ((ptrdiff_t)addr - base) << " --> " << arg_g2_loc << " (expected: " << std::dec << expected_line << ")");
  return arg_g2_loc.line() == expected_line;
}

int main()
{
  Debug( main_reached() );
  Debug( libcw_do.set_ostream(&std::cout) );
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );

  Dout(dc::always, "base = 0x" << std::hex << base);

  // Print the source location for an address inside main. This preserves the
  // original intent of this test: exercising location lookup for code in this
  // function without relying on the removed allocation-debugging malloc channel
  // to report the allocation call site as a side effect.
  libcwd::location_ct const f_loc(&&f_call_location);
  int const expected_line = __LINE__ + 3;
  Dout(dc::notice, "Calling f() from main: " << f_loc << " (expected: " << expected_line << ")");
f_call_location:
  return f_loc.line() == expected_line && f() ? EXIT_SUCCESS : EXIT_FAILURE;
}

uintptr_t executable_load_base()
{
  Dl_info info;
  if (dladdr(reinterpret_cast<void*>(&main), &info) == 0)
    return 0;

  return reinterpret_cast<uintptr_t>(info.dli_fbase);
}
