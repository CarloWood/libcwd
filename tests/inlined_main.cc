// $Header$
//
// Copyright (C) 2003, by
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
#include "debug.h"

extern void f();

int a;

int h28()
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  Dout(dc::always, "h28() was called from " << location_ct(addr));
  return 0;
}

int h29()
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  Dout(dc::always, "h29() was called from " << location_ct(addr));
  return 0;
}

int h38()
{
  char const* addr = (char const*)__builtin_return_address(0) + libcwd::builtin_return_address_offset;
  Dout(dc::always, "h38() was called from " << location_ct(addr));
  return 0;
}

int main()
{
  Debug( check_configuration() );
  Debug( libcw_do.set_ostream(&std::cout) );
  Debug( libcw_do.on() );
  Debug( dc::notice.on() );

  // Print the source location for an address inside main. This preserves the
  // original intent of this test: exercising location lookup for code in this
  // function without relying on the removed allocation-debugging malloc channel
  // to report the allocation call site as a side effect.
  Dout(dc::notice, "Calling f() from main: " << location_ct(&&current_location));
current_location:
  f();
}
