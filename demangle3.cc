// $Header$
//
// Copyright (C) 2002 - 2003, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//
  
/*!
\addtogroup group_demangle demangle_type() and demangle_symbol()
\ingroup group_type_info

Libcwd comes with its own demangler functions.&nbsp;

demangle_type() writes the \em mangled type name \p input
to the string \p output; \p input should be the mangled name
as returned by <CODE>typeid(OBJECT).name()</CODE> (using gcc-2.95.1 or higher).

demangle_symbol() writes the \em mangled symbol name \p input
to the string \p output; \p input should be the mangled name 
as returned by <CODE>asymbol::name</CODE> (\c asymbol is a structure defined
by libbfd), which is what is returned by \ref location_ct::mangled_function_name()
and pc_mangled_function_name().

The direct use of these functions should be avoided, instead use the function type_info_of().

*/

//
// This file has been tested with gcc-3.0.
//
// The description of how the mangling is done in the new ABI was found on
// http://www.codesourcery.com/cxx-abi/abi.html#mangling
//
// To compile a standalone demangler:
// g++ -g -DSTANDALONE -DCWDEBUG -Iinclude demangle3.cc -Wl,-rpath,`pwd`/.libs -L.libs -lcwd -o c++filt
//
 
#undef CPPFILTCOMPATIBLE
  
#include "sys.h"
#include "cwd_debug.h"
#include <libcwd/demangle.h>
#include <libcwd/private_assert.h>

#if __GXX_ABI_VERSION > 0

#include <limits>

#ifdef STANDALONE
#ifdef CWDEBUG
namespace libcw {
  namespace debug {
    namespace channels {
      namespace dc {
        channel_ct demangler("DEMANGLER");
      } // namespace dc
    } // namespace channels
  } // namespace debug
} // namespace libcw
#endif
#define DoutEntering(x) Dout(dc::demangler|continued_cf|flush_cf, "Entering " x "(\"" << &M_str[M_pos] << "\", \"" << output << "\") ")
#define RETURN do { if (M_result) Dout(dc::finish, '[' << M_pos << "; \"" << output << "\"]" ); else Dout(dc::finish, "(failed)"); return M_result; } while(0)
#define FAILURE do { if (M_result) { M_result = false; Dout(dc::finish, "[failure]"); } else Dout(dc::finish, "(failed)"); return false; } while(0)
#else // !STANDALONE
#undef CWDEBUG
#undef Dout
#undef DoutFatal
#undef Debug
#define Dout(cntrl, data)
#define DoutFatal(cntrl, data) LibcwDoutFatal(::std, /*nothing*/, cntrl, data)
#define Debug(x)
#define DoutEntering(x)
#define RETURN return M_result
#define FAILURE do { M_result = false; return false; } while(0)
#endif // !STANDALONE

using libcw::debug::_private_::internal_string;
using libcw::debug::_private_::internal_vector;

namespace libcw {
  namespace debug {

namespace {

  enum substitution_nt {
    type,
    template_template_param,
    nested_name_prefix,
    nested_name_template_prefix,
    unscoped_template_name,
  };

  struct substitution_st {
    int M_start_pos;
    substitution_nt M_type;
    int M_number_of_prefixes;
    
    substitution_st(int start_pos, substitution_nt type, int number_of_prefixes) :
        M_start_pos(start_pos), M_type(type), M_number_of_prefixes(number_of_prefixes) { }
  };

  class demangler_ct {
    friend class qualifiers_ct;
  private:
    char const* M_str;
    int M_pos;
    bool M_result;
    int M_inside_template_args;
    int M_inside_type;
    int M_inside_substitution;
    bool M_saw_destructor;
    bool M_name_is_cdtor;
    bool M_name_is_template;
    bool M_name_is_conversion_operator;
    bool M_template_args_need_space;
    internal_string M_function_name;
    internal_vector<int> M_template_arg_pos;
    int M_template_arg_pos_offset;
    internal_vector<substitution_st> M_substitutions_pos;
#ifdef CWDEBUG
    bool M_inside_add_substitution;
#endif

  public:
    explicit demangler_ct(char const* in) :
        M_str(in), M_pos(0), M_result(true), M_inside_template_args(0), M_inside_type(0), M_inside_substitution(0),
	M_saw_destructor(false), M_name_is_cdtor(false), M_name_is_template(false), M_name_is_conversion_operator(false),
	M_template_args_need_space(false), M_template_arg_pos_offset(0)
#ifdef CWDEBUG
        , M_inside_add_substitution(false)
#endif
	{ }

  private:
    inline char current(void) const { return M_str[M_pos]; }
    inline char next(void) { return M_str[++M_pos]; }
    inline char eat_current(void) { return M_str[M_pos++]; }
    struct demangler_ct& skip(int count) { M_pos += count; return *this; }
    void store(int& saved_pos) { saved_pos = M_pos; }
    void restore(int saved_pos) { M_pos = saved_pos; M_result = true; }
    void add_substitution(int start_pos, substitution_nt sub_type, int number_of_prefixes = 0)
    {
      if (!M_inside_substitution)
      {
#ifdef CWDEBUG
        if (M_inside_add_substitution)
	  return;
#endif
	M_substitutions_pos.push_back(substitution_st(start_pos, sub_type, number_of_prefixes));
#ifdef CWDEBUG
	if (!DEBUGCHANNELS::dc::demangler.is_on())
	  return;
        internal_string substitution_name("S");
	int n = M_substitutions_pos.size() - 1;
	if (n > 0)
	  substitution_name += (n <= 10) ? (char)(n + '0' - 1) : (char)(n + 'A' - 11);
	substitution_name += '_';
	internal_string subst;
	int saved_pos = M_pos;
	M_pos = start_pos;
	M_inside_add_substitution = true;
	Debug( dc::demangler.off() );
	switch(sub_type)
	{
	  case type:
	    decode_type(subst);
	    break;
	  case template_template_param:
	    decode_template_param(subst);
	    break;
	  case nested_name_prefix:
	  case nested_name_template_prefix:
	    for (int cnt = number_of_prefixes; cnt > 0; --cnt)
	    {
	      if (current() == 'I')
	      {
	        subst += ' ';
		decode_template_args(subst);
	      }
	      else
	      {
		if (cnt < number_of_prefixes)
		  subst += "::";
	        if (current() == 'S')
		  decode_substitution(subst);
		else
		  decode_unqualified_name(subst);
	      }
	    }
	    break;
	  case unscoped_template_name:
	    decode_unscoped_name(subst);
	    break;
	}
	M_pos = saved_pos;
	Debug( dc::demangler.on() );
	Dout(dc::demangler, "\e[31mAdding substitution " << substitution_name
	    << " : " << subst
	    << " (from " << location_ct((char*)__builtin_return_address(0) + builtin_return_address_offset)
	    << " <- " << location_ct((char*)__builtin_return_address(1) + builtin_return_address_offset)
	    << " <- " << location_ct((char*)__builtin_return_address(2) + builtin_return_address_offset)
	    << ").\e[0m");
	M_inside_add_substitution = false;
#endif
      }
    }

  private:
    bool decode_bare_function_type(internal_string& output);
    bool decode_builtin_type(internal_string& output);
    bool decode_call_offset(internal_string& output);
    bool decode_class_enum_type(internal_string& output);
    bool decode_expression(internal_string& output);
    bool decode_literal(internal_string& output);
    bool decode_local_name(internal_string& output);
    bool decode_name(internal_string& output, internal_string& nested_name_qualifiers);
    bool decode_nested_name(internal_string& output, internal_string& qualifiers);
    bool decode_number(internal_string& output);
    bool decode_operator_name(internal_string& output);
    bool decode_source_name(internal_string& output);
    bool decode_substitution(internal_string& output, qualifiers_ct* qualifiers = NULL);
    bool decode_template_args(internal_string& output);
    bool decode_template_param(internal_string& output, qualifiers_ct* qualifiers = NULL);
    bool decode_unqualified_name(internal_string& output);
    bool decode_unscoped_name(internal_string& output);
    inline bool decode_decimal_integer(internal_string& output);
    inline bool decode_special_name(internal_string& output);

