// SPDX-FileCopyrightText: 2000-2004, 2006, 2018-2020, 2022, 2024, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include "macros.h"
#include "libcwd/debug.h"
#include "libcwd/demangle.h"
#include "libcwd/type_info.h"

#include "cwd_debug.h"

namespace libcwd {

/**
 * \brief Returned by type_info_of() for unknown types.
 * \ingroup group_type_info
 */
TypeInfo const unknown_type_info_c(0);

namespace _private_ {

extern void demangle_type(char const* in, std::string& out);

// Warning: This LEAKS memory!
// For internal use only

#if __GXX_ABI_VERSION == 0
char const* extract_exact_name(char const* encap_mangled_name, LIBCWD_TSD_PARAM)
{
  size_t len = strlen(encap_mangled_name + 27); // Strip "Q22libcwd_type_info_exact1Z" from the beginning.
  char* exact_name = new char[len + 1]; // LEAK58
  strncpy(exact_name, encap_mangled_name + 27, len);
  exact_name[len] = 0;
  return exact_name;
}
#else
char const* skip_substitution(char const* const ptr, char const* const begin)
{
  char const* S_ptr = ptr;
  // S_ptr now points to a trailing '_'.
  for (int digits = 0; S_ptr > begin && digits < 3; ++digits)
  {
    --S_ptr;
    // All legal substitution patterns have the form "S[0-9A-Z]*_" where we
    // limit the number of base-36 digits to two (because it seems extremely
    // unlikely that there are more than 1296 substitutions).
    //
    // We allow "SS_" where the second S is a digit; but not "SSS_" because
    // it's already unlikely to have 841 substitutions, too.
    if (*S_ptr == 'S' && (digits > 0 || S_ptr[-1] != 'S'))
      return S_ptr;
    // Substitutions use base-36 with 'digits' [0-9A-Z].
    if (!('0' <= *S_ptr && *S_ptr <= '9') && !('A' <= *S_ptr && *S_ptr <= 'Z'))
      break;
  }
  return ptr;
}

// As an example, the input could be:
//
// encap_mangled_name =
// 22libcwd_type_info_exactIRKN4evio14AcceptedSocketI23MyAcceptedSocketDecoderNS0_12OutputStreamEEEE
// stripped_mangled_name =                         N4evio14AcceptedSocketI23MyAcceptedSocketDecoderNS _12OutputStreamEEE
// where the space in the latter is not really there (just added for alignment)               -------^
char const* extract_exact_name(char const* encap_mangled_name, char const* stripped_mangled_name, LIBCWD_TSD_PARAM)
{
  encap_mangled_name += 25; // Strip "22libcwd_type_info_exactI" from the beginning.
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
    LIBCWD_ASSERT(*ptr == *encap_ptr); // This is how we assume the encapsulation works!
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
  char* exact_name = new char[qlen + len + 1]; // LEAK58
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
  {
    std::string out;
    demangle_type(mangled_name, out);
    label = strcpy(new char[out.size() + 1], out.c_str()); // LEAK44
  }
  return label;
}

TypeInfo type_info<void*>::s_value_;

bool type_info<void*>::s_initialized_;

TypeInfo const& type_info<void*>::value()
{
  if (!s_initialized_)
  {
    s_value_.init(typeid(void*).name(), sizeof(void*), 0 /* unknown */);
    s_initialized_ = true;
  }
  return s_value_;
}

} // namespace _private_

} // namespace libcwd

libcwd::TypeInfo libcwd_type_info_exact<void*>::s_value_;

bool libcwd_type_info_exact<void*>::s_initialized_;

libcwd::TypeInfo const& libcwd_type_info_exact<void*>::value()
{
  if (!s_initialized_)
  {
#if __GXX_ABI_VERSION == 0
    s_value_.init(
        ::libcwd::_private_::extract_exact_name(typeid(libcwd_type_info_exact<void*>).name(), LIBCWD_TSD_INSTANCE),
        sizeof(void*), 0 /* unknown */);
#else
    s_value_.init(::libcwd::_private_::extract_exact_name(typeid(libcwd_type_info_exact<void*>).name(),
                                                          typeid(void*).name(), LIBCWD_TSD_INSTANCE),
                  sizeof(void*), 0 /* unknown */);
#endif
    s_initialized_ = true;
  }
  return s_value_;
}
