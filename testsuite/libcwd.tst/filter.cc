// $Header$
//
// Copyright (C) 2002, by
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
#include <cerrno>

extern void* realloc1000_no_AllocTag(void* p);
extern void* realloc1000_with_AllocTag(void* p);
extern void* new1000(size_t s);

MAIN_FUNCTION
{ PREFIX_CODE

  Debug( check_configuration() );

#if CWDEBUG_LOCATION
  // Make sure we initialized the bfd stuff before we turn on WARNING.
  Debug( (void)pc_mangled_function_name((void*)exit) );
#endif

#if CWDEBUG_ALLOC && !defined(THREADTEST)
  // Don't show allocations that are allocated before main()
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif

  // Select channels
  ForAllDebugChannels( while (debugChannel.is_on()) debugChannel.off(); );
  Debug( dc::warning.on() );
  Debug( dc::notice.on() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() );

  std::vector<int>* p = new std::vector<int>;
  AllocTag(p, "filter.cc");
  for(int i = 0; i < 1000; ++i)
    p->push_back(3);

  Debug( dc::malloc.on() );

  Debug(
    ooam_filter_ct filter(show_objectfile|show_path);
    std::vector<std::string> masks;
    masks.push_back("/usr/include/*");
    filter.hide_sourcefiles_matching(masks);
    filter.hide_untagged_allocations(true);
    list_allocations_on(libcw_do, filter)
  );

  Debug(
    ooam_filter_ct filter(show_objectfile|show_path);
    filter.hide_untagged_allocations(true);
    list_allocations_on(libcw_do, filter)
  );

  delete p;

  libcw::debug::ooam_filter_ct list_filter(libcw::debug::show_objectfile|libcw::debug::show_time);
  std::vector<std::string> list_masks;
  list_masks.push_back("*/dl-*");
  list_filter.hide_sourcefiles_matching(list_masks);
  list_filter.hide_untagged_allocations(true);

  void* handle;

  do
  {
#ifdef THREADTEST
    char const* module_name = "./module_r.so";
#else
    char const* module_name = "./module.so";
#endif
    handle = dlopen(module_name, RTLD_NOW|RTLD_GLOBAL);
    Dout(dc::notice, "dlopen(" << module_name << ", RTLD_NOW|RTLD_GLOBAL) == " << handle);

    if (!handle)
    {
      if (errno != EINTR && errno != EAGAIN)
      {
        char const* error_str = dlerror();
        DoutFatal(dc::fatal, "Failed to load \"" << module_name << "\": " << error_str);
      }
    }
  }
  while (!handle);

#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
  char const* sym1 = "realloc1000_no_AllocTag__FPv";
  char const* sym2 = "realloc1000_with_AllocTag__FPv";
  char const* sym3 = "new1000__FUi";
#else
  char const* sym1 = "_Z23realloc1000_no_AllocTagPv";
  char const* sym2 = "_Z25realloc1000_with_AllocTagPv";
  char const* sym3 = "_Z7new1000j";
#endif

  typedef void* (*f1_type)(void*);
  typedef void* (*f2_type)(size_t);
  f1_type f1, f2;
  f2_type f3;

  f1 = (f1_type)dlsym(handle, sym1);
  f2 = (f1_type)dlsym(handle, sym2);
  f3 = (f2_type)dlsym(handle, sym3);

  libcw::debug::ooam_filter_ct marker1_filter(libcw::debug::show_objectfile|libcw::debug::show_path);
  marker1_filter.hide_untagged_allocations(true);
  libcw::debug::ooam_filter_ct marker2_filter(libcw::debug::show_objectfile|libcw::debug::show_path);
  std::vector<std::string> masks;
  masks.push_back("module*");
  marker2_filter.hide_objectfiles_matching(masks);
  libcw::debug::marker_ct* marker1 = new libcw::debug::marker_ct("marker1", marker1_filter);
  void* p1 = malloc(500);
  void* p4 = malloc(123);
  AllocTag(p4, "Allocated between the two markers");
  libcw::debug::marker_ct* marker2 = new libcw::debug::marker_ct("marker2", marker2_filter);
  p1 = (*f1)(p1);
  void* p2 = malloc(600);
  p2 = (*f2)(p2);
  void* p3 = (*f3)(1000);
  Debug( list_allocations_on(libcw_do, list_filter));
  delete marker2;
  delete marker1;
  Debug( list_allocations_on(libcw_do, list_filter));

//  Debug( libcw_do.off() );
  delete [] (char*)p3;

  Dout(dc::notice, "dlclose(" << handle << ")");
  dlclose(handle);
  Debug( list_allocations_on(libcw_do, list_filter));

  free(p2);
  free(p1);
  free(p4);

  EXIT(0);
}
