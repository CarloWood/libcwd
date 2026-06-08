// Verify debug object state, formatting, and stream switching under CTest.
//
// This ports testsuite/libcwd.tst/do.cc. The test keeps the same three debug
// objects, channel toggles, margin/marker/indent manipulations, and ostream
// changes, while temporarily redirecting std::cout and std::cerr to one in-memory
// stream so the legacy combined output can be compared exactly.

#include "cwd_sys.h"
#include "test_support.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

#ifndef CWDEBUG
#error "CWDEBUG not defined"
#endif

libcwd::DebugObject my_own_do;
namespace example {
libcwd::DebugObject my_own_do;
}

#define MyOwnDout(cntrl, data) LibcwDout(::libcwd::channels, my_own_do, cntrl, data)
#define ExampleDout(cntrl, data) LibcwDout(::libcwd::channels, example::my_own_do, cntrl, data)

namespace {

// Redirect one ostream to another stream buffer for the object's lifetime.
//
// The caller owns both streams and must keep the destination alive until this
// object is destroyed. Copying is disabled so each saved stream buffer is
// restored exactly once.
class RedirectStream
{
 private:
  std::ostream& M_stream;
  std::streambuf* M_original;

 public:
  RedirectStream(std::ostream& stream, std::ostream& destination)
      : M_stream(stream), M_original(stream.rdbuf(destination.rdbuf()))
  {
  }
  RedirectStream(RedirectStream const&) = delete;
  RedirectStream& operator=(RedirectStream const&) = delete;
  ~RedirectStream() { M_stream.rdbuf(M_original); }
};

// Execute the legacy do.cc debug-output scenario.
//
// Returns false only when a stream-pointer sanity check fails. The intended side
// effect is the same sequence of Dout, MyOwnDout, and ExampleDout writes that the
// legacy DejaGnu test expected.
bool run_do_scenario()
{
#if CWDEBUG_LOCATION
  // Make sure symbol lookup has initialized before WARNING output is tested.
  Debug((void)pc_mangled_function_name((void*)exit));
#endif

  std::ostringstream dummy;

  std::ostream* my_own_os = my_own_do.get_ostream();
  std::ostream* libcwd_os = libcwd::libcw_do.get_ostream();
  std::ostream* coutp = &std::cout;
  std::ostream* cerrp = &std::cerr;

  if (my_own_os != cerrp || libcwd_os != cerrp)
  {
    std::cerr << "Initial ostream not cerr\n";
    return false;
  }

  Debug(libcw_do.on());
#if CWDEBUG_DEBUG
  // Get rid of the `first_time'.
  Debug(my_own_do.on());
  Debug(my_own_do.off());
  Debug(example::my_own_do.on());
  Debug(example::my_own_do.off());
#endif
  Debug(dc::notice.on());
  Debug(dc::debug.on());

  int a, b, c;
  a = b = c = 0;

  Dout(dc::notice, "Dout Turned on " << ++a);
  MyOwnDout(dc::notice, "MyOwnDout Turned off " << ++b);
  ExampleDout(dc::notice, "ExampleDout Turned off " << ++c);

  Debug(my_own_do.on());

  Dout(dc::debug, "Dout Turned on " << ++a);
  MyOwnDout(dc::debug, "MyOwnDout Turned on " << ++b);
  ExampleDout(dc::debug, "ExampleDout Turned off " << ++c);

  Debug(example::my_own_do.on());
  Debug(libcw_do.off());

  Dout(dc::notice, "Dout Turned off " << ++a);
  MyOwnDout(dc::notice, "MyOwnDout Turned on " << ++b);
  ExampleDout(dc::notice, "ExampleDout Turned on " << ++c);

  for (int i = 0; i < 5; ++i)
  {
    Debug(my_own_do.off());
    MyOwnDout(dc::debug, "MyOwnDout " << ++b);
    ExampleDout(dc::debug, "ExampleDout " << ++c);
  }

  for (int i = 0; i < 5; ++i)
  {
    Debug(my_own_do.on());
    MyOwnDout(dc::notice, "MyOwnDout " << ++b);
    ExampleDout(dc::notice, "ExampleDout " << ++c);
  }

  Debug(libcw_do.on());
  Dout(dc::debug, a << b << c);

  Debug(libcw_do.margin().assign("***********libcw_do*", 20));
  Debug(my_own_do.margin().assign("**********my_own_do*", 20));
  Debug(example::my_own_do.margin().assign("*example::my_own_do*", 20));

  Debug(libcw_do.marker().assign("|marker1|", 9));
  Debug(my_own_do.marker().assign("|marker2|", 9));
  Debug(example::my_own_do.marker().assign("|marker3|", 9));

  Dout(dc::debug, "No indent");
  Dout(dc::notice, "No indent");
  Dout(dc::warning, "No indent");

  Debug(libcw_do.set_indent(8));
  Debug(my_own_do.set_indent(11));
  Debug(example::my_own_do.set_indent(13));

  Dout(dc::warning, "Dout text " << libcwd::libcw_do.get_indent() << ", \"" << libcwd::libcw_do.margin().c_str()
                                 << "\", \"" << libcwd::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \""
                                           << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \""
                                               << example::my_own_do.margin().c_str() << "\", \""
                                               << example::my_own_do.marker().c_str() << "\".");

  Debug(libcw_do.push_margin());
  Debug(my_own_do.push_margin());
  Debug(example::my_own_do.push_margin());

  Debug(libcw_do.margin().append("1Alibcw_do1", 11));
  Debug(my_own_do.margin().append("1Amy_own_do1", 12));
  Debug(example::my_own_do.margin().append("1Aexample::my_own_do1", 21));

  Dout(dc::warning, "Dout text " << libcwd::libcw_do.get_indent() << ", \"" << libcwd::libcw_do.margin().c_str()
                                 << "\", \"" << libcwd::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \""
                                           << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \""
                                               << example::my_own_do.margin().c_str() << "\", \""
                                               << example::my_own_do.marker().c_str() << "\".");

  Debug(libcw_do.margin().prepend("1Plibcw_do1", 11));
  Debug(my_own_do.margin().prepend("1Pmy_own_do1", 12));
  Debug(example::my_own_do.margin().prepend("1Pexample::my_own_do1", 21));

  Debug(libcw_do.push_margin());
  Debug(my_own_do.push_margin());
  Debug(example::my_own_do.push_margin());

  Dout(dc::warning, "Dout text " << libcwd::libcw_do.get_indent() << ", \"" << libcwd::libcw_do.margin().c_str()
                                 << "\", \"" << libcwd::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \""
                                           << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \""
                                               << example::my_own_do.margin().c_str() << "\", \""
                                               << example::my_own_do.marker().c_str() << "\".");

  Debug(libcw_do.margin().assign("", 0));
  Debug(my_own_do.margin().assign("*", 1));
  Debug(example::my_own_do.margin().assign("XYZ", 3));

  Dout(dc::warning, "Dout text " << libcwd::libcw_do.get_indent() << ", \"" << libcwd::libcw_do.margin().c_str()
                                 << "\", \"" << libcwd::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \""
                                           << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \""
                                               << example::my_own_do.margin().c_str() << "\", \""
                                               << example::my_own_do.marker().c_str() << "\".");

  Debug(libcw_do.margin().append("2Alibcw_do2", 11));
  Debug(my_own_do.margin().append("2Amy_own_do2", 12));
  Debug(example::my_own_do.margin().append("2Aexample::my_own_do2", 21));

  Dout(dc::warning, "Dout text " << libcwd::libcw_do.get_indent() << ", \"" << libcwd::libcw_do.margin().c_str()
                                 << "\", \"" << libcwd::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \""
                                           << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \""
                                               << example::my_own_do.margin().c_str() << "\", \""
                                               << example::my_own_do.marker().c_str() << "\".");

  Debug(libcw_do.pop_margin());
  Debug(my_own_do.pop_margin());
  Debug(example::my_own_do.pop_margin());

  Dout(dc::warning, "Dout text " << libcwd::libcw_do.get_indent() << ", \"" << libcwd::libcw_do.margin().c_str()
                                 << "\", \"" << libcwd::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \""
                                           << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \""
                                               << example::my_own_do.margin().c_str() << "\", \""
                                               << example::my_own_do.marker().c_str() << "\".");

  Debug(libcw_do.margin().append("3Alibcw_do3", 11));
  Debug(my_own_do.margin().append("3Amy_own_do3", 12));
  Debug(example::my_own_do.margin().append("3Aexample::my_own_do3", 21));

  Dout(dc::warning, "Dout text " << libcwd::libcw_do.get_indent() << ", \"" << libcwd::libcw_do.margin().c_str()
                                 << "\", \"" << libcwd::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \""
                                           << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \""
                                               << example::my_own_do.margin().c_str() << "\", \""
                                               << example::my_own_do.marker().c_str() << "\".");

  Debug(libcw_do.pop_margin());
  Debug(my_own_do.pop_margin());
  Debug(example::my_own_do.pop_margin());

  Dout(dc::warning, "Dout text " << libcwd::libcw_do.get_indent() << ", \"" << libcwd::libcw_do.margin().c_str()
                                 << "\", \"" << libcwd::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \""
                                           << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \""
                                               << example::my_own_do.margin().c_str() << "\", \""
                                               << example::my_own_do.marker().c_str() << "\".");

  Debug(libcw_do.margin().assign("> ", 2));
  Debug(my_own_do.margin().assign("* ", 2));
  Debug(example::my_own_do.margin().assign("- ", 2));

  Debug(libcw_do.marker().assign(": ", 2));
  Debug(my_own_do.marker().assign(": ", 2));
  Debug(example::my_own_do.marker().assign(": ", 2));

  Debug(libcw_do.set_indent(0));
  Debug(my_own_do.set_indent(0));
  Debug(example::my_own_do.set_indent(0));

  my_own_do.set_ostream(&std::cout);
  my_own_os = my_own_do.get_ostream();
  libcwd_os = libcwd::libcw_do.get_ostream();

  if (my_own_os != coutp || libcwd_os != cerrp)
  {
    std::cerr << "set_ostream failed\n";
    return false;
  }

  MyOwnDout(dc::notice, "This is written to cout");
  my_own_do.set_ostream(&dummy);
  MyOwnDout(dc::notice, "This is written to an ostringstream");
  std::cout << std::flush;
  Dout(dc::notice, "This is written to cerr");
  my_own_do.set_ostream(cerrp);
  MyOwnDout(dc::notice, "This is written to cerr");
  Dout(dc::warning, "Was written to ostringstream: \"" << dummy.str() << '"');

  return true;
}

// Compare the captured output with the exact legacy expectations.
//
// The final warning contains the newline that was stored in the intermediate
// ostringstream, so it is intentionally represented by two expected lines.
bool output_matches_expected(std::istream& captured)
{
  char const* expected[] = {
      "NOTICE  : Dout Turned on 1",
      "DEBUG   : Dout Turned on 2",
      "DEBUG   : MyOwnDout Turned on 1",
      "NOTICE  : MyOwnDout Turned on 2",
      "NOTICE  : ExampleDout Turned on 1",
      "DEBUG   : ExampleDout 2",
      "DEBUG   : ExampleDout 3",
      "DEBUG   : ExampleDout 4",
      "DEBUG   : ExampleDout 5",
      "DEBUG   : ExampleDout 6",
      "NOTICE  : ExampleDout 7",
      "NOTICE  : ExampleDout 8",
      "NOTICE  : ExampleDout 9",
      "NOTICE  : ExampleDout 10",
      "NOTICE  : MyOwnDout 3",
      "NOTICE  : ExampleDout 11",
      "DEBUG   : 2311",
      "***********libcw_do*DEBUG   |marker1|No indent",
      "***********libcw_do*NOTICE  |marker1|No indent",
      "***********libcw_do*WARNING |marker1|No indent",
      "***********libcw_do*WARNING |marker1|        Dout text 8, \"***********libcw_do*\", \"|marker1|\".",
      "**********my_own_do*WARNING |marker2|           MyOwnDout text 11, \"**********my_own_do*\", \"|marker2|\".",
      "*example::my_own_do*WARNING |marker3|             ExampleDout text 13, \"*example::my_own_do*\", \"|marker3|\".",
      "***********libcw_do*1Alibcw_do1WARNING |marker1|        Dout text 8, \"***********libcw_do*1Alibcw_do1\", "
      "\"|marker1|\".",
      "**********my_own_do*1Amy_own_do1WARNING |marker2|           MyOwnDout text 11, "
      "\"**********my_own_do*1Amy_own_do1\", \"|marker2|\".",
      "*example::my_own_do*1Aexample::my_own_do1WARNING |marker3|             ExampleDout text 13, "
      "\"*example::my_own_do*1Aexample::my_own_do1\", \"|marker3|\".",
      "1Plibcw_do1***********libcw_do*1Alibcw_do1WARNING |marker1|        Dout text 8, "
      "\"1Plibcw_do1***********libcw_do*1Alibcw_do1\", \"|marker1|\".",
      "1Pmy_own_do1**********my_own_do*1Amy_own_do1WARNING |marker2|           MyOwnDout text 11, "
      "\"1Pmy_own_do1**********my_own_do*1Amy_own_do1\", \"|marker2|\".",
      "1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do1WARNING |marker3|             ExampleDout text "
      "13, \"1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do1\", \"|marker3|\".",
      "WARNING |marker1|        Dout text 8, \"\", \"|marker1|\".",
      "*WARNING |marker2|           MyOwnDout text 11, \"*\", \"|marker2|\".",
      "XYZWARNING |marker3|             ExampleDout text 13, \"XYZ\", \"|marker3|\".",
      "2Alibcw_do2WARNING |marker1|        Dout text 8, \"2Alibcw_do2\", \"|marker1|\".",
      "*2Amy_own_do2WARNING |marker2|           MyOwnDout text 11, \"*2Amy_own_do2\", \"|marker2|\".",
      "XYZ2Aexample::my_own_do2WARNING |marker3|             ExampleDout text 13, \"XYZ2Aexample::my_own_do2\", "
      "\"|marker3|\".",
      "1Plibcw_do1***********libcw_do*1Alibcw_do1WARNING |marker1|        Dout text 8, "
      "\"1Plibcw_do1***********libcw_do*1Alibcw_do1\", \"|marker1|\".",
      "1Pmy_own_do1**********my_own_do*1Amy_own_do1WARNING |marker2|           MyOwnDout text 11, "
      "\"1Pmy_own_do1**********my_own_do*1Amy_own_do1\", \"|marker2|\".",
      "1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do1WARNING |marker3|             ExampleDout text "
      "13, \"1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do1\", \"|marker3|\".",
      "1Plibcw_do1***********libcw_do*1Alibcw_do13Alibcw_do3WARNING |marker1|        Dout text 8, "
      "\"1Plibcw_do1***********libcw_do*1Alibcw_do13Alibcw_do3\", \"|marker1|\".",
      "1Pmy_own_do1**********my_own_do*1Amy_own_do13Amy_own_do3WARNING |marker2|           MyOwnDout text 11, "
      "\"1Pmy_own_do1**********my_own_do*1Amy_own_do13Amy_own_do3\", \"|marker2|\".",
      "1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do13Aexample::my_own_do3WARNING |marker3|            "
      " ExampleDout text 13, \"1Pexample::my_own_do1*example::my_own_do*1Aexample::my_own_do13Aexample::my_own_do3\", "
      "\"|marker3|\".",
      "***********libcw_do*WARNING |marker1|        Dout text 8, \"***********libcw_do*\", \"|marker1|\".",
      "**********my_own_do*WARNING |marker2|           MyOwnDout text 11, \"**********my_own_do*\", \"|marker2|\".",
      "*example::my_own_do*WARNING |marker3|             ExampleDout text 13, \"*example::my_own_do*\", \"|marker3|\".",
      "* NOTICE  : This is written to cout",
      "> NOTICE  : This is written to cerr",
      "* NOTICE  : This is written to cerr",
      "> WARNING : Was written to ostringstream: \"* NOTICE  : This is written to an ostringstream",
      "\"",
      nullptr};

  return libcwd_ctest::matches_expected_output(captured, expected);
}

} // namespace

int main()
{
  Debug(main_reached());

  std::stringstream captured;
  bool scenario_ok;
  {
    RedirectStream redirect_cout(std::cout, captured);
    RedirectStream redirect_cerr(std::cerr, captured);
    scenario_ok = run_do_scenario();
  }

  return output_matches_expected(captured) && scenario_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
