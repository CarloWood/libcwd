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
#include <libcw/return_address.h>

RCSTAG_CC("$Id$")

#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
static void* return_address[6];
#define store_call_address(i) return_address[i] = __builtin_return_address(0)
extern "C" int _start();
#endif

void libcw_bfd_test3(void)
{
  for (int i = 0; i <= 5; ++i)
  {
    void* retadr;
    
    switch (i)
    {
#ifdef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
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
      case 5:
        retadr = __builtin_return_address(5);
	break;
#else
      case 1:
      case 2:
      case 3:
      case 4:
        retadr = return_address[i];
	break;
      case 5:
        retadr = (char*)&_start + 10;	// Whatever...
        break;
#endif
      default:
        retadr = __builtin_return_address(0);
	break;
    }

    location_st loc;
    libcw_bfd_pc_location(loc, (char*)retadr + libcw_bfd_builtin_return_address_offset);
    Dout(dc::notice, "called from " << loc);

#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
    if (i < 5 && libcw_return_address(i) != retadr)
    {
      retadr = libcw_return_address(i);
      libcw_bfd_pc_location(loc, (char*)retadr + libcw_bfd_builtin_return_address_offset);
      DoutFatal(dc::fatal, "libcw_return_address(" << i << ") returns " << loc << "!");
    }
#endif

    if (loc.line == 0)
      break;
  }
}
 
void libcw_bfd_test2(void)
{
#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(1);
#endif
  libcw_bfd_test3();
}

void libcw_bfd_test1(void)
{
#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(2);
#endif
  libcw_bfd_test2();
}

void libcw_bfd_test(void)
{
#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(3);
#endif
  libcw_bfd_test1();
}

int main(int argc, char* argv[])
{
  Debug( check_configuration() );

  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off(); );
#if !defined(__sun__) || !defined(__svr4__)
  Debug( dc::warning.on() );	// On Solaris we fail to find the start of libdl
#endif
  Debug( dc::bfd.on() );
  Debug( dc::notice.on() );
  Debug( dc::system.on() );

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&cout) );

  // Turn debug object on
  Debug( libcw_do.on() );

  // Run test
#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(4);
#endif
  libcw_bfd_test();

  return 0;
}
