// $Header$
//
// Copyright (C) 2002 - 2007, by
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
as returned by <CODE>elfxx::asymbol_st::name</CODE>
which is what is returned by
\ref libcwd::location_ct::mangled_function_name "location_ct::mangled_function_name()"
and pc_mangled_function_name().

The direct use of these functions should be avoided, instead use the function type_info_of().

*/

//
// This file has been tested with gcc-3.x
//
// The description of how the mangling is done in the new ABI was found on
// http://www.codesourcery.com/cxx-abi/abi.html#mangling revision 1.74.
//
// To compile a standalone demangler:
// g++ -g -DSTANDALONE -DCWDEBUG -Iinclude demangle3.cc -Wl,--rpath,`pwd`/.libs -L.libs -lcwd -o c++filt
//

#include "sys.h"
#include "cwd_debug.h"
#include "libcwd/debug.h"
#include "libcwd/demangle.h"
#include "libcwd/private_assert.h"

#include <limits>

#ifdef STANDALONE
#ifdef CWDEBUG
namespace libcwd {
  namespace channels {
    namespace dc {
      channel_ct demangler("DEMANGLER");
    } // namespace dc
  } // namespace channels
} // namespace libcwd
#endif
#define _GLIBCXX_DEMANGLER_DEBUG(x) __Debug(x)
#define _GLIBCXX_DEMANGLER_DOUT(cntrl, data) __Dout(cntrl, data)
#define _GLIBCXX_DEMANGLER_DOUT_ENTERING(x) \
    __Dout(dc::demangler|continued_cf|flush_cf, "Entering " << x << "(\"" << &M_str[M_pos] << "\", \"" << output << "\") ")
#define _GLIBCXX_DEMANGLER_RETURN \
    do { \
      if (M_result) \
        __Dout(dc::finish, '[' << M_pos << "; \"" << output << "\"]" ); \
      else \
        __Dout(dc::finish, "(failed)"); return M_result; \
    } while(0)
#define _GLIBCXX_DEMANGLER_FAILURE \
    do { \
      if (M_result) \
      { \
        M_result = false; \
	__Dout(dc::finish, "[failure]"); \
      } \
      else \
        __Dout(dc::finish, "(failed)"); \
	return false; \
    } while(0)
#define _GLIBCXX_DEMANGLER_DOUT_ENTERING2(x) \
  do { \
    __Dout(dc::demangler|continued_cf|flush_cf, "Entering " << x); \
    if (qualifiers) \
      __Dout(dc::continued, " [with qualifiers: " << *qualifiers << ']'); \
    __Dout(dc::continued, "(\"" << &M_str[M_pos] << "\", \"" << prefix << "\", \"" << postfix << "\") "); \
  } while(0)
#define _GLIBCXX_DEMANGLER_RETURN2 \
  do { \
    if (M_result) \
      __Dout(dc::finish, '[' << M_pos << "; \"" << prefix << "\", \"" << postfix << "\"]" ); \
    else \
      __Dout(dc::finish, "(failed)"); \
    return M_result; \
  } while(0)
#define _GLIBCXX_DEMANGLER_DOUT_ENTERING3(x) \
  do { \
    __Dout(dc::demangler|continued_cf|flush_cf, "Entering " << x); \
    __Dout(dc::continued, " [with qualifier list: " << *this << ']'); \
    __Dout(dc::continued, " (\"" << prefix << "\", \"" << postfix << "\") "); \
  } while(0)
#define _GLIBCXX_DEMANGLER_RETURN3 \
  do { \
    __Dout(dc::finish, "[\"" << prefix << "\", \"" << postfix << "\"]" ); \
    return; \
  } while(0)
#endif // STANDALONE

#include "cwd_demangle.h"	// The actual demangler code.

