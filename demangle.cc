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

//
// void demangle_type(char const* in, string& out);
//
// where, `in' is a mangled type name as returned by typeid(OBJECT).name().
// When `in' is NULL then the string "(null)" is returned.
//
// void demangle_symbol(char const* in, string& out);
//
// where, `in' is a mangled symbol name as returned by asymbol::name (where asymbol is defined by libbfd),
// which is the same as `location_st::func'.  When `in' is NULL then the string "(null)" is returned.
//
// Currently this file has been tested with gcc-2.95.2 and gcc-2.96 (RedHat).
//
// For a standalone demangler, compile this file as:
// g++ -pthread -DSTANDALONE -DCWDEBUG -Iinclude demangle.cc -Wl,--rpath,`pwd`/.libs -L.libs -lcwd -o c++filt
//

#undef CPPFILTCOMPATIBLE

#include "sys.h"

#if __GXX_ABI_VERSION == 0

#include <cctype>
#include "cwd_debug.h"
#include <libcwd/demangle.h>

#ifdef HAVE_LIMITS
#include <limits>
#else
#include <limits.h>
#ifndef LONG_LONG_MIN
#define LONG_LONG_MIN (1LL << (8 * sizeof(long long) - 1))
#endif
#ifndef LONG_LONG_MAX
#define LONG_LONG_MAX (-(LONG_LONG_MIN + 1LL))
#endif
#ifndef ULONG_LONG_MAX
#define ULONG_LONG_MAX ((unsigned long long)-1)
#endif
namespace libcw {
  namespace debug {
    template<typename T>
      struct numeric_limits {
	static const bool is_signed = false;
	static T min() { return 0; }
	static T max() { return 0; }
      };
    template<>
      struct numeric_limits<bool> {
	static const bool is_signed = false;
	static bool min() { return 0; }
	static bool max() { return 0; }
      };
    template<>
      struct numeric_limits<char> {
	static const bool is_signed = (CHAR_MIN < 0);
	static char min() { return CHAR_MIN; }
	static char max() { return CHAR_MAX; }
      };
    template<>
      struct numeric_limits<unsigned char> {
	static const bool is_signed = false;
	static unsigned char min() { return 0; }
	static unsigned char max() { return UCHAR_MAX; }
      };
    template<>
      struct numeric_limits<signed char> {
	static const bool is_signed = true;
	static signed char min() { return SCHAR_MIN; }
	static signed char max() { return SCHAR_MAX; }
      };
    template<>
      struct numeric_limits<short> {
	static const bool is_signed = true;
	static short min() { return SHRT_MIN; }
	static short max() { return SHRT_MAX; }
      };
    template<>
      struct numeric_limits<unsigned short> {
	static const bool is_signed = false;
	static unsigned short min() { return 0; }
	static unsigned short max() { return USHRT_MAX; }
      };
    template<>
      struct numeric_limits<int> {
	static const bool is_signed = true;
	static int min() { return INT_MIN; }
	static int max() { return INT_MAX; }
      };
    template<>
      struct numeric_limits<unsigned int> {
	static const bool is_signed = false;
	static unsigned int min() { return 0; }
	static unsigned int max() { return UINT_MAX; }
      };
    template<>
      struct numeric_limits<long> {
	static const bool is_signed = true;
	static long min() { return LONG_MIN; }
	static long max() { return LONG_MAX; }
      };
    template<>
      struct numeric_limits<unsigned long> {
	static const bool is_signed = false;
	static unsigned long min() { return 0; }
	static unsigned long max() { return ULONG_MAX; }
      };
    template<>
      struct numeric_limits<long long> {
	static const bool is_signed = true;
	static long long min() { return LONG_LONG_MIN; }
	static long long max() { return LONG_LONG_MAX; }
      };
    template<>
      struct numeric_limits<unsigned long long> {
	static const bool is_signed = false;
	static unsigned long long min() { return 0; }
	static unsigned long long max() { return ULONG_LONG_MAX; }
      };
    template<>
      struct numeric_limits<wchar_t> {
	static const bool is_signed = true;
	static wchar_t min() { return (1 << (8 * sizeof(wchar_t) - 1)); }
	static wchar_t max() { return -((1 << (8 * sizeof(wchar_t) - 1)) + 1); }
      };
  } // namespace debug
} // namespace libcw
#endif	// HAVE_LIMITS

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
#else // !STANDALONE
#undef CWDEBUG
#undef Dout
#undef DoutFatal
#undef Debug
#define Dout(cntrl, data)
#define DoutFatal(cntrl, data) LibcwDoutFatal(::std, /*nothing*/, cntrl, data)
#define Debug(x)
#endif // !STANDALONE

namespace libcw {
  namespace debug {

    namespace {

      using _private_::internal_string;
      using _private_::internal_vector;

      // {anonymous}::
      enum cdtor_nt {
	constructor,
	destructor,
	normal_symbol
      };

      // {anonymous}::
      struct current_st {
	internal_vector<char const*> template_parameters;
	internal_vector<char const*> previous_types;
      };

