// $Header$
//
// Copyright (C) 2000 - 2001, by
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
#include <libcw/class_debug.h>
#endif
#ifndef LIBCW_CLASS_CHANNEL_H
#include <libcw/class_channel.h>
#endif
#ifndef LIBCW_CLASS_FATAL_CHANNEL_H
#include <libcw/class_fatal_channel.h>
#endif
#ifndef LIBCW_CLASS_CHANNEL_INL
#include <libcw/class_channel.inl>
#endif
#ifndef LIBCW_CLASS_FATAL_CHANNEL_INL
#include <libcw/class_fatal_channel.inl>
#endif
#ifndef LIBCW_CLASS_DEBUG_STRING_INL
#include <libcw/class_debug_string.inl>
#endif

namespace libcw {
  namespace debug {

/** \addtogroup group_formatting */
/* \{ */

// This is here only for backwards compatibility.
// You should call margin.assign() directly.
__inline__
void
debug_ct::set_margin(std::string const& s)
{
  margin.assign(s);
}

// This is here only for backwards compatibility.
// You should call marker.assign() directly.
__inline__
void
debug_ct::set_marker(std::string const& s)
{
  marker.assign(s);
}

/**
 * \brief Set number of spaces to indent.
 */
__inline__
void
debug_ct::set_indent(unsigned short i)
{
  indent = i;
}

/**
 * \brief Increment number of spaces to indent.
 */
__inline__
void
debug_ct::inc_indent(unsigned short i)
{
  indent += i;
}

/**
 * \brief Decrement number of spaces to indent.
 */
__inline__
void
debug_ct::dec_indent(unsigned short i)
{
  indent = (i > indent) ? 0 : (indent - i);
}

/**
 * \brief Get the current indentation.
 */
__inline__
unsigned short
debug_ct::get_indent(void) const
{
  return indent;
}

// This is here only for backwards compatibility.
// You should use margin directly.
__inline__
std::string
debug_ct::get_margin(void) const
{
  return std::string(margin.c_str(), margin.size());
}

// This is here only for backwards compatibility.
// You should use marker directly.
__inline__
std::string
debug_ct::get_marker(void) const
{
  return std::string(marker.c_str(), marker.size());
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
  return orig_os;
}

/**
 * \brief Get the \em current ostream device (might be a temporary stringstream).
 */
__inline__
std::ostream&
debug_ct::get_os(void) const
{
  return *os;
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
 * \brief Set output device.
 * \ingroup group_destination
 *
 * Assign a new \c ostream to this %debug object (default is <CODE>std::cerr</CODE>).
 */
__inline__
void
debug_ct::set_ostream(std::ostream* _os)
{
  orig_os = os = _os;
}

/**
 * \brief Turn this %debug object off.
 */
__inline__
void
debug_ct::off(void)
{
  ++_off;
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
#ifdef DEBUGDEBUGOUTPUT
  if (first_time && _off == -1)
    first_time = false;
  else
    --_off;
#else
  --_off;
#endif
}

__inline__
channel_set_st&
debug_ct::operator|(channel_ct const& dc)
{
  channel_set.mask = 0;
  channel_set.label = dc.get_label();
  channel_set.on = dc.is_on();
  return channel_set;
}

__inline__
channel_set_st&
debug_ct::operator&(fatal_channel_ct const& fdc)
{
  channel_set.mask = fdc.get_maskbit();
  channel_set.label = fdc.get_label();
  channel_set.on = true;
  return channel_set;
}

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_DEBUG_INL
