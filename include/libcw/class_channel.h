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

/** \file class_channel.h
 * Do not include this header file directly, instead include "\ref preparation_step2 "debug.h"".
 */

#ifndef LIBCW_CLASS_CHANNEL_H
#define LIBCW_CLASS_CHANNEL_H

#ifndef LIBCW_DEBUG_CONFIG_H
#include <libcw/debug_config.h>
#endif
#ifndef LIBCW_MAX_LABEL_LEN_H
#include <libcw/max_label_len.h>
#endif
#ifndef LIBCW_CONTROL_FLAG_H
#include <libcw/control_flag.h>
#endif
#ifndef LIBCW_PRIVATE_TSD_H
#include <libcw/private_TSD.h>
#endif

namespace libcw {
  namespace debug {

/**
 * \class channel_ct class_channel.h libcw/debug.h
 * \ingroup group_debug_channels
 *
 * \brief This object represents a debug channel, it has a fixed label.
 * A debug channel can be viewed upon as a single bit: on or off.
 *
 * Whenever %debug output is written, one or more %debug %channels must be specified (as first
 * parameter of the \ref Dout macro).&nbsp;
 * The %debug output is then written to the \link group_destination ostream \endlink of the
 * \link debug_ct debug object \endlink unless the %debug object
 * is turned off or when all specified %debug %channels are off.&nbsp;
 * Each %debug channel can be turned on and off independently.&nbsp;
 *
 * Multiple %debug %channels can be given by using <CODE>operator|</CODE> between the channel names.&nbsp;
 * This shouldn't be read as `or' but merely be seen as the bit-wise OR operation on the bit-masks
 * that these %channels actually represent.
 *
 * <b>Example:</b>
 *
 * \code
 * Dout( dc::notice, "Libcw is a great library" );
 * \endcode
 *
 * gives as result
 *
 * \exampleoutput <PRE>
 * NOTICE: Libcw is a great library</PRE>
 * \endexampleoutput
 *
 * and
 *
 * \code
 * Dout( dc::hello, "Hello World!" );
 * Dout( dc::kernel|dc::io, "This is written when either the kernel"
 *     "or io channel is turned on." );
 * \endcode
 *
 * gives as result
 *
 * \exampleoutput <PRE>
 * HELLO : Hello World!
 * KERNEL: This is written when either the kernel or io channel is turned on.</PRE>
 * \endexampleoutput
 */

class channel_ct {
private:
#ifdef LIBCWD_THREAD_SAFE
  int WNS_index;
    // A unique id that is used as index into the TSD array `off_cnt_array'.
#else // !LIBCWD_THREAD_SAFE
  int off_cnt;
    // A counter of the nested calls to off().
    // The channel is turned off when the value of `off' is larger or equal then zero
    // and `on' when it has the value -1.
#endif // !LIBCWD_THREAD_SAFE

  char WNS_label[max_label_len_c];
    // A reference name for the represented debug channel
    // This label will be printed in front of each output written to
    // this debug channel.

  bool WNS_initialized;
    // Set to true when initialized.

public:
  //---------------------------------------------------------------------------
  // Constructor
  //

  // MT: All channel objects must be global so that `WNS_initialized' is false 
  //     at the start of the program and initialization occurs before other threads
  //     share the object.
  explicit channel_ct(char const* label);

  // MT: May only be called from the constructors of global objects (or single threaded functions).
  void NS_initialize(char const* label);
    // Force initialization in case the constructor of this global object
    // wasn't called yet.  Does nothing when the object was already initialized.

public:
  //---------------------------------------------------------------------------
  // Manipulators
  //

  void off(void);
  void on(void);

  struct OnOffState {
    int off_cnt;
  };

  void force_on(OnOffState& state, char const* label);
  void restore(OnOffState const& state);

public:
  //---------------------------------------------------------------------------
  // Accessors
  //

  char const* get_label(void) const;
  bool is_on(void) const;
#ifdef LIBCWD_THREAD_SAFE
  bool is_on(LIBCWD_TSD_PARAM) const;
#endif
};

  } // namespace debug
} // namespace libcw

#endif // LIBCW_CLASS_CHANNEL_H

