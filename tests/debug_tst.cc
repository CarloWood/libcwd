// $Header$
//
// Copyright (C) 2000, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "libcw/sys.h"
#include <unistd.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include "libcw/h.h"
#include "libcw/debug.h"

RCSTAG_CC("$Id$")

#ifdef DEBUG
#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif
    namespace channels {
      namespace dc {
	channel_ct foo("FOO");
	channel_ct bar("BAR");
	channel_ct run("RUN");
      };
    };
#ifndef DEBUGNONAMESPACE
  };
};
#endif
#endif // DEBUG

int filedes[2];

void grab_cerr(void)
{
  // Close fd of cerr.
  close(2);

  // Create pipe that reads from fd 2
  if (pipe(filedes) == -1)
    DoutFatal( dc::core|error_cf, "pipe" );
  if (filedes[0] == 2)
  {
    filedes[0] = dup(2);
    close(2);
  }
  if (filedes[1] != 2)
  {
    if (dup2(filedes[1], 2) == -1)
      DoutFatal( dc::core|error_cf, "dup2(" << filedes[1] << ", 2)" );
    close(filedes[1]);
  }

  // Make the reading end non-blocking
  int res;
  if ((res = fcntl(filedes[0], F_GETFL, 0)) == -1)
    DoutFatal( dc::core|error_cf, "fcntl(" << filedes[0] << ", F_GETFL)" );
  else if (fcntl(filedes[0], F_SETFL, res | O_NONBLOCK) == -1)
    DoutFatal( dc::core|error_cf, "fcntl(" << filedes[0] << ", F_SETL, O_NONBLOCK)" );
}

void flush_cerr(void)
{
  // Prepare a buffer
  char cbuf[2048];
  size_t cbuflen;
  strcpy(cbuf, "[31m");

  // Read from the pipe into the buffer
  int len;
  for(cbuflen = strlen(cbuf);
      (len = read(filedes[0], &cbuf[cbuflen], sizeof(cbuf) - cbuflen));
      cbuflen += len)
  {
    if (len == -1)
    {
      if (errno == EAGAIN)
        break;
      DoutFatal( dc::core|error_cf, "read" );
    }
  }
  // Filter away any ANSI color escape sequences
  char *p1, *p2;
  for(p1 = p2 = &cbuf[5]; p1 < &cbuf[cbuflen];)
  {
    if (p1[0] == '\e' && p1[1] == '[')
      while(*p1++ != 'm');
    *p2++ = *p1++;
  }

  strcpy(p2, "[0m");

  Debug( libcw_do.get_os() << cbuf );
}

const char *nested_foo(bool with_error, bool to_cerr)
{
  if (with_error)
  {
    errno = 0;
    if (to_cerr)
    {
      Dout( dc::foo|cerr_cf|error_cf, "Inside `nested_foo()'" );
      flush_cerr();
    }
    else
      Dout( dc::foo|error_cf, "Inside `nested_foo()'" );
  }
  else
  {
    if (to_cerr)
    {
      Dout( dc::foo|cerr_cf, "Inside `nested_foo()'" );
      flush_cerr();
    }
    else
      Dout( dc::foo, "Inside `nested_foo()'" );
  }
  return "Foo";
}

const char *nested_bar(bool bar_with_error, bool bar_to_cerr, bool foo_with_error, bool foo_to_cerr)
{
  if (bar_with_error)
  {
    if (bar_to_cerr)
    {
      errno = EADV;
      Dout( dc::bar|cerr_cf|error_cf, "Entering `nested_bar()'" );
      flush_cerr();
      errno = EADV;
      Dout( dc::bar|cerr_cf|error_cf, "`nested_foo(" << foo_with_error << ", " << foo_to_cerr << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it." );
      flush_cerr();
      errno = EADV;
      Dout( dc::bar|cerr_cf|error_cf, "Leaving `nested_bar()'" );
      flush_cerr();
    }
    else
    {
      errno = EADV;
      Dout( dc::bar|error_cf, "Entering `nested_bar()'" );
      errno = EADV;
      Dout( dc::bar|error_cf, "`nested_foo(" << foo_with_error << ", " << foo_to_cerr << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it." );
      errno = EADV;
      Dout( dc::bar|error_cf, "Leaving `nested_bar()'" );
    }
  }
  else
  {
    if (bar_to_cerr)
    {
      Dout( dc::bar|cerr_cf, "Entering `nested_bar()'" );
      flush_cerr();
      Dout( dc::bar|cerr_cf, "`nested_foo(" << foo_with_error << ", " << foo_to_cerr << ")' returns the string \"" << nested_foo(foo_with_error, foo_to_cerr) << "\" when I call it." );
      flush_cerr();
      Dout( dc::bar|cerr_cf, "Leaving `nested_bar()'" );
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

int main(void)
{
#ifdef DEBUG
  grab_cerr();

  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off(); );
  Debug( dc::notice.on() );
  Debug( dc::system.on() );
  Debug( dc::foo.on() );
  Debug( dc::bar.on() );
  Debug( dc::run.on() );

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&cout) );

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
  errno = 0;
  Dout( dc::notice|error_cf|cerr_cf, "This is a single line with an error message behind it written to cerr" );
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
    USE(a);
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
  Dout( dc::run|continued_cf, "Libcw " );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcw " );
  Dout( dc::foo, "Single interruption before." );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcw " );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::foo, "Single interruption after." );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcw " );
  Dout( dc::foo, "Single interruption before and" );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::foo, "a single interruption after." );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcw " );
  Dout( dc::foo, "Double interruption before," );
  Dout( dc::foo, "double means two lines." );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcw " );
  Dout( dc::continued, "is an awesome " );
  Dout( dc::foo, "Double interruption after," );
  Dout( dc::foo, "double means two lines." );
  Dout( dc::finish, "library!" );

  cout << "---------------------------------------------------------------------------\n";
  Dout( dc::run|continued_cf, "Libcw " );
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
      Dout( dc::run|continued_cf, "Libcw " );
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
    Dout( dc::run|continued_cf, "Libcw " );
    Dout( dc::notice, "`nested_bar(" << a << ", " << b << ", " << c << ", " << d << ")' returns the string \"" << nested_bar(a, b, c, d) << "\" when I call it." );
    Dout( dc::continued, "is an awesome " );
    Dout( dc::finish, "library!" );
  }

  for(int i = 0; i < 16; ++i)
  {
    bool a = (i & 8), b = (i & 4), c = (i & 2), d = (i & 1);
    cout << "---------------------------------------------------------------------------\n";
    Dout( dc::run|continued_cf, "Libcw " );
    Dout( dc::continued, "is an awesome " );
    Dout( dc::notice, "`nested_bar(" << a << ", " << b << ", " << c << ", " << d << ")' returns the string \"" << nested_bar(a, b, c, d) << "\" when I call it." );
    Dout( dc::finish, "library!" );
  }

  //===================================================================================
  // Continued tests, depth 2.

  cout << "===========================================================================\n";
  cout << " Continued tests, depth 2\n\n";

#else // !DEBUG
  cerr << "Define DEBUG in libcw/debugging_defs.h\n";
#endif // !DEBUG

  return 0;
}
