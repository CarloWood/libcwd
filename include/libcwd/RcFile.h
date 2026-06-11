// SPDX-FileCopyrightText: 2003-2004, 2018, 2020, 2025-2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef LIBCWD_SYS_H
#error You need to #include "sys.h" at the top of every source file (which in turn should #include "sys.h").
#endif

#ifndef LIBCWD_CLASS_RCFILE_H
#define LIBCWD_CLASS_RCFILE_H

#include <iosfwd>
#include <string>
#include <vector>

namespace libcwd {

class Channel;

/**
 * \class RcFile RcFile.h libcwd/debug.h
 * \ingroup chapter_rcfile
 *
 * \brief This object represents a runtime configuration file.
 *
 * Libcwd contains one object of this type, <code>libcwd::rcfile</code>.
 * This is the object that is used by
 * <code>\link libcwd::read_rcfile read_rcfile() \endlink</code>.
 */
class RcFile
{
 private:
  std::string konsole_command_; // How to execute a command in a window.
  std::string gdb_bin_; // Path to 'gdb'.

  char const* rcname_; // Name of rcfile.
  bool env_set_; // Whether or not LIBCWD_RCFILE_NAME is set.
  bool read_called_;

#if CWDEBUG_LOCATION
  bool elfutils_on_;
#endif

 public:
  /**
   * \brief Construct a rcfile object.
   */
  RcFile() : env_set_(false), read_called_(false) { }
  virtual ~RcFile() { }

 private:
  void print_delayed_msg(int env_var, std::string const& value) const;
  void set_all_channels_on();
  void set_all_channels_off(bool warning_on);

  static bool S_exists(char const* name);
  std::string determine_rcfile_name();

  enum action_nt
  {
    toggle,
    on,
    off
  };
  void process_channel(Channel& debugChannel, std::string const& mask, action_nt const action);
  void process_channels(std::string list, action_nt const action);

 public:
  /**
   * \brief Initialize this object by reading the rcfile.
   */
  void read();
  /**
   * \brief Returns the command line string as set with the 'xterm' keyword.
   */
  std::string const& konsole_command() const { return konsole_command_; }
  /**
   * \brief Returns the command line string as set with the 'gdb_bin' keyword.
   */
  std::string const& gdb_bin() const { return gdb_bin_; }
  /**
   * \brief Returns true when this object is initialized.
   */
  bool read_called() const { return read_called_; }

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