  public:
    static int decode_encoding(char const*, internal_string&);
    bool decode_type(internal_string& output, qualifiers_ct* qualifiers = NULL);
  };

  enum simple_qualifier_nt {
    complex_or_imaginary = 'G',
    pointer = 'P',
    reference = 'R'
  };

  enum cv_qualifier_nt {
    cv_qualifier = 'K'
  };

  enum param_qualifier_nt {
    vendor_extension = 'U',
    array = 'A',
    pointer_to_member = 'M'
  };

  class qualifier_ct {
  private:
    char M_qualifier1;
    char M_qualifier2;
    char M_qualifier3;
    mutable unsigned char cnt;
    internal_string M_optional_type;
    int M_start_pos;
    bool M_part_of_substitution;
  public:
    qualifier_ct(int start_pos, simple_qualifier_nt qualifier, int inside_substitution) :
        M_qualifier1(qualifier),
	M_start_pos(start_pos),
	M_part_of_substitution(inside_substitution) { }
    qualifier_ct(int start_pos, cv_qualifier_nt qualifier, char const* start, int count, int inside_substitution) :
        M_qualifier1(start[0]),
	M_qualifier2((count > 1) ? start[1] : '\0'),
	M_qualifier3((count > 2) ? start[2] : '\0'),
	M_start_pos(start_pos),
	M_part_of_substitution(inside_substitution) { }
    qualifier_ct(int start_pos, param_qualifier_nt qualifier, internal_string optional_type, int inside_substitution) :
        M_qualifier1(qualifier),
	M_optional_type(optional_type),
	M_start_pos(start_pos),
	M_part_of_substitution(inside_substitution) { }
    int start_pos(void) const { return M_start_pos; }
    char first_qualifier(void) const { cnt = 1; return M_qualifier1; }
    char next_qualifier(void) const { return (++cnt == 2) ? M_qualifier2 : (cnt == 3) ? M_qualifier3 : 0; }
    internal_string const& optional_type(void) const { return M_optional_type; }
    bool part_of_substitution(void) const { return M_part_of_substitution; }
  };

  class qualifiers_ct {
  private:
    bool M_printing_suppressed;
    internal_vector<qualifier_ct> M_qualifier_starts;
    demangler_ct& M_demangler;
  public:
    qualifiers_ct(demangler_ct& demangler) : M_printing_suppressed(false), M_demangler(demangler) { }
    void add_qualifier_start(simple_qualifier_nt qualifier, int start_pos, int inside_substitution)
        { M_qualifier_starts.push_back(qualifier_ct(start_pos, qualifier, inside_substitution)); }
    void add_qualifier_start(cv_qualifier_nt qualifier, int start_pos, int count, int inside_substitution)
        { M_qualifier_starts.push_back(qualifier_ct(start_pos, qualifier, &M_demangler.M_str[start_pos], count, inside_substitution)); }
    void add_qualifier_start(param_qualifier_nt qualifier, int start_pos, internal_string optional_type, int inside_substitution)
        { M_qualifier_starts.push_back(qualifier_ct(start_pos, qualifier, optional_type, inside_substitution)); }
    void decode_qualifiers(internal_string& output, bool member_function_pointer_qualifiers);
    bool suppressed(void) const { return M_printing_suppressed; }
    void printing_suppressed(void) { M_printing_suppressed = true; }
    size_t size(void) const { return M_qualifier_starts.size(); }
  };

  //
  // <decimal-integer> ::= 0
  //                   ::= 1|2|3|4|5|6|7|8|9 [<digit>+]
  // <digit>           ::= 0|1|2|3|4|5|6|7|8|9
  //
  // {anonymous}::
  inline bool demangler_ct::decode_decimal_integer(internal_string& output)
  {
    char c = current();
    if (c == '0')
    {
      output += '0';
      eat_current();
    }
    else if (!isdigit(c))
      M_result = false;
    else
    {
      do
      {
        output += c;
      }
      while (isdigit((c = next())));
    }
    return M_result;
  }

  // <number> ::= [n] <decimal-integer>
  //
  // {anonymous}::
  bool demangler_ct::decode_number(internal_string& output)
  {
    DoutEntering("decode_number");
    if (current() != 'n')
      decode_decimal_integer(output);
    else
    {
      output += '-';
      eat_current();
      decode_decimal_integer(output);
    }
    RETURN;
  }

  // <builtin-type> ::= v  # void
  //                ::= w  # wchar_t
  //                ::= b  # bool
  //                ::= c  # char
  //                ::= a  # signed char
  //                ::= h  # unsigned char
  //                ::= s  # short
  //                ::= t  # unsigned short
  //                ::= i  # int
  //                ::= j  # unsigned int
  //                ::= l  # long
  //                ::= m  # unsigned long
  //                ::= x  # long long, __int64
  //                ::= y  # unsigned long long, __int64
  //                ::= n  # __int128
  //                ::= o  # unsigned __int128
  //                ::= f  # float
  //                ::= d  # double
  //                ::= e  # long double, __float80
  //                ::= g  # __float128
  //                ::= z  # ellipsis
  //                ::= u <source-name>    # vendor extended type
  //
  char const* const builtin_type_c[26] = {
    "signed char",	// a
    "bool",		// b
    "char",		// c
    "double",		// d
    "long double",	// e
    "float",		// f
    "__float128",	// g
    "unsigned char",	// h
    "int",		// i
    "unsigned int",	// j
    NULL,		// k
    "long",		// l
    "unsigned long",	// m
    "__int128",		// n
    "unsigned __int128",// o
    NULL,		// p
    NULL,		// q
    NULL,		// r
    "short",		// s
    "unsigned short",	// t
    NULL,		// u
    "void",		// v
    "wchar_t",		// w
    "long long",	// x
    "unsigned long long",// y
    "..."		// z
  };

  //
  // {anonymous}::
  bool demangler_ct::decode_builtin_type(internal_string& output)
  {
    DoutEntering("decode_builtin_type");
    char const* bt;
    if (!islower(current()) || !(bt = builtin_type_c[current() - 'a']))
      FAILURE;
    output += bt;
    eat_current();
    RETURN;
  }

  // <class-enum-type> ::= <name>
  //
  // {anonymous}::
  bool demangler_ct::decode_class_enum_type(internal_string& output)
  {
    DoutEntering("decode_class_enum_type");
    internal_string nested_name_qualifiers;
    if (!decode_name(output, nested_name_qualifiers))
      FAILURE;
    output += nested_name_qualifiers;
    RETURN;
  }

