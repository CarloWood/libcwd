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
#include "sys.h"
#include <iostream>
#include <libcw/debug.h>
#include <libcw/lockable_auto_ptr.h>

using namespace libcw;
using namespace libcw::debug;

class B {};
class A : public B {};

int main(int argc, char *argv[])
{
  Debug( check_configuration() );

#ifdef DEBUGMALLOC
  // Don't show allocations that are allocated before main()
  make_all_allocations_invisible_except(NULL);
#endif

  // Select channels
  Debug( dc::notice.on() );

  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );

  // Turn debug object on
  Debug( libcw_do.on() );

  A* a;

  a = new A;
  AllocTag(a, "A1");
  lockable_auto_ptr<A> ap1(a);

  ASSERT(ap1.get() == a);
  ASSERT(ap1.is_owner() && !ap1.strict_owner());

  lockable_auto_ptr<A> ap2(ap1);

  ASSERT(ap1.get() == a);
  ASSERT(ap2.get() == a);
  ASSERT(!ap1.is_owner());
  ASSERT(ap2.is_owner() && !ap2.strict_owner());

  lockable_auto_ptr<B> bp1(ap2);

  ASSERT(ap2.get() == a);
  ASSERT(bp1.get() == a);
  ASSERT(!ap2.is_owner());
  ASSERT(bp1.is_owner() && !bp1.strict_owner());

  a = new A;
  AllocTag(a, "A2");
  lockable_auto_ptr<A> ap3(a);
  lockable_auto_ptr<B> bp2;
  bp2 = ap3;

  ASSERT(ap3.get() == a);
  ASSERT(bp2.get() == a);
  ASSERT(!ap3.is_owner());
  ASSERT(bp2.is_owner() && !bp2.strict_owner());

  a = new A;
  AllocTag(a, "A3");
  lockable_auto_ptr<A> ap4(a);
  ap4.lock();

  ASSERT(ap4.get() == a);
  ASSERT(ap4.is_owner() && ap4.strict_owner());

  lockable_auto_ptr<A> ap5(ap4);

  ASSERT(ap4.get() == a);
  ASSERT(ap5.get() == a);
  ASSERT(ap4.is_owner() && ap4.strict_owner());
  ASSERT(!ap5.is_owner());

  lockable_auto_ptr<B> bp3(ap5);

  ASSERT(ap5.get() == a);
  ASSERT(bp3.get() == a);
  ASSERT(!ap5.is_owner());
  ASSERT(!bp3.is_owner());

  a = new A;
  AllocTag(a, "A4");
  lockable_auto_ptr<A> ap6(a);
  ap6.lock();
  lockable_auto_ptr<B> bp4;
  bp4 = ap6;

  ASSERT(ap6.get() == a);
  ASSERT(bp4.get() == a);
  ASSERT(ap6.is_owner() && ap6.strict_owner());
  ASSERT(!bp4.is_owner());

  Dout(dc::notice, "Test successful");
}