      // {anonymous} prototypes.
      bool symbol_type(current_st&, char const*, cdtor_nt, internal_string&, internal_string&);
      internal_string symbol_name(char const*, size_t);
      bool eat_template_type(current_st&, char const*&, internal_string&, internal_string*);
      bool eat_scope_type(current_st&, char const*&, internal_string&, internal_string&);
      bool eat_type(current_st&, char const*&, internal_string&);
      int eat_digits(char const*& input);
      bool eat_type_internal(current_st&, char const*&, internal_string&, internal_string&, internal_string*, bool = false);

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
#ifdef STANDALONE
      if (input != main_in)
	Debug( dc::demangler.off() );
#endif
      Dout(dc::demangler, "Entering demangle_symbol(\"" << input << "\", internal_string& output)");

      if (input == NULL)
      {
	output += "(null)";
#ifdef STANDALONE
	if (input != main_in)
	  Debug( dc::demangler.on() );
#endif
	return;
      }

      // `input' is of the form:
      //
      // <symbol>				--> <symbol>
      // <symbol>__<types>			--> <symbol>(<type1>, <type2>, ..., <typeN>)
      // <symbol>__[C]<scope-type>		--> <scope-type>::<symbol>(void) [const]
      // <symbol>__[C]<scope-type><types>	--> <scope-type>::<symbol>(<type1>, <type2>, ..., <typeN>) [const]
      // __<scope-type><types>			--> <scope-type>::<class-name>(<type1>, <type2>, ..., <typeN>)
      // __<scope-type>				--> <scope-type>::<class-name>(void)
      // _._<scope-type>			--> <scope-type>::~<class-name>()
      // _<scope-type>.<symbol>			--> <scope-type>::<symbol>
      // __vt_<scope-type>			--> <scope-type> virtual table
      // __vt_<scope-type>.symbol		--> <scope-type>::<symbol> virtual table
      // __ti<type>				--> <type> type_info node
      // __tf<type>				--> <type> type_info function
      //
      // <symbol> may contain "__", only the first "__" that is followed by a
      // valid <symbol-type> is interpreted as the start of that <symbol-type>.
      //
      // <symbol-type> exists of a part that is prepended to <symbol> and
      // a part that is appended to <symbol>.
      //
      // If <symbol> is empty, then it is a constructor.  If `input' starts
      // with "_._" then it is a destructor.  In both cases only a sub set
      // of <symbol-type> is allowed (only the ones that start with a <scope-type>).

      size_t symbol_len = 0;
      char const* symbol = input;
      internal_string prefix;
      internal_string postfix;
      current_st current;
      char const* p = input;

      if (p[0] == '_' && p[1] == 'G' && !strncmp(p, "_GLOBAL_.", 9) && (p[9] == 'D' || p[9] == 'I') && p[10] == '.')
      {
	if (p[9] == 'D')
	  prefix = "global destructors keyed to ";
	else
	  prefix = "global constructors keyed to ";
	p += 11;
	symbol += 11;
      }
      if (p[0] == '_' && p[1] == '_' && (p[2] == 't' || p[2] == 'v'))
      {
	char const* q = p + 5;
	if (p[2] == 'v' && p[3] == 't' && p[4] == '_' && !eat_type(current, q, prefix))
	{
	  if (*q == '.')
	  {
	    size_t original_length = prefix.size();
	    ++q;
	    prefix += "::";
	    if (eat_type(current, q, prefix) || *q != 0)
	    {
	      --q;
	      prefix.erase(original_length);
	    }
	  }
	  if (*q == 0)
	  {
	    output += prefix;
	    output += " virtual table";
#ifdef STANDALONE
	    if (input != main_in)
	      Debug( dc::demangler.on() );
#endif
	    return;
	  }
	}
	--q;
	if (p[2] == 't' && (p[3] == 'i' || p[3] == 'f') && !eat_type(current, q, prefix) && *q == 0)
	{
	  output += prefix;
	  if (p[3] == 'i')
	    output += " type_info node";
	  else
	    output += " type_info function";
#ifdef STANDALONE
	  if (input != main_in)
	    Debug( dc::demangler.on() );
#endif
	  return;
	}
	char const* p2 = &p[8];
	if (p[2] == 't' && !strncmp(p, "__thunk_", 8) && eat_digits(p2) >= 0 && *p2 == '_')
	{
	  prefix += "virtual function thunk (delta:-";
	  prefix += internal_string(p + 8, p2 - p - 8);
	  prefix += ") for ";
	  p = p2 + 1;
	  symbol = p;
	}
      }
      if (p[0] != '_'
	  || (p[1] != '_' && (p[1] != '.' || p[2] != '_'))
	  || symbol_type(current, p + (p[1] == '_' ? 2 : 3), p[1] == '_' ? constructor : destructor, prefix, postfix))
      {
	bool double_underscore = false;
	do
	{
	  ++p;
	  ++symbol_len;

	  // Continue until we found the next "__" in the input internal_string.
	  if (p[0] == '_' && p[1] == '_')
	  {
	    double_underscore = true;
	    // If the rest is a valid <symbol-type> (or an error occured) then break out of the loop.
	    if (!symbol_type(current, p + 2, normal_symbol, prefix, postfix))
	      break;
	  }
	}
	while (*p);

	// If this failed then try _<scope-type>.<symbol>, otherwise reset prefix and symbol when we saw at least one "__".
	if (!*p)
	{
	  size_t original_size = prefix.size();
	  p = symbol + 1;
	  if (*symbol == '_' && !eat_type(current, p, prefix) && *p == '.')
	  {
	    symbol_len -= (p + 1 - symbol);
	    symbol = p + 1;
	    prefix += "::";
	  }
	  else if (double_underscore)
	  {
	    symbol_len += (symbol - input);
	    symbol = input;
	    prefix.erase();
	  }
	  else
	    prefix.erase(original_size);
	}
      }