  // <substitution> ::= S <seq-id> _
  //                ::= S_
  //                ::= St # ::std::
  //                ::= Sa # ::std::allocator
  //                ::= Sb # ::std::basic_string
  //                ::= Ss # ::std::basic_string<char, std::char_traits<char>, std::allocator<char> >
  //                ::= Si # ::std::basic_istream<char,  std::char_traits<char> >
  //                ::= So # ::std::basic_ostream<char,  std::char_traits<char> >
  //                ::= Sd # ::std::basic_iostream<char, std::char_traits<char> >
  //
  // <seq-id> ::= 0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z [<seq-id>]	# Base 36 number
  //
  // {anonymous}::
  bool demangler_ct::decode_substitution(internal_string& output, qualifiers_ct* qualifiers)
  {
    DoutEntering("decode_substitution");
    unsigned int value = 0;
    char c = next();
    if (c != '_')
    {
      switch(c)
      {
	case 'a':
	{
	  output += "std::allocator";
	  if (!M_inside_template_args)
	  {
	    M_function_name = "allocator";
	    M_name_is_template = true;
	    M_name_is_cdtor = false;
	    M_name_is_conversion_operator = false;
	  }
	  eat_current();
	  if (qualifiers)
	    qualifiers->printing_suppressed();
	  RETURN;
	}
	case 'b':
	{
	  output += "std::basic_string";
	  if (!M_inside_template_args)
	  {
	    M_function_name = "basic_string";
	    M_name_is_template = true;
	    M_name_is_cdtor = false;
	    M_name_is_conversion_operator = false;
	  }
	  eat_current();
	  if (qualifiers)
	    qualifiers->printing_suppressed();
	  RETURN;
	}
	case 'd':
	  output += "std::iostream";
	  if (!M_inside_template_args)
	  {
	    M_function_name = "iostream";
	    M_name_is_template = true;
	    M_name_is_cdtor = false;
	    M_name_is_conversion_operator = false;
	  }
	  eat_current();
	  if (qualifiers)
	    qualifiers->printing_suppressed();
	  RETURN;
	case 'i':
	  output += "std::istream";
	  if (!M_inside_template_args)
	  {
	    M_function_name = "istream";
	    M_name_is_template = true;
	    M_name_is_cdtor = false;
	    M_name_is_conversion_operator = false;
	  }
	  eat_current();
	  if (qualifiers)
	    qualifiers->printing_suppressed();
	  RETURN;
	case 'o':
	  output += "std::ostream";
	  if (!M_inside_template_args)
	  {
	    M_function_name = "ostream";
	    M_name_is_template = true;
	    M_name_is_cdtor = false;
	    M_name_is_conversion_operator = false;
	  }
	  eat_current();
	  if (qualifiers)
	    qualifiers->printing_suppressed();
	  RETURN;
	case 's':
	  output += "std::string";
	  if (!M_inside_template_args)
	  {
	    M_function_name = "string";
	    M_name_is_template = true;
	    M_name_is_cdtor = false;
	    M_name_is_conversion_operator = false;
	  }
	  eat_current();
	  if (qualifiers)
	    qualifiers->printing_suppressed();
	  RETURN;
	case 't':
	  output += "std";
	  eat_current();
	  if (qualifiers)
	    qualifiers->printing_suppressed();
	  RETURN;
	default:
	  for(;; c = next())
	  {
	    if (isdigit(c))
	      value = value * 36 + c - '0';
	    else if (isupper(c))
	      value = value * 36 + c - 'A' + 10;
	    else if (c == '_')
	      break;
	    else
	      FAILURE;
	  }
	  ++value;
	  break;
      }
    }
    eat_current();
    if (value >= M_substitutions_pos.size() ||
        M_inside_type > 20)				// Rather than core dump.
      FAILURE;
    ++M_inside_substitution;
    int saved_pos = M_pos;
    substitution_st& substitution(M_substitutions_pos[value]);
    M_pos = substitution.M_start_pos;
    switch(substitution.M_type)
    {
      case type:
	decode_type(output, qualifiers);
	break;
      case template_template_param:
        decode_template_param(output, qualifiers);
        break;
      case nested_name_prefix:
      case nested_name_template_prefix:
        for (int cnt = substitution.M_number_of_prefixes; cnt > 0; --cnt)
	{
	  if (current() == 'I')
	  {
	    if (M_template_args_need_space)
	      output += ' ';
	    M_template_args_need_space = false;
	    if (!decode_template_args(output))
	      FAILURE;
	  }
	  else
	  {
	    if (cnt < substitution.M_number_of_prefixes)
	      output += "::";
	    if (current() == 'S')
	    {
	      if (!decode_substitution(output))
		FAILURE;
	    }
	    else if (!decode_unqualified_name(output))
	      FAILURE;
	  }
	}
	if (qualifiers)
	  qualifiers->printing_suppressed();
        break;
      case unscoped_template_name:
        decode_unscoped_name(output);
	if (qualifiers)
	  qualifiers->printing_suppressed();
        break;
    }
    M_pos = saved_pos;
    --M_inside_substitution;
    RETURN;
  }

  // <template-param> ::= T_				# first template parameter
  //                  ::= T <parameter-2 non-negative number> _
  //
  // {anonymous}::
  bool demangler_ct::decode_template_param(internal_string& output, qualifiers_ct* qualifiers)
  {
    DoutEntering("decode_template_parameter");
    if (current() != 'T')
      FAILURE;
    unsigned int value = 0;
    char c;
    if ((c = next()) != '_')
    {
      while(isdigit(c))
      {
        value = value * 10 + c - '0';
	c = next();
      }
      ++value;
    }
    if (eat_current() != '_')
      FAILURE;
    value += M_template_arg_pos_offset;
    if (value >= M_template_arg_pos.size())
      FAILURE;
    int saved_pos = M_pos;
    M_pos = M_template_arg_pos[value];
    if (M_inside_type > 20)		// Rather than core dump.
      FAILURE;
    ++M_inside_substitution;
    if (current() == 'X')
    {
      eat_current();
      decode_expression(output);
    }
    else if (current() == 'L')
      decode_literal(output);
    else
      decode_type(output, qualifiers);
    --M_inside_substitution;
    M_pos = saved_pos;
    RETURN;
  }

  bool demangler_ct::decode_literal(internal_string& output)
  {
    DoutEntering("decode_literal");
    eat_current();	// Eat the 'L'.
    if (current() == '_')
    {
      if (next() != 'Z')
	FAILURE;
      eat_current();
      if ((M_pos += decode_encoding(M_str + M_pos, output)) < 0)
	FAILURE;
    }
    else
    {
      // Special cases
      if (current() == 'b')
      {
        if (next() == '0')
	  output += "false";
	else
	  output += "true";
	eat_current();
	RETURN;
      }
      char c = current();
      if (c == 'i' || c == 'j' || c == 'l' || c == 'm' || c == 'x' || c == 'y')
        eat_current();
      else
      {
	output += '(';
	if (!decode_type(output))
	  FAILURE;
	output += ')';
      }
      if (!decode_number(output))
	FAILURE;
      if (c == 'j' || c == 'm' || c == 'y')
        output += 'u';
      if (c == 'l' || c == 'm')
        output += 'l';
      if (c == 'x' || c == 'y')
        output += "ll";
    }
    RETURN;
  }

  // <operator-name> ::= nw				# new           
  //                 ::= na				# new[]
  //                 ::= dl				# delete        
  //                 ::= da				# delete[]      
  //                 ::= ng				# - (unary)     
  //                 ::= ad				# & (unary)     
  //                 ::= de				# * (unary)     
  //                 ::= co				# ~             
  //                 ::= pl				# +             
  //                 ::= mi				# -             
  //                 ::= ml				# *             
  //                 ::= dv				# /             
  //                 ::= rm				# %             
  //                 ::= an				# &             
  //                 ::= or				# |             
  //                 ::= eo				# ^             
  //                 ::= aS				# =             
  //                 ::= pL				# +=            
  //                 ::= mI				# -=            
  //                 ::= mL				# *=            
  //                 ::= dV				# /=            
  //                 ::= rM				# %=            
  //                 ::= aN				# &=            
  //                 ::= oR				# |=            
  //                 ::= eO				# ^=            
  //                 ::= ls				# <<            
  //                 ::= rs				# >>            
  //                 ::= lS				# <<=           
  //                 ::= rS				# >>=           
  //                 ::= eq				# ==            
  //                 ::= ne				# !=            
  //                 ::= lt				# <             
  //                 ::= gt				# >             
  //                 ::= le				# <=            
  //                 ::= ge				# >=            
  //                 ::= nt				# !             
  //                 ::= aa				# &&            
  //                 ::= oo				# ||            
  //                 ::= pp				# ++            
  //                 ::= mm				# --            
  //                 ::= cm				# ,             
  //                 ::= pm				# ->*           
  //                 ::= pt				# ->            
  //                 ::= cl				# ()            
  //                 ::= ix				# []            
  //                 ::= qu				# ?             
  //                 ::= sz				# sizeof        
  //                 ::= sr				# scope resolution (::), see below        
  //                 ::= cv <type>			# (cast)        
  //                 ::= v <digit> <source-name>	# vendor extended operator
  //
  //
  // Symbol operator codes exist of two characters, we need to find a
  // quick hash so that their names can be looked up in a table.
  //
  // The puzzle :)
  // Shift the rows so that there is at most one character per column.
  //
  // A perfect solution:
  //                                              horizontal
  //    .....................................     offset + 'a'
  // a, ||a||d|||||||||n||||s||||||||||||||||||	    2
  // c, || || ||lm|o||| |||| ||||||||||||||||||	   -3
  // d, || a| |e  | ||l |||| |||v||||||||||||||	    3
  // e, ||  | |   o q|  |||| ||| ||||||||||||||	   -4
  // g, |e  | |      |  t||| ||| ||||||||||||||	   -3
  // i, |   | |      |   ||| ||| ||||||||||x|||    12
  // l, |   | |      e   ||| ||| ||st|||||| |||	    9
  // m, |   | |          ||| ||| |i  lm|||| |||	   18
  // n, a   e g          ||t |w| |     |||| |||	    0
  // o,                  ||  | | |     ||o| r||	   19
  // p,                  lm  p | t     || |  ||	    6
  // q,                        |       || u  ||	   14
  // r,                        |       |m    |s	   20
  // s,                        r       z     | 	    6
  //    .....................................
  // ^            ^__ second character
  // |___ first character
  //

