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

#ifndef LIBCW_CWPRINT_H
#define LIBCW_CWPRINT_H

RCSTAG_H(cwprint, "$Id$")

namespace libcw {
  namespace debug {

//
// cwprint
//
// PRINTABLE_OBJECT must have the signature:
//
// class PRINTABLE_OBJECT {
//   ...
// public:
//   void print_on(ostream& os) const;
// };
//
// Usage,
// 
// 	Dout( dc::channel, cwprint(printable_object) );
//
// this will write `printable_object' to the debug ostream by
// calling printable_object.print_on(os).
//

template<class PRINTABLE_OBJECT>
  class cwprint_tct {
  private:
    PRINTABLE_OBJECT const& M_printable_object;
  public:
    cwprint_tct(PRINTABLE_OBJECT const& printable_object) : M_printable_object(printable_object) { }
    friend inline ostream& operator<<(ostream& os, cwprint_tct<PRINTABLE_OBJECT> const& L_cwprint)
	{ L_cwprint.M_printable_object.print_on(os); return os; }
  };

template<class T>
  inline cwprint_tct<T>
  cwprint(T const& printable_object)
  {
    return cwprint_tct<T>(printable_object);
  }

//
// cwprint_using
//
// PRINTABLE_OBJECT must have the signature:
//
// class PRINTABLE_OBJECT {
//   ...
// public:
//   void arbitrary_method_name(ostream& os) const;
// };
//
// Usage,
// 
// 	Dout( dc::channel, cwprint_using(printable_object, &PRINTABLE_OBJECT::arbitrary_method_name) );
//
// this will write `printable_object' to the debug ostream by
// calling printable_object.arbitrary_method_name(os).
//

template<class PRINTABLE_OBJECT>
  class cwprint_using_tct {
    typedef void (PRINTABLE_OBJECT::* print_on_method_t)(ostream&) const;
  private:
    PRINTABLE_OBJECT const& M_printable_object;
    print_on_method_t M_print_on_method;
  public:
    cwprint_using_tct(PRINTABLE_OBJECT const& printable_object, print_on_method_t print_on_method) :
	M_printable_object(printable_object), M_print_on_method(print_on_method) { }
    friend ostream& operator<<(ostream& os, cwprint_using_tct<PRINTABLE_OBJECT> L_cwprint_using)
	{ (L_cwprint_using.M_printable_object.*L_cwprint_using.M_print_on_method)(os); return os; }
  };

template<class T>
  inline cwprint_using_tct<T>
  cwprint_using(T const& printable_object, void (T::*print_on_method)(ostream&) const)
  {
    return cwprint_using_tct<T>(printable_object, print_on_method);
  }

//---------------------------------------------------------------------------------------
// Printable objects
// 

//
// environment_ct
//

class environment_ct {
  char const* const* __envp;
public:
  environment_ct(char const* const envp[]) : __envp(envp) { }
  void print_on(ostream& os) const;
};

//
// argv_ct
//

class argv_ct {
  char const* const* __argv;
public:
  argv_ct(char const* const argv[]) : __argv(argv) { }
  void print_on(ostream& os) const;
};

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CWPRINT_H