      // Write the result out, translate the symbol internal_string if needed.

      Dout(dc::demangler, "symbol_len = " << symbol_len);
      Dout(dc::demangler, "prefix = \"" << prefix << '"');
      Dout(dc::demangler, "symbol = \"" << internal_string(symbol, symbol_len) << '"');
      Dout(dc::demangler, "postfix = \"" << postfix << '"');

      output += prefix;
      output += symbol_name(symbol, symbol_len);

      // Put a space between operator<< or operator< and their template parameter list.
      if (symbol[0] == '_' && symbol_len == 4 && symbol[1] == '_'
	  && symbol[2] == 'l' && (symbol[3] == 's' || symbol[3] == 't')
	  && postfix.size() > 0 && postfix[0] == '<')
	output += ' ';

      output += postfix;
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
    void demangle_type(char const* input, internal_string& output)
    {
#ifdef STANDALONE
      if (input != main_in)
	Debug( dc::demangler.off() );
#endif
      Dout(dc::demangler, "Entering demangle_type(\"" << input << "\", internal_string& output)");

      if (input == NULL)
      {
	output += "(null)";
#ifdef STANDALONE
	if (input != main_in)
	  Debug( dc::demangler.on() );
#endif
	return;
      }

      current_st current;
      char const* in = input;
      if (eat_type(current, in, output))
	output.assign(input, strlen(input));

#ifdef STANDALONE
      if (input != main_in)
	Debug( dc::demangler.on() );
#endif
    }

} // namespace _private_

    namespace {
      // Implementation of local functions

      // Returns the length of a mangled internal_string or the number of template parameters by reading the digits from the input internal_string.
      // {anonymous}::
      int eat_digits(char const*& input)
      {
	Dout(dc::demangler, "Entering eat_digits(\"" << input << "\")");
	// Skip a possible seperator.
	if (*input == '_')
	{
	  // If a previous <type> did end on a digit then that was seperated from this with an underscore.
	  if (!isdigit(input[-1]) || !isdigit(input[1]))
	    return -1;
	  ++input;
	}
	else if (!isdigit(*input))
	  return -1;
	int len = *input - '0';
	while (isdigit(*++input))
	  len = 10 * len + *input - '0';
	return len;
      }