  // Putting that solution in tables:

  // {anonymous}::
  char const offset_table_c[1 + CHAR_MAX - CHAR_MIN /* ascii value of first character */ ] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
#if (CHAR_MIN<0)
    // Add -CHAR_MIN extra zeroes (128):
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
    //   a    b    c    d    e    f    g    h    i    j    k    l    m    n    o    p    q    r    s    t    u    v
    0, -95,   0,-100, -94,-101,   0,-100,   0, -85,   0,   0, -88, -79, -97, -78, -91, -83, -77, -91,   0,   0,   0,
#else
    0, 161,   0, 156, 162, 155,   0, 156,   0, 171,   0,   0, 168, 177, 159, 178, 165, 173, 179, 165,   0,   0,   0,
#endif
    // ... more zeros
  };

  // {anonymous}::
  struct entry_st { char const* opcode; char const* symbol_name; bool unary; } const symbol_name_table_c[39] = {
    { "na",  "operator new[]", true },
    { "ge",  "operator>=", false },
    { "aa",  "operator&&", false },
    { "da",  "operator delete[]", true },
    { "ne",  "operator!=", false },
    { "ad",  "operator&", true },	// unary
    { "ng",  "operator-", true },	// unary
    { "de",  "operator*", true },	// unary
    { "cl",  "operator()", true },
    { "cm",  "operator,", false },
    { "eo=", "operator^", false },
    { "co",  "operator~", false },
    { "eq",  "operator==", false },
    { "le",  "operator<=", false },
    { "dl",  "operator delete", true },
    { "an=", "operator&", false },
    { "gt",  "operator>", false },
    { "pl=", "operator+", false },
    { "pm",  "operator->*", false },
    { "nt",  "operator!", true },
    { "as=", "operator", false },
    { "pp",  "operator++", true },
    { "nw",  "operator new", true },
    { "sr",  "::", true },
    { "dv=", "operator/", false },
    { "pt",  "operator->", false },
    { "mi=", "operator-", false },
    { "ls=", "operator<<", false },
    { "lt",  "operator<", false },
    { "ml=", "operator*", false },
    { "mm",  "operator--", true },
    { "sz",  "sizeof", true },
    { "rm=", "operator%", false },
    { "oo",  "operator||", false },
    { "qu",  "operator?", false },
    { "ix",  "operator[]", true },
    { "or=", "operator|", false },
    { "", NULL, false },
    { "rs=", "operator>>", false }
  };

  // {anonymous}::
  bool demangler_ct::decode_operator_name(internal_string& output)
  {
    DoutEntering("decode_operator_name");

    char opcode0 = current();
    char opcode1 = tolower(next());

    register char hash;
    if ((hash = offset_table_c[opcode0 - CHAR_MIN]))
    {
      hash += opcode1;
      if (
#if (CHAR_MIN<0)
	  hash >= 0 &&
#endif
	  hash < 39)
      {
	int index = static_cast<int>(static_cast<unsigned char>(hash));
	entry_st entry = symbol_name_table_c[index];
	if (entry.opcode[0] == opcode0 && entry.opcode[1] == opcode1 && (opcode1 == current() || entry.opcode[2] == '='))
	{
	  output += entry.symbol_name;
	  if (opcode1 != current())
	    output += '=';
	  eat_current();
	  if (hash == 27 || hash == 28)
	    M_template_args_need_space = true;
	  RETURN;
	}
	else if (opcode0 == 'c' && opcode1 == 'v')
	{
	  eat_current();
	  output += "operator ";
	  if (!decode_type(output))
	    FAILURE;
          if (!M_inside_template_args)
	    M_name_is_conversion_operator = true;
	  RETURN;
	}
      }
    }
    FAILURE;
  }

  //
  // <expression> ::= <unary operator-name> <expression>
  //              ::= <binary operator-name> <expression> <expression>
  //              ::= <expr-primary>
  //
  // <expr-primary> ::= <template-param>			# Starts with a T
  //                ::= L <type> <value number> E		# literal
  //                ::= L <mangled-name> E			# external name
  //
  // {anonymous}::
  bool demangler_ct::decode_expression(internal_string& output)
  {
    DoutEntering("decode_expression");
    if (current() == 'T')
    {
      if (!decode_template_param(output))
        FAILURE;
      RETURN;
    }
    else if (current() == 'L')
    {
      if (!decode_literal(output))
        FAILURE;
      if (current() != 'E')
	FAILURE;
      eat_current();
      RETURN;
    }
    else
    {
      char opcode0 = current();
      char opcode1 = tolower(next());

      register char hash;
      if ((hash = offset_table_c[opcode0 - CHAR_MIN]))
      {
	hash += opcode1;
	if (
#if (CHAR_MIN<0)
	    hash >= 0 &&
#endif
	    hash < 39)
	{
	  int index = static_cast<int>(static_cast<unsigned char>(hash));
	  entry_st entry = symbol_name_table_c[index];
	  if (entry.opcode[0] == opcode0 && entry.opcode[1] == opcode1 && (opcode1 == current() || entry.opcode[2] == '='))
	  {
	    char const* p = entry.symbol_name;
	    if (!strncmp("operator", p, 8))
	      p += 8;
	    if (*p == ' ')
	      ++p;
	    if (entry.unary)
	      output += p;
	    bool is_eq = (opcode1 != current());
	    eat_current();
	    output += '(';
	    if (!decode_expression(output))
	      FAILURE;
	    output += ')';
	    if (!entry.unary)
	    {
	      output += ' ';
	      output += p;
	      if (is_eq)
		output += '=';
	      output += ' ';
	      output += '(';
	      if (!decode_expression(output))
		FAILURE;
	      output += ')';
	    }
	    RETURN;
	  }
	}
      }
    }
    FAILURE;
  }

  //
  // <template-args> ::= I <template-arg>+ E
  // <template-arg> ::= <type>					# type or template
  //                ::= L <type> <value number> E		# literal
  //                ::= L_Z <encoding> E			# external name
  //                ::= X <expression> E			# expression
  // {anonymous}::
  bool demangler_ct::decode_template_args(internal_string& output)
  {
    DoutEntering("decode_template_args");
    if (eat_current() != 'I')
      FAILURE;
    int prev_size = M_template_arg_pos.size();
    ++M_inside_template_args;
    if (M_template_args_need_space)
    {
      output += ' ';
      M_template_args_need_space = false;
    }
    output += '<';
    for(;;)
    {
      if (M_inside_template_args == 1 && !M_inside_type)
	M_template_arg_pos.push_back(M_pos);
      if (current() == 'X')
      {
        eat_current();
	if (!decode_expression(output))
	  FAILURE;
	if (current() != 'E')
	  FAILURE;
        eat_current();
      }
      else if (current() == 'L')
      {
        if (!decode_literal(output))
	  FAILURE;
	if (current() != 'E')
	  FAILURE;
        eat_current();
      }
      else if (!decode_type(output))
        FAILURE;
      if (current() == 'E')
        break;
      output += ", ";
    }
    eat_current();
    if (*(output.rbegin()) == '>')
      output += ' ';
    output += '>';
    --M_inside_template_args;
    if (!M_inside_template_args && !M_inside_type)
    {
      M_name_is_template = true;
      M_template_arg_pos_offset = prev_size;
    }
    RETURN;
  }

