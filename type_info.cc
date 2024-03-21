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
#include "libcwd/debug.h"
#include "libcwd/demangle.h"
#include "libcwd/type_info.h"
#include "macros.h"

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
  {
    size_t len = strlen(encap_mangled_name + 27);		// Strip "Q22libcwd_type_info_exact1Z" from the beginning.
    set_alloc_checking_off(LIBCWD_TSD);
    char* exact_name = new char[len + 1];			// LEAK58
    set_alloc_checking_on(LIBCWD_TSD);
    strncpy(exact_name, encap_mangled_name + 27, len);
    exact_name[len] = 0;
    return exact_name;
  }
#else
  char const* skip_substitution(char const* const ptr, char const* const begin)
  {
    char const* S_ptr = ptr;
    while (S_ptr > begin)
    {
      --S_ptr;
      if (*S_ptr == 'S')
        return S_ptr;
      // Substitutions use base-36 with 'digits' [0-9A-Z].
      if (!('0' <= *S_ptr && *S_ptr <= '9') && !('A' <= *S_ptr && *S_ptr <= 'Z'))
        break;
    }
    return ptr;
  }

  // As an example, the input could be:
  //
  // encap_mangled_name = 22libcwd_type_info_exactIRKN4evio14AcceptedSocketI23MyAcceptedSocketDecoderNS0_12OutputStreamEEEE
  // stripped_mangled_name =                         N4evio14AcceptedSocketI23MyAcceptedSocketDecoderNS _12OutputStreamEEE
  // where the space in the latter is not really there (just added for alignment)               -------^
  char const* extract_exact_name(char const* encap_mangled_name, char const* stripped_mangled_name LIBCWD_COMMA_TSD_PARAM)
  {
    encap_mangled_name += 25;		                        // Strip "22libcwd_type_info_exactI" from the beginning.
    // encap_mangled_name now points to the qualifiers string (if any) "RK".
    char const* encap_ptr = encap_mangled_name + strlen(encap_mangled_name) - 1;
    // encap_ptr now points to the last E of encap_mangled_name.
    size_t len = strlen(stripped_mangled_name);
    char const* ptr = stripped_mangled_name + len;
    // ptr now points to the terminating zero of stripped_mangled_name.
    while (ptr > stripped_mangled_name)
    {
      --ptr;
      --encap_ptr;
      LIBCWD_ASSERT(*ptr == *encap_ptr);        // This is how we assume the encapsulation works!
      if (*ptr == '_')
      {
        // Now we want to put both pointers on the 'S',
        // but only if this could be a substitution.
        ptr = skip_substitution(ptr, stripped_mangled_name);
        encap_ptr = skip_substitution(encap_ptr, encap_mangled_name);
      }
    }
    // ptr is now equal to stripped_mangled_name, and encap_ptr points to the first character after the qualifiers.
    size_t qlen = encap_ptr - encap_mangled_name;
    set_alloc_checking_off(LIBCWD_TSD);
    char* exact_name = new char[qlen + len + 1];	// LEAK58
    set_alloc_checking_on(LIBCWD_TSD);
    // Add the qualifiers to the mangled name:
    strncpy(exact_name, encap_mangled_name, qlen);
    strcpy(exact_name + qlen, stripped_mangled_name);
    return exact_name;
  }
#endif

  // Idem

  char const* make_label(char const* mangled_name)
  {
    char const* label;
    LIBCWD_TSD_DECLARATION;
    set_alloc_checking_off(LIBCWD_TSD);
    {
      internal_string out;
      demangle_type(mangled_name, out);
      label = strcpy(new char[out.size() + 1], out.c_str());	// LEAK44
    }
    set_alloc_checking_on(LIBCWD_TSD);
    return label;
  }

  type_info_ct type_info<void*>::S_value;

  bool type_info<void*>::S_initialized;

  type_info_ct const& type_info<void*>::value()
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

libcwd::type_info_ct const& libcwd_type_info_exact<void*>::value()
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

