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

/** \file libcwd/cwprint.h
 *
 * \brief Definition of the utilities \link libcw::debug::cwprint cwprint \endlink and
 *        \link libcw::debug::cwprint_using cwprint_using \endlink.
 *
 * This header file provides the declaration and definition of two debug utility functions,
 * intended to print objects to an ostream without using the \c operator<< inserter for
 * that class, if any.
 *
 * \sa libcw::debug::cwprint
 *  \n libcw::debug::cwprint_using
 */

#ifndef LIBCW_CWPRINT_H
#define LIBCW_CWPRINT_H

namespace libcw {
  namespace debug {

//===================================================================================================
// cwprint
//

template<class PRINTABLE_OBJECT>
  class cwprint_tct {
  private:
    PRINTABLE_OBJECT const& M_printable_object;
  public:
    cwprint_tct(PRINTABLE_OBJECT const& printable_object) : M_printable_object(printable_object) { }

    friend
    __inline__	// Must be defined inside the class declaration in order to avoid a compiler warning.
    std::ostream&
    operator<<(std::ostream& os, cwprint_tct<PRINTABLE_OBJECT> const& __cwprint)
    {
      __cwprint.M_printable_object.print_on(os);
      return os;
    }
  };

/**
 * \interface cwprint cwprint.h libcwd/cwprint.h
 * \ingroup group_special
 *
 * \brief Print an object to a %debug stream without needing an operator<<.
 *
 * This utility can be used to print an object to a %debug stream without using
 * the normal \c operator<< of that object (if any exists at all).&nbsp;
 * The purpose is to allow the printing of objects to %debug streams in a \e different
 * way then when you'd normally print them to say <CODE>std::cout</CODE>.
 *
 * The \a printable_object (see example below) must have the signature:
 *
 * \code
 * class Class {
 *   ...
 * public:
 *   void print_on(std::ostream& os) const;
 * };
 * \endcode
 *
 * \sa cwprint_using
 *
 * <b>Example:</b>
 *
 * \code
 * Dout( dc::channel, cwprint(printable_object) );
 * \endcode
 *
 * this will write \a printable_object to the %debug stream by
 * calling the method \c print_on.
 */

template<class T>
  __inline__
  cwprint_tct<T>
  cwprint(T const& printable_object)
  {
    return cwprint_tct<T>(printable_object);
  }

//===================================================================================================
// cwprint_using
//

template<class PRINTABLE_OBJECT>
  class cwprint_using_tct {
    typedef void (PRINTABLE_OBJECT::* print_on_method_t)(std::ostream&) const;
  private:
    PRINTABLE_OBJECT const& M_printable_object;
    print_on_method_t M_print_on_method;
  public:
    cwprint_using_tct(PRINTABLE_OBJECT const& printable_object, print_on_method_t print_on_method) :
	M_printable_object(printable_object), M_print_on_method(print_on_method) { }

    friend
    __inline__	// Must be defined inside the class declaration in order to avoid a compiler warning.
    std::ostream&
    operator<<(std::ostream& os, cwprint_using_tct<PRINTABLE_OBJECT> __cwprint_using)
    {
      (__cwprint_using.M_printable_object.*__cwprint_using.M_print_on_method)(os);
      return os;
    }
  };

/**
 * \interface cwprint_using cwprint.h libcwd/cwprint.h
 * \ingroup group_special
 *
 * \brief Print an object to an ostream using an arbitrary method of that object.
 *
 * This utility can be used to print an object to a %debug stream without using
 * the normal \c operator<< of that object (if any exists at all).&nbsp;
 * The purpose is to allow the printing of objects to %debug streams in a \e different
 * way then when you'd normally print them to say <CODE>std::cout</CODE>.
 *
 * The \a object (see example below) must have the signature:
 *
 * \code
 * class Class {
 *   ...
 * public:
 *   void arbitrary_method_name(std::ostream& os) const;
 * };
 * \endcode
 *
 * \sa cwprint
 *
 * <b>Example:</b>
 *
 * \code 
 * Dout( dc::channel, cwprint_using(object, &Class::arbitrary_method_name) );
 * \endcode
 *
 * this will write \a object to the %debug stream by
 * calling the method \a arbitrary_method_name.
 */

// The use of T_OR_BASE_OF_T as extra parameter is a compiler bug around.
// Without it you'd run into bug
// http://gcc.gnu.org/cgi-bin/gnatsweb.pl?cmd=view%20audit-trail&database=gcc&pr=38
// when `print_on_method' is a method of the base class of T.

template<class T, class T_OR_BASE_OF_T>
  __inline__
  cwprint_using_tct<T_OR_BASE_OF_T>
  cwprint_using(T const& printable_object, void (T_OR_BASE_OF_T::*print_on_method)(std::ostream&) const)
  {
    T_OR_BASE_OF_T const& base(printable_object);
    return cwprint_using_tct<T_OR_BASE_OF_T>(base, print_on_method);
  }

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CWPRINT_H