  // <bare-function-type> ::= <signature type>+		# types are parameter types
  //
  // {anonymous}::
  bool demangler_ct::decode_bare_function_type(internal_string& output)
  {
    DoutEntering("decode_bare_function_type");
    if (M_saw_destructor)
    {
      if (eat_current() != 'v' || (current() != 'E' && current() != 0))
        FAILURE;
      output += "()";
      M_saw_destructor = false;
      RETURN;
    }
    output += '(';
    M_template_args_need_space = false;
    if (!decode_type(output))			// Must have at least one parameter
      FAILURE;
    while (current() != 'E' && current() != 0)
    {
      output += ", ";
      if (!decode_type(output))
	FAILURE;
    }
    output += ')';
    RETURN;
  }

  // <type> ::= <builtin-type>					# Starts with a lower case character != r.
  //        ::= <function-type>					# Starts with F
  //        ::= <class-enum-type>				# Starts with N, S, C, D, Z, a digit or a lower case character.
  //								#   since a lower case character would be an operator name, that would
  //								#   be an error.  The S is a substitution or St (::std::).  A 'C' would
  //								#   be a constructor and thus also an error.
  //        ::= <template-param>				# Starts with T
  //        ::= <substitution>                         		# Starts with S
  //        ::= <template-template-param> <template-args>	# Starts with T or S, equivalent with the above.
  //
  //        ::= <array-type>					# Starts with A
  //        ::= <pointer-to-member-type>			# Starts with M
  //        ::= <CV-qualifiers> <type>				# Starts with r, V or K
  //        ::= P <type>   # pointer-to				# Starts with P
  //        ::= R <type>   # reference-to			# Starts with R
  //        ::= C <type>   # complex pair (C 2000)		# Starts with C
  //        ::= G <type>   # imaginary (C 2000)			# Starts with G
  //        ::= U <source-name> <type>     			# vendor extended type qualifier, starts with U
  //
  // <template-template-param> ::= <template-param>
  //                           ::= <substitution>

  // My own analysis of how to decode qualifiers:
  //
  // F is a <function-type>, <T> is a <builtin-type>, <class-enum-type>, <template-param> or <template-template-param> <template-args>.
  // <Q> represents a series of qualifiers (not G or C).
  // <C> is an unqualified type.  <R> is a qualified type.
  // <B> is the bare-function-type without return type.  <I> is the array index.
  //
  //								Substitutions:
  // <Q>M<C><Q2>F<R><B>E        ==> R (C::*Q)B Q2               "<C>", "F<R><B>E" (<R> and <B> recursive), "M<C><Q2>F<R><B>E".
  // <Q>F<R><B>E 		==> R (Q)B			"<R>", "<B>" (<B> recursive) and "F<R><B>E".
  // <Q>G<T>     		==> imaginary T Q		"<T>", "G<T>" (<T> recursive).
  // <Q>C<T>     		==> complex T Q			"<T>", "C<T>" (<T> recursive).
  // <Q><T>      		==> T Q				"<T>" (<T> recursive).
  //
  // where Q is any of:
  //
  // <Q>P   		==> *Q					"P..."
  // <Q>R   		==> &Q					"R..."
  // <Q>[K|V|r]+	==> [ const| volatile| restrict]+Q	"KVr..."
  // <Q>U<S>		==>  SQ					"U<S>..."
  // A<I>		==>  [I]				"A<I>..." (<I> recursive).
  // <Q>A<I>		==>  (Q) [I]				"A<I>..." (<I> recursive).
  // <Q>M<C>		==> C::*Q				"M<C>..." (<C> recursive).
  //  
  // A <substitution> is handled with an input position switch during which new substitutions are
  // turned off.  Because recursive handling of types (and therefore the order in which substitutions
  // must be generated) must be done left to right, but the generation of Q needs processing right to left,
  // substitutions per <type> are generated by reading the input left to right and marking the starts of
  // all substitutions only - implicitly finishing them at the end of the type.  Then the output and real
  // substitutions are generated.
  //
  // The following comment was for the demangling of g++ version 3.0.x.  The mangling (and I believe
  // even the ABI description) have been fixed now (as of g++ version 3.1).
  //
  // g++ 3.0.x only:
  // The ABI specifies for pointer-to-member function types the format <Q>M<T>F<R><B>E.  In other words,
  // the qualifier <Q2> (see above) is implicitely contained in <T> instead of explicitly part of the M
  // format.  I am convinced that this is a bug in the ABI.  Unfortunately, this is how we have to
  // demangle things as it has a direct impact on the order in which substitutions are stored.
  // This ill-formed design results in rather ill-formed demangler code too however :/
  //
  // <Q2> is now explicitely part of the M format.
  // For some weird reason, g++ (3.2.1) does not add substitutions for qualified member function pointers.
  // I think that is another bug.
  //

  void qualifiers_ct::decode_qualifiers(internal_string& output, bool member_function_pointer_qualifiers = false)
  {
    internal_string postfix;
    for(internal_vector<qualifier_ct>::reverse_iterator iter = M_qualifier_starts.rbegin(); iter != M_qualifier_starts.rend();)
    {
      if (!member_function_pointer_qualifiers && !(*iter).part_of_substitution())
      {
	int saved_inside_substitution = M_demangler.M_inside_substitution;
        M_demangler.M_inside_substitution = 0;
	M_demangler.add_substitution((*iter).start_pos(), type);
	M_demangler.M_inside_substitution = saved_inside_substitution;
      }
      char qualifier = (*iter).first_qualifier();
      for(; qualifier; qualifier = (*iter).next_qualifier())
      {
	switch(qualifier)
	{
	  case 'P':
	    output += "*";
	    break;
	  case 'R':
	    output += "&";
	    break;
	  case 'K':
	    output += " const";
	    continue;
	  case 'V':
	    output += " volatile";
	    continue;
	  case 'r':
	    output += " restrict";
	    continue;
	  case 'A':
	  {
	    internal_string index = (*iter).optional_type();
	    if (++iter != M_qualifier_starts.rend())
	    {
	      output += " (";
	      postfix = ") [" + index + "]" + postfix;
	    }
	    else
	    {
	      output += " [";
	      output += index;
	      output += "]";
	    }
	    break;
	  }
	  case 'M':
	    output += " ";
	    output += (*iter).optional_type();
	    output += "::*";
	    break;
	  case 'U':
	    output += " ";
	    output += (*iter).optional_type();
	    break;
	  case 'G':	// Only here so we added a substitution.
	    break;
	}
	break;
      }
      if (qualifier != 'A')
	++iter;
    }
    output += postfix;
    M_printing_suppressed = false;
  }

