// $Header$
//
// Copyright (C) 2000 - 2004, by
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
#include "cwd_debug.h"
#include <libcwd/demangle.h>
#include <libcwd/type_info.h>

namespace libcwd {

/**
 * \brief Returned by type_info_of() for unknown types.
 * \ingroup group_type_info
 */
type_info_ct const unknown_type_info_c(0);

namespace _private_ {

  extern void demangle_type(char const* in, _private_::internal_string& out);

  // Warning: This LEAKS memory!
  // For internal use only

#if __GXX_ABI_VERSION == 0
  char const* extract_exact_name(char const* encap_mangled_name LIBCWD_COMMA_TSD_PARAM)
#else
  char const* extract_exact_name(char const* encap_mangled_name, char const* stripped_mangled_name LIBCWD_COMMA_TSD_PARAM)
#endif
  {
#if __GXX_ABI_VERSION == 0
    size_t len = strlen(encap_mangled_name + 27);		// Strip "Q22libcwd_type_info_exact1Z" from the beginning.
#else
    size_t len = strlen(encap_mangled_name + 25) - 1;		// Strip "22libcwd_type_info_exactI" from the beginning and "E" from the end.
#endif
    set_alloc_checking_off(LIBCWD_TSD);
    char* exact_name = new char[len + 1];			// Leaks memory.
    set_alloc_checking_on(LIBCWD_TSD);
#if __GXX_ABI_VERSION == 0
    strncpy(exact_name, encap_mangled_name + 27, len);
#else
    // The substitution offset in encap_mangled_name is wrong, but in stripped_mangled_name
    // there are leading qualifiers missing.  Construct the real mangled name:
    size_t qlen = len - strlen(stripped_mangled_name);
    if (qlen)
      strncpy(exact_name, encap_mangled_name + 25, qlen);
    strncpy(exact_name + qlen, stripped_mangled_name, len - qlen);
#endif
    exact_name[len] = 0;
    return exact_name;
  }

  // Idem

  char const* make_label(char const* mangled_name)
  {
    char const* label;
    LIBCWD_TSD_DECLARATION;
    set_alloc_checking_off(LIBCWD_TSD);
    {
      internal_string out;
      demangle_type(mangled_name, out);
      label = strcpy(new char[out.size() + 1], out.c_str());	// Leaks memory.
    }
    set_alloc_checking_on(LIBCWD_TSD);
    return label;
  }

  type_info_ct type_info<void*>::S_value;

  bool type_info<void*>::S_initialized;
  
  type_info_ct const& type_info<void*>::value(void)
  {
    if (!S_initialized)
    {
      S_value.init(typeid(void*).name(), sizeof(void*), 0 /* unknown */);
      S_initialized = true;
    }
    return S_value;
  }

} // namespace _private_

} // namespace libcwd

libcwd::type_info_ct libcwd_type_info_exact<void*>::S_value;

bool libcwd_type_info_exact<void*>::S_initialized;

libcwd::type_info_ct const& libcwd_type_info_exact<void*>::value(void)
{
  if (!S_initialized)
  {
#if __GXX_ABI_VERSION == 0
    S_value.init(::libcwd::_private_::extract_exact_name(typeid(libcwd_type_info_exact<void*>).name() LIBCWD_COMMA_TSD_INSTANCE), sizeof(void*), 0 /* unknown */);
#else
    S_value.init(::libcwd::_private_::extract_exact_name(typeid(libcwd_type_info_exact<void*>).name(), typeid(void*).name() LIBCWD_COMMA_TSD_INSTANCE), sizeof(void*), 0 /* unknown */);
#endif
    S_initialized = true;
  }
  return S_value;
}

