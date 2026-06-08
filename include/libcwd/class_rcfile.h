// SPDX-FileCopyrightText: 2003-2004, 2018, 2020, 2025-2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_SYS_H
#error "You need to #include "sys.h" at the top of every source file (which in turn should #include "sys.h")."
#endif

#ifndef LIBCWD_CLASS_RCFILE_H
#define LIBCWD_CLASS_RCFILE_H

#include <iosfwd>
#include <vector>
#include <string>

namespace libcwd {

class Channel;

/**
 * \class RcFile class_rcfile.h libcwd/debug.h
 * \ingroup chapter_rcfile
 *
 * \brief This object represents a runtime configuration file.
 *
 * Libcwd contains one object of this type, <code>libcwd::rcfile</code>.
 * This is the object that is used by
 * <code>\link libcwd::read_rcfile read_rcfile() \endlink</code>.
 */
class RcFile {
private:
  std::string M_konsole_command;			// How to execute a command in a window.
  std::string M_gdb_bin;				// Path to 'gdb'.

  char const* M_rcname;					// Name of rcfile.
  bool M_env_set;					// Whether or not LIBCWD_RCFILE_NAME is set.
  bool M_read_called;

#if CWDEBUG_LOCATION
  bool M_elfutils_on;
#endif

public:
  /**
   * \brief Construct a rcfile object.
   */
  RcFile() : M_env_set(false), M_read_called(false) { }
  virtual ~RcFile() { }

private:
  void M_print_delayed_msg(int env_var, std::string const& value) const;
  void set_all_channels_on();
  void set_all_channels_off(bool warning_on);

  static bool S_exists(char const* name);
  std::string M_determine_rcfile_name();

  enum action_nt { toggle, on, off };
  void M_process_channel(Channel& debugChannel, std::string const& mask, action_nt const action);
  void M_process_channels(std::string list, action_nt const action);

public:
  /**
   * \brief Initialize this object by reading the rcfile.
   */
  void read();
  /**
   * \brief Returns the command line string as set with the 'xterm' keyword.
   */
  std::string const& konsole_command() const { return M_konsole_command; }
  /**
   * \brief Returns the command line string as set with the 'gdb_bin' keyword.
   */
  std::string const& gdb_bin() const { return M_gdb_bin; }
  /**
   * \brief Returns true when this object is initialized.
   */
  bool read_called() const { return M_read_called; }

protected:
  /**
   * \brief Virtual function called for unknown keywords.
   *
   * By using this class as a base and overriding this function
   * it is possible to extend the keywords that are recognized.
   *
   * This function should return \c true when the keyword is <em>not</em> handled.
   * The default behaviour is to always return \c true.
   */
  virtual bool unknown_keyword(std::string const& keyword, std::string const& value);
};

extern RcFile rcfile;

/**
 * \brief Calls libcwd::rcfile.read().
 *
 * \sa group_rcfile
 */
inline void read_rcfile()
{
  rcfile.read();
}

} // namespace libcwd

#endif // LIBCWD_CLASS_RCFILE_H
