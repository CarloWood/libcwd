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
#include <iostream>
#include <libcw/debug.h>
#include <libcw/type_info.h>

RCSTAG_CC("$Id$")

using std::cout;
using std::endl;
using libcw::debug::type_info_of;

class A {
private:
  char x[64];
public:
  A(void) { }
};

class B {
  friend class A;
private:
  char x[64];
  B(void) { }
#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
public:
  B(int hack) { }
#endif
};

int main(void)
{
  Debug( check_configuration() );
  {
    int i;
    cout << type_info_of(i).demangled_name() << " : " << type_info_of(i).ref_size() << endl;
    int* j;
    cout << type_info_of(j).demangled_name() << " : " << type_info_of(j).ref_size() << endl;
    A a;
    cout << type_info_of(a).demangled_name() << " : " << type_info_of(a).ref_size() << endl;
    A* b;
    cout << type_info_of(b).demangled_name() << " : " << type_info_of(b).ref_size() << endl;
    void* p;
    cout << type_info_of(p).demangled_name() << " : " << type_info_of(p).ref_size() << endl;
  }
  {
    const int i = 0;
    cout << type_info_of(i).demangled_name() << " : " << type_info_of(i).ref_size() << endl;
    const int* j = &i;
    cout << type_info_of(j).demangled_name() << " : " << type_info_of(j).ref_size() << endl;
    const A a;
    cout << type_info_of(a).demangled_name() << " : " << type_info_of(a).ref_size() << endl;
    const A* b = &a;
    cout << type_info_of(b).demangled_name() << " : " << type_info_of(b).ref_size() << endl;
  }
  {
#if __GNUC__ == 2 && __GNUC_MINOR__ < 97
    // Initialize the static objects by using the hack in `type_info_of'.
    type_info_of<B const>();
    B const* bp;
    type_info_of(bp);
    B const* const* bpp;
    type_info_of(bpp);
    type_info_of<B const* const* const*>();
#endif
    cout << libcw::debug::type_info<B const>::value.demangled_name() << " : " << libcw::debug::type_info<B const>::value.ref_size() << endl;
    cout << libcw::debug::type_info<B const*>::value.demangled_name() << " : " << libcw::debug::type_info<B const*>::value.ref_size() << endl;
    cout << libcw::debug::type_info<void*>::value.demangled_name() << " : " << libcw::debug::type_info<void*>::value.ref_size() << endl;
  }
  {
    A const* a1;
    A const* const* a2;
    A const* const* const* a3;
    cout << type_info_of(a1).demangled_name() << endl;
    cout << type_info_of(a2).demangled_name() << endl;
    cout << type_info_of(a3).demangled_name() << endl;
  }
  {
    cout << libcw::debug::type_info<B const*>::value.demangled_name() << endl;
    cout << libcw::debug::type_info<B const* const*>::value.demangled_name() << endl;
    cout << libcw::debug::type_info<B const* const* const*>::value.demangled_name() << endl;
  }

  exit(0);
}
