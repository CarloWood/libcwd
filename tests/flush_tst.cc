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
#include "libcw/dbstreambuf.h"

RCSTAG_CC("$Id$")

#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif
    channel_ct generate_dc("GENERATE");
#ifndef DEBUGNONAMESPACE
  };
};
#endif

void generate_tables(void)
{
  sleep(6);
  Dout( generate_dc, "Inside generate_tables()" );
  sleep(6);
  Dout( generate_dc, "Leaving generate_tables()" );
  return;
}

int main(void)
{
  // Select channels
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );

  // Turn debug object on
  Debug( libcw_do.on() );

  // Print channels
//  Debug( list_channels_on(libcw_do) );

  //Debug( io_dc.off() );
  Dout( notice_dc|flush_cf|continued_cf, "Generating tables part1... " );
  generate_tables();
  Dout( continued_dc, "part2... " );
  generate_tables();
  Dout( finish_dc, "done" );
}
