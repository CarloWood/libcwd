#include "sys.h"
#include <libcw/debug.h>
#ifndef LIBCWD_USE_STRSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#include <iostream>

using namespace std;

// CWDEBUG must be define in this test

#ifndef CWDEBUG
#error "CWDEBUG not defined"
#endif

// Creation of debug objects

libcw::debug::debug_ct my_own_do;
namespace example { libcw::debug::debug_ct my_own_do; }
#define MyOwnDout(cntrl, data) LibcwDout(::libcw::debug::channels, my_own_do, cntrl, data)
#define ExampleDout(cntrl, data) LibcwDout(::libcw::debug::channels, example::my_own_do, cntrl, data)

#ifdef THREADTEST
pthread_mutex_t dummy_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

MAIN_FUNCTION
{ PREFIX_CODE
  Debug( check_configuration() );

#if CWDEBUG_LOCATION
  // Make sure we initialized the bfd stuff before we turn on WARNING.
  Debug( (void)pc_mangled_function_name((void*)exit) );
#endif

#ifndef LIBCWD_USE_STRSTREAM
  ostringstream dummy;	// Do this before turning on debug output
#else
  ostrstream dummy;
#endif

  // Test initial ostreams.
  ostream* my_own_os = my_own_do.get_ostream();
  ostream* libcw_os = libcw::debug::libcw_do.get_ostream();
  ostream* coutp = &cout;
  ostream* cerrp = &cerr;

  if (my_own_os != cerrp
#ifndef THREADTEST
     || libcw_os != cerrp	// Already set to cout in threads_threads.cc
#endif
     )
    DoutFatal(dc::fatal, "Initial ostream not cerr");

  THREADED( Debug( my_own_do.set_ostream(cerrp, &cerr_mutex) ) );
  THREADED( Debug( example::my_own_do.set_ostream(cerrp, &cerr_mutex) ) );

  Debug( libcw_do.on() );
#if CWDEBUG_DEBUG
  // Get rid of the `first_time'.
  Debug( my_own_do.on() );
  Debug( my_own_do.off() );
  Debug( example::my_own_do.on() );
  Debug( example::my_own_do.off() );
#endif
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

  Debug( libcw_do.margin().assign("***********libcw_do*", 20) );
  Debug( my_own_do.margin().assign("**********my_own_do*", 20) );
  Debug( example::my_own_do.margin().assign("*example::my_own_do*", 20) );

  Debug( libcw_do.marker().assign("|marker1|", 9) );
  Debug( my_own_do.marker().assign("|marker2|", 9) );
  Debug( example::my_own_do.marker().assign("|marker3|", 9) );

  Dout(dc::debug, "No indent");
  Dout(dc::notice, "No indent");
  Dout(dc::warning, "No indent");

  Debug( libcw_do.set_indent(8) );
  Debug( my_own_do.set_indent(11) );
  Debug( example::my_own_do.set_indent(13) );

  // 1.1.2.3 Retrieving the prefix formatting attributes

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.margin().c_str() << "\", \"" << libcw::debug::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \"" << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.margin().c_str() << "\", \"" << example::my_own_do.marker().c_str() << "\".");

  // Manipulating the format strings.

  Debug( libcw_do.push_margin() );
  Debug( my_own_do.push_margin() );
  Debug( example::my_own_do.push_margin() );

  Debug( libcw_do.margin().append("1Alibcw_do1", 11) );
  Debug( my_own_do.margin().append("1Amy_own_do1", 12) );
  Debug( example::my_own_do.margin().append("1Aexample::my_own_do1", 21) );

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.margin().c_str() << "\", \"" << libcw::debug::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \"" << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.margin().c_str() << "\", \"" << example::my_own_do.marker().c_str() << "\".");

  Debug( libcw_do.margin().prepend("1Plibcw_do1", 11) );
  Debug( my_own_do.margin().prepend("1Pmy_own_do1", 12) );
  Debug( example::my_own_do.margin().prepend("1Pexample::my_own_do1", 21) );

  Debug( libcw_do.push_margin() );
  Debug( my_own_do.push_margin() );
  Debug( example::my_own_do.push_margin() );

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.margin().c_str() << "\", \"" << libcw::debug::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \"" << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.margin().c_str() << "\", \"" << example::my_own_do.marker().c_str() << "\".");

  Debug( libcw_do.margin().assign("", 0) );
  Debug( my_own_do.margin().assign("*", 1) );
  Debug( example::my_own_do.margin().assign("XYZ", 3) );

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.margin().c_str() << "\", \"" << libcw::debug::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \"" << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.margin().c_str() << "\", \"" << example::my_own_do.marker().c_str() << "\".");

  Debug( libcw_do.margin().append("2Alibcw_do2", 11) );
  Debug( my_own_do.margin().append("2Amy_own_do2", 12) );
  Debug( example::my_own_do.margin().append("2Aexample::my_own_do2", 21) );

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.margin().c_str() << "\", \"" << libcw::debug::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \"" << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.margin().c_str() << "\", \"" << example::my_own_do.marker().c_str() << "\".");

  Debug( libcw_do.pop_margin() );
  Debug( my_own_do.pop_margin() );
  Debug( example::my_own_do.pop_margin() );

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.margin().c_str() << "\", \"" << libcw::debug::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \"" << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.margin().c_str() << "\", \"" << example::my_own_do.marker().c_str() << "\".");

  Debug( libcw_do.margin().append("3Alibcw_do3", 11) );
  Debug( my_own_do.margin().append("3Amy_own_do3", 12) );
  Debug( example::my_own_do.margin().append("3Aexample::my_own_do3", 21) );

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.margin().c_str() << "\", \"" << libcw::debug::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \"" << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.margin().c_str() << "\", \"" << example::my_own_do.marker().c_str() << "\".");

  Debug( libcw_do.pop_margin() );
  Debug( my_own_do.pop_margin() );
  Debug( example::my_own_do.pop_margin() );

  Dout(dc::warning, "Dout text " << libcw::debug::libcw_do.get_indent() << ", \"" << libcw::debug::libcw_do.margin().c_str() << "\", \"" << libcw::debug::libcw_do.marker().c_str() << "\".");
  MyOwnDout(dc::warning, "MyOwnDout text " << my_own_do.get_indent() << ", \"" << my_own_do.margin().c_str() << "\", \"" << my_own_do.marker().c_str() << "\".");
  ExampleDout(dc::warning, "ExampleDout text " << example::my_own_do.get_indent() << ", \"" << example::my_own_do.margin().c_str() << "\", \"" << example::my_own_do.marker().c_str() << "\".");

  // 1.1.2.4 Setting and getting the output stream

  Debug( libcw_do.margin().assign("> ", 2) );
  Debug( my_own_do.margin().assign("* ", 2) );
  Debug( example::my_own_do.margin().assign("- ", 2) );

  Debug( libcw_do.marker().assign(": ", 2) );
  Debug( my_own_do.marker().assign(": ", 2) );
  Debug( example::my_own_do.marker().assign(": ", 2) );

  Debug( libcw_do.set_indent(0) );
  Debug( my_own_do.set_indent(0) );
  Debug( example::my_own_do.set_indent(0) );

  my_own_do.set_ostream(&cout COMMA_THREADED(&cout_mutex));
  my_own_os = my_own_do.get_ostream();
  libcw_os = libcw::debug::libcw_do.get_ostream();

  if (my_own_os != coutp
#ifndef THREADTEST
      || libcw_os != cerrp
#endif
      )
    DoutFatal(dc::fatal, "set_ostream failed");

  MyOwnDout(dc::notice, "This is written to cout");
  my_own_do.set_ostream(&dummy COMMA_THREADED(&dummy_mutex));
  MyOwnDout(dc::notice, "This is written to an ostringstream");
  cout << flush;
  Dout(dc::notice, "This is written to cerr");
  my_own_do.set_ostream(cerrp COMMA_THREADED(&cerr_mutex));
  MyOwnDout(dc::notice, "This is written to cerr");
#ifdef LIBCWD_USE_STRSTREAM
  dummy << ends;
#endif
  Dout(dc::warning, "Was written to ostringstream: \"" << dummy.str() << '"');

  EXIT(0);
}