  //
  // {anonymous}::
  bool demangler_ct::decode_type(internal_string& output, qualifiers_ct* qualifiers)
  {
    DoutEntering("decode_type");
    ++M_inside_type;
    bool recursive_template_param_or_substitution_call;
    if (!(recursive_template_param_or_substitution_call = qualifiers))
      qualifiers = new qualifiers_ct(*this);
#ifdef CWDEBUG
    else
      Dout(dc::continued, "with qualifiers ");
#endif
    // First eat all qualifiers.
    bool failure = false;
    for(;;)		// So we can use 'continue' to eat the next qualifier.
    {
      int start_pos = M_pos;
      switch(current())
      {
        case 'P':
	  qualifiers->add_qualifier_start(pointer, start_pos, M_inside_substitution);
	  eat_current();
          continue;
	case 'R':
	  qualifiers->add_qualifier_start(reference, start_pos, M_inside_substitution);
	  eat_current();
	  continue;
        case 'K':
	case 'V':
	case 'r':
        {
	  char c;
	  int count = 0;
	  do
	  {
	    ++count;
	    c = next();
	  }
	  while(c == 'K' || c == 'V' || c == 'r');
	  qualifiers->add_qualifier_start(cv_qualifier, start_pos, count, M_inside_substitution);
	  continue;
	}
	case 'U':
	{
	  eat_current();
	  internal_string source_name;
          if (!decode_source_name(source_name))
	  {
	    failure = true;
	    break;
	  }
          qualifiers->add_qualifier_start(vendor_extension, start_pos, source_name, M_inside_substitution);
	  continue;
	}
	case 'A':
	{
	  // <array-type> ::= A <positive dimension number> _ <element type>
	  //              ::= A [<dimension expression>] _ <element type>
	  //
	  internal_string index;
	  int saved_pos;
	  store(saved_pos);
	  if (next() == 'n' || !decode_number(index))
	  {
	    restore(saved_pos);
	    if (next() != '_' && !decode_expression(index))
	    {
	      failure = true;
	      break;
	    }
	  }
	  if (eat_current() != '_')
	  {
	    failure = true;
	    break;
	  }
          qualifiers->add_qualifier_start(array, start_pos, index, M_inside_substitution);
	  continue;
	}
	case 'M':
	{
	  // <Q>M<C> or <Q>M<C><Q2>F<R><B>E
	  eat_current();
	  internal_string class_type;
	  if (!decode_type(class_type))				// substitution: "<C>".
	  {
	    failure = true;
	    break;
	  }
	  char c = current();
	  if (c == 'F' || c == 'K' || c == 'V' || c == 'r')	// Must be CV-qualifiers and a member function pointer.
	  {
	    // <Q>M<C><Q2>F<R><B>E      ==> R (C::*Q)B Q2               "<C>", "F<R><B>E" (<R> and <B> recursive), "M<C><Q2>F<R><B>E".
	    int count = 0;
	    int Q2_start_pos = M_pos;
	    while(c == 'K' || c == 'V' || c == 'r')		// Decode <Q2>
	    {
	      ++count;
	      c = next();
	    }
	    qualifiers_ct class_type_qualifiers(*this);
	    if (count)
	      class_type_qualifiers.add_qualifier_start(cv_qualifier, Q2_start_pos, count, M_inside_substitution);
	    internal_string member_function_qualifiers;
            // It is unclear why g++ doesn't add a substitution for "<Q2>F<R><B>E" as it should I think.
            class_type_qualifiers.decode_qualifiers(member_function_qualifiers, true /* suppress adding a substitution */);
	    int function_pos = M_pos;
	    if (eat_current() != 'F')
	    {
	      failure = true;
	      break;
	    }
	    // Return type.
	    // Constructors, destructors and conversion operators don't have a return type, but seem to never get here.
	    if (!decode_type(output))							// substitution: <R> recursive
	    {
	      failure = true;
	      break;
	    }
	    output += " (";
	    output += class_type;
	    output += "::*";
	    internal_string bare_function_type;
	    if (!decode_bare_function_type(bare_function_type) || eat_current() != 'E')	// substitution: <B> recursive
	    {
	      failure = true;
	      break;
	    }
            // I don't think this substitution is actually ever used.
            add_substitution(function_pos, type);				// substitution: "F<R><B>E".
	    add_substitution(start_pos, type);					// substitution: "M<C><Q2>F<R><B>E".
	    qualifiers->decode_qualifiers(output);				// substitution: all qualified types if any.
	    output += ")";
	    output += bare_function_type;
	    output += member_function_qualifiers;
	    goto decode_type_exit;
	  }
          qualifiers->add_qualifier_start(pointer_to_member, start_pos, class_type, M_inside_substitution);
	  continue;
	}
        default:
	  break;
      }
      break;
    }
    if (!failure)
    {
      // <Q>G<T>     		==> imaginary T Q		"<T>", "G<T>" (<T> recursive).
      // <Q>C<T>     		==> complex T Q			"<T>", "C<T>" (<T> recursive).
      if (current() == 'C' || current() == 'G')
      {
	output += current() == 'C' ? "complex " : "imaginary ";
	qualifiers->add_qualifier_start(complex_or_imaginary, M_pos, M_inside_substitution);
	eat_current();
      }
      int start_pos = M_pos;
      switch(current())
      {
	case 'F':
	{
	  // <Q>F<R><B>E 		==> R (Q)B			"<R>", "<B>" (<B> recursive) and "F<R><B>E".
	  eat_current();
	  // Return type.
	  if (!decode_type(output))				// substitution: "<R>".
	  {
	    failure = true;
	    break;
	  }
	  output += " (";
	  internal_string bare_function_type;
	  if (!decode_bare_function_type(bare_function_type) || eat_current() != 'E')	// substitution: "<B>" (<B> recursive).
	  {
	    failure = true;
	    break;
	  }
	  add_substitution(start_pos, type);			// substitution: "F<R><B>E"
	  qualifiers->decode_qualifiers(output);		// substitution: all qualified types, if any.
	  output += ")";
	  output += bare_function_type;
	  break;
	}
	case 'T':
	  if (!decode_template_param(output, qualifiers))
	  {
	    failure = true;
	    break;
	  }
	  if (current() == 'I')
	  {
	    add_substitution(start_pos, template_template_param);	// substitution: "<template-template-param>".
	    if (!decode_template_args(output))
	    {
	      failure = true;
	      break;
	    }
	  }
	  if (!recursive_template_param_or_substitution_call && qualifiers->suppressed())
	  {
	    add_substitution(start_pos, type);			// substitution: "<template-param>" or "<template-template-param> <template-args>".
	    qualifiers->decode_qualifiers(output);		// substitution: all qualified types, if any.
	  }
	  break;
	case 'S':
	  if (M_str[M_pos + 1] != 't')
	  {
	    if (!decode_substitution(output, qualifiers))
	    {
	      failure = true;
	      break;
	    }
	    if (current() == 'I')
	    {
	      if (!decode_template_args(output))
	      {
		failure = true;
		break;
	      }
	      if (!recursive_template_param_or_substitution_call && qualifiers->suppressed())
		add_substitution(start_pos, type);			// substitution: "<template-template-param> <template-args>".
	    }
	    if (!recursive_template_param_or_substitution_call && qualifiers->suppressed())
	      qualifiers->decode_qualifiers(output);			// substitution: all qualified types, if any.
	    break;
	  }
	  /* Fall-through for St */
	case 'N':
	case 'Z':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  // <Q><T>      		==> T Q				"<T>" (<T> recursive).
	  if (!decode_class_enum_type(output))
	  {
	    failure = true;
	    break;
	  }
          if (!recursive_template_param_or_substitution_call)
	  {
	    add_substitution(start_pos, type);				// substitution: "<class-enum-type>".
	    qualifiers->decode_qualifiers(output);			// substitution: all qualified types, if any.
	  }
          else
	    qualifiers->printing_suppressed();
	  break;
	default:
	  // <Q><T>      		==> T Q				"<T>" (<T> recursive).
	  if (!decode_builtin_type(output))
	  {
	    failure = true;
	    break;
	  }
	  // If decode_type was called from decode_template_param then we need to suppress calling qualifiers here
	  // in order to get a substitution added anyway (for the <template-param>).
          if (!recursive_template_param_or_substitution_call)
	    qualifiers->decode_qualifiers(output);
          else
	    qualifiers->printing_suppressed();
	  break;
      }
    }
decode_type_exit:
    --M_inside_type;
    if (!recursive_template_param_or_substitution_call)
      delete qualifiers;
    if (failure)
      FAILURE;
    RETURN;
  }

  // <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name> E
  //               ::= N [<CV-qualifiers>] <template-prefix> <template-args> E
  //
  // <prefix> ::= <prefix> <unqualified-name>
  //          ::= <template-prefix> <template-args>
  //          ::= # empty
  //          ::= <substitution>
  //
  // <template-prefix> ::= <prefix> <template unqualified-name>
  //                   ::= <substitution>
  //
  // {anonymous}::
  bool demangler_ct::decode_nested_name(internal_string& output, internal_string& qualifiers)
  {
    DoutEntering("decode_nested_name");

    if (current() != 'N')
      FAILURE;

    // <CV-qualifiers> ::= [r] [V] [K]       # restrict (C99), volatile, const
    char const* qualifiers_start = &M_str[M_pos + 1];
    for (char c = next(); c == 'K' || c == 'V' || c == 'r'; c = next());
    for (char const* qualifier_ptr = &M_str[M_pos - 1]; qualifier_ptr >= qualifiers_start; --qualifier_ptr)
      switch(*qualifier_ptr)
      {
        case 'K':
	  qualifiers += " const";
	  break;
	case 'V':
	  qualifiers += " volatile";
	  break;
	case 'r':
	  qualifiers += " restrict";
	  break;
      }

    int number_of_prefixes = 0;
    int substitution_start = M_pos;
    for(;;)
    {
      ++number_of_prefixes;
      if (current() == 'S')
      {
        if (!decode_substitution(output))
	  FAILURE;
      }
      else if (current() == 'I')
      {
	if (!decode_template_args(output))
	  FAILURE;
	if (current() != 'E')
	  add_substitution(substitution_start, nested_name_prefix, number_of_prefixes);
	  								// substitution: "<template-prefix> <template-args>".
      }
      else
      {
        if (!decode_unqualified_name(output))
	  FAILURE;
	if (current() != 'E')
	  add_substitution(substitution_start, (current() == 'I') ? nested_name_template_prefix : nested_name_prefix, number_of_prefixes);
	  								// substitution: "<prefix> <unqualified-name>" or
									// "<prefix> <template unqualified-name>".
      }
      if (current() == 'E')
      {
        eat_current();
        RETURN;
      }
      if (current() != 'I')
	output += "::";
      else if (M_template_args_need_space)
        output += ' ';
      M_template_args_need_space = false;
    }
    FAILURE;
  }

