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
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <libcw/debug.h>

RCSTAG_CC("$Id$")

using namespace libcw::debug;

class AA {
private:
  void* ptr;
public:
  void* leakAA;
public:
  AA(void)
  {
    ptr = malloc(1212);
    AllocTag(ptr, "AA::ptr");
    leakAA = malloc(7334);
    AllocTag(leakAA, "AA::leakAA");
  }
  ~AA() { free(ptr); }
};

class test_object {
public:
  AA* a;
  AA* b;
  void* ptr;
  void* leak;
public:
  test_object(void)
  {
    a = new AA;
    AllocTag(a, "test_object::a");
    ptr = malloc(12345);
    AllocTag(ptr, "test_object::ptr");
    leak = malloc(111);
    AllocTag(leak, "test_object::leak");
    b = new AA;
    AllocTag(b, "test_object::b");
  }
  ~test_object() { delete b; free(ptr); delete a; }
};

int main(int argc, char* argv[])
{
#if !defined(DEBUGMALLOC) || !defined(DEBUGUSEBFD)
  DoutFatal(dc::fatal, "Expected Failure.");
#endif

  Debug( check_configuration() );

  // Don't show allocations that are allocated before main()
  make_all_allocations_invisible_except(NULL);

#ifdef DEBUGUSEBFD
  // Make sure we initialized the bfd stuff before we turn on WARNING.
  Debug( (void)pc_mangled_function_name((void*)main) );
#endif

  // Select channels
  Debug( dc::malloc.on() );
  Debug( dc::warning.on() );

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );

  // Turn debug object on
  Debug( libcw_do.on() );

  void* ptr1 = malloc(1111U); AllocTag2(ptr1, "ptr1");
#ifdef DEBUGMALLOC
  if (test_delete(ptr1))
    DoutFatal(dc::core, "Huh 1 ?");
#endif
  void* ptr2 = malloc(2222); AllocTag2(ptr2, "ptr2");
#ifdef DEBUGMALLOC
  if (test_delete(ptr1) || test_delete(ptr2))
    DoutFatal(dc::core, "Huh 2 ?");
#endif
  void* ptr3 = malloc(3333); AllocTag2(ptr3, "ptr3");
#ifdef DEBUGMALLOC
  if (test_delete(ptr1) || test_delete(ptr2) || test_delete(ptr3))
    DoutFatal(dc::core, "Huh 3 ?");
#endif
  void* ptr4 = malloc(4444); AllocTag2(ptr4, "ptr4");
#ifdef DEBUGMALLOC
  if (test_delete(ptr1) || test_delete(ptr2) || test_delete(ptr3) || test_delete(ptr4))
    DoutFatal(dc::core, "Huh 4 ?");
  if (!test_delete((void*)0x8000000))
    DoutFatal(dc::core, "Huh 5 ?");
#endif

#ifdef DEBUGMARKER
  marker_ct* marker = new marker_ct("test marker");
#endif

  test_object* t = NEW( test_object );
  void* leak1 = t->leak;
  void* leak1a = t->a->leakAA;
  void* leak1b = t->b->leakAA;
  void* leak2;
  void* leak2a;
  void* leak2b;
  void* ptr5 = malloc(5555);
  AllocTag(ptr5, "ptr5");
  {
    test_object NoHeap;
    leak2 = NoHeap.leak;
    leak2a = NoHeap.a->leakAA;
    leak2b = NoHeap.b->leakAA;

    Debug( list_allocations_on(libcw_do) );
  }

  free(ptr1);
  free(ptr4);
#ifdef DEBUGMARKER
  Debug( move_outside(marker, t) );
#endif
  delete t;
  free(ptr5);

  Debug( list_allocations_on(libcw_do) );

#ifdef DEBUGMARKER
  delete marker;
#endif

  free(leak1);
  Debug( list_allocations_on(libcw_do) );
  free(leak1a);
  Debug( list_allocations_on(libcw_do) );
  free(leak1b);
  Debug( list_allocations_on(libcw_do) );
  free(leak2a);
  Debug( list_allocations_on(libcw_do) );
  free(leak2);
  Debug( list_allocations_on(libcw_do) );
  free(leak2b);
  Debug( list_allocations_on(libcw_do) );
  free(ptr2);
  free(ptr3);

  return 0;
}
