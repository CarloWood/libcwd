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

//
// This code is intended to serve as simple example of how to write
// a program that uses libcwd.so.
//
// Compile as:
//
// g++ -g -DCWDEBUG -c hello_world.cc
// g++ hello_world.o -lcwd -o hello_world
//
// On less divine operating systems as linux, you might need to
// link using: g++ hello_world.o -lcwd -lbfd -liberty -o hello_world
//

#include <libcw/sys.h>
#include <libcw/debug.h>

RCSTAG_CC("$Id$")

// Define our own debug channel:
#ifdef CWDEBUG
namespace libcw {
  namespace debug {
    namespace channels {
      namespace dc {
	channel_ct const hello("HELLO");
      }
    }
  }
}
#endif

int main(void)
{
  //----------------------------------------------------------------------
  // The following calls will be done in almost every program using libcwd

  Debug( check_configuration() );

#ifdef DEBUGMALLOC
  // Don't show allocations that are allocated before main()
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif

  // Select channels (note that where 'on' is used, 'off' can be specified
  // and vica versa).
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( dc::notice.off() );	// Just an example

  // Write debug output to cout (the default is cerr)
  Debug( libcw_do.set_ostream(&cout) );

  // Turn debug object on.  Turning it off can be done recursively.
  // It starts with 'off' depth 1.
  Debug( libcw_do.on() );

  // List all debug channels (not very usefull unless you allow to turn
  // channels on and off from the commandline; this is supported in libcw).
  Debug( list_channels_on(libcw_do) );

  // End of common block
  //----------------------------------------------------------------------

  // Write "Hello World" to our own channel:
  Dout( dc::hello, "Hello World!" );

  return 0;
}