      // {anonymous}::
      bool symbol_type(current_st& current, char const* input, cdtor_nt cdtor, internal_string& prefix, internal_string& postfix)
      {
	Dout(dc::demangler, "Entering symbol_type(\"" << input << "\", internal_string& prefix, internal_string& postfix)");

	// `input' is of the form:
	//
	// when `cdtor' is `normal_symbol':
	//
	// <scope-type>				-> <scope-type>::SYMBOL(void)
	// <scope-type><types>			-> <scope-type>::SYMBOL(<type1>, <type2>, ..., <typeN>)
	// <nonscope-type>[<types>]			-> SYMBOL(<nonscope-type>[, <type1>, <type2>, ..., <typeN>])
	// F<types>					-> SYMBOL(<type1>, <type2>, ..., <typeN>)
	// H<digits><template-types>_[C]<types>[_<type>]	-> [<type> ]SYMBOL<<ttype1>, <ttype2>, ..., <ttypeN>[ ]>(<type1>, <type2>, ..., <typeN>) [const]
	//
	// if any of the above is prepended with a 'C', then the postfix is appended " const".
	// Note that a pointer and reference are non-scope types, so none of the above except <nonscope-type>
	// can start with a 'P' or 'R'.
	//
	// when `cdtor' is `constructor':
	//
	// <scope-type>				-> <scope-type>::<class-name>(void)
	// <scope-type><types>			-> <scope-type>::<class-name>(<type1>, <type2>, ..., <typeN>)
	// 
	// when `cdtor' is `destructor':
	//
	// <scope-type>				-> <scope-type>::~<class-name>()
	// <scope-type><types>			-> <scope-type>::~<class-name>(<type1>, <type2>, ..., <typeN>)
	// 
	// where <class-name> is the last qualifier of <scope-type>.

	bool is_const = (*input == 'C');
	bool is_template_function = (*input == 'H');

	if (is_const)
	  ++input;

	if (*input == 'H' || *input == 'F')
	{
	  if (cdtor != normal_symbol)
	    return true;

	  ++input;

	  if (is_template_function)
	  {
	    // Process <digits><template-types>

	    int number_of_template_parameters = eat_digits(input);
	    if (number_of_template_parameters <= 0)
	      return true;

	    postfix += '<';

	    internal_string template_type;
	    for (int count = number_of_template_parameters; count > 0; --count)
	    {
	      current.template_parameters.push_back(input);
	      template_type.erase();
	      if (eat_template_type(current, input, template_type, NULL))
	      {
		postfix.erase();
		return true;
	      }
	      postfix += template_type;
	      if (count > 1)
		postfix += ", ";
	    }

	    // The next must be an '_', eat it.
	    if (*input != '_')
	    {
	      postfix.erase();
	      return true;
	    }
	    ++input;

	    if (template_type[template_type.size() - 1] == '>')
	      postfix += ' ';

	    postfix += '>';
	  }
	}

	if (*input == 'C')
	{
	  is_const = true;
	  ++input;
	}

	if (*input == 0)
	{
	  postfix.erase();
	  return true;
	}

	// Process <scope-type>, if any.
	internal_string last_class_name;
	internal_string scope_type;
	char const* input_store = input;
	if (!eat_scope_type(current, input, scope_type, last_class_name))
	{
	  current.previous_types.push_back(input_store);
	  scope_type += "::";
	  switch(cdtor)
	  {
	    case constructor:
	      postfix += last_class_name;
	      break;
	    case normal_symbol:
	      break;
	    case destructor:
	      if (*input != 0)
	      {
		postfix.erase();
		return true;
	      }
	      prefix += scope_type + '~' + last_class_name +
#ifdef CPPFILTCOMPATIBLE
		  "(void)";
#else
		  "()";
#endif
	      if (is_const)
		prefix += " const";
	      return false;
	  }
	}
	else if (cdtor != normal_symbol)
	{
	  postfix.erase();
	  return true;
	}
	else
	  input = input_store;

	// Must be a <nonscope-type> then.
	// Process <types>

	postfix += '(';

	// There must be at least one <type>.
	if (*input == 0)
	  postfix += "void";
	else
	{
	  current.previous_types.push_back(input);
	  if (eat_type(current, input, postfix))
	  {
	    postfix.erase();
	    return true;
	  }

	  // Eat optional other <type>s.
	  while (*input && *input != '_')
	  {
	    postfix += ", ";
	    current.previous_types.push_back(input);
	    if (eat_type(current, input, postfix))
	    {
	      postfix.erase();
	      return true;
	    }
	  }
	}

	postfix += ')';

	// Process [_<type>], if any.
	if (cdtor == normal_symbol && is_template_function && *input == '_')
	{
	  ++input;
	  if (eat_type(current, input, prefix))
	  {
	    postfix.erase();
	    prefix.erase();
	    return true;
	  }
	  prefix += ' ';
	}

	prefix += scope_type;

	// We must have reached the end of the internal_string now
	if (*input != 0)
	{
	  postfix.erase();
	  prefix.erase();
	  return true;
	}

	if (is_const)
	  postfix += " const";

	return false;
      }

      //
      // Symbol operator codes exist of two characters, we need to find a
      // quick hash so that their names can be looked up in a table.
      //
      // The puzzle :)
      // Shift the rows so that there is at most one character per column.
      //
      // A near perfect solution:
      //                                              horizontal
      //    .....................................     offset + 'a'
      // a, |||a||d||||||||||||||s                      3
      // c, ||||||||||||||||||||||||||||||||||||lmno   25
      // d, ||||l|||||||||v                            -7
      // e, |||||||||||||||||||||||||||||||||qr        17
      // g, ||e||||||||||||||t                         -2
      // l, |||||||||||e|||||||||||||st                 7
      // m, d||||i||lmn|||||||||x                      -3
      // n, |e||||||||||||||t||w                       -3
      // o, |||||||||||||||||||||||||||||o||r          15
      // p, |||||||||||||||||||||||||||l|||p|||t       16
      // r, |||||||||||||||f||||||m|||||s              10
      // s, ||||||||||||||||||||||||z                  -1
      // v, ||||||||||||cd|||||||||n                   10
      //    .....................................
      // ^            ^__ second character
      // |___ first character
      //

      // Putting that solution in tables:

      // {anonymous}::
      char offset_table[1 + CHAR_MAX - CHAR_MIN /* ascii value of first character */ ] = {
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
	0, -94,   0, -72,-104, -80,   0, -99,   0,   0,   0,   0, -90,-100,-100, -82, -81,   0, -87, -98,   0,   0, -87,
#else
	0, 162,   0, 184, 152, 176,   0, 157,   0,   0,   0,   0, 166, 156, 156, 174, 175,   0, 169, 158,   0,   0, 169,
#endif
	// ... more zeros
      };

      // {anonymous}::
      struct entry_st { char const* opcode; char const* symbol_name; } symbol_name_table[40] = {
	{ "md=", "operator%" },
	{ "ne",  "operator!=" },
	{ "ge",  "operator>=" },
	{ "aa",  "operator&&" },
	{ "dl",  "operator delete" },
	{ "mi=", "operator-" },
	{ "ad=", "operator&" },
	{   "",   NULL },
	{ "ml=", "operator*" },
	{ "mm",  "operator--" },
	{ "mn",  "operator<?" },
	{ "le",  "operator<=" },
	{ "vc",  "operator[]" },
	{ "vd",  "operator delete []" },
	{ "dv=", "operator/" },
	{ "rf",  "operator->" },
	{ "nt",  "operator!" },
	{ "gt",  "operator>" },
	{   "",   NULL },
	{ "nw",  "operator new" },
	{ "mx",  "operator>?" },
	{ "as",  "operator=" },
	{ "rm",  "operator->*" },
	{ "vn",  "operator new []" },
	{ "sz",  "sizeof" },
	{ "ls=", "operator<<" },
	{ "lt",  "operator<" },
	{ "pl=", "operator+" },
	{ "rs=", "operator>>" },
	{ "oo",  "operator||" },
	{   "",   NULL },
	{ "pp",  "operator++" },
	{ "or=", "operator|" },
	{ "eq",  "operator==" },
	{ "er=", "operator^" },
	{ "pt",  "operator->" },
	{ "cl",  "operator()" },
	{ "cm",  "operator," },
	{ "cn",  "operator?:" },
	{ "co",  "operator~" }
      };

