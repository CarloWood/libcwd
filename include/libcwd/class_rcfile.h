// $Header$
//
// Copyright (C) 2003, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#ifndef LIBCWD_SYS_H
#error "You need to #include "sys.h" at the top of every source file (which in turn should #include <libcwd/sys.h>)."
#endif

#ifndef LIBCWD_CLASS_RCFILE_H
#define LIBCWD_CLASS_RCFILE_H

#ifndef LIBCW_IOSFWD
#define LIBCW_IOSFWD
#include <iosfwd>
#endif
#ifndef LIBCW_VECTOR
#define LIBCW_VECTOR
#include <vector>
#endif
#ifndef LIBCW_STRING
#define LIBCW_STRING
#include <string>
#endif

namespace libcw {
  namespace debug {

class channel_ct;

/**
 * \class rcfile_ct class_rcfile.h libcwd/debug.h
 * \ingroup chapter_rcfile
 *
 * \brief This object represents a runtime configuration file.
 *
 * Libcwd contains one object of this type, <code>libcw::debug::rcfile</code>.
 * This is the object that is used by
 * <code>\link libcw::debug::read_rcfile read_rcfile() \endlink</code>.
 */
class rcfile_ct {
private:
  std::string M_konsole_command;			// How to execute a command in a window.
  std::string M_gdb_bin;				// Path to 'gdb'.

  char const* M_rcname;					// Name of rcfile.
  bool M_env_set;					// Whether or not LIBCWD_RCFILE_NAME is set.
  bool M_read_called;

  bool M_malloc_on;					// Used on rcfile_ct::read.
  bool M_bfd_on;

public:
  /**
   * \brief Construct a rcfile object.
   */
  rcfile_ct() : M_env_set(false), M_read_called(false) { }
  virtual ~rcfile_ct() { }

private:
  void M_print_delayed_msg(void) const;

  static bool S_exists(char const* name);
  std::string M_determine_rcfile_name(void);

  enum action_nt { toggle, on, off };
  void M_process_channel(channel_ct& debugChannel, std::string const& mask, action_nt const action);
  void M_process_channels(std::string list, action_nt const action);

public:
  /**
   * \brief Initialize this object by reading the rcfile.
   */
  void read(void);
  /**
   * \brief Returns the command line string as set with the 'xterm' keyword.
   */
  std::string const& konsole_command(void) const { return M_konsole_command; }
  /**
   * \brief Returns the command line string as set with the 'gdb_bin' keyword.
   */
  std::string const& gdb_bin(void) const { return M_gdb_bin; }
  /**
   * \brief Returns true when this object is initialized.
   */
  bool read_called(void) const { return M_read_called; }

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

extern rcfile_ct rcfile;

/**
 * \brief Calls libcw::debug::rcfile.read().
 *
 * \sa group_rcfile
 */
inline void read_rcfile(void)
{
  rcfile.read();
}

  } // namespace debug
} // namespace libcw

#endif // LIBCWD_CLASS_RCFILE_H
