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

#include <libcw/sys.h>
#include <libcw/h.h>
#include <libcw/debug.h>
#include <libcw/bfd.h>

RCSTAG_CC("$Id$")

#ifdef DEBUGUSEBFD

void libcw_bfd_test3(void)
{
  for (int i = 0; i <= 5; ++i)
  {
    void* retadr;
    
    switch (i)
    {
      case 0:
        retadr = __builtin_return_address(0);
	break;
      case 1:
        retadr = __builtin_return_address(1);
	break;
      case 2:
        retadr = __builtin_return_address(2);
	break;
      case 3:
        retadr = __builtin_return_address(3);
	break;
      case 4:
        retadr = __builtin_return_address(4);
	break;
      default:
        retadr = __builtin_return_address(5);
	break;
    }

    location_st loc = libcw_bfd_pc_location((char*)retadr - 1);
      Dout(dc::notice, hex << retadr << dec << " -> (" << loc << ")");

    if (loc.line == 0)
      break;
  }
}
 
void libcw_bfd_test2(void)
{
  libcw_bfd_test3();
}

void libcw_bfd_test1(void)
{
  libcw_bfd_test2();
}

void libcw_bfd_test(void)
{
  libcw_bfd_test1();
}

int main(int argc, char* argv[])
{
  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off(); );
  Debug( dc::warning.on() );
  Debug( dc::bfd.on() );
  Debug( dc::notice.on() );
  Debug( dc::system.on() );

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&cout) );

  // Turn debug object on
  Debug( libcw_do.on() );

  // Run test
  libcw_bfd_test();

  return 0;
}

#else

#include <iostream>

int main(void)
{
  cout << "DEBUGUSEBFD not defined\n";
  return 0;
}

#endif