  // <local-name> := Z <function encoding> E <entity name> [<discriminator>]
  //              := Z <function encoding> E s [<discriminator>]
  // <discriminator> := _ <non-negative number>
  //
  // {anonymous}::
  bool demangler_ct::decode_local_name(internal_string& output)
  {
    DoutEntering("decode_local_name");
    if (current() != 'Z')
      FAILURE;
    if ((M_pos += decode_encoding(M_str + M_pos + 1, output) + 1) < 0 || eat_current() != 'E')
      FAILURE;
    output += "::";
    if (current() == 's')
    {
      eat_current();
      output += "string literal";
    }
    else
    {
      internal_string nested_name_qualifiers;
      if (!decode_name(output, nested_name_qualifiers))
	FAILURE;
      output += nested_name_qualifiers;
    }
    internal_string discriminator;
    if (current() == '_' && next() != 'n' && !decode_number(discriminator))
      FAILURE;
    RETURN;
  }

  // <source-name> ::= <positive length number> <identifier>
  //
  // {anonymous}::
  bool demangler_ct::decode_source_name(internal_string& output)
  {
    DoutEntering("decode_source_name");
    int length = current() - '0';
    if (length < 1 || length > 9)
      FAILURE;
    while(isdigit(next()))
      length = 10 * length + current() - '0';
    char const* ptr = &M_str[M_pos];
    if (length > 11 && !strncmp(ptr, "_GLOBAL_", 8) && ptr[9] == 'N' && ptr[8] == ptr[10])
    {
      output += "(anonymous namespace)";
      skip(length);
    }
    else
      while(length--)
      {
	if (current() == 0)
	  FAILURE;
	output += eat_current();
      }
    RETURN;
  }

  // <unqualified-name> ::= <operator-name>				# Starts with lower case
  //                    ::= <ctor-dtor-name>  				# Starts with 'C' or 'D'
  //                    ::= <source-name>   				# Starts with a digit
  //
  // {anonymous}::
  bool demangler_ct::decode_unqualified_name(internal_string& output)
  {
    DoutEntering("decode_unqualified_name");
    if (isdigit(current()))
    {
      if (!M_inside_template_args)
      {
	M_function_name.clear();
	M_name_is_template = false;
	M_name_is_cdtor = false;
	M_name_is_conversion_operator = false;
	if (!decode_source_name(M_function_name))
	  FAILURE;
	output += M_function_name;
      }
      else if (!decode_source_name(output))
        FAILURE;
      RETURN;
    }
    if (islower(current()))
    {
      if (!M_inside_template_args)
      {
	M_function_name.clear();
	M_name_is_template = false;
	M_name_is_cdtor = false;
	M_name_is_conversion_operator = false;
        if (!decode_operator_name(M_function_name))
	  FAILURE;
	output += M_function_name;
      }
      RETURN;
    }
    if (current() == 'C' || current() == 'D')
    {
      if (M_inside_template_args)
        FAILURE;
      // <ctor-dtor-name> ::= C1				# complete object (in-charge) constructor
      //                  ::= C2				# base object (not-in-charge) constructor
      //                  ::= C3				# complete object (in-charge) allocating constructor
      //                  ::= D0				# deleting (in-charge) destructor
      //                  ::= D1				# complete object (in-charge) destructor
      //                  ::= D2				# base object (not-in-charge) destructor
      //
      // {anonymous}::
      if (current() == 'C')
      {
	char c = next();
	if (c < '1' || c > '3')
	  FAILURE;
      }
      else
      {
	char c = next();
	if (c < '0' || c > '2')
	  FAILURE;
	output += '~';
	M_saw_destructor = true;
      }
      M_name_is_cdtor = true;
      eat_current();
      output += M_function_name;
      RETURN;
    }
    FAILURE;
  }

  // <unscoped-name> ::= <unqualified-name>		# Starts not with an 'S'
  //                 ::= St <unqualified-name>		# ::std::
  //
  // {anonymous}::
  bool demangler_ct::decode_unscoped_name(internal_string& output)
  {
    DoutEntering("decode_unscoped_name");
    if (current() == 'S')
    {
      if (next() != 't')
        FAILURE;
      eat_current();
      output += "std::";
    }
    decode_unqualified_name(output);
    RETURN;
  }

  // <name> ::= <nested-name>						# Starts with 'N'
  //        ::= <unscoped-name>						# Starts with 'S', 'C', 'D', a digit or a lower case character.
  //        ::= <unscoped-template-name> <template-args>		# idem
  //        ::= <local-name>						# Starts with 'Z'
  //
  // <unscoped-template-name> ::= <unscoped-name>
  //                          ::= <substitution>
  // {anonymous}::
  bool demangler_ct::decode_name(internal_string& output, internal_string& nested_name_qualifiers)
  {
    DoutEntering("decode_name");
    int substitution_start = M_pos;
    if (current() == 'S' && M_str[M_pos + 1] != 't')
    {
      if (!decode_substitution(output))
        FAILURE;
    }
    else if (current() == 'N')
    {
      decode_nested_name(output, nested_name_qualifiers);
      RETURN;
    }
    else if (current() == 'Z')
    {
      decode_local_name(output);
      RETURN;
    }
    else if (!decode_unscoped_name(output))
      FAILURE;
    if (current() == 'I')
    {
      // Must have been an <unscoped-template-name>.
      add_substitution(substitution_start, unscoped_template_name);
      if (!decode_template_args(output))
	FAILURE;
    }
    M_template_args_need_space = false;
    RETURN;
  }

  // <call-offset> ::= h <nv-offset> _
  //               ::= v <v-offset> _
  // <nv-offset>   ::= <offset number>				 # non-virtual base override
  // <v-offset>    ::= <offset number> _ <virtual offset number> # virtual base override, with vcall offset
  //
  // {anonymous}::
  bool demangler_ct::decode_call_offset(internal_string& output)
  {
    DoutEntering("decode_call_offset");
    if (current() == 'h')
    {
      internal_string dummy;
      eat_current();
      if (decode_number(dummy) && current() == '_')
      {
	eat_current();
        RETURN;
      }
    }
    else if (current() == 'v')
    {
      internal_string dummy;
      eat_current();
      if (decode_number(dummy) && current() == '_')
      {
        eat_current();
	if (decode_number(dummy) && current() == '_')
	{
	  eat_current();
	  RETURN;
	}
      }
    }
    FAILURE;
  }

