// $Header$
//
// Copyright (C) 2002 - 2003, by
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
#include <iostream>
#include "debug.h"
#include <libcwd/demangle.h>

void test(void)
{
  libcw::debug::location_ct loc((char*)__builtin_return_address(0) + libcw::debug::builtin_return_address_offset);
  std::string funcname;
  libcw::debug::demangle_symbol(loc.mangled_function_name(), funcname);
  Dout(dc::notice, "Called from " << funcname );
}

class A {
  public:
    A(void);
};

A::A(void)
{
  Dout(dc::notice, "Called from " << location_ct((char*)__builtin_return_address(0) + libcw::debug::builtin_return_address_offset) );
  test();
}

int main(void)
{
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( libcw_do.set_ostream(&std::cout) );
  Debug( libcw_do.on() );

  delete NEW(A);

  return 0;
}
