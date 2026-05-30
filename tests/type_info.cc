// Verify libcwd::type_info_of demangled names and referred-object sizes under CTest.
//
// This is a CTest-backed form of testsuite/libcwd.tst/type_info.cc. It writes
// the same sequence of type_info_of results to an in-memory stream and compares
// the complete line-oriented output against the legacy exact expectations.

#include "cwd_sys.h"
#include "test_support.h"

#include <cstdlib>
#include <ostream>
#include <sstream>

#include <libcwd/type_info.h>

using libcwd::type_info_of;

// Type with a known object size used to verify that pointer ref_size() reports
// the size of the referred-to object rather than the size of the pointer itself.
struct A
{
  char x[64];
  A() { }
};

// Type with a private constructor used to verify type_info_of<T>() without
// requiring a constructible object. The payload size matches A so pointer
// ref_size() checks remain easy to compare with the legacy expectations.
struct B
{
  friend struct A;
  char x[64];

 private:
  B() { }
};

namespace {

// Emit the legacy type_info test matrix to output.
//
// The output contains one line per expression. The first groups check runtime
// object expressions and explicit template arguments with demangled_name() plus
// ref_size(); the final groups check multi-level pointer const qualification in
// the demangled names. The function has no side effects other than writing to
// the supplied stream.
void write_type_info_output(std::ostream& output)
{
  {
    int i = 42;
    output << type_info_of(i).demangled_name() << " : " << type_info_of(i).ref_size() << '\n';
    int* j = &i;
    output << type_info_of(j).demangled_name() << " : " << type_info_of(j).ref_size() << '\n';
    A a;
    output << type_info_of(a).demangled_name() << " : " << type_info_of(a).ref_size() << '\n';
    A* b = &a;
    output << type_info_of(b).demangled_name() << " : " << type_info_of(b).ref_size() << '\n';
    void* p = j;
    output << type_info_of(p).demangled_name() << " : " << type_info_of(p).ref_size() << '\n';
  }
  {
    int const i = 0;
    output << type_info_of(i).demangled_name() << " : " << type_info_of(i).ref_size() << '\n';
    int const* j = &i;
    output << type_info_of(j).demangled_name() << " : " << type_info_of(j).ref_size() << '\n';
    A const a;
    output << type_info_of(a).demangled_name() << " : " << type_info_of(a).ref_size() << '\n';
    A const* b = &a;
    output << type_info_of(b).demangled_name() << " : " << type_info_of(b).ref_size() << '\n';
  }
  {
    output << type_info_of<B const&>().demangled_name() << " : " << type_info_of<B const&>().ref_size() << '\n';
    output << type_info_of<B const*>().demangled_name() << " : " << type_info_of<B const*>().ref_size() << '\n';
    output << type_info_of<void*>().demangled_name() << " : " << type_info_of<void*>().ref_size() << '\n';
  }
  {
    A const* a1 = nullptr;
    A const* const* a2 = nullptr;
    A const* const* const* a3 = nullptr;
    output << type_info_of(a1).demangled_name() << '\n';
    output << type_info_of(a2).demangled_name() << '\n';
    output << type_info_of(a3).demangled_name() << '\n';
  }
  {
    output << type_info_of<B const*>().demangled_name() << '\n';
    output << type_info_of<B const* const*>().demangled_name() << '\n';
    output << type_info_of<B const* const* const*>().demangled_name() << '\n';
  }
}

} // namespace

int main()
{
  Debug(main_reached());

  std::stringstream captured;
  write_type_info_output(captured);

  char const* expected[] = {
    "int : 0",
    "int* : 4",
    "A : 0",
    "A* : 64",
    "void* : 0",
    "int : 0",
    "int const* : 4",
    "A : 0",
    "A const* : 64",
    "B const& : 0",
    "B const* : 64",
    "void* : 0",
    "A const*",
    "A const* const*",
    "A const* const* const*",
    "B const*",
    "B const* const*",
    "B const* const* const*",
    nullptr
  };

  return libcwd_ctest::matches_expected_output(captured, expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
