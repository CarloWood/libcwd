// $Header$
//
// Copyright (C) 2000 - 2001, by
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
#include <libcw/debug.h>
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
#if !defined(DEBUGMALLOC) || !defined(DEBUGUSEBFD)
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );

#ifdef DEBUGMALLOC
  // Don't show allocations that are allocated before main()
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif

#ifdef DEBUGUSEBFD
  // Make sure we initialized the bfd stuff before we turn on WARNING.
  Debug( (void)pc_mangled_function_name((void*)main) );
#endif

  // Select channels
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( dc::debug.off() );
#ifdef DEBUGUSEBFD
  Debug( dc::bfd.off() );
#endif

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );

  // Turn debug object on
  Debug( libcw_do.on() );

  // List all debug channels
  Debug( list_channels_on(libcw_do) );

  char* cp = new char[50];
  AllocTag(cp, "Test of \"new char[50]\"");

  int* i = new int;
  AllocTag(i, "Test of \"new int\"");

  void* vp = malloc(33);
  AllocTag(vp, "Test of \"(void*)malloc(33)\"");

  int* vpi = (int*)malloc(55);
  AllocTag(vpi, "Test of \"(int*)malloc(55)\"");

  void* cp2 = calloc(22, 10);
  AllocTag(cp2, "Test of \"(void*)calloc(22, 10)\"");

  int* cp2i = (int*)calloc(55, 10);
  AllocTag(cp2i, "Test of \"(int*)calloc(55, 10)\"");

  void* mp = malloc(11);
  AllocTag(mp, "Test of \"(void*)malloc(1100)\"");

  void* rp = realloc(mp, 1000);
  AllocTag(rp, "Test of \"(void*)realloc(mp, 1000)\"");

  int* mpi = (int*)malloc(66);
  AllocTag(mpi, "Test of \"(int*)malloc(66)\"");

  int* rpi = (int*)realloc(mpi, 1000);
  AllocTag(rpi, "Test of \"(int*)realloc(mpi, 1000)\"");

  Debug( list_allocations_on(libcw_do) );

  delete [] cp;
  delete i;
  free(vp);
  free(vpi);
  free(cp2);
  free(cp2i);
  free(rp);
  free(rpi);
  return 0;
}
