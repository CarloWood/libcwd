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

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <dlfcn.h>

extern bool f();
uintptr_t executable_load_base();

int a = -12345;
uintptr_t base = executable_load_base();

int y_default(unsigned int expected_line)
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  libcwd::Location const y_default_loc(addr);
  Dout(dc::always, "y_default() was called from 0x" << std::hex << ((ptrdiff_t)addr - base) << " --> " << y_default_loc
                                                    << " (expected: " << std::dec << expected_line << ")");
  return y_default_loc.line() == expected_line;
}

int arg2_i(unsigned int expected_line)
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  libcwd::Location const arg2_i_loc(addr);
  Dout(dc::always, "arg2_i() was called from 0x" << std::hex << ((ptrdiff_t)addr - base) << " --> " << arg2_i_loc
                                                 << " (expected: " << std::dec << expected_line << ")");
  return arg2_i_loc.line() == expected_line;
}

int arg_g1(unsigned int expected_line)
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  libcwd::Location const arg_g1_loc(addr);
  Dout(dc::always, "arg_g1() was called from 0x" << std::hex << ((ptrdiff_t)addr - base) << " --> " << arg_g1_loc
                                                 << " (expected: " << std::dec << expected_line << ")");
  // Unfortunately, a DWARF producer is not required to have more fine grained line tables than one line per statement.
  // Using g++ -g produces the same line as that of the call to g() in this case.
  return arg_g1_loc.line() == expected_line || arg_g1_loc.line() == expected_line - 1;
}

int arg_g2(unsigned int expected_line)
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  libcwd::Location const arg_g2_loc(addr);
  Dout(dc::always, "arg_g2() was called from 0x" << std::hex << ((ptrdiff_t)addr - base) << " --> " << arg_g2_loc
                                                 << " (expected: " << std::dec << expected_line << ")");
  // Unfortunately, a DWARF producer is not required to have more fine grained line tables than one line per statement.
  // Using g++ -g produces the same line as that of the call to g() in this case.
  return arg_g2_loc.line() == expected_line || arg_g2_loc.line() == expected_line - 2;
}

int main()
{
  Debug(main_reached());
  Debug(libcw_do.set_ostream(&std::cout));
  Debug(libcw_do.on());
  Debug(dc::notice.on());

  void* pc = &&f_call_location;
  uintptr_t offset = reinterpret_cast<uintptr_t>(pc) - base;
  Dout(dc::always, "main: base = 0x" << std::hex << base << ", pc = " << pc << ", pc - base = 0x" << offset);

  // Print the source location for an address inside main. This preserves the
  // original intent of this test: exercising location lookup for code in this
  // function without relying on the removed allocation-debugging malloc channel
  // to report the allocation call site as a side effect.
  libcwd::Location const f_loc(pc);
  // gcc resolves the address of f_call_location to the line following it.
  // In the case of clang libcwd resolves that address to the line of first instruction after the label.
  // In this case that is the same line in both cases.
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