      // {anonymous}::
      internal_string symbol_name(char const* symbol, size_t symbol_len)
      {
	// Only consider symbols of the form /__../ and /__a../.
	if (symbol_len > 3 && symbol[0] == '_' && symbol[1] == '_' && symbol_len < 6 && (symbol_len == 4 || symbol[2] == 'a'))
	{
	  char const* opcode = symbol + 2;

	  // Skip the 'a' in /__a../.
	  if (symbol_len == 5)
	    ++opcode;

	  register char hash;
	  if ((hash = offset_table[*opcode - CHAR_MIN]))
	  {
	    hash += opcode[1];
	    if (
#if (CHAR_MIN<0)
		hash >= 0 &&
#endif
		hash < 40)
	    {
	      entry_st entry = symbol_name_table[hash];
	      if (entry.opcode[0] == opcode[0] && entry.opcode[1] == opcode[1] && (symbol_len == 4 || entry.opcode[2] == '='))
	      {
		internal_string operator_name(entry.symbol_name);
		if (symbol_len == 5)
		  operator_name += '=';
		return operator_name;
	      }
	    }
	  }
	}
	return internal_string(symbol, symbol_len);
      }

      // {anonymous}::
      template<class INTEGRAL_TYPE>
	bool eat_integral_type(char const*& input, internal_string& output)
	{
	  Dout(dc::demangler, "Entering eat_integral_type<" << type_info_of(INTEGRAL_TYPE(0)).demangled_name() << ">(\"" << input << "\", internal_string& output)");

	  // `input' is of the form
	  //
	  // [m]<digit>[<digit>...]
	  // _[m]<digit>[<digit>...]_
	  //

	  bool underscore = (*input == '_');
	  if (underscore)
	    ++input;
	  bool is_negative = (*input == 'm' && numeric_limits<INTEGRAL_TYPE>::is_signed);
	  if (is_negative)
	  {
	    ++input;
	    output += '-';
	  }

	  if (!isdigit(*input))
	    return true;

	  INTEGRAL_TYPE const pre_max = numeric_limits<INTEGRAL_TYPE>::max() / 10;
	  INTEGRAL_TYPE const pre_min = numeric_limits<INTEGRAL_TYPE>::min() / 10;
	  char const pre_max_last_digit = '0' + numeric_limits<INTEGRAL_TYPE>::max() % 10;
	  char const pre_min_last_digit = '0' - numeric_limits<INTEGRAL_TYPE>::min() % 10;

	  INTEGRAL_TYPE x = 0;

	  if (is_negative)
	    do
	    {
	      if (x < pre_min || (x == pre_min && *input > pre_min_last_digit))
		return true;
	      x = x * 10 - (*input - '0');
	      output += *input;
	    }
	    while (isdigit(*++input));
	  else
	    do
	    {
	      if (x > pre_max || (x == pre_max && *input > pre_max_last_digit))
		return true;
	      x = x * 10 + (*input - '0');
	      output += *input;
	    }
	    while (isdigit(*++input));

	  if (underscore && *input == '_')
	    ++input;

#ifndef CPPFILTCOMPATIBLE
	  if (!numeric_limits<INTEGRAL_TYPE>::is_signed)
	    output += 'U';
	  if (sizeof(INTEGRAL_TYPE) > sizeof(int))
	    output += 'L';
#endif

	  return false;
	}

      // {anonymous}::
      bool eat_float_type(char const*& input, internal_string& output)
      {
	Dout(dc::demangler, "Entering eat_float_type(\"" << input << "\", internal_string& output)");
	if (*input == 'm')
	{
	  ++input;
	  output += '-';
	}
	if (!isdigit(*input))
	  return true;
	output += *input++;
	if (*input != '.')
	  return true;
	output += *input++;
	while(isdigit(*input))
	  output += *input++;
	if (*input != 'e')
	  return true;
	output += *input++;
	if (*input == 'm')
	{
	  ++input;
	  output += '-';
	}
	while(isdigit(*input))
	  output += *input++;
	return false;
      }

