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
#include <libcwd/debug.h>
#include <libcwd/type_info.h>
#include <iostream>

using std::cout;
using std::endl;
using libcwd::type_info_of;

struct A {
  char x[64];
  A() { }
};

struct B {
  friend struct A;
  char x[64];
 private:
  B() { }
};

MAIN_FUNCTION
{ PREFIX_CODE
  Debug( check_configuration() );
  {
    int i = 42;
    cout << type_info_of(i).demangled_name() << " : " << type_info_of(i).ref_size() << endl;
    int* j = &i;
    cout << type_info_of(j).demangled_name() << " : " << type_info_of(j).ref_size() << endl;
    A a;
    cout << type_info_of(a).demangled_name() << " : " << type_info_of(a).ref_size() << endl;
    A* b = &a;
    cout << type_info_of(b).demangled_name() << " : " << type_info_of(b).ref_size() << endl;
    void* p = j;
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
    cout << type_info_of<B const&>().demangled_name() << " : " << type_info_of<B const&>().ref_size() << endl;
    cout << type_info_of<B const*>().demangled_name() << " : " << type_info_of<B const*>().ref_size() << endl;
    cout << type_info_of<void*>().demangled_name() << " : " << type_info_of<void*>().ref_size() << endl;
  }
  {
    A const* a1 = nullptr;
    A const* const* a2 = nullptr;
    A const* const* const* a3 = nullptr;
    cout << type_info_of(a1).demangled_name() << endl;
    cout << type_info_of(a2).demangled_name() << endl;
    cout << type_info_of(a3).demangled_name() << endl;
  }
  {
    cout << type_info_of<B const*>().demangled_name() << endl;
    cout << type_info_of<B const* const*>().demangled_name() << endl;
    cout << type_info_of<B const* const* const*>().demangled_name() << endl;
  }

  EXIT(0);
}
