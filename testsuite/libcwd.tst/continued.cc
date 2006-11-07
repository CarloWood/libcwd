#include "sys.h"
#include <libcwd/debug.h>
#include <unistd.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#ifdef LIBCWD_USE_STRSTREAM
#include <strstream>
#else
#include <sstream>
#endif
#include <time.h>

struct timespec const one_ms = { 0, 1000000 };

static void slowdown(void)
{
  struct timespec rem = one_ms; 
  while (nanosleep(&rem, &rem) == -1);
}

using namespace std;

#ifdef REAL_CERR
#define DEBUG_CERR
#define USE_REAL_CERR
#endif
#ifdef WITHOUT_CERR
#define DEBUG_CERR
#define cerr_cf 0
#endif
#ifdef WITH_CERR
#define DEBUG_CERR
#endif

namespace libcwd {
  namespace channels {
    namespace dc {
      channel_ct foo("FOO");
      channel_ct bar("BAR");
      channel_ct run("RUN");
    }
  }
}

#ifndef REAL_CERR
#ifdef LIBCWD_USE_STRSTREAM
// No idea why, but it doesn't work when strstream is dynamic.
static char ssbuf[1024];
static strstream ss(ssbuf, sizeof(ssbuf));
#else
static stringstream ss;
#endif
static streambuf* old_buf;
#endif

#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
#define ios_base ios			// Kludge.
#endif

void grab_cerr(void)
{
#ifndef REAL_CERR
  old_buf = cerr.rdbuf();
  cerr.rdbuf(ss.rdbuf());
#endif
}

void release_cerr(void)
{
#ifndef REAL_CERR
  cerr.rdbuf(old_buf);
#endif
}

void flush_cout(void)
{
  std::cout << flush;
  slowdown();
}

void flush_cerr(void)
{
#ifdef REAL_CERR
  cerr << flush;
#else
  size_t curlen = ss.rdbuf()->pubseekoff(0, std::ios_base::cur, std::ios_base::out) - ss.rdbuf()->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  std::string buf;
  buf.append(ss.str(), 0, curlen);
#ifdef DEBUG_CERR
  cout << buf;
#else
  bool color = false;
  char const* p = buf.data();
  for (size_t i = 0; i < curlen; ++i)
  {
    if (!color)
    {
      cout << "\e[31m";
      color = true;
    }
    cout.put(*p);
    if (*p++ == '\n')
    {
      cout << "\e[0m";
      color = false;
    }
  }
#endif
  cout << flush;
  slowdown();
  ss.rdbuf()->pubseekoff(0, std::ios_base::beg, std::ios_base::in|std::ios_base::out);
#endif
}

char const* nested_foo(bool with_error, bool to_cerr)
{
  if (with_error)
  {
    if (to_cerr)
    {
      flush_cout();
      errno = 0;
      Dout( dc::foo|cerr_cf|error_cf, "CERR: Inside `nested_foo()'" );
      flush_cerr();
    }
    else
    {
      errno = 0;
      Dout( dc::foo|error_cf, "Inside `nested_foo()'" );
    }
  }
  else
  {
    if (to_cerr)
    {
      flush_cout();
      Dout( dc::foo|cerr_cf, "CERR: Inside `nested_foo()'" );
      flush_cerr();
    }
    else
      Dout( dc::foo, "Inside `nested_foo()'" );
  }
  return "Foo";
}