      // {anonymous}::
      bool eat_template_type(current_st& current, char const*& input, internal_string& template_type, internal_string* last_class_name)
      {
	Dout(dc::demangler, "Entering eat_template_type(\"" << input << "\", internal_string& template_type, " << (last_class_name ? "\", internal_string* last_class_name" : "\", NULL") << ')');
	// `input' is of the form
	//
	// Z<type>
	// <integral-type><value>
	//
	// where <integral-type> is one of:
	//
	//                            <value>
	// b	boolean			0 (false) or 1 (true)
	// f  float			<float-type>
	// d  double			<float-type>
	// r  long double		<float-type>
	// w  wchar_t			<integral-type>
	// Sc signed char		<integral-type>
	// Uc unsigned char		<integral-type>
	// c  char			<integral-type>
	// Us unsigned short		<integral-type>
	// s  short			<integral-type>
	// Ui unsigned int		<integral-type>
	// i  int			<integral-type>
	// Ul unsigned long		<integral-type>
	// l  long			<integral-type>
	// Ux unsigned long long	<integral-type>
	// x  long long		<integral-type>
	// PF<types>_<type><digits><symbol>	--> &<symbol>
	//

	if (*input == 'Z')
	{
	  ++input;
	  // Process Z<type>
	  internal_string postfix;
	  if (last_class_name && *input == 't')
	    return true;	// Don't ask me why, but this is not a scope_type when it is a template parameter.
	  if (eat_type_internal(current, input, template_type, postfix, last_class_name))
	    return true;
	  template_type += postfix;
	}
	else if (!last_class_name)
	{
	  // Process <type><value>
	  switch (*input)
	  {
	    case 'b':
	      if (input[1] == '0')
		template_type = "false";
	      else if (input[1] == '1')
		template_type = "true";
	      else
		return true;
	      input += 2;
	      break;
	    case 'f':
	    case 'd':
	      return eat_float_type(input, template_type);
	    case 'r':
	      if (eat_float_type(input, template_type))
		return true;
	      template_type += 'L';
	      break;
	    case 'w':
	      return eat_integral_type<wchar_t>(++input, template_type);
	    case 'P':
	    {
	      internal_string tmp;
	      if (eat_type(current, input, tmp))
		return true;
	      int len = eat_digits(input);
	      if (len <= 0)
		return true;
	      tmp.assign(input, len);
	      input += len;
	      tmp += '\0';
	      template_type = "&";
	      _private_::demangle_symbol(tmp.data(), template_type);
	      break;
	    }
	    default:
	    {
	      bool is_unsigned = (*input == 'U');
	      if (*input == 'U' || (*input == 'S' && input[1] == 'c'))
		++input;
	      ++input;
	      if (is_unsigned)
		switch (input[-1])
		{
		  case 'c':
		    return eat_integral_type<unsigned char>(input, template_type);
		  case 's':
		    return eat_integral_type<unsigned short>(input, template_type);
		  case 'i':
		    return eat_integral_type<unsigned int>(input, template_type);
		  case 'l':
		    return eat_integral_type<unsigned long>(input, template_type);
		  case 'x':
		    return eat_integral_type<unsigned long long>(input, template_type);
		  default:
		    return true;
		}
	      else
		switch (input[-1])
		{
		  case 'c':
		    if (input[-2] == 'S')
		      return eat_integral_type<signed char>(input, template_type);
		    else
		      return eat_integral_type<char>(input, template_type);
		  case 's':
		    return eat_integral_type<short>(input, template_type);
		  case 'i':
		    return eat_integral_type<int>(input, template_type);
		  case 'l':
		    return eat_integral_type<long>(input, template_type);
		  case 'x':
		    return eat_integral_type<long long>(input, template_type);
		  default:
		    return true;
		}
	    }
	  }
	}
	return false;
      }