namespace
{

// class decimal_float
//
// This internal class represents a floating point value
// with a precision of `mantissa_size_c' * 4 digits.
// With `mantissa_size_c' set to 5 that gives 20 digits,
// being more than 66 bits.  However, due to round off
// errors, the last `size of binary exponent' bits are
// unreliable.
//
// For IEEE double precision, the size of the exponent
// is 11 bits and thus is this class only accurate in
// 55 bits.  Therefore, only 17 digits may be printed
// in that case.  This size is precisely significant for
// the 52 bits of precision in the IEEE format as well.
//
// For IEEE single precision, the size of the exponent
// is 8 bits and we will have 58 bits of precision.  It
// makes no sense to print more than 7 digits in that
// case because IEEE single precision has only 23 bits
// of precision by itself.
//
// The internal value of a `decimal_float' object is
//
// (            mantissa[0] +
//   10000    * mantissa[1] +
//   10000**2 * mantissa[2] +
//   10000**3 * mantissa[3] +
//   10000**4 * mantissa[4]   ) * 10**exponent
//
// The member `max_precision_reached' is set to true
// when the value of the object cannot be multiplied
// with 10 anymore without changing the exponent,
// in other words, when mantissa[4] >= 1000.
// Its value is fuzzy however and only influences
// the maximum achievable accuracy.

static int const mantissa_size_c = 5;

struct decimal_float_data {
  unsigned long mantissa[mantissa_size_c];
  int exponent;
  bool max_precision_reached;
};

class decimal_float {
private:
  decimal_float_data M_data;

private:
  void M_do_overflow(unsigned long prev_borrow);
  void M_do_carry();

public:
  void set_2pow2pow(int power);
  void set_2pow(int power);
  void set_expstart(int expsize);
  void set_zero();
  void divide_by_two(bool decrement_exponent);
  bool decrement_exponent();
  decimal_float& operator+=(decimal_float const& term);
  decimal_float& operator*=(decimal_float const& factor);
  void print_to_with_precision(char* buf, int precision) const;
};

// These names must match the corresponding index of the table below
// and the p* constants must be contigious and in the given order.
// Same for the constants m1023 ... m15.
//        Name    Table-index
int const m1023 = 0;		// Corresponds with an exponent of 11 bits.
#if 0
int const m511  = 1;
int const m255  = 2;
int const m127  = 3;		// Corresponds with an exponent of 8 bits.
int const m63   = 4;
int const m31   = 5;
int const m15   = 6;
#endif
int const m1    = 7;
int const m0    = 8;		// Must have the same exponent as m1.
int const p1    = 9;
#if 0
int const p2    = 10;
int const p4    = 11;
int const p8    = 12;
int const p16   = 13;
int const p32   = 14;
int const p64   = 15;
int const p128  = 16;
int const p256  = 17;
int const p512  = 18;
int const p1024 = 19;
#endif

decimal_float_data const constants[] = {
  { { 6915, 3600, 2925, 5369, 1112 }, -327, true  },	// m1023; 2 ** -1023
  { { 3487,   41, 4624, 6681, 1491 }, -173, true  },	// m511; 2 ** -511
  { { 9251, 8888, 1101, 2337, 1727 },  -96, true  },	// m255; 2 ** -255
  { { 5398, 1437, 5411, 4717, 5877 },  -58, true  },	// m127; 2 ** -127
  { { 4340, 5504, 7248, 2021, 1084 },  -38, true  },	// m63; 2 ** -63
  { { 5781, 7392, 7307, 6128, 4656 },  -29, true  },	// m31; 2 ** -31
  { {    0,    0, 1250, 7578, 3051 },  -24, true  },	// m15; 2 ** -15
  { {    5,    0,    0,    0,    0 },   -1, false },	// m1; 2 ** -1
  { {   10,    0,    0,    0,    0 },   -1, false },	// m0; 2 ** 0
  { {    2,    0,    0,    0,    0 },    0, false },	// p1; 2 ** 1
  { {    4,    0,    0,    0,    0 },    0, false },	// p2; 2 ** 2
  { {   16,    0,    0,    0,    0 },    0, false },	// p4; 2 ** 4
  { {  256,    0,    0,    0,    0 },    0, false },	// p8; 2 ** 8
  { { 5536,    6,    0,    0,    0 },    0, false },	// p16; 2 ** 16
  { { 7296, 9496,   42,    0,    0 },    0, false },	// p32; 2 ** 32
  { { 1616,  955,  737, 6744, 1844 },    0, true  },	// p64; 2 ** 64
  { { 6346, 9384, 6920, 8236, 3402 },   19, true  },	// p128; 2 ** 128
  { { 9542, 3161, 9237, 9208, 1157 },   58, true  },	// p256; 2 ** 256
  { { 7100, 4259, 9299, 7807, 1340 },  135, true  },	// p512; 2 ** 512
  { { 9077, 2315, 3486, 6931, 1797 },  289, true  }	// p1024; 2 ** 1024
};

// Handle excess digits in the most significant
// element of the mantissa array.
void decimal_float::M_do_overflow(unsigned long prev_borrow)
{
  // assert(M_data.mantissa[mantissa_size_c - 1] > 9999);

  // Even after shifting the excess digits right,
  // M_data.mantissa[mantissa_size_c - 1] will still be
  // larger than 999.
  M_data.max_precision_reached = true;

  // Count the excess digits and update the exponent
  // already in order to keep the value constant.
  unsigned long divider = 10;
  ++M_data.exponent;
  while (prev_borrow >= divider)
  {
    divider *= 10;
    ++M_data.exponent;
  }

  // Finally, shift the value in the mantissa a few
  // digits to the right.  Round off what falls off.
  unsigned long multiplier = 10000 / divider;
  for (int i = mantissa_size_c - 1; i >= 0; --i)
  {
    unsigned long borrow = M_data.mantissa[i] % divider;
    if (i == 0)
      M_data.mantissa[i] += divider / 2;	// Round off.
    M_data.mantissa[i] /= divider;
    M_data.mantissa[i] += prev_borrow * multiplier;
    prev_borrow = borrow;
  }
}

// Handle excess digits in the elements of the mantissa array.
void decimal_float::M_do_carry()
{
  for (int i = 0; i < mantissa_size_c - 1; ++i)
  {
    if (M_data.mantissa[i] >= 10000)
    {
      M_data.mantissa[i + 1] += M_data.mantissa[i] / 10000;
      M_data.mantissa[i] %= 10000;
    }
  }
  if (M_data.mantissa[mantissa_size_c - 1] >= 10000)
    M_do_overflow(0);
}

// Initialize the object to a value of 2 ** (2 ** power).
inline void decimal_float::set_2pow2pow(int power)
{
  // assert(0 <= power && power <= 10);
  std::memcpy(&M_data, &constants[p1 + power], sizeof(decimal_float_data));
}

// Initialize the object to a value of 2 ** power.
inline void decimal_float::set_2pow(int power)
{
  // assert(power == 0 || power == 1);
  std::memcpy(&M_data, &constants[power], sizeof(decimal_float_data));
}

// Initialize the object to a value of 2 ** (1 - (2 ** (expsize - 1))).
inline void decimal_float::set_expstart(int expsize)
{
  // assert(expsize <= 11 && expsize >= 5);
  std::memcpy(&M_data, &constants[m1023 + 11 - expsize],
      sizeof(decimal_float_data));
}

// Initialize the object to zero.  Must have the same exponent as m0.
inline void decimal_float::set_zero()
{
  std::memcpy(&M_data, &constants[m0], sizeof(decimal_float_data));
  M_data.mantissa[0] = 0;
}

// Divide the value of this object by two in one of two possible ways:
// If `decrement_exponent' is true - then decrement the exponent by
// one and multiply the mantissa with five.  Otherwise just divide
// the mantissa by two.
void decimal_float::divide_by_two(bool decrement_exponent)
{
  if (decrement_exponent)
  {
    for (int i = 0; i < mantissa_size_c; ++i)
      M_data.mantissa[i] *= 5;
    M_do_carry();
    --M_data.exponent;
  }
  else
  {
    unsigned long prev_borrow = M_data.mantissa[mantissa_size_c - 1] % 2;
    if ((M_data.mantissa[mantissa_size_c - 1] /= 2) < 1000)
      M_data.max_precision_reached = false;
    for (int i = mantissa_size_c - 2; i >= 0; --i)
    {
      unsigned long borrow = M_data.mantissa[i] % 2;
      M_data.mantissa[i] /= 2;
      M_data.mantissa[i] += prev_borrow * 5000;
      prev_borrow = borrow;
    }
    if (prev_borrow)
    {
      // Round off.
      if (++M_data.mantissa[0] == 10000)
        M_do_carry();
    }
  }
}

// If possible, multiply the mantissa with 10 and
// decrement the exponent with one.  This is used
// to keep the exponent of this object equal to
// another object that is being divided by two.
bool decimal_float::decrement_exponent()
{
  if (M_data.max_precision_reached)
    return false;
  for (int i = 0; i < mantissa_size_c; ++i)
    M_data.mantissa[i] *= 10;
  M_do_carry();
  if (M_data.mantissa[mantissa_size_c - 1] >= 1000)
    M_data.max_precision_reached = true;
  --M_data.exponent;
  return true;
}

// Add two decimal_floats that have the same exponent.
decimal_float& decimal_float::operator+=(decimal_float const& term)
{
  // assert(M_data.exponent == term.M_data.exponent);
  for (int i = 0; i < mantissa_size_c; ++i)
    M_data.mantissa[i] += term.M_data.mantissa[i];
  M_do_carry();
  return *this;
}

// The real work.  Multiply this object with `factor'.
decimal_float& decimal_float::operator*=(decimal_float const& factor)
{
  // Count the number of most-significant mantissa elements (digits)
  // that contain zero (of either factor), but stop counting if we reach
  // mantissa_size_c - 1.
  int zero_digits = 0;
  while (M_data.mantissa[mantissa_size_c - 1 - zero_digits] == 0)
    if (++zero_digits == mantissa_size_c - 1)
      break;
  if (zero_digits < mantissa_size_c - 1)
  {
    int offset = mantissa_size_c - 1 + zero_digits;
    while (factor.M_data.mantissa[offset - zero_digits] == 0)
      if (++zero_digits == mantissa_size_c - 1)
	break;
  }

  // Set `this_mantissa' to point to the mantissa of this object;
  // make a copy only when we would overwrite its values when
  // still needed below.
  unsigned long tmp_mantissa[mantissa_size_c];
  unsigned long* this_mantissa;
  if (zero_digits == 0)
    this_mantissa = M_data.mantissa;
  else
  {
    std::memcpy(tmp_mantissa, M_data.mantissa, sizeof(tmp_mantissa));
    this_mantissa = tmp_mantissa;
  }

  // Set lss (least significant (loop) size, but don't ask me why it is
  // called that way - it gets a bit complicated here) to, heh, what works.
  int lss = mantissa_size_c - 1 - zero_digits;
  // Now hold your breath...
  M_data.exponent += factor.M_data.exponent + 4 * lss;
  unsigned long sum = 0;
  for (int i = 0; i < lss; ++i)
    sum += this_mantissa[i] * factor.M_data.mantissa[lss - 1 - i];
  sum += 5000;	// Round off.
  sum /= 10000;
  for (int j = 0; j < mantissa_size_c; ++j)
  {
    int loop_bgn = std::max(0, lss + j - (mantissa_size_c - 1));
    int loop_end = std::min(mantissa_size_c - 1, lss + j);
    for (int i = loop_bgn; i <= loop_end; ++i)
      sum += this_mantissa[i] * factor.M_data.mantissa[lss + j - i];
    M_data.mantissa[j] = sum;
    sum /= 10000;
    M_data.mantissa[j] -= 10000 * sum;
  }
  // ... ahhh. Ok. And do overflow management when needed.
  if (sum > 0)
    M_do_overflow(sum);
  return *this;
}

// Print to `buf', writes: "<digit>.<digits>e<sign><exponent>\0".
// sizeof(buf) must be at least 7 larger than `precision':
// strlen(<digit><digits>) <= precision.
// strlen(.e<sign><exponent>) == 6, plus one for the trailing '\0'.
void decimal_float::print_to_with_precision(char* buf, int precision) const
{
  // First do the round off.
  decimal_float tmp(*this);
  if (!M_data.max_precision_reached)
  {
    // In this case we are probably exact, so we
    // don't really need to round off.  Because leading
    // zeroes are not printed, add number of leading 0's
    // to `precision'.
    for (int i = mantissa_size_c - 1; i >= 0; --i)
      for(unsigned long shift = 1000; shift; shift /= 10)
      {
        if (M_data.mantissa[i] >= shift)
	{
	  i = 0;
	  break;
        }
	++precision;
      }
  }
  int e = 4 * mantissa_size_c - 1;
  if (precision < mantissa_size_c * 4)
  {
    int cut = e - precision;
    int cuti = cut / 4;
    int cutr = cut % 4;
    unsigned long shift = 10;
    while(cutr--)
      shift *= 10;
    tmp.M_data.mantissa[cuti] += shift / 2;	// Round off.
    if (tmp.M_data.mantissa[cuti] >= 10000)
      tmp.M_do_carry();
  }

  bool leading = true;
  char* p = buf;
  int tz = 0;
  for (int i = mantissa_size_c - 1; i >= 0 && precision; --i)
  {
    unsigned long digit = tmp.M_data.mantissa[i];
    for(int shift = 1000; shift; shift /= 10)
    {
      int d = digit / shift;
      digit -= d * shift;
      if (leading && d != 0)
        leading = false;
      if (leading)	// Suppress leading zeroes.
        --e;
      else
      {
	if (d == 0)	// Suppress and count trailing zeroes.
	  ++tz;
	else
	{
	  if (p == buf + 1)
	    *p++ = '.';
	  while(tz--)
	    *p++ = '0';
	  *p++ = d + '0';
	  tz = 0;
	}
	if (--precision == 0)
	  break;
      }
    }
  }
  e += tmp.M_data.exponent;
  if (e != 0)
  {
    *p++ = 'e';
    if (e > 0)
      *p++ = '+';
    else
    {
      *p++ = '-';
      e = -e;
    }
    int shift = 100;
    leading = true;
    while(shift)
    {
      int d = e / shift;
      e -= d * shift;
      if (leading && d != 0)
        leading = false;
      if (!leading)
        *p++ = '0' + d;
      shift /= 10;
    }
  }
  *p = 0;
}

// print_IEEE_fp(buf, words, nbits_exponent, nbits_fraction, precision)
//
// buf                  : Output buffer, the result is written to this buffer
//                        and terminated with a trailing '\0'.  The size of
//                        this buffer must be at least `precision + 7'.
// words                : A pointer to an array with unsigned integers.  Only
//                        the 32 least significant bits of each element are
//                        used.  The first element contains the word with the
//                        most significant bits.  Bit 31 in each element
//                        represents the most significant bit of that 32-bit
//                        word.  See the description of IEEE 754-1985 for
//                        the exact meaning.  Basically they mean:
//                        SEEEEEEEEMMMMMMMMMMMMMMMMMMMM, where S is the sign
//                        bit, EEEEEEEE the exponent and MMMMMMMMMMMMMMMMMMMM
//                        the mantissa.
// nbits_exponent       : The number of bits in `words' that represent the
//                        exponent.
// nbits_fraction       : The total number of bits in `words' representing the
//                        mantissa, a value larger than 52 will result in
//                        inaccurate output.
// precision            : The maximum number of digits in the mantissa
//                        (including the first digit before the dot) that
//                        will be written to `buf'.  This precision should
//                        correspond to the accuracy given by `nbits_fraction'
//                        of course.

void print_IEEE_fp(char* buf, unsigned long* words,
    int nbits_exponent, int nbits_fraction, int precision)
{
  // This function was written using
  // http://www.psc.edu/general/software/packages/ieee/ieee.html
  // as reference for the IEEE 754-1985 standard.

  // assert( nbits_fraction <= 52 && nbits_exponent <= 11 &&
  //         nbits_exponent >= 5 && precision <= 17 );
  unsigned long sign_mask = 1 << nbits_exponent;
  unsigned long exponent_mask = sign_mask - 1;
  unsigned long sign_and_exponent = words[0] >> (31 - nbits_exponent);
  unsigned long exponent = sign_and_exponent & exponent_mask;
  int nelem_fraction = (nbits_fraction + nbits_exponent) / 32 + 1;
  unsigned long mask_first_elem = 0xffffffffUL >> (nbits_exponent + 1);
  unsigned long mask_last_elem = ~((1UL << (nelem_fraction * 32 -
      nbits_fraction - nbits_exponent - 1)) - 1);
  unsigned long fraction_nonzero = (words[0] & mask_first_elem);
  if (nelem_fraction == 1)
    fraction_nonzero &= mask_last_elem;
  else
  {
    fraction_nonzero |= (words[nelem_fraction - 1] & mask_last_elem);
    for (int i = 1; i < nelem_fraction - 1; ++i)
      fraction_nonzero |= words[i];
  }
  if (exponent == exponent_mask && fraction_nonzero)
  {
    strcpy(buf, "nan");
    return;
  }
  if ((sign_and_exponent & sign_mask))
    *buf++ = '-';
  if (exponent == exponent_mask && !fraction_nonzero)
  {
    strcpy(buf, "inf");
    return;
  }
  bool normalized = true;
  if (exponent == 0)
  {
    if (!fraction_nonzero)
    {
      strcpy(buf, "0");
      return;
    }
    else
    {
      normalized = false;
      exponent = 1;
    }
  }

  decimal_float result;
  if (normalized)
    result.set_2pow(m0);		// 1.0 ; The 1 in (1.F)
  else
    result.set_zero();			// 0.0 ; The 0 in (0.F)
  decimal_float bit;
  bit.set_2pow(m1);			// 0.5 ; 2 ** (-bit_cnt - 1)
  unsigned long mask = 0x40000000 >> nbits_exponent;
  for (int bit_cnt = 0; bit_cnt < nbits_fraction; ++bit_cnt)
  {
    if ((*words & mask))
      result += bit;
    bool success = result.decrement_exponent();
    bit.divide_by_two(success);		// Keep exponents equal for easy addition.
    if (!(mask >>= 1))
    {
      mask = 0x80000000;
      ++words;
    }
  }
  decimal_float two_pow_exponent;
  two_pow_exponent.set_expstart(nbits_exponent);
      // 2 ** (1 - 2 ** (nbits_exponent - 1)),
      // where 1 - 2 ** (nbits_exponent - 1) is
      // the two's-complement exponent 'offset'.
  mask = 1 << (nbits_exponent - 1);
  for (int bit_cnt = nbits_exponent - 1; bit_cnt >= 0; --bit_cnt)
  {
    if ((exponent & mask))
    {
      bit.set_2pow2pow(bit_cnt);	// 2 ** (2 ** bit_cnt)
      two_pow_exponent *= bit;
    }
    mask >>= 1;
  }
  result *= two_pow_exponent;
  result.print_to_with_precision(buf, precision);
}

} // namespace

