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
	channel_ct generate("GENERATE");
      };
    };
#ifndef DEBUGNONAMESPACE
  };
};
#endif
#endif

void generate_tables(void)
{
  sleep(6);
  Dout( dc::generate, "Inside generate_tables()" );
  sleep(6);
  Dout( dc::generate, "Leaving generate_tables()" );
  return;
}

int main(void)
{
  Debug( check_configuration() );

  // Select channels
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );

  // Turn debug object on
  Debug( libcw_do.on() );

  // Print channels
//  Debug( list_channels_on(libcw_do) );

  //Debug( dc::io.off() );
  Dout( dc::notice|flush_cf|continued_cf, "Generating tables part1... " );
  generate_tables();
  Dout( dc::continued, "part2... " );
  generate_tables();
  Dout( dc::finish, "done" );
}
