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
#include "alloctag_debug.h"
#include <iostream>
#include <libcwd/lockable_auto_ptr.h>

using namespace libcw;
using namespace libcw::debug;

class Bl {};
class Al : public Bl {};

MAIN_FUNCTION
{ PREFIX_CODE
  Debug( check_configuration() );

#if CWDEBUG_ALLOC && !defined(THREADTEST)
  // Don't show allocations that are allocated before main()
  make_all_allocations_invisible_except(NULL);
#endif

  // Select channels
  Debug( dc::notice.on() );
#ifndef THREADTEST
  // Write debug output to cout
  Debug( libcw_do.set_ostream(&std::cout) );
#endif
  // Turn debug object on
  Debug( libcw_do.on() );

  Al* a;

  a = new Al;
  AllocTag(a, "A1");
  lockable_auto_ptr<Al> ap1(a);

  LIBCWD_ASSERT(ap1.get() == a);
  LIBCWD_ASSERT(ap1.is_owner() && !ap1.strict_owner());

  lockable_auto_ptr<Al> ap2(ap1);

  LIBCWD_ASSERT(ap1.get() == a);
  LIBCWD_ASSERT(ap2.get() == a);
  LIBCWD_ASSERT(!ap1.is_owner());
  LIBCWD_ASSERT(ap2.is_owner() && !ap2.strict_owner());

  lockable_auto_ptr<Bl> bp1(ap2);

  LIBCWD_ASSERT(ap2.get() == a);
  LIBCWD_ASSERT(bp1.get() == a);
  LIBCWD_ASSERT(!ap2.is_owner());
  LIBCWD_ASSERT(bp1.is_owner() && !bp1.strict_owner());

  a = new Al;
  AllocTag(a, "A2");
  lockable_auto_ptr<Al> ap3(a);
  lockable_auto_ptr<Bl> bp2;
  bp2 = ap3;

  LIBCWD_ASSERT(ap3.get() == a);
  LIBCWD_ASSERT(bp2.get() == a);
  LIBCWD_ASSERT(!ap3.is_owner());
  LIBCWD_ASSERT(bp2.is_owner() && !bp2.strict_owner());

  a = new Al;
  AllocTag(a, "A3");
  lockable_auto_ptr<Al> ap4(a);
  ap4.lock();

  LIBCWD_ASSERT(ap4.get() == a);
  LIBCWD_ASSERT(ap4.is_owner() && ap4.strict_owner());

  lockable_auto_ptr<Al> ap5(ap4);

  LIBCWD_ASSERT(ap4.get() == a);
  LIBCWD_ASSERT(ap5.get() == a);
  LIBCWD_ASSERT(ap4.is_owner() && ap4.strict_owner());
  LIBCWD_ASSERT(!ap5.is_owner());

  lockable_auto_ptr<Bl> bp3(ap5);

  LIBCWD_ASSERT(ap5.get() == a);
  LIBCWD_ASSERT(bp3.get() == a);
  LIBCWD_ASSERT(!ap5.is_owner());
  LIBCWD_ASSERT(!bp3.is_owner());

  a = new Al;
  AllocTag(a, "A4");
  lockable_auto_ptr<Al> ap6(a);
  ap6.lock();
  lockable_auto_ptr<Bl> bp4;
  bp4 = ap6;

  LIBCWD_ASSERT(ap6.get() == a);
  LIBCWD_ASSERT(bp4.get() == a);
  LIBCWD_ASSERT(ap6.is_owner() && ap6.strict_owner());
  LIBCWD_ASSERT(!bp4.is_owner());

  Dout(dc::notice, "Test successful");

  EXIT(0);
}
