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

#ifndef LIBCW_CLASS_DEBUG_INL
#define LIBCW_CLASS_DEBUG_INL

#ifndef LIBCW_CLASS_DEBUG_H
#include <libcwd/class_debug.h>
#endif
#ifndef LIBCW_CLASS_CHANNEL_H
#include <libcwd/class_channel.h>
#endif
#ifndef LIBCW_CLASS_FATAL_CHANNEL_H
#include <libcwd/class_fatal_channel.h>
#endif
#ifndef LIBCW_CLASS_CHANNEL_INL
#include <libcwd/class_channel.inl>
#endif
#ifndef LIBCW_CLASS_FATAL_CHANNEL_INL
#include <libcwd/class_fatal_channel.inl>
#endif
#ifndef LIBCW_CLASS_DEBUG_STRING_INL
#include <libcwd/class_debug_string.inl>
#endif

namespace libcw {
  namespace debug {

/** \addtogroup group_formatting */
/* \{ */

__inline__
debug_string_ct&
debug_ct::margin(void)
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(margin);
}

__inline__
debug_string_ct const&
debug_ct::margin(void) const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(margin);
}

__inline__
debug_string_ct&
debug_ct::marker(void)
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(marker);
}

__inline__
debug_string_ct const&
debug_ct::marker(void) const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(marker);
}

// This is here only for backwards compatibility.
// You should call margin().assign(s) directly.
__inline__
void
debug_ct::set_margin(std::string const& s)
{
  margin().assign(s);
}

// This is here only for backwards compatibility.
// You should call marker().assign(s) directly.
__inline__
void
debug_ct::set_marker(std::string const& s)
{
  marker().assign(s);
}

/**
 * \brief Set number of spaces to indent.
 */
__inline__
void
debug_ct::set_indent(unsigned short i)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_TSD_MEMBER(indent) = i;
}

/**
 * \brief Increment number of spaces to indent.
 */
__inline__
void
debug_ct::inc_indent(unsigned short i)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_TSD_MEMBER(indent) += i;
}

/**
 * \brief Decrement number of spaces to indent.
 */
__inline__
void
debug_ct::dec_indent(unsigned short i)
{
  LIBCWD_TSD_DECLARATION;
  int prev_indent = LIBCWD_TSD_MEMBER(indent);
  LIBCWD_TSD_MEMBER(indent) = (i > prev_indent) ? 0 : (prev_indent - i);
}

/**
 * \brief Get the current indentation.
 */
__inline__
unsigned short
debug_ct::get_indent(void) const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(indent);
}

// This is here only for backwards compatibility.
// You should use margin directly.
__inline__
std::string
debug_ct::get_margin(void) const
{
  return std::string(margin().c_str(), margin().size());
}

// This is here only for backwards compatibility.
// You should use marker directly.
__inline__
std::string
debug_ct::get_marker(void) const
{
  return std::string(marker().c_str(), marker().size());
}

/** \} */

/** \addtogroup group_destination */
/** \{ */

/**
 * \brief Get the \c ostream device as set with set_ostream().
 */
__inline__
std::ostream*
debug_ct::get_ostream(void) const
{
  return real_os;
}

__inline__
void
debug_ct::private_set_ostream(std::ostream* os)
{
  real_os = os;
#if CWDEBUG_DEBUG
  LIBCWD_TSD_DECLARATION;
  LIBCWD_ASSERT( LIBCWD_TSD_MEMBER(tsd_initialized) );
#endif
}

/** \} */

/**
 * \brief Constructor
 *
 * A %debug object must be global.
 *
 * \sa group_debug_object
 * \sa chapter_custom_do
 */
__inline__
debug_ct::debug_ct(void)
{
  NS_init();
}

/**
 * \brief Turn this %debug object off.
 */
__inline__
void
debug_ct::off(void)
{
  LIBCWD_TSD_DECLARATION;
  ++LIBCWD_TSD_MEMBER_OFF;
}

/**
 * \brief Cancel last call to off().
 *
 * Calls to off() and on() has to be done in pairs (first off() then on()).
 * These pairs can be nested.
 *
 * \anchor eval_example
 * <b>Example:</b>
 *
 * \code
 * int i = 0;
 * Debug( libcw_do.off() );
 * Dout( dc::notice, "Adding one to " << i++ );
 * Debug( libcw_do.on() );
 * Dout( dc::notice, "i == " << i );
 * \endcode
 *
 * Outputs:
 *
 * <PRE class="example-output">NOTICE : i == 0
 * </PRE>
 *
 * Note that the statement <CODE>i++</CODE> was never executed.
 */
__inline__
void
debug_ct::on(void)
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGOUTPUT
  if (LIBCWD_TSD_MEMBER(first_time) && LIBCWD_TSD_MEMBER_OFF == -1)
    LIBCWD_TSD_MEMBER(first_time) = false;
  else
    --LIBCWD_TSD_MEMBER_OFF;
#else
  --LIBCWD_TSD_MEMBER_OFF;
#endif
}

__inline__
channel_set_st&
channel_set_bootstrap_st::operator|(channel_ct const& dc)
{
  mask = 0;
  label = dc.get_label();
  on = dc.is_on();
  return *reinterpret_cast<channel_set_st*>(this);
}

__inline__
channel_set_st&
channel_set_bootstrap_st::operator&(fatal_channel_ct const& fdc)
{
  mask = fdc.get_maskbit();
  label = fdc.get_label();
  on = true;
  return *reinterpret_cast<channel_set_st*>(this);
}

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_DEBUG_INL
