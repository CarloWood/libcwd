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

#ifdef __GNUG__
#pragma implementation
#endif
#include "libcw/sys.h"
#include <cstdio>
#include <getopt.h>
#include "libcw/h.h"
#include "libcw/debug.h"

RCSTAG_CC("$Id$")

int main(int argc, char **argv)
{
  // Don't show allocations that are allocated before main()
  make_all_allocations_invisible_except(NULL);

  // Select channels
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
#ifdef DEBUGUSEBDF
  Debug( dc::bfd.off() );
#endif

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&cout) );

  // Turn debug object on
  Debug( libcw_do.on() );

  // List all debug channels
  Debug( list_channels_on(libcw_do) );

  int *p = new int[4];
  AllocTag(p, "Test array");
  Debug( list_allocations_on(libcw_do) );
  p[4] = 5;
  delete[] p;

  return 0;
}
