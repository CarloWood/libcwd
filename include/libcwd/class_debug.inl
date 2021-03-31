// $Header$
//
// Copyright (C) 2000 - 2007, by
//
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_CLASS_DEBUG_INL
#define LIBCWD_CLASS_DEBUG_INL

#ifndef LIBCWD_CLASS_DEBUG_H
#include "class_debug.h"
#endif
#ifndef LIBCWD_CLASS_CHANNEL_H
#include "class_channel.h"
#endif
#ifndef LIBCWD_CLASS_FATAL_CHANNEL_H
#include "class_fatal_channel.h"
#endif
#ifndef LIBCWD_CLASS_CHANNEL_INL
#include "class_channel.inl"
#endif
#ifndef LIBCWD_CLASS_FATAL_CHANNEL_INL
#include "class_fatal_channel.inl"
#endif
#ifndef LIBCWD_CLASS_DEBUG_STRING_INL
#include "class_debug_string.inl"
#endif

namespace libcwd {

/** \addtogroup group_formatting */
/** \{ */

inline
debug_string_ct&
debug_ct::color_on()
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(color_on);
}

inline
debug_string_ct const&
debug_ct::color_on() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(color_on);
}

inline
debug_string_ct&
debug_ct::color_off()
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(color_off);
}

inline
debug_string_ct const&
debug_ct::color_off() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(color_off);
}

inline
debug_string_ct&
debug_ct::margin()
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(margin);
}

inline
debug_string_ct const&
debug_ct::margin() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(margin);
}

inline
debug_string_ct&
debug_ct::marker()
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(marker);
}

inline
debug_string_ct const&
debug_ct::marker() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(marker);
}

/**
 * \brief Set number of spaces to indent.
 */
inline
void
debug_ct::set_indent(unsigned short i)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_TSD_MEMBER(indent) = i;
}

/**
 * \brief Increment number of spaces to indent.
 */
inline
void
debug_ct::inc_indent(unsigned short i)
{
  LIBCWD_TSD_DECLARATION;
  LIBCWD_TSD_MEMBER(indent) += i;
}

/**
 * \brief Decrement number of spaces to indent.
 */
inline
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
inline
unsigned short
debug_ct::get_indent() const
{
  LIBCWD_TSD_DECLARATION;
  return LIBCWD_TSD_MEMBER(indent);
}

/** \} */

/** \addtogroup group_destination */
/** \{ */

/**
 * \brief Get the \c ostream device as set with set_ostream().
 */
inline
std::ostream*
debug_ct::get_ostream() const
{
  return real_os;
}

inline
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
inline
debug_ct::debug_ct()
{
  LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUG
  if (!NS_init(LIBCWD_TSD))
    core_dump();
#else
  (void)NS_init(LIBCWD_TSD);
#endif
}

/**
 * \brief Turn this %debug object off.
 */
inline
void
debug_ct::off()
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
inline
void
debug_ct::on()
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

#if LIBCWD_THREAD_SAFE
inline
bool
debug_ct::is_on(LIBCWD_TSD_PARAM) const
{
  return __libcwd_tsd.do_off_array[WNS_index] == -1;
}
#endif

inline
channel_set_st&
channel_set_bootstrap_st::operator|(channel_ct const& dc)
{
  mask = 0;
  label = dc.get_label();
  on = dc.is_on();
  return *reinterpret_cast<channel_set_st*>(this);
}

inline
channel_set_st&
channel_set_bootstrap_fatal_st::operator|(fatal_channel_ct const& fdc)
{
  mask = fdc.get_maskbit();
  label = fdc.get_label();
  on = true;
  return *reinterpret_cast<channel_set_st*>(this);
}

} // namespace libcwd

#endif // LIBCWD_CLASS_DEBUG_INL
