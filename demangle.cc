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

//
// This file exports:
//
// int demangle(char const* in, string& out)
//
// where, `in' is a mangled type name as returned by typeid(OBJECT).name().
//
// On success, 0 is returned and the demangled type name is written to the string `out'.
//
// If no explicit error in the mangled name could be detected, but the demangler also
// didn't know how to demangle it, then 0 is returned and the mangled name itself is
// written to the string `out'.
//
// Otherwise -1 is returned and the value of `out' is undefined,
// or when DEBUG is defined, a coredump is enforced.
// This should happen only in the case of a bug in the demangler, or
// when a different compiler is used than the one this was written for.
//
// Currently this file has only been tested with egcs-1.1.1
//

#ifdef __GNUG__
#pragma implementation
#endif
#include "libcw/sys.h"
#include <strstream>
#include "libcw/h.h"
#include "libcw/debug.h"
#include "libcw/demangle.h"

RCSTAG_CC("$Id$")

#ifdef DEBUG
#define ERROR DoutFatal( dc::core, "Demangler error" )
#else
#define ERROR return -1
#endif

// I found the following emperical (using typeid().name()):
//
// Internal types:
//
// b = bool
// c = char
// s = short
// i = int
// l = long
// x = long long
// f = float
// d = double
// r = long double
// e = ... (elipsis)
// w = wchar_t
//
// When the type is char, short, int, long or long long
// it can be signed or unsigned.  The following character then
// preceeds the character of the above table:
//
// Sc = signed char
// U<int> = unsigned <int>, where <int> is char, short, int, long or long long
//
// For each '*' (pointer) a 'P' is prepended, a 'C' follows the P
// when it is const:
//
// P<type> = <type>*
// PC<type> = <type> const*
//
// <digits><class name> = simple class/struct/enum name follows,
//   where <digits> is one or more digits (<class name> never starts with
//   a digit).
//
// Q<number><first><second><third>... = nested types, <number> deep.
//
// where <number> = <digit> = a number 2 ... 9
//                  _<digits> = a number >= 10
//
// t<digits><name><digits><ttype><ttype>... where the first number is
//   the length of <name> and the second number is the number of parameters.
//
// <ttype> = Z<type>
//         = <type><value>[_]	(non-type template argument)
//
// If the <type> if the non-type template argument starts with a 'P', its
// a pointer to an object with external linkage, possibly a function, which
// is not implemented yet.  The <value> in that case is a variable name,
// which is also not implemented in this demangler.
//
// Not implemented:
//
// F<types>_<type> = function with parameters and return type
//   where <types> is one or more <type>'s (<type> never starts with a '_').
//

namespace demangle_types {
  int const all = 0;					// all types ok
  int const integral = 1;				// bool, char, short, int, long, long long
  int const is_unsigned = 2;				// unsigned integral template argument input

  int const expl_signed = 1;				// explicitly signed
  int const expl_unsigned = 2;				// signed
  int const pointer = 4;				// pointer
  int const templ = 8;					// template
};

/* Mangled integral types are never larger then what a signed long long can hold */
static int read_digits(char const*& in, long long& value)
{
  char const* p = in;
  bool is_negative = false;
  if (!isdigit(*p))
  {
    if (*p++ != 'm')
      ERROR;
    is_negative = true;
  }
  long long x = *p++ - '0';
  while(isdigit(*p))
    x = x * 10 + (*p++ - '0');
  if (is_negative)
    value = -x;
  else
    value = x;
  char buf[21];
  ostrstream str(buf, sizeof(buf));
  str << value;
  size_t len = str.pcount();
  if (len != (size_t)(p - in))
    ERROR;
  if (is_negative)
  {
    if (strncmp(in + 1, str.str() + 1, len - 1))
      ERROR;
  }
  else if (strncmp(in, str.str(), len))
    ERROR;
  in = p;
  return 0;
}

template<class INTEGRAL_TYPE>
static int read_integral_type(char const*& in, string& out)
{
#ifdef DEBUGMALLOC
  set_alloc_checking_off();	// Suppress memory allocations of the ostrstream
#endif
  long long value;
  if (read_digits(in, value) == -1)
  {
#ifdef DEBUGMALLOC
    set_alloc_checking_on();
#endif
    return -1;
  }
  if (1)
  {
    char buf[23];
    ostrstream str(buf, sizeof(buf));
    INTEGRAL_TYPE n = value;
    if (value >= 0 && (long long)n != value)
    {
#ifdef DEBUGMALLOC
      set_alloc_checking_on();
#endif
      ERROR;
    }
    str << n;
    if (((INTEGRAL_TYPE)-1) < 1)
    {
      if ((long long)n != value)
      {
#ifdef DEBUGMALLOC
	set_alloc_checking_on();
#endif
	ERROR;
      }
    }
    else
    {
      str << 'U';
      if (value < 0 && n != value + ((INTEGRAL_TYPE)-1) + 1)
      {
#ifdef DEBUGMALLOC
	set_alloc_checking_on();
#endif
	ERROR;
      }
    }
    size_t len = str.pcount();
    out.append(buf, len);
  }
#ifdef DEBUGMALLOC
  set_alloc_checking_on();
#endif
  if (*in == '_')	// Seperator
  {
    ++in;
    if (!isdigit(*in))
      ERROR;
  }
  return 0;
}