namespace libcwd {

#ifdef STANDALONE
static char const* main_in;
#endif

namespace _private_ {

class implementation_details : public __gnu_cxx::demangler::implementation_details {
public:
  implementation_details(unsigned int flags) : __gnu_cxx::demangler::implementation_details(flags) { }
protected:
  virtual bool
  decode_real(char* output, unsigned long* input, size_t size_of_real) const;
};

// GNU/Linux uses IEEE encoding for floats.
bool
implementation_details::decode_real(char* output, unsigned long* input, size_t size_of_real) const
{
  if (size_of_real != 4 && size_of_real !=8)		// Only decode single and double precision IEEE.
    return false;

  if (size_of_real == 8)
    print_IEEE_fp(output, input, 11, 52, 17);
  else
    print_IEEE_fp(output, input, 8, 23, 8);

  return true;
}

#if CWDEBUG_ALLOC
typedef __gnu_cxx::demangler::session<internal_allocator> session_type;
#else
typedef __gnu_cxx::demangler::session<std::allocator<char> > session_type;
#endif

//
// demangle_symbol
//
// Demangle `input' and append to `output'.
// `input' should be a mangled_function_name as for instance returned
// by `libcwd::pc_mangled_function_name'.
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

  _GLIBCXX_DEMANGLER_DOUT(dc::demangler, "Entering demangle_symbol(\"" << input << "\")");

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
  // <mangled-name> ::= _GLOBAL_ _<type>_ <disambiguation part>	// GNU extension, <type> can be I or D.
  //
  bool failure = (input[0] != '_');

