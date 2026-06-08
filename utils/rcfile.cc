#include "cwd_sys.h"
#include "match.h"
#include "libcwd/debug.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/stat.h> // Needed for 'stat'.
#include <sys/types.h> // Needed for 'stat', 'getuid' and 'getpwuid'.
#include <unistd.h> // Needed for 'stat', 'access' and 'getuid'.
#include "cwd_debug.h"
#ifdef HAVE_PWD_H
#include <pwd.h> // Needed for 'getpwuid'.
#endif

namespace libcwd {

namespace channels {
namespace dc {
Channel rcfile("RCFILE", false);
}
} // namespace channels

bool RcFile::S_exists(char const* name)
{
  struct stat buf;
  if (stat(name, &buf) == -1 || !S_ISREG(buf.st_mode))
    return false;
  if (access(name, R_OK) == -1)
    DoutFatal(dc::fatal | error_cf, "read_rcfile: " << name);
  return true;
}

void RcFile::M_print_delayed_msg(int env_var, std::string const& value) const
{
  Dout(dc::rcfile, "Using environment variable "
                       << (env_var == 0 ? "LIBCWD_RCFILE_NAME" : "LIBCWD_RCFILE_OVERRIDE_NAME") << " with value \""
                       << value << "\".");
}

std::string RcFile::M_determine_rcfile_name()
{
  // Can be overridden with the environment variable LIBCWD_RCFILE_NAME
  if (!(M_rcname = getenv("LIBCWD_RCFILE_NAME")))
    M_rcname = ".libcwdrc"; // Default rcfile name.
  else
    M_env_set = true;
  std::string rcfile_name;
  // Does this file exist in the current directory?
  if (S_exists(M_rcname))
    rcfile_name = M_rcname;
  else
  {
    // Does it exist in $HOME/?
    char const* homedir = nullptr;

    // Prefer $HOME when it exists, is non-empty and already contains the rcfile.
    if (char const* env_home = getenv("HOME"); env_home && env_home[0])
    {
      std::string env_candidate = env_home;
      env_candidate += '/';
      env_candidate += M_rcname;
      if (S_exists(env_candidate.c_str()))
      {
        homedir = env_home;
        rcfile_name = std::move(env_candidate);
      }
    }

    // Otherwise fall back to the passwd entry for this uid.
    if (rcfile_name.empty())
    {
#ifdef HAVE_PWD_H
      if (struct passwd* pwent = getpwuid(getuid()))
        homedir = pwent->pw_dir;
#else
      homedir = getenv("HOME");
#endif
      if (homedir)
      {
        rcfile_name = homedir;
        rcfile_name += '/';
        rcfile_name += M_rcname;
      }
    }
    if (!homedir || !S_exists(rcfile_name.c_str()))
    {
      if (!homedir)
        homedir = "$HOME";
      if (M_env_set)
      {
        M_print_delayed_msg(0, M_rcname);
        DoutFatal(dc::fatal, "read_rcfile: Could not read $LIBCWD_RCFILE_NAME (\""
                                 << M_rcname << "\") from either \".\" or \"" << homedir << "\".");
      }
      else
      {
        // Fall back to $datadir/libcwd/libcwdrc
        rcfile_name = CW_DATADIR "/libcwd/libcwdrc";
        if (!S_exists(rcfile_name.c_str()))
          DoutFatal(dc::fatal, "read_rcfile: Could not read rcfile \""
                                   << M_rcname << "\" from either \".\" or \"" << homedir
                                   << "\" and could not read default rcfile \"" << rcfile_name << "\" either!");
        else
        {
          bool warning_on = channels::dc::warning.is_on();
          if (!warning_on)
            channels::dc::warning.on();
          Dout(dc::warning, "Neither ./" << M_rcname << " nor " << homedir << '/' << M_rcname << " exist.");
          Dout(dc::warning, "Using default rcfile \"" << rcfile_name << "\".");
          if (!warning_on)
            channels::dc::warning.off();
        }
      }
    }
  }
  return rcfile_name;
}

void RcFile::M_process_channel(Channel& debugChannel, std::string const& mask, action_nt const action)
{
  std::string label = debugChannel.get_label();
  std::string::size_type pos = label.find(' ');
  if (pos != std::string::npos)
    label.erase(pos);
  std::transform(label.begin(), label.end(), label.begin(), (int (*)(int))toupper);
  if (_private_::match(mask.data(), mask.length(), label.c_str()))
  {
#if CWDEBUG_LOCATION
    if (label == "ELFUTILS")
    {
      if (!M_elfutils_on && (action == on || action == toggle))
      {
        M_elfutils_on = true;
        Dout(dc::rcfile, "Turned on ELFUTILS");
      }
      else if (M_elfutils_on && (action == off || action == toggle))
      {
        M_elfutils_on = false;
        debugChannel.off();
        Dout(dc::rcfile, "Turned off ELFUTILS");
      }
    }
#endif
    else if (!debugChannel.is_on() && (action == on || action == toggle))
    {
      do
      {
        debugChannel.on();
        Dout(dc::rcfile, "Turned on " << label);
      } while (!debugChannel.is_on());
    }
    else if (debugChannel.is_on() && (action == off || action == toggle))
    {
      debugChannel.off();
      Dout(dc::rcfile, "Turned off " << label);
    }
  }
}

void RcFile::M_process_channels(std::string list, action_nt const action)
{
  Debug(libcw_do.inc_indent(4));
  while (list.length())
  {
    int pos = list.find_first_not_of(", \t\n\v");
    if (pos == -1)
      break;
    list.erase(0, pos);
    pos = list.find_first_of(", \t\n\v");
    std::string mask = list;
    if (pos != -1)
      mask.erase(pos);
    std::transform(mask.begin(), mask.end(), mask.begin(), (int (*)(int))toupper);
    ForAllDebugChannels(M_process_channel(debugChannel, mask, action));
    if (pos == -1)
      break;
    list.erase(0, pos);
  }
  Debug(libcw_do.dec_indent(4));
}

bool RcFile::unknown_keyword(std::string const&, std::string const&)
{
  return true;
}

void RcFile::set_all_channels_on()
{
  Dout(dc::rcfile, "Turning all channels on by default.");
  ForAllDebugChannels(std::string label = debugChannel.get_label(); std::string::size_type pos = label.find(' ');
                      if (pos != std::string::npos) label.erase(pos);
                      while (!debugChannel.is_on() && label != "ELFUTILS") debugChannel.on());
#if CWDEBUG_LOCATION
  M_elfutils_on = true;
#endif
}

void RcFile::set_all_channels_off(bool warning_on)
{
  Dout(dc::rcfile, "Turning all channels" << (warning_on ? ", except WARNING," : "") << " off by default.");
  ForAllDebugChannels(if (debugChannel.is_on()) debugChannel.off());
  if (warning_on)
    channels::dc::warning.on();
#if CWDEBUG_LOCATION
  M_elfutils_on = false;
#endif
}

void RcFile::read()
{
  Channel::OnOffState state;
  channels::dc::rcfile.force_on(state, channels::dc::rcfile.get_label());
  bool rcfile_on = true;
  std::string name = M_determine_rcfile_name();
  std::ifstream rc;

#if CWDEBUG_LOCATION
  M_elfutils_on = libcwd::channels::dc::elfutils.is_on();
#endif

  std::array<int, 2> channels_default_set = {0, 0};
  bool default_is_on;
  for (int rcfiles = 0; rcfiles < 2; ++rcfiles) // Allow to jump back for LIBCWD_RCFILE_OVERRIDE_NAME.
  {
    rc.open(name.c_str(), std::ios_base::in);
    std::string line;
    int lines_read = 0;
    int did_reset_channels_on_channels_on_or_off = 0;
    bool syntax_error = false;
    while (getline(rc, line))
    {
      ++lines_read;
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
        value.erase(0, line.find_first_of("=") + 1);
        value.erase(0, value.find_first_not_of(" \n\t\v"));
        value.erase(value.find_last_not_of(" \t\n\v") + 1);
        if (keyword == "silent")
        {
          if (value == "on")
            Debug(if (dc::rcfile.is_on()) {
              dc::rcfile.off();
              rcfile_on = false;
            });
          else if (value == "off")
            Debug(if (!dc::rcfile.is_on()) {
              dc::rcfile.on();
              rcfile_on = true;
            });
          continue;
        }
        if (M_env_set)
        {
          M_print_delayed_msg(rcfiles, name);
          M_env_set = false; // Don't print message again.
        }
        Dout(dc::rcfile, name << ':' << lines_read << ": " << keyword << " = " << value);
        if (keyword == "gdb")
          M_gdb_bin = value;
        else if (keyword == "xterm")
          M_konsole_command = value;
        else if (keyword == "channels_default")
        {
          if (channels_default_set[rcfiles])
          {
            bool warning_on = channels::dc::warning.is_on();
            if (!warning_on)
              channels::dc::warning.on();
            Dout(dc::warning, "rcfile: " << name << ':' << lines_read << ": channels_default already set in line "
                                         << channels_default_set[rcfiles] << "!  Entry ignored!");
            if (!warning_on)
              channels::dc::warning.off();
            continue;
          }
          channels_default_set[rcfiles] = lines_read;
          if (value == "on")
          {
            set_all_channels_on();
            default_is_on = true;
          }
          else if (value == "off")
          {
            set_all_channels_off(false);
            default_is_on = false;
          }
          else
          {
            syntax_error = true;
            break;
          }
        }
        else if (keyword == "channels_toggle")
        {
          if (!channels_default_set[rcfiles] && !channels_default_set[0])
            DoutFatal(dc::fatal, "read_rcfile: " << name << ':' << lines_read
                                                 << ": channels_toggle used before channels_default.");
          M_process_channels(value, toggle);
        }
        else if (keyword == "channels_on")
        {
          if (!did_reset_channels_on_channels_on_or_off && rcfiles == 1)
          {
            bool saw_default = channels_default_set[0] != 0 || channels_default_set[1] != 0;
            if (!saw_default || default_is_on)
            {
              set_all_channels_off(!saw_default);
              did_reset_channels_on_channels_on_or_off = 1;
            }
          }
          M_process_channels(value, on);
        }
        else if (keyword == "channels_off")
        {
          if (!did_reset_channels_on_channels_on_or_off && rcfiles == 1)
          {
            bool saw_default = channels_default_set[0] != 0 || channels_default_set[1] != 0;
            if (saw_default && !default_is_on)
            {
              set_all_channels_on();
              did_reset_channels_on_channels_on_or_off = 1;
            }
          }
          M_process_channels(value, off);
        }
        else if (unknown_keyword(keyword, value))
        {
          Channel::OnOffState warning_state;
          channels::dc::warning.force_on(warning_state, channels::dc::warning.get_label());
          Dout(dc::warning, "read_rcfile: " << name << ':' << lines_read << ": Unknown keyword '" << keyword << "'.");
          channels::dc::warning.restore(warning_state);
        }
      }
    }
    if (syntax_error)
      DoutFatal(dc::fatal, "read_rcfile: " << name << ':' << lines_read << ": syntax error.");
    rc.close();

    char const* override_name;
    if (!(override_name = getenv("LIBCWD_RCFILE_OVERRIDE_NAME")))
      break;
    else
      name = override_name;

    // Handle environment variable LIBCWD_RCFILE_OVERRIDE_NAME.
    M_env_set = true;
  }

  // Must balance calls of off with on before calling restore.
  if (!rcfile_on)
    channels::dc::rcfile.on();
  channels::dc::rcfile.restore(state);
#if CWDEBUG_LOCATION
  if (M_elfutils_on)
    while (!channels::dc::elfutils.is_on()) channels::dc::elfutils.on();
#endif
  M_read_called = true;
}

RcFile rcfile;

} // namespace libcwd
