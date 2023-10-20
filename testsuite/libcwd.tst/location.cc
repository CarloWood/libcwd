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
#include "../../macros.h"
#include <libcwd/debug.h>
#include <iostream>

void writeProcMapsToFile();

#ifdef __OPTIMIZE__
#undef CW_RECURSIVE_BUILTIN_RETURN_ADDRESS
#define CW_RECURSIVE_BUILTIN_RETURN_ADDRESS 0
#endif

#if !CW_RECURSIVE_BUILTIN_RETURN_ADDRESS
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

void __attribute__ ((noinline)) libcwd_bfd_test3()
{
#if CWDEBUG_LOCATION
  for (int i = 0; i <= 5; ++i)
  {
    void* retadr;

    switch (i)
    {
#if CW_RECURSIVE_BUILTIN_RETURN_ADDRESS
PRAGMA_DIAGNOSTIC_PUSH_IGNORE_frame_address
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
PRAGMA_DIAGNOSTIC_POP
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
    libcwd::location_ct loc((char*)retadr + libcwd::builtin_return_address_offset);
    Dout(dc::notice, "called from " << loc);

#ifdef CW_FRAME_ADDRESS_OFFSET
    if (i < 5 && frame_return_address(i) != retadr)
      DoutFatal(dc::fatal, "frame_return_address(" << i << ") returns " <<
          libcwd::location_ct((char*)frame_return_address(i) + builtin_return_address_offset) << "!");
#endif

    if (!loc.is_known() || strcmp(loc.mangled_function_name(), "__libc_start_main") == 0)
      break;
#else // !CWDEBUG_LOCATION
#ifdef CW_FRAME_ADDRESS_OFFSET
    if (i < 5 && frame_return_address(i) != retadr)
      DoutFatal(dc::fatal, "frame_return_address(" << i << ") returns " << frame_return_address(i) << '!');
#endif
#endif
  }
#endif
}

void __attribute__ ((noinline)) libcwd_bfd_test2()
{
#if !CW_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(1);
#endif
  libcwd_bfd_test3();
  asm volatile ("");    // Stop tail chaining.
}

void __attribute__ ((noinline)) libcwd_bfd_test1()
{
#if !CW_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(2);
#endif
  libcwd_bfd_test2();
  asm volatile ("");
}

void __attribute__ ((noinline)) libcwd_bfd_test()
{
#if !CW_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(3);
#endif
  libcwd_bfd_test1();
  asm volatile ("");
}

MAIN_FUNCTION
{ PREFIX_CODE
#if !CWDEBUG_LOCATION
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );
  writeProcMapsToFile();

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
#if CWDEBUG_LOCATION
  // Choose location format
  Debug( location_format(show_objectfile|show_function) );
#endif

  // Run test
#if !CW_RECURSIVE_BUILTIN_RETURN_ADDRESS
  store_call_address(4);
#endif
  libcwd_bfd_test();

  Dout(dc::notice, "Program end");

  Debug( libcw_do.off() );

  EXIT(0);
}
