#include <libcw/sys.h>
#include <libcw/debug.h>

// DEBUG must be define in this test

#ifndef DEBUG
#error "DEBUG not defined"
#endif

// Creation of debug objects

libcw::debug::debug_ct my_own_do;
namespace example { libcw::debug::debug_ct my_own_do; }
#define MyOwnDout(cntrl, data) LibcwDout(::libcw::debug::channels, my_own_do, cntrl, data)
#define ExampleDout(cntrl, data) LibcwDout(::libcw::debug::channels, example::my_own_do, cntrl, data)

int main(void)
{
  Debug( check_configuration() );

  // Make sure we initialized the bfd stuff before we turn on WARNING.
  Debug( (void)pc_mangled_function_name((void*)main) );

  ostrstream dummy;	// Do this before turning on debug output

  Debug( libcw_do.on() );
  Debug( dc::notice.on() );
  Debug( dc::debug.on() );

  int a, b, c;
  a = b = c = 0;

  // 1.1.2.1 Turning debug output on and off

  Dout(dc::notice, "Dout Turned on " << ++a);
  MyOwnDout(dc::notice, "MyOwnDout Turned off " << ++b);
  ExampleDout(dc::notice, "ExampleDout Turned off " << ++c);

  Debug( my_own_do.on() );

  Dout(dc::debug, "Dout Turned on " << ++a);
  MyOwnDout(dc::debug, "MyOwnDout Turned on " << ++b);
  ExampleDout(dc::debug, "ExampleDout Turned off " << ++c);

  Debug( example::my_own_do.on() );
  Debug( libcw_do.off() );

  Dout(dc::notice, "Dout Turned off " << ++a);
  MyOwnDout(dc::notice, "MyOwnDout Turned on " << ++b);
  ExampleDout(dc::notice, "ExampleDout Turned on " << ++c);

  for (int i = 0; i < 5; ++i)
  {
    Debug( my_own_do.off() );
    MyOwnDout(dc::debug, "MyOwnDout " << ++b);
    ExampleDout(dc::debug, "ExampleDout " << ++c);
  }

  for (int i = 0; i < 5; ++i)
  {
    Debug( my_own_do.on() );
    MyOwnDout(dc::notice, "MyOwnDout " << ++b);
    ExampleDout(dc::notice, "ExampleDout " << ++c);
  }

  Debug( libcw_do.on() );
  Dout(dc::debug, a << b << c);

  // 1.1.2.2 Setting the prefix formatting attributes

  Debug( dc::warning.on() );

  Debug( libcw_do.set_margin("***********libcw_do*") );
  Debug( my_own_do.set_margin("**********my_own_do*") );
  Debug( example::my_own_do.set_margin("*example::my_own_do*") );

  Debug( libcw_do.set_marker("|marker1|") );
  Debug( my_own_do.set_marker("|marker2|") );
  Debug( example::my_own_do.set_marker("|marker3|") );

  Dout(dc::debug, "No indent");
  Dout(dc::notice, "No indent");
  Dout(dc::warning, "No indent");

  Debug( libcw_do.set_indent(8) );
  Debug( my_own_do.set_indent(11) );
  Debug( example::my_own_do.set_indent(13) );

  // 1.1.2.3 Retrieving the prefix formatting attributes

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.get_margin() << "\", \"" << libcw::debug::libcw_do.get_marker() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.get_margin() << "\", \"" << my_own_do.get_marker() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.get_margin() << "\", \"" << example::my_own_do.get_marker() << "\".");

  // 1.1.2.4 Setting and getting the output stream

  Debug( libcw_do.set_margin("> ") );
  Debug( my_own_do.set_margin("* ") );
  Debug( example::my_own_do.set_margin("- ") );

  Debug( libcw_do.set_marker(": ") );
  Debug( my_own_do.set_marker(": ") );
  Debug( example::my_own_do.set_marker(": ") );

  Debug( libcw_do.set_indent(0) );
  Debug( my_own_do.set_indent(0) );
  Debug( example::my_own_do.set_indent(0) );

  ostream* my_own_os = my_own_do.get_ostream();
  ostream* libcw_os = libcw::debug::libcw_do.get_ostream();
  ostream* coutp = &cout;
  ostream* cerrp = &cerr;

  if (my_own_os != cerrp || libcw_os != cerrp)
    DoutFatal(dc::fatal, "Initial ostream not cerr");

  my_own_do.set_ostream(&cout);
  my_own_os = my_own_do.get_ostream();
  libcw_os = libcw::debug::libcw_do.get_ostream();

  if (my_own_os != coutp || libcw_os != cerrp)
    DoutFatal(dc::fatal, "set_ostream failed");

  MyOwnDout(dc::notice, "This is written to cout");
  my_own_do.set_ostream(&dummy);
  MyOwnDout(dc::notice, "This is written to an ostrstream");
  Dout(dc::notice, "This is written to cerr");
  my_own_do.set_ostream(cerrp);
  MyOwnDout(dc::notice, "This is written to cerr");
  dummy << ends;
  Dout(dc::warning, "Was written to ostrstream: \"" << dummy.str() << '"');

  exit(0);
}
