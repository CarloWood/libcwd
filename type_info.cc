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

#include <libcw/sys.h>
#include <libcw/debug.h>
#include <libcw/demangle.h>
#include <libcw/type_info.h>

RCSTAG_CC("$Id$")

namespace libcw {
  namespace debug {

    type_info_ct unknown_type_info;

    // Warning: This LEAKS memory!
    // For internal use only

    char const* make_label(char const* mangled_name)
    {
      char const* demangled_name;
      size_t len;
      std::string out;
      demangle_type(mangled_name, out);
      demangled_name = out.c_str();
      len = out.size();
      set_alloc_checking_off();
      char* label = new char[len + 1];
      set_alloc_checking_on();
      strcpy(label, demangled_name);
      return label;
    }

    type_info_ct const _internal_::type_info<void*>::value(typeid(void*), sizeof(void*), 0 /* unknown */);

  } // namespace debug
} // namespace libcw
