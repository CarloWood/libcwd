// $Header$
//
// Copyright (C) 2000 - 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include <libcwd/debug.h>
#include <iostream>

#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
static void* return_address[6];
#define store_call_address(i) return_address[i] = __builtin_return_address(0)
#define START _start
extern "C" int START();
#endif

#ifdef CW_FRAME_ADDRESS_OFFSET
static void* frame_return_address(unsigned int frame)
{
  void* frame_ptr = __builtin_frame_address(0);
#if CW_FRAME_ADDRESS_OFFSET == 0
  ++frame;
#endif
  do
  {
    void** frame_ptr_ptr = reinterpret_cast<void**>(frame_ptr) + CW_FRAME_ADDRESS_OFFSET;
    if (frame-- == 0)
      return frame_ptr_ptr[1];
    frame_ptr = *frame_ptr_ptr;
  }
  while (frame_ptr);
  return NULL;
}
#endif

void libcwd_bfd_test3(void)
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
        retadr = (char*)&START + 10;	// Whatever...
        break;
#endif
      default:
        retadr = __builtin_return_address(0);
	break;
    }

#if CWDEBUG_LOCATION
    libcw::debug::location_ct loc((char*)retadr + libcw::debug::builtin_return_address_offset);
    Dout(dc::notice, "called from " << loc);

#ifdef CW_FRAME_ADDRESS_OFFSET
    if (i < 5 && frame_return_address(i) != retadr)
      DoutFatal(dc::fatal, "frame_return_address(" << i << ") returns " <<
          libcw::debug::location_ct((char*)frame_return_address(i) + builtin_return_address_offset) << "!");
#endif

    if (!loc.is_known())
      break;
#else // !CWDEBUG_LOCATION
#ifdef CW_FRAME_ADDRESS_OFFSET
    if (i < 5 && frame_return_address(i) != retadr)
      DoutFatal(dc::fatal, "frame_return_address(" << i << ") returns " << frame_return_address(i) << '!');
#endif
#endif
  }
}
 
void libcwd_bfd_test2(void)
{
#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(1);
#endif
  libcwd_bfd_test3();
}

void libcwd_bfd_test1(void)
{
#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(2);
#endif
  libcwd_bfd_test2();
}

void libcwd_bfd_test(void)
{
#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(3);
#endif
  libcwd_bfd_test1();
}

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_LOCATION
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );

  // Select channels
  ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off(); );
#if !defined(__sun__) || !defined(__svr4__)
  Debug( dc::warning.on() );	// On Solaris we fail to find the start of libdl
#endif
#if CWDEBUG_LOCATION
  Debug( dc::bfd.on() );
#endif
  Debug( dc::notice.on() );
  Debug( dc::system.on() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() );

  // Run test
#ifndef HAVE_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(4);
#endif
  libcwd_bfd_test();

  Debug( libcw_do.off() );

  EXIT(0);
}
