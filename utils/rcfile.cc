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

#include "sys.h"
#include "cwd_debug.h"
#include "match.h"
#include <iostream>
#include <cstdlib>
#include <fstream>

#include <sys/types.h>	// Needed for 'stat', 'getuid' and 'getpwuid'.
#include <sys/stat.h>	// Needed for 'stat'.
#include <unistd.h>	// Needed for 'stat', 'access' and 'getuid'.
#include <pwd.h>	// Needed for 'getpwuid'.

namespace libcwd {

  namespace channels {
    namespace dc {
      channel_ct rcfile("RCFILE", false);
    }
  }

bool rcfile_ct::S_exists(char const* name)
{
  struct stat buf;
  if (stat(name, &buf) == -1 || !S_ISREG(buf.st_mode))
    return false;
  if (access(name, R_OK) == -1)
    DoutFatal(dc::fatal|error_cf, "read_rcfile: " << name);
  return true;
}

void rcfile_ct::M_print_delayed_msg(void) const
{
  Dout(dc::rcfile, "Using environment variable LIBCWD_RCFILE_NAME with value \"" << M_rcname << "\".");
}

std::string rcfile_ct::M_determine_rcfile_name(void)
{
  // Can be overridden with the environment variable LIBCWD_RCFILE_NAME
  if (!(M_rcname = getenv("LIBCWD_RCFILE_NAME")))
    M_rcname = ".libcwdrc";					// Default rcfile name.
  else
    M_env_set = true;
  std::string rcfile_name;
  // Does this file exist in the current directory?
  if (S_exists(M_rcname))
    rcfile_name = M_rcname;
  else
  {
    // Does it exist in $HOME/?
    struct passwd* pwent = getpwuid(getuid());
    rcfile_name = pwent->pw_dir;
    rcfile_name += '/';
    rcfile_name += M_rcname;
    if (!S_exists(rcfile_name.c_str()))
    {
      if (M_env_set)
      {
        M_print_delayed_msg();
	DoutFatal(dc::fatal, "read_rcfile: Could not read $LIBCWD_RCFILE_NAME (\"" << M_rcname <<
            "\") from either \".\" or \"" << pwent->pw_dir << "\".");
      }
      else
      {
	// Fall back to $datadir/libcwd/libcwdrc
	rcfile_name = CW_DATADIR "/libcwdrc";
	if (!S_exists(rcfile_name.c_str()))
	  DoutFatal(dc::fatal, "read_rcfile: Could not read rcfile \"" << M_rcname <<
	      "\" from either \".\" or \"" << pwent->pw_dir <<
	      "\" and could not read default rcfile \"" << rcfile_name << "\" either!");
	else
	{
	  bool warning_on = channels::dc::warning.is_on();
	  if (!warning_on)
	    channels::dc::warning.on();
	  Dout(dc::warning, "Neither ./" << M_rcname << " nor " << pwent->pw_dir << '/' << M_rcname << " exist.");
          Dout(dc::warning, "Using default rcfile \"" << rcfile_name << "\".");
	  if (!warning_on)
	    channels::dc::warning.off();
	}
      }
    }
  }
  return rcfile_name;
}

void rcfile_ct::M_process_channel(channel_ct& debugChannel, std::string const& mask, action_nt const action)
{
  std::string label = debugChannel.get_label();
  std::string::size_type pos = label.find(' ');
  if (pos != std::string::npos)
    label.erase(pos);
  std::transform(label.begin(), label.end(), label.begin(), (int(*)(int)) toupper);
  if (_private_::match(mask.data(), mask.length(), label.c_str()))
  {
    if (label == "MALLOC")
    {
      if (!M_malloc_on && (action == on || action == toggle))
      {
	M_malloc_on = true;
	Dout(dc::rcfile, "Turned on MALLOC");
      }
      else if (M_malloc_on && (action == off || action == toggle))
      {
        M_malloc_on = false;
	debugChannel.off();
	Dout(dc::rcfile, "Turned off MALLOC");
      }
    }
    else if (label == "BFD")
    {
      if (!M_bfd_on && (action == on || action == toggle))
      {
	M_bfd_on = true;
	Dout(dc::rcfile, "Turned on BFD");
      }
      else if (M_bfd_on && (action == off || action == toggle))
      {
        M_bfd_on = false;
	debugChannel.off();
	Dout(dc::rcfile, "Turned off BFD");
      }
    }
    else if (!debugChannel.is_on() && (action == on || action == toggle))
    {
      do
      {
	debugChannel.on();
	Dout(dc::rcfile, "Turned on " << label);
      }
      while (!debugChannel.is_on());
    }
    else if (debugChannel.is_on() && (action == off || action == toggle))
    {
      debugChannel.off();
      Dout(dc::rcfile, "Turned off " << label);
    }
  }
}

void rcfile_ct::M_process_channels(std::string list, action_nt const action)
{
  Debug( libcw_do.inc_indent(4) );
  while(list.length())
  {
    int pos = list.find_first_not_of(", \t\n\v");
    if (pos == -1)
      break;
    list.erase(0, pos);
    pos = list.find_first_of(", \t\n\v");
    std::string mask = list;
    if (pos != -1)
      mask.erase(pos);
    std::transform(mask.begin(), mask.end(), mask.begin(), (int(*)(int)) toupper);
    ForAllDebugChannels( M_process_channel(debugChannel, mask, action) );
    if (pos == -1)
      break;
    list.erase(0, pos);
  }
  Debug( libcw_do.dec_indent(4) );
}

bool rcfile_ct::unknown_keyword(std::string const&, std::string const&)
{
  return true;
}

void rcfile_ct::read(void)
{
  Debug( while(!dc::rcfile.is_on()) dc::rcfile.on() );
  std::string name = M_determine_rcfile_name();
  std::ifstream rc;
  rc.open(name.c_str(), std::ios_base::in);
  std::string line;
  int lines_read = 0;
  int channels_default_set = 0;
  bool syntax_error = false;
  M_malloc_on = libcwd::channels::dc::malloc.is_on();
  M_bfd_on = libcwd::channels::dc::bfd.is_on();
  while(getline(rc, line))
  {
    lines_read++;
    line.erase(0, line.find_first_not_of(" \t\n\v"));
    line.erase(line.find_last_not_of(" \t\n\v") + 1); 
    if ((line.find_first_of('#') > 0) && (line.length() != 0))
    {
      if (line.find_first_of("=") == std::string::npos || line[0] == '=')
      {
        syntax_error = true;
	break;
      }
      std::string keyword = line;
      keyword.erase(line.find_first_of(" \t\n\v="), line.length() - 1);
      std::string value = line;
      value.erase(0, line.find_first_of ("=") + 1);
      value.erase(0, value.find_first_not_of(" \n\t\v"));
      value.erase(value.find_last_not_of(" \t\n\v") + 1);
      if (keyword == "silent")
      {
        if (value == "on")
	  Debug( if (dc::rcfile.is_on()) dc::rcfile.off() );
	else if (value == "off")
	  Debug( while(!dc::rcfile.is_on()) dc::rcfile.on() );
	continue;
      }
      if (M_env_set)
      {
        M_print_delayed_msg();
	M_env_set = false;	// Don't print message again.
      }
      Dout(dc::rcfile, name << ':' << lines_read << ": " << keyword << " = " << value);
      if (keyword == "gdb")
        M_gdb_bin = value;
      else if (keyword == "xterm")
        M_konsole_command = value;
      else if (keyword == "channels_default")
      {
        if (channels_default_set)
	{
	  bool warning_on = channels::dc::warning.is_on();
	  if (!warning_on)
	    channels::dc::warning.on();
	  Dout(dc::warning, "rcfile: " << name << ':' << lines_read <<
	      ": channels_default already set in line " << channels_default_set << "!  Entry ignored!");
	  if (!warning_on)
	    channels::dc::warning.off();
	  continue;
	}
	channels_default_set = lines_read;
        if (value == "on")
	{
	  ForAllDebugChannels(
	    std::string label = debugChannel.get_label();
	    std::string::size_type pos = label.find(' ');
	    if (pos != std::string::npos)
	      label.erase(pos);
	    while (!debugChannel.is_on() && label != "MALLOC" && label != "BFD")
	      debugChannel.on()
	  );
	  M_malloc_on = M_bfd_on = true;
	}
        else if (value == "off")
	{
	  ForAllDebugChannels(
	    if (debugChannel.is_on())
	      debugChannel.off()
	  );
	  M_malloc_on = M_bfd_on = false;
	}
	else
	{
	  syntax_error = true;
	  break;
	}
      }
      else if (keyword == "channels_toggle")
      {
        if (!channels_default_set)
	  DoutFatal(dc::fatal, "read_rcfile: " << name << ':' << lines_read <<
	      ": channels_toggle used before channels_default.");
        M_process_channels(value, toggle);
      }
      else if (keyword == "channels_on")
        M_process_channels(value, on);
      else if (keyword == "channels_off")
        M_process_channels(value, off);
      else if (unknown_keyword(keyword, value))
      {
	bool warning_on = channels::dc::warning.is_on();
	if (!warning_on)
	  channels::dc::warning.on();
        Dout(dc::warning, "read_rcfile: " << name << ':' << lines_read << ": Unknown keyword '" << keyword << "'.");
	if (!warning_on)
	  channels::dc::warning.off();
      }
    }
  }
  if (syntax_error)
    DoutFatal(dc::fatal, "read_rcfile: " << name << ':' << lines_read << ": syntax error.");
  rc.close();
  Debug(dc::rcfile.off());
  if (M_malloc_on)
    while (!channels::dc::malloc.is_on())
      channels::dc::malloc.on();
  if (M_bfd_on)
    while (!channels::dc::bfd.is_on())
      channels::dc::bfd.on();
  M_read_called = true;
}

rcfile_ct rcfile;

} // namespace libcwd
