// SPDX-FileCopyrightText: 2000-2005, 2013, 2018, 2020, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

/** \file class_channel.h
 * Do not include this header file directly, instead include \ref preparation_step2 "debug.h".
 */

#ifndef LIBCWD_CLASS_CHANNEL_H
#define LIBCWD_CLASS_CHANNEL_H

#include "libcwd/config.h"
#include "max_label_len.h"
#include "control_flag.h"
#include "private_struct_TSD.h"

namespace libcwd {

/**
 * \class channel_ct class_channel.h libcwd/debug.h
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
 * Dout( dc::notice, "Libcwd is a great library" );
 * \endcode
 *
 * gives as result
 *
 * \exampleoutput
 * NOTICE: Libcwd is a great library
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
 * \exampleoutput
 * HELLO : Hello World!
 * KERNEL: This is written when either the kernel or io channel is turned on.
 * \endexampleoutput
 */

class channel_ct {
private:
  int WNS_index;
    // A unique id that is used as index into the TSD array `off_cnt_array'.

  char WNS_label[max_label_len_c + 1];			// +1 for zero termination.
    // A reference name for the represented debug channel
    // This label will be printed in front of each output written to
    // this debug channel.

  bool WNS_initialized;
    // Set to true when initialized.

  static channel_ct const off_channel;
    // Channel that is always off.

public:
  //---------------------------------------------------------------------------
  // Constructor
  //

  // MT: All channel objects must be global so that `WNS_initialized' is false
  //     at the start of the program and initialization occurs before other threads
  //     share the object.
  explicit channel_ct(char const* label, bool add_to_channel_list = true);

  // MT: May only be called from the constructors of global objects (or single threaded functions).
  void NS_initialize(char const* label LIBCWD_COMMA_TSD_PARAM, bool add_to_channel_list);
    // Force initialization in case the constructor of this global object
    // wasn't called yet. Does nothing when the object was already initialized.

public:
  //---------------------------------------------------------------------------
  // Manipulators
  //

  void off();
  void on();

  struct OnOffState {
    int off_cnt;
  };

  void force_on(OnOffState& state, char const* label);
  void restore(OnOffState const& state);

  channel_ct const& operator()(bool cond) const { return cond ? *this : off_channel; }

public:
  //---------------------------------------------------------------------------
  // Accessors
  //

  char const* get_label() const;
  bool is_on() const;
  bool is_on(LIBCWD_TSD_PARAM) const;
};

} // namespace libcwd

#endif // LIBCWD_CLASS_CHANNEL_H