      // {anonymous}::
      bool eat_type_internal(current_st& current, char const*& input, internal_string& prefix, internal_string& postfix, internal_string* last_class_name = NULL, bool has_qualifiers = false)
      {
	Dout(dc::demangler, "Entering eat_type_internal(\"" << input << "\", \"" << prefix << "\", \"" << postfix << (last_class_name ? "\", internal_string* last_class_name" : "\", NULL") << ", " << (has_qualifiers ? "true" : "false") << ')');

	// `input' is of the form:
	//
	// non-scope types:
	//							PREFIX					POSTFIX
	// v							void
	// e							...
	// b							bool
	// f							float
	// d							double
	// r							long double
	// w							wchar_t
	// Sc							signed char
	// Uc							unsigned char
	// c							char
	// Us							unsigned short
	// s							short
	// Ui							unsigned
	// i							int
	// Ul							unsigned long
	// l							long
	// Ux							unsigned long long
	// x							long long
	// G<digits><global-name>				<global-name>
	// P<type>						PREFIX*					POSTFIX
	// R<type>						PREFIX&					POSTFIX
	// P[M<scope-type>][C]F<types>_<type>			<type> ([<scope-type>::]*		)(<type>, <type>, ..., <type>) [const]
	// R[M<scope-type>][C]F<types>_<type>			<type> ([<scope-type>::]&		)(<type>, <type>, ..., <type>) [const]
	// [<other-qualifiers>]A<digits>_			[(<other-qualifiers>			)] \[<digits>\]
	// O<scope-type>_					PREFIX <scope-type>::			POSTFIX
	//
	// scope types:
	// <digits><class-name>					<class-name>
	// t<digits><name><digits><ttype><ttype>...<ttype>	<name><<ttype>, <ttype>, ..., <ttype>>
	//
	// possible scope types:
	// C<type>						PREFIX const				POSTFIX
	// Q<number><scope-type><scope-type>...<type>		<scope-type>::<scope-type>::...::<type>
	//
	// Recursive input:
	// X<digit><digit>
	// X_<digits>_<digit>
	// T<digit>
	// T<digits>_
	// N<digit><digit>
	// N_<digits>_<digit>
	//
	// If `last_class_name' is non-NULL then the found type must be a <scope-type> and
	// is also written to `last_class_name'.  If `input' starts with a 'Q' then the
	// found type is only a scope type if the last <type> in the list is a <scope-type>,
	// only that type is written to `last_class_name' in that case.  Same thing when
	// `input' starts with a 'C'.
	// The only other scope types are <digits><class-name> and t<digits><name><digits><ttype><ttype>...<ttype>.
	//
	// If `input' starts with 'X' then this type refers to the last 'H' in the mangled name.
	// The first digit (or digits if it starts with a '_') is the template parameter that is being refered to,
	// the second <digit> seems to be ignored.  If `input' starts with 'T', then this type refers to a previous
	// type that was processed during this call to demangle_symbol.  If `input' starts with 'N', then this type
	// also refers to a previous type that was processed during this call to demangle_symbol, but needs to be
	// repeated as often as the first digit (or digits if it starts with a '_').

	if (!last_class_name)
	{
	  if (*input == 'N')
	  {
	    int count;
	    ++input;
	    if (isdigit(*input))
	      count = *input++ - '0';
	    else if (*input != '_')
	      return true;
	    else if ((count = eat_digits(++input)) < 0)
	      return true;
	    if (!isdigit(*input))
	      return true;
	    int index = eat_digits(input);
	    if (index < 0 || (size_t)(index + 1) >= current.previous_types.size())
	      return true;
	    char const* const previous_type(current.previous_types[index]);
	    for (int i = 0; i < count; ++i)
	    {
	      if (i != 0)
		postfix += ", ";
	      char const* recursive_input = previous_type;
	      current.previous_types.push_back(recursive_input);
	      if (eat_type(current, recursive_input, postfix))
		return true;
	    }
	    return false;
	  }
	  if (*input == 'S')
	  {
	    ++input;
	    if (*input != 'c')
	      return true;
	    prefix += "signed ";
	  }
	  if (*input == 'U')
	  {
	    ++input;
	    if (*input != 'c' && *input != 's' && *input != 'i' && *input != 'l' && *input != 'x')
	      return true;
	    prefix += "unsigned ";
	  }
	  if (*input == 'X')
	  {
	    int index;
	    ++input;
	    if (isdigit(*input))
	      index = *input++ - '0';
	    else if (*input != '_')
	      return true;
	    else if ((index = eat_digits(++input)) < 0)
	      return true;
	    if (!isdigit(*input++))
	      return true;
	    if ((size_t)index >= current.template_parameters.size())
	      return true;
	    char const* recursive_input = current.template_parameters[index];
	    return eat_template_type(current, recursive_input, prefix, last_class_name);
	  }
	  else if (*input == 'T')
	  {
	    ++input;
	    int index = eat_digits(input);
	    if (index < 0 || (index > 9 && *input++ != '_'))
	      return true;
	    if ((size_t)index >= current.previous_types.size())
	    {
	      char buf[32];
	      char* p = &buf[31];
	      *p = 0;
	      do { *--p = '0' + (index % 10); } while((index /= 10) > 0);
	      prefix += 'T';
	      prefix += p;
	      return false;
	    }
	    char const* recursive_input = current.previous_types[index];
	    return eat_type_internal(current, recursive_input, prefix, postfix, last_class_name);
	  }
	  for(;;)	// Skip all G's.
	  {
	    switch(*input++)
	    {
	      case 'c':
		prefix += "char";
		return false;
	      case 'i':
		prefix += "int";
		return false;
	      case 'b':
		prefix += "bool";
		return false;
	      case 'v':
		prefix += "void";
		return false;
	      case 'e':
		prefix += "...";
		return false;
	      case 'f':
		prefix += "float";
		return false;
	      case 'd':
		prefix += "double";
		return false;
	      case 'r':
		prefix += "long double";
		return false;
	      case 'w':
		prefix += "wchar_t";
		return false;
	      case 's':
		prefix += "short";
		return false;
	      case 'l':
		prefix += "long";
		return false;
	      case 'x':
		prefix += "long long";
		return false;
	      case 'P':
	      case 'R':
	      {
		char what = (input[-1] == 'P') ? '*' : '&';
		internal_string member_function_pointer_scope;
		if (*input == 'M')
		{
		  ++input;
		  if (eat_type_internal(current, input, member_function_pointer_scope, postfix, NULL, true))
		    return true;
		  member_function_pointer_scope += postfix;
		  member_function_pointer_scope += "::";
		}
		bool is_const = (*input == 'C');
		if (is_const)
		  ++input;
		if (*input == 'F')
		{
		  ++input;
		  postfix += ")(";

		  // There must be at least one <type>.
		  if (eat_type(current, input, postfix))
		    return true;
		  // Eat optional other <type>s.
		  while (*input && *input != '_')
		  {
		    postfix += ", ";
		    if (eat_type(current, input, postfix))
		      return true;
		  }

		  postfix += ')';
		  if (is_const)
		    postfix += " const";

		  // Process _<type>.
		  if (*input++ != '_')
		    return true;
		  if (eat_type(current, input, prefix))
		    return true;
		  prefix += " (";
		  prefix += member_function_pointer_scope;
		  prefix += what;
		  return false;
		}
		if (eat_type_internal(current, input, prefix, postfix, NULL, true))
		  return true;
		if (is_const)
		  prefix += " const";
		prefix += what;
		return false;
	      }
	      case 'A':
		if (has_qualifiers)
		  postfix += ')';
		postfix += " [";
		if (!isdigit(*input))
		  return true;
		while (isdigit(*input))
		  postfix += *input++;
		if (*input++ != '_')
		  return true;
		postfix += ']';
		if (eat_type_internal(current, input, prefix, postfix, NULL, has_qualifiers))
		  return true;
		if (has_qualifiers)
		  prefix += " (";
		return false;
	      case 'O':
	      {
		internal_string member_scope;
		internal_string postfix2;
		if (eat_type_internal(current, input, member_scope, postfix2, NULL, true))
		  return true;
		member_scope += postfix2;
		member_scope += "::";
		if (*input++ != '_')
		  return true;
		if (eat_type_internal(current, input, prefix, postfix, NULL))
		  return true;
		if (*prefix.rbegin() != '(')
		  prefix += " ";
		prefix += member_scope;
		return false;
	      }
	      default:
		--input;
		break;
	    }
	    if (*input != 'G')
	      break;
	    if (isdigit(*++input))
	    {
	      int len = eat_digits(input);
	      if (len <= 0)
		return true;
	      prefix.append(input, len);
	      input += len;
	      return false;
	    }
	  }
	}
	if (*input == 'Q')
	{
	  // Process <number>
	  input += 2;
	  int number;
	  if (input[-1] >= '1' && input[-1] <= '9')
	    number = input[-1] - '0';
	  else if (input[-1] != '_' || (number = eat_digits(input)) <= 0)
	    return true;
	  // Process <scope-type><scope-type>...
	  while (--number)
	  {
	    if (eat_type_internal(current, input, prefix, postfix, NULL))
	      return true;
	    prefix += "::";
	  }
	  // Process last <type>
	  return eat_type_internal(current, input, prefix, postfix, last_class_name);
	}
	else if (*input == 't')
	{
	  ++input;
	  int len = eat_digits(input);
	  if (len <= 0)
	    return true;
	  prefix.append(input, len);
	  if (last_class_name)
	    last_class_name->assign(input, len);
	  input += len;
	  int count = eat_digits(input);
	  if (count <= 0)
	    return true;
	  prefix += '<';
	  internal_string template_type;
	  while(count--)
	  {
	    template_type.erase();
	    if (eat_template_type(current, input, template_type, NULL))
	      return true;
	    prefix += template_type;
	    if (count > 0)
	      prefix += ", ";
	  }
	  if (template_type[template_type.size() - 1] == '>')
	    prefix += ' ';
	  prefix += '>';
	  return false;
	}
	// Process <digits><class-name>
	int len = eat_digits(input);
	if (len > 0)
	{
	  if (len >= 11 && !strncmp(input, "_GLOBAL_", 8) && input[9] == 'N' && input[8] == input[10])
	  {
	    prefix += "{anonymous}";
	    if (last_class_name)
	      *last_class_name = "{anonymous}";
	  }
	  else
	  {
	    prefix.append(input, len);
	    if (last_class_name)
	      last_class_name->assign(input, len);
	  }
	  input += len;
	  return false;
	}
	if (*input == 'C' && !eat_type_internal(current, ++input, prefix, postfix, last_class_name, has_qualifiers))
	{
	  prefix += " const";
	  return false;
	}
	// This isn't a valid mangled type name.
	return true;
      }

