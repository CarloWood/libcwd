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
#include <libcw/h.h>
#include <libcw/debug.h>
#include <libcw/demangle.h>
#include <libcw/type_info.h>

RCSTAG_CC("$Id$")

type_info_ct unknown_type_info;

// Warning: This LEAKS memory!
// For internal use only

char const* make_label(char const* mangled_name)
{
  char const* demangled_name;
  size_t len;
  string out;
  if (demangle(mangled_name, out) == -1)
  {
    demangled_name = mangled_name;
    len = strlen(demangled_name);
  }
  else
  {
    demangled_name = out.c_str();
    len = out.size();
  }
#ifdef DEBUGMALLOC
  set_alloc_checking_off();
#endif
  char* label = new char[len + 1];
#ifdef DEBUGMALLOC
  set_alloc_checking_on();
#endif
  strcpy(label, demangled_name);
  return label;
}

type_info_ct const& libcw::_internal_::type_info_of(void const* obj, int)
{ 
  static type_info_ct const type_info_singleton(typeid(const_cast<void*>(obj)), sizeof(void*), 0 /* unknown */);
  return type_info_singleton;
}