  //
  // <special-name> ::= TV <type>				# virtual table
  //                ::= TT <type>				# VTT structure (construction vtable index)
  //                ::= TI <type>				# typeinfo structure
  //                ::= TS <type>				# typeinfo name (null-terminated byte string)
  //                ::= GV <object name>			# Guard variable for one-time initialization of static objects in a local scope
  //                ::= T <call-offset> <base encoding>		# base is the nominal target function of thunk
  //                ::= Tc <call-offset> <call-offset> <base encoding> # base is the nominal target function of thunk
  //								# first call-offset is 'this' adjustment
  //								# second call-offset is result adjustment
  //
  // {anonymous}::
  inline bool demangler_ct::decode_special_name(internal_string& output)
  {
    DoutEntering("decode_special_name");
    if (current() == 'G')
    {
      if (next() != 'V')
        FAILURE;
      output += "guard variable for ";
      internal_string nested_name_qualifiers;
      eat_current();
      if (!decode_name(output, nested_name_qualifiers))
        FAILURE;
      output += nested_name_qualifiers;
      RETURN;
    }
    else if (current() != 'T')
      FAILURE;
    switch(next())
    {
      case 'V':
        output += "vtable for ";
	eat_current();
	decode_type(output);
        RETURN;
      case 'T':
        output += "VTT for ";
	eat_current();
	decode_type(output);
        RETURN;
      case 'I':
        output += "typeinfo for ";
	eat_current();
	decode_type(output);
        RETURN;
      case 'S':
        output += "typeinfo name for ";
	eat_current();
	decode_type(output);
        RETURN;
      case 'c':
        output += "covariant return thunk to ";
	if (!decode_call_offset(output)
	    || !decode_call_offset(output)
	    || (M_pos += decode_encoding(M_str + M_pos, output)) < 0)
	  FAILURE;
        RETURN;
      case 'C':		// GNU extension?
      {
        internal_string first;
        output += "construction vtable for ";
	eat_current();
	if (!decode_type(first))
	  FAILURE;
        while(isdigit(current()))
	  eat_current();
	if (eat_current() != '_')
	  FAILURE;
	if (!decode_type(output))
	  FAILURE;
	output += "-in-";
	output += first;
        RETURN;
      }
      default:
        if (current() == 'v')
	  output += "virtual thunk to ";
	else
	  output += "non-virtual thunk to ";
	if (!decode_call_offset(output) || (M_pos += decode_encoding(M_str + M_pos, output)) < 0)
	  FAILURE;
        RETURN;
    }
  }

  //
  // <encoding> ::= <function name> <bare-function-type>	# Starts with 'C', 'D', 'N', 'S', a digit or a lower case character.
  //            ::= <data name>					# idem
  //            ::= <special-name>				# Starts with 'T' or 'G'.
  //
  // {anonymous}::
  int demangler_ct::decode_encoding(char const* in, internal_string& output)
  {
    Dout(dc::demangler, "Entering decode_encoding(\"" << in << "\", \"" << output << "\")");
    demangler_ct demangler(in);
    internal_string nested_name_qualifiers;
    int saved_pos;
    demangler.store(saved_pos);
    if (demangler.decode_special_name(output))
      return demangler.M_pos;
    demangler.restore(saved_pos);
    internal_string name;
    if (!demangler.decode_name(name, nested_name_qualifiers))
      return INT_MIN;
    if (demangler.current() == 0 || demangler.current() == 'E')
    {
      output += name;
      output += nested_name_qualifiers;
      return demangler.M_pos;
    }
    // Must have been a <function name>.
    if (demangler.M_name_is_template && !(demangler.M_name_is_cdtor || demangler.M_name_is_conversion_operator))
    {
      if (!demangler.decode_type(output))	// Return type of function
	return INT_MIN;
      output += ' ';
    }
    output += name;
    if (!demangler.decode_bare_function_type(output))
      return INT_MIN;
    output += nested_name_qualifiers;
    return demangler.M_pos;
  }

} // namespace

#ifdef STANDALONE
static char const* main_in;
#endif

namespace _private_ {

//
// demangle_symbol
//
// Demangle `input' and append to `output'.
// `input' should be a mangled_function_name as for instance returned
// by `libcw::debug::pc_mangled_function_name'.
//
void demangle_symbol(char const* input, internal_string& output)
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( __libcwd_tsd.internal );
#endif

#ifdef STANDALONE
  if (input != main_in)
    Debug( dc::demangler.off() );
#endif

  Dout(dc::demangler, "Entering demangle_symbol(\"" << input << "\")");

  if (input == NULL)
  {
    output += "(null)";
#ifdef STANDALONE
    if (input != main_in)
      Debug( dc::demangler.on() );
#endif
    return;
  }

  //
  // <mangled-name> ::= _Z <encoding>
  // <mangled-name> ::= _GLOBAL_ _<type>_ _Z <encoding>		// GNU extension, <type> can be I or D.
  //
  bool failure = (input[0] != '_');

  if (!failure)
  {
    if (input[1] == 'G')
    {
      if (!strncmp(input, "_GLOBAL__", 9) && (input[9] == 'D' || input[9] == 'I') && input[10] == '_' &&
	  input[11] == '_' && input[12] == 'Z')
      {
	if (input[9] == 'D')
	  output.assign("global destructors keyed to ", 28);
	else
	  output.assign("global constructors keyed to ", 29);
	int pos = demangler_ct::decode_encoding(input + 13, output) + 13;
	if (pos < 0 || input[pos] != 0)
	  failure = true;
      }
      else
	failure = true;
    }
    else if (input[1] == 'Z')
    {
      int pos = demangler_ct::decode_encoding(input + 2, output) + 2;
      if (pos < 0 || input[pos] != 0)
	failure = true;
    }
    else
      failure = true;
  }

  if (failure)
    output.assign(input, strlen(input));	// Failure to demangle, return the mangled name.

#ifdef STANDALONE
  if (input != main_in)
    Debug( dc::demangler.on() );
#endif
}

//
// demangle_type
//
// Demangle `input' and append to `output'.
// `input' should be a mangled type as for returned
// by the stdc++ `typeinfo::name()'.
//
void demangle_type(char const* in, internal_string& output)
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( __libcwd_tsd.internal );
#endif
#ifdef STANDALONE
  if (in != main_in)
    Debug( dc::demangler.off() );
#endif
  Dout(dc::demangler, "Entering demangle_type(\"" << in << "\")");
  if (in == NULL)
  {
    output += "(null)";
#ifdef STANDALONE
    if (in != main_in)
      Debug( dc::demangler.on() );
#endif
    return;
  }
  demangler_ct demangler(in);
  if (!demangler.decode_type(output))
    output.assign(in, strlen(in));
#ifdef STANDALONE
  if (in != main_in)
    Debug( dc::demangler.on() );
#endif
}

    } // namespace _private_
  } // namespace debug
} // namespace libcw

#ifdef STANDALONE
#include <iostream>
 
int main(int argc, char* argv[])
{
  Debug( libcw_do.on() );
  Debug( dc::demangler.on() );
  std::string out;
  libcw::debug::main_in = argv[1];
  libcw::debug::demangle_symbol(argv[1], out);
  std::cout << out << std::endl;
  return 0;
}
 
#endif // STANDALONE

#endif // __GXX_ABI_VERSION > 0

namespace libcw {
  namespace debug {
    namespace _private_ {

extern void demangle_symbol(char const* input, _private_::internal_string& output);
extern void demangle_type(char const* input, _private_::internal_string& output);

    } // namespace _private_

/** \addtogroup group_demangle */
/** \{ */

/**
 * \brief Demangle mangled symbol name \p input and write the result to string \p output.
 */
void demangle_symbol(char const* input, std::string& output)
{
  LIBCWD_TSD_DECLARATION;
  _private_::set_alloc_checking_off(LIBCWD_TSD);
  {
    _private_::internal_string result;
    _private_::demangle_symbol(input, result);
    _private_::set_alloc_checking_on(LIBCWD_TSD);
    output.append(result.data(), result.size());
    _private_::set_alloc_checking_off(LIBCWD_TSD);
  }
  _private_::set_alloc_checking_on(LIBCWD_TSD);
}

/**
 * \brief Demangle mangled type name \p input and write the result to string \p output.
 */
void demangle_type(char const* input, std::string& output)
{
  LIBCWD_TSD_DECLARATION;
  _private_::set_alloc_checking_off(LIBCWD_TSD);
  {
    _private_::internal_string result;
    _private_::demangle_type(input, result);
    _private_::set_alloc_checking_on(LIBCWD_TSD);
    output.append(result.data(), result.size());
    _private_::set_alloc_checking_off(LIBCWD_TSD);
  }
  _private_::set_alloc_checking_on(LIBCWD_TSD);
}

/** \} */

  } // namespace debug
} // namespace libcw