static int read_float_type(char const*& in, string& out)
{
  if (*in == 'm')
  {
    ++in;
    out += '-';
  }
  if (!isdigit(*in))
    ERROR;
  out += *in++;
  if (*in != '.')
    ERROR;
  out += *in++;
  while(isdigit(*in))
    out += *in++;
  if (*in != 'e')
    ERROR;
  out += *in++;
  if (*in == 'm')
  {
    ++in;
    out += '-';
  }
  while(isdigit(*in))
    out += *in++;
  if (*in == '_')	// Seperator
  {
    ++in;
    if (!isdigit(*in))
      ERROR;
  }
  return 0;
}

static int demangle_non_type_template_argument(char const*& in, string& out, int flags)
{
  int ret = 0;
  switch (*in)
  {
    case 'b':
      ++in;
      if (*in == '0')
	out += "false";
      else if (*in == '1')
	out += "true";
      else
	ERROR;
      ++in;
      break;
    case 'c':
    {
      ++in;
      if (((flags & demangle_types::is_unsigned) && read_integral_type<unsigned char>(in, out) == -1)
          || (!(flags & demangle_types::is_unsigned) && read_integral_type<char>(in, out) == -1))
        return -1;
      break;
    }
    case 's':
    {
      ++in;
      if (((flags & demangle_types::is_unsigned) && read_integral_type<unsigned short>(in, out) == -1)
          || (!(flags & demangle_types::is_unsigned) && read_integral_type<short>(in, out) == -1))
        return -1;
      break;
    }
    case 'i':
    {
      ++in;
      if (((flags & demangle_types::is_unsigned) && read_integral_type<unsigned int>(in, out) == -1)
          || (!(flags & demangle_types::is_unsigned) && read_integral_type<int>(in, out) == -1))
        return -1;
      break;
    }
    case 'l':
    {
      ++in;
      if (((flags & demangle_types::is_unsigned) && read_integral_type<unsigned long>(in, out) == -1)
          || (!(flags & demangle_types::is_unsigned) && read_integral_type<long>(in, out) == -1))
        return -1;
      if (sizeof(long) > sizeof(int))
	out += 'L';
      break;
    }
    case 'x':
    {
      ++in;
      if (((flags & demangle_types::is_unsigned) && read_integral_type<unsigned long long>(in, out) == -1)
          || (!(flags & demangle_types::is_unsigned) && read_integral_type<long long>(in, out) == -1))
        return -1;
      out += 'L';
      break;
    }
    case 'f':
      ++in;
      if ((flags & demangle_types::is_unsigned) || read_float_type(in, out) == -1)
        ERROR;
      break;
    case 'd':
      ++in;
      if ((flags & demangle_types::is_unsigned) || read_float_type(in, out) == -1)
        ERROR;
      break; 
    case 'r':
      ++in;
      if ((flags & demangle_types::is_unsigned) || read_float_type(in, out) == -1)
        ERROR;
      out += 'L';
      break;
    case 'w':
      ++in;
      if ((flags & demangle_types::is_unsigned) || read_integral_type<wchar_t>(in, out) == -1)
        ERROR;
      break;
    case 'S':
    {
      ++in;			// Eat 'S'
      if (*in != 'c')
        break;
      if ((ret = demangle_non_type_template_argument(in, out, flags)) != -1)
	ret |= demangle_types::expl_signed;
      break;
    }
    case 'U':
    {
      ++in;			// Eat 'U'
      if (*in != 'c' && *in != 's' && *in != 'i' && *in != 'l' && *in != 'x')
        break;
      if ((ret = demangle_non_type_template_argument(in, out, flags|demangle_types::is_unsigned)) != -1)
        ret |= demangle_types::expl_unsigned;
      break;
    }
    case 'P':
    {
      return -1; // Not implemented yet
      char const* cpb = in;
      while (*in == 'C' || *in == 'P')	// Eat all 'C's and 'P's.
        ++in;
      char const* cp = in - 1;
      if ((ret = demangle_non_type_template_argument(in, out, flags)) == -1)
        return -1;
      out += ' ';
      while (cp >= cpb)
      {
        if (*cp == 'C')
	{
	  out += "const";
	  if (cp != cpb)
	    out += ' ';
	}
        else
	{
	  out += '*';
	  ret |= demangle_types::pointer;
	}
        --cp;
      }
      break;
    }
    default:
      ERROR;
  }
  return ret;
}

