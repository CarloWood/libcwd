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
#include <unistd.h>
#include "libcw/h.h"
#include "libcw/debug.h"

RCSTAG_CC("$Id$")

#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif
    const channel_ct alt_dc("ALT");
#ifndef DEBUGNONAMESPACE
  };
};
#endif

int main(int argc, char **argv)
{
  // Don't show allocations that are allocated before main()
  make_all_allocations_invisible_except(NULL);

  // Select channels
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&cout) );

  // Turn debug object on
  Debug( libcw_do.on() );

  // List all debug channels
  Debug( list_channels_on(libcw_do) );

#ifdef DEBUG
  int a = 100;
  float f = 1.234;
#endif

  char *cp = new char[50];
  AllocTag(cp, "Test of \"new char[50]\"");

  int *i = new int;
  AllocTag(i, "Test of \"new int\"");

  void *vp = malloc(33);
  AllocTag(vp, "Test of \"(void*)malloc(33)\"");

  int *vpi = (int *)malloc(55);
  AllocTag(vpi, "Test of \"(int*)malloc(55)\"");

  void *cp2 = calloc(22, 10);
  AllocTag(cp2, "Test of \"(void*)calloc(22, 10)\"");

  int *cp2i = (int *)calloc(55, 10);
  AllocTag(cp2i, "Test of \"(int*)calloc(55, 10)\"");

  void *mp = malloc(11);
  AllocTag(mp, "Test of \"(void*)malloc(1100)\"");

  void *rp = realloc(mp, 1000);
  AllocTag(rp, "Test of \"(void*)realloc(mp, 1000)\"");

  int *mpi = (int *)malloc(66);
  AllocTag(mpi, "Test of \"(int*)malloc(66)\"");

  int *rpi = (int *)realloc(mpi, 1000);
  AllocTag(rpi, "Test of \"(int*)realloc(mpi, 1000)\"");

#ifdef DEBUGMALLOC
  debugmalloc_marker_ct *marker = new debugmalloc_marker_ct;
#endif

  Dout( malloc_dc|wait_cf|continued_cf, "1. Hello" << " " << "world: " );
  Dout( finish_dc, dec << a << ' ' << hex << a );

  Dout( notice_dc|warning_dc|continued_cf, "2. " );
#ifdef DEBUG
  ::libcw::debug::libcw_do.get_ostream()->form("%d %08u %.4f", a, a, f);
#endif
  Dout( finish_dc, "" );

  Dout( notice_dc|continued_cf|cerr_cf, "3. This is being written to cerr." );

  Dout( notice_dc|flush_cf, "4. This is flushed.\n" );
  sleep(2);
  Dout( finish_dc, "This is a continued message, after the flush." );

  Dout( warning_dc|flush_cf|cerr_cf, "5. Hit return." );
  cin.get();

  Dout( notice_dc, "And now something invisible:" );

  Debug( libcw_do.off() );
  Debug( libcw_do.off() );
  Dout( alt_dc, "This should NOT be displayed" );
  Debug( libcw_do.on() );
  Dout( alt_dc, "This should NOT be displayed" );
  Debug( libcw_do.on() );
  Dout( notice_dc, "This should be displayed (again)" );

  Dout( notice_dc|continued_cf, "Here is a nested debug call (1) that " );
  Dout( warning_dc, "Interrupt1!" );
  Dout( notice_dc, "Interrupt notice" );
  Dout( warning_dc, "Interrupt2!" );
  Dout( notice_dc|continued_cf, "Here is another nested debug call (2) that " );
  Dout( notice_dc, "Interrupt notice" );
  Dout( warning_dc, "Interrupt3!" );
  Dout( notice_dc, "Interrupt notice" );
  Dout( warning_dc, "Interrupt4!" );
  Dout( finish_dc, "ends now (2)." );
  Dout( warning_dc, "Interrupt5!" );
  Dout( warning_dc, "Interrupt6!" );
  Dout( finish_dc, "ends now (1)." );

  Dout( notice_dc, "End of test." );

#ifdef DEBUGMALLOC
  delete marker;
#endif

  Debug( list_allocations_on(libcw_do) );

  return 0;
}