      // {anonymous}::
      bool eat_scope_type(current_st& current, char const*& input, internal_string& type, internal_string& last_class_name)
      {
	Dout(dc::demangler, "Entering eat_scope_type(\"" << input << "\", internal_string& type, internal_string& last_class_name)");
	internal_string postfix;
	if (eat_type_internal(current, input, type, postfix, &last_class_name))
	{
	  type.erase();
	  return true;
	}
	type += postfix;
	return false;
      }

      // {anonymous}::
      bool eat_type(current_st& current, char const*& input, internal_string& type)
      {
	Dout(dc::demangler, "Entering eat_type(\"" << input << "\", internal_string& type)");
	internal_string postfix;
	size_t original_length = type.size();
	if (eat_type_internal(current, input, type, postfix, NULL))
	{
	  type.erase(original_length);
	  return true;
	}
	type += postfix;
	return false;
      }

    } // namespace

  } // namespace debug
} // namespace libcw

#ifdef STANDALONE

#include <iostream>
using namespace ::libcw::debug;

int main(int argc, char* argv[])
{
  Debug( libcw_do.on() );
  Debug( dc::demangler.on() );

  string out;
  libcw::debug::main_in = argv[1];
  demangle_symbol(argv[1], out);
  cout << out << endl;
  return 0;
}

#endif // STANDALONE

#endif // __GXX_ABI_VERSION == 0

