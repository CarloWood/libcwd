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
  class cwprint_ct {
  private:
    PRINTABLE_OBJECT const& printable_object;
  public:
    cwprint_ct(PRINTABLE_OBJECT const& po) : printable_object(po) { }
    inline void print_on(ostream& os) const { printable_object.print_on(os); }
  };

template<class PRINTABLE_OBJECT>
  inline ostream& operator<<(ostream& os, cwprint_ct<PRINTABLE_OBJECT> const& printable_object)
  {
    printable_object.print_on(os);
    return os;
  }

template<class PRINTABLE_OBJECT>
  inline cwprint_ct<PRINTABLE_OBJECT> cwprint(PRINTABLE_OBJECT const& po)
  {
    return cwprint_ct<PRINTABLE_OBJECT>(po);
  }

//
// environment_ct
//

#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif
    class environment_ct {
      char const* const* __envp;
    public:
      environment_ct(char const* const envp[]) : __envp(envp) { }
      void print_on(ostream& os) const;
    };
    class argv_ct {
      char const* const* __argv;
    public:
      argv_ct(char const* const argv[]) : __argv(argv) { }
      void print_on(ostream& os) const;
    };
#ifndef DEBUGNONAMESPACE
  };
};
#endif

#endif // LIBCW_CWPRINT_H