  if (!failure)
  {
    if (input[1] == 'Z')
    {
      // C++ name?
      implementation_details id(implementation_details::style_void);
      int cnt = session_type::decode_encoding(output, input + 2, INT_MAX, id);
      if (cnt < 0 || input[cnt + 2] != 0)
	failure = true;
    }
    else if (input[1] == 'G')
    {
      // Possible _GLOBAL__ extension?
      if (!std::strncmp(input, "_GLOBAL__", 9) && (input[9] == 'D' || input[9] == 'I') && input[10] == '_')
      {
	if (input[9] == 'D')
	  output.assign("global destructors keyed to ", 28);
	else
	  output.assign("global constructors keyed to ", 29);
	// Output the disambiguation part as-is.
	output += input + 11;
      }
      else
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
void demangle_type(char const* input, internal_string& output)
{
#if CWDEBUG_DEBUGM
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( __libcwd_tsd.internal );
#endif
#ifdef STANDALONE
  if (input != main_in)
    Debug( dc::demangler.off() );
#endif
  _GLIBCXX_DEMANGLER_DOUT(dc::demangler, "Entering demangle_type(\"" << input << "\")");
  if (input == NULL)
  {
    output += "(null)";
#ifdef STANDALONE
    if (input != main_in)
      Debug( dc::demangler.on() );
#endif
    return;
  }
  implementation_details id(implementation_details::style_void);
  session_type demangler_session(input, INT_MAX, id);
  if (!demangler_session.decode_type(output) || demangler_session.remaining_input_characters())
    output.assign(input, strlen(input));	// Failure to demangle, return the mangled type name.
#ifdef STANDALONE
  if (input != main_in)
    Debug( dc::demangler.on() );
#endif
}

  } // namespace _private_
} // namespace libcwd

#ifdef STANDALONE
#include <iostream>

int main(int argc, char* argv[])
{
  Debug( libcw_do.on() );
  Debug( dc::demangler.on() );
  std::string out;
  libcwd::main_in = argv[1];
  libcwd::demangle_symbol(argv[1], out);
  std::cout << out << std::endl;
  return 0;
}

#endif // STANDALONE

namespace libcwd {

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

} // namespace libcwd