static int internal_demangle(char const*& in, string& out, int flags)
{
  int ret = 0;
  switch (*in)
  {
    case 'b':
      ++in;
      out += "bool";
      break;
    case 'c':
      out += "char";
      ++in;
      break;
    case 's':
      out += "short";
      ++in;
      break;
    case 'i':
      out += "int";
      ++in;
      break;
    case 'l':
      out += "long";
      ++in;
      break;
    case 'x':
      out += "long long";
      ++in;
      break;
    case 'f':
      if ((flags & demangle_types::integral))
        ERROR;
      out += "float";
      ++in;
      break;
    case 'd':
      if ((flags & demangle_types::integral))
        ERROR;
      out += "double";
      ++in;
      break; 
    case 'r':
      if ((flags & demangle_types::integral))
        ERROR;
      out += "long double";
      ++in;
      break;
    case 'e':
      if ((flags & demangle_types::integral))
        ERROR;
      out += "...";
      ++in;
      break;
    case 'w':
      if ((flags & demangle_types::integral))
        ERROR;
      out += "wchar_t";
      ++in;
      break;
    case 'v':
      if ((flags & demangle_types::integral))
        ERROR;
      out += "void";
      ++in;
      break;
    case 'Q':
    {
      if ((flags & demangle_types::integral))
        ERROR;
      // Nested types
      ++in;			// Eat 'Q'
      int n = *in++ - '0';	// Nest depth (<digit>)
      if (n == ('_' - '0'))	// Depth larger then 9? (_<digits>_)
      {
	n = *in++ - '0';
	do
	  n = n * 10 + *in++ - '0';
	while (isdigit(*in));
	if (*in++ != '_')
	  ERROR;
      }
      if (*in == 'Q' || n < 2)
	ERROR;
      while(n-- > 0 && (ret = internal_demangle(in, out, flags)) != -1)
	if (n > 0)
	  out += "::";
      break;
    }
    case 't':
    {
      if ((flags & demangle_types::integral))
        ERROR;
      ++in;			// Eat 't'
      int n = *in++ - '0';
      while (isdigit(*in))
        n = 10 * n + *in++ - '0';
      out.append(in, n);
      in += n;
      out += '<';
      n = *in++ - '0';
      while (isdigit(*in))
        n = 10 * n + *in++ - '0';
      bool last_is_templ = false;
      for(;n > 0; --n)
      {
        if (*in == 'Z')
	{
	  ++in;			// Eat 'Z'
	  int res = internal_demangle(in, out, flags);
	  if (res == -1)
	    return -1;
	  if (n == 1)
	    last_is_templ = (res & demangle_types::templ);
	}
	else
	{
	  if (demangle_non_type_template_argument(in, out, 0) == -1)
	    return -1;
	}
        if (n > 1)
	  out += ", ";
      }
      if (last_is_templ)
        out += ' ';
      out += '>';
      ret = demangle_types::templ;
      break;
    }
    case '_':
    {
      ++in;
      if (!isdigit(*in))
        ERROR;
      /* FALL THROUGH */
    }
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      if ((flags & demangle_types::integral))
        ERROR;
      // Read class, struct, enum or namespace name
      int n = *in++ - '0';
      while (isdigit(*in))
        n = 10 * n + *in++ - '0';
      out.append(in, n);
      in += n;
      ret = 0;
      break;
    }
    case 'S':
    {
      out += "signed ";
      ++in;			// Eat 'S'
      if (*in != 'c')
        ERROR;
      if ((ret = internal_demangle(in, out, flags)) != -1)
	ret |= demangle_types::expl_signed;
      break;
    }
    case 'U':
    {
      out += "unsigned ";
      ++in;			// Eat 'U'
      if (*in != 'c' && *in != 's' && *in != 'i' && *in != 'l' && *in != 'x')
        ERROR;
      if ((ret = internal_demangle(in, out, flags)) != -1)
        ret |= demangle_types::expl_unsigned;
      break;
    }
    case 'C':
    case 'P':
    {
      char const* cpb = in;
      while (*in == 'C' || *in == 'P')	// Eat all 'C's and 'P's.
        ++in;
      char const* cp = in - 1;
      if ((ret = internal_demangle(in, out, flags)) == -1)
        return -1;
      while (cp >= cpb)
      {
        if (*cp == 'C')
	  out += " const";
        else
	{
	  out += '*';
	  ret |= demangle_types::pointer;
	}
        --cp;
      }
      break;
    }
    default:
      ERROR;
  }
  return ret;
}

int demangle(char const* in, string& out)
{
  if (internal_demangle(in, out, demangle_types::all) == -1 || *in != 0)
    return -1;
  return 0;
}