char const* nested_bar(bool bar_with_error, bool bar_to_cerr, bool foo_with_error, bool foo_to_cerr)
{
  if (bar_with_error)
  {
    if (bar_to_cerr)
    {
      flush_cout();
      errno = EINVAL;
      Dout( dc::bar|cerr_cf|error_cf, "CERR: Entering `nested_bar()'" );
      flush_cerr();
      flush_cout();
      errno = EINVAL;
      Dout( dc::bar|cerr_cf|error_cf, "CERR: `nested_foo(" << foo_with_error << ", " << foo_to_cerr << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it." );
      flush_cerr();
      flush_cout();
      errno = EINVAL;
      Dout( dc::bar|cerr_cf|error_cf, "CERR: Leaving `nested_bar()'" );
      flush_cerr();
    }
    else
    {
      errno = EINVAL;
      Dout( dc::bar|error_cf, "Entering `nested_bar()'" );
      errno = EINVAL;
      Dout( dc::bar|error_cf, "`nested_foo(" << foo_with_error << ", " << foo_to_cerr << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it." );
      errno = EINVAL;
      Dout( dc::bar|error_cf, "Leaving `nested_bar()'" );
    }
  }
  else
  {
    if (bar_to_cerr)
    {
      flush_cout();
      Dout( dc::bar|cerr_cf, "CERR: Entering `nested_bar()'" );
      flush_cerr();
      flush_cout();
      Dout( dc::bar|cerr_cf, "CERR: `nested_foo(" << foo_with_error << ", " << foo_to_cerr << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it." );
      flush_cerr();
      flush_cout();
      Dout( dc::bar|cerr_cf, "CERR: Leaving `nested_bar()'" );
      flush_cerr();
    }
    else
    {
      Dout( dc::bar, "Entering `nested_bar()'" );
      Dout( dc::bar, "`nested_foo(" << foo_with_error << ", " << foo_to_cerr << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it." );
      Dout( dc::bar, "Leaving `nested_bar()'" );
    }
  }
  return "Bar";
}

char const* continued_func(unsigned int what)
{
  if (--what == 0)
    return "BOTTOM";
  Dout( dc::run|continued_cf, "1" );
  // The order of evaluation of x() and y() in f(x(), y()) is undetermined,
  // therefore we call the recursive functions outside the << << <<, forcing
  // a fixed order.
  Dout( dc::foo, ""; char const* str3 = nested_foo(what & 2, what & 1); char const* str2 = continued_func(what); char const* str1 = nested_foo(what & 1, what & 2); (*LIBCWD_DO_TSD_MEMBER(::libcwd::libcw_do, current_bufferstream)) << str1 << what << str2 << what << str3 );
  Dout( dc::continued, "2" );
  Dout( dc::foo, ""; char const* str3 = nested_foo(what & 2, what & 1); char const* str2 = continued_func(what); char const* str1 = nested_foo(what & 1, what & 2); (*LIBCWD_DO_TSD_MEMBER(::libcwd::libcw_do, current_bufferstream)) << str1 << what << str2 << what << str3 );
  Dout( dc::finish, "3" );
  return ":";
}

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_LOCATION
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );

  grab_cerr();

  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off(); );
  Debug( dc::notice.on() );
  Debug( dc::system.on() );
  Debug( dc::foo.on() );
  Debug( dc::bar.on() );
  Debug( dc::run.on() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() );

  // Print channels
  Debug( list_channels_on(libcw_do) );

  //===================================================================================
  // Unnested tests

  cout << "===========================================================================\n";
  cout << " Unnested tests\n\n";

  // Simple, one line debug output
  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::notice, "This is a single line" );

  // The same, but splitting it by using `nonewline_cf' and `noprefix_cf'
  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::notice|nonewline_cf, "This is " );
  Dout( dc::notice|noprefix_cf, "a single line" );

  // The same, but writing an error message behind it
  cout << "---------------------------------------------------------------------------\n";
  errno = 0;
  Dout( dc::notice|error_cf, "This is a single line with an error message behind it" );

  // Test writing forced to cerr
  cout << "---------------------------------------------------------------------------\n";
  flush_cout();
  errno = 0;
  Dout( dc::notice|error_cf|cerr_cf, "CERR: This is a single line with an error message behind it written to cerr" );
  flush_cerr();

  //===================================================================================
  // Simple nests

  cout << "===========================================================================\n";
  cout << " Simple nests\n\n";

  // Single depth
  for(int i = 0; i < 4; ++i)
  {
    bool a = (i & 2), b = (i & 1);
    cout << "---------------------------------------------------------------------------\n";
    Dout( dc::notice, "`nested_foo(" << a << ", " << b << ")' returns the string \"" << nested_foo(a, b) << "\" when I call it." );
  }
    
  // Double depth
  for(int i = 0; i < 16; ++i)
  {
    bool a = (i & 8), b = (i & 4), c = (i & 2), d = (i & 1);
    cout << "---------------------------------------------------------------------------\n";
    Dout( dc::notice, "`nested_bar(" << a << ", " << b << ", " << c << ", " << d << ")' returns the string \"" << nested_bar(a, b, c, d) << "\" when I call it." );
  }

  //===================================================================================
  // Continued tests, single depth

  cout << "===========================================================================\n";
  cout << " Continued tests, single depth\n\n";

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Hello " );
  Dout( dc::finish, "World" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcwd " );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcwd " );
  Dout( dc::foo, "Single interruption before." );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcwd " );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::foo, "Single interruption after." );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcwd " );
  Dout( dc::foo, "Single interruption before and" );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::foo, "a single interruption after." );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcwd " );
  Dout( dc::foo, "Double interruption before," );
  Dout( dc::foo, "double means two lines." );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcwd " );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::foo, "Double interruption after," );
  Dout( dc::foo, "double means two lines." );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcwd " );
  Dout( dc::foo, "Double interruption before and" );
  Dout( dc::foo, "(double means two lines)" );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::foo, "a double interruption after" );
  Dout( dc::foo, "(double means two lines)" );
  Dout( dc::finish, "library!" );

  for(int i = 0; i < 4; ++i)
    for(int j = 0; j < 4; ++j)
    {
      bool a, b;
      cout << "---------------------------------------------------------------------------\n";
      Dout( dc::run|continued_cf, "Libcwd " );
      a = (i & 2);
      b = (i & 1);
      Dout( dc::notice, "`nested_foo(" << a << ", " << b << ")' returns the string \"" << nested_foo(a, b) << "\" when I call it." );
      Dout( dc::continued, "is an awesome " );
      a = (j & 2);
      b = (j & 1);
      Dout( dc::notice, "`nested_foo(" << a << ", " << b << ")' returns the string \"" << nested_foo(a, b) << "\" when I call it." );
      Dout( dc::finish, "library!" );
    }
    
  for(int i = 0; i < 16; ++i)
  {
    bool a = (i & 8), b = (i & 4), c = (i & 2), d = (i & 1);
    cout << "---------------------------------------------------------------------------\n";
    Dout( dc::run|continued_cf, "Libcwd " );
    Dout( dc::notice, "`nested_bar(" << a << ", " << b << ", " << c << ", " << d << ")' returns the string \"" << nested_bar(a, b, c, d) << "\" when I call it." );
    Dout( dc::continued, "is an awesome " );
    Dout( dc::finish, "library!" );
  }

  for(int i = 0; i < 16; ++i)
  {
    bool a = (i & 8), b = (i & 4), c = (i & 2), d = (i & 1);
    cout << "---------------------------------------------------------------------------\n";
    Dout( dc::run|continued_cf, "Libcwd " );
    Dout( dc::continued, "is an awesome " );
    Dout( dc::notice, "`nested_bar(" << a << ", " << b << ", " << c << ", " << d << ")' returns the string \"" << nested_bar(a, b, c, d) << "\" when I call it." );
    Dout( dc::finish, "library!" );
  }

  //===================================================================================
  // Continued tests, depth 2.

  cout << "===========================================================================\n";
  cout << " Continued tests, deep\n\n";

  Dout( dc::notice, continued_func(5) );

  release_cerr();

  EXIT(0);
}
