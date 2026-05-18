/**
 * cwds -- Application-side libcwd support code.
 *
 * @file
 * @brief This file contains the definitions of debug related objects and functions.
 *
 * @Copyright (C) 2016  Carlo Wood.
 *
 * pub   dsa3072/C155A4EEE4E527A2 2018-08-16 Carlo Wood (CarloWood on Libera) <carlo@alinoe.com>
 * fingerprint: 8020 B266 6305 EE2F D53E  6827 C155 A4EE E4E5 27A2
 *
 * This file is part of cwds.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <sys.h>                        // Needed for platform-specific code

#ifdef CWDEBUG

#include <cctype>                       // Needed for std::isprint
#include <cstdio>                       // Needed for sprintf
#include <iomanip>                      // Needed for setfill
#include <map>
#include <string>
#include <sstream>
#include <unistd.h>                     // Needed for pipe
#ifdef USE_LIBCW
#include <libcw/memleak.h>		// memleak_filter
#endif
#ifdef DEBUGGLOBAL
#include "utils/Singleton.h"            // This header is part of git submodule https://github.com/CarloWood/ai-utils
#endif
#ifdef TRACY_ENABLE
#include <common/TracySystem.hpp>
#endif
#include <debug.h>

#if LIBCWD_THREAD_SAFE
namespace libcwd {
pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
namespace _private_ {
// Non-const pointer - but do NOT write to it.
// main_thread_tsd is defined in libcwd v1.1.1 and higher (libcwd.so.5.1), or see libcwd github (added 23 apr 2019).
extern ::libcwd::_private_::TSD_st* main_thread_tsd;
} // namespace _private_
} // namespace libcwd
#endif

NAMESPACE_DEBUG_START

// Anonymous namespace, this map and its initialization functions are private to this file
// for Thead-safeness reasons.
namespace {

/**
 * The type of rcfile_dc_states.
 *
 * @internal
 */
using rcfile_dc_states_type = std::map<std::string, bool>;

/**
 * Map containing the default debug channel states used at the start of each new thread.
 *
 * @internal
 *
 * The first thread calls main, which calls NAMESPACE_DEBUG::init which will initialize this
 * map with all debug channel labels and whether or not they were turned on in the
 * rcfile or not.
 */
rcfile_dc_states_type rcfile_dc_states;

/**
 * Set the default state of debug channel @a dc_label.
 *
 * @internal
 *
 * This function is called once for each debug channel.
 */
void set_state(char const* dc_label, bool is_on)
{
  std::pair<rcfile_dc_states_type::iterator, bool> res =
      rcfile_dc_states.insert(rcfile_dc_states_type::value_type(std::string(dc_label), is_on));
  if (!res.second)
    Dout(dc::warning, "Calling set_state() more than once for the same label!");
  return;
}

/**
 * Save debug channel states.
 *
 * @internal
 *
 * One time initialization function of rcfile_dc_state.
 * This must be called from NAMESPACE_DEBUG::init after reading the rcfile.
 */
void save_dc_states()
{
  // We may only call this function once: it reflects the states as stored
  // in the rcfile and that won't change.  Therefore it is not needed to
  // lock `rcfile_dc_states', it is only written to by the first thread
  // (once, via main -> init) when there are no other threads yet.
  static bool second_time = false;
  if (second_time)
  {
    Dout(dc::warning, "Calling save_dc_states() more than once!");
    return;
  }
  second_time = true;
  ForAllDebugChannels( set_state(debugChannel.get_label(), debugChannel.is_on()) );
}

} // anonymous namespace

/**
 * Returns the the original state of a debug channel.
 *
 * @internal
 *
 * For a given @a dc_label, which must be the exact name (<tt>channel_ct::get_label</tt>) of an
 * existing debug channel, this function returns @c true when the corresponding debug channel was
 * <em>on</em> at the startup of the application, directly after reading the libcwd runtime
 * configuration file (.libcwdrc).
 *
 * If the label/channel did not exist at the start of the application, it will return @c false
 * (note that libcwd disallows adding debug channels to modules - so this would probably
 * a bug).
 */
bool is_on_in_rcfile(char const* dc_label)
{
  rcfile_dc_states_type::const_iterator iter = rcfile_dc_states.find(std::string(dc_label));
  if (iter == rcfile_dc_states.end())
  {
    Dout(dc::warning, "is_on_in_rcfile(\"" << dc_label << "\"): \"" << dc_label << "\" is an unknown label!");
    return false;
  }
  return iter->second;
}

// Initialize this in main once, before starting other threads.
libcwd::thread_init_t thread_init_default = libcwd::from_rcfile;
std::atomic_bool threads_created = ATOMIC_VAR_INIT(false);

/**
 * Initialize debugging code from new threads.
 *
 * This function needs to be called at the start of each new thread,
 * because a new thread starts in a completely reset state.
 *
 * The function turns on all debug channels that were turned on
 * after reading the rcfile at the start of the application.
 * Furthermore it initializes the debug ostream, its mutex and the
 * margin of the default debug object (Dout).
 */
void init_thread(std::string thread_name, libcwd::thread_init_t thread_init)
{
  if (thread_init == libcwd::thread_init_default)
    thread_init = thread_init_default;
  if (thread_init == libcwd::from_rcfile)
  {
    // Turn on all debug channels that are turned on as per rcfile configuration.
    ForAllDebugChannels(
        if (!debugChannel.is_on() && is_on_in_rcfile(debugChannel.get_label()))
          debugChannel.on();
    );
  }
#if LIBCWD_THREAD_SAFE
  else if (thread_init == libcwd::copy_from_main)
  {
    // Turn on all debug channels that are turned on in the main thread.
    ForAllDebugChannels(
        if (!debugChannel.is_on() && debugChannel.is_on(*libcwd::_private_::main_thread_tsd))
          debugChannel.on();
    );
    if (!libcwd::libcw_do.is_on(*libcwd::_private_::main_thread_tsd))
      thread_init = libcwd::debug_off;
  }
#endif

  if (thread_init != libcwd::debug_off)
  {
    // Turn on debug output.
    Debug( libcw_do.on() );
  }

#if LIBCWD_THREAD_SAFE
  if (!libcwd::libcw_do.has_mutex())
    libcwd::libcw_do.set_ostream(libcwd::libcw_do.get_ostream(), &libcwd::cout_mutex);
#endif

  static bool first_thread = true;
  if (!thread_name.empty())
  {
    std::string margin = thread_name.substr(0, 15) + std::string(16 - std::min(15UL, thread_name.length()), ' ');
    Debug(libcw_do.margin().assign(margin));
    Dout(dc::notice, "Thread started. Set debug margin to \"" << margin << "\".");
#ifdef TRACY_ENABLE
    tracy::SetThreadName(thread_name.c_str());
#elif LIBCWD_THREAD_SAFE
    pthread_setname_np(pthread_self(), thread_name.c_str());
#endif
  }
  else if (!first_thread)			// So far, the application has only one thread.  So don't add a thread id.
  {
    std::ostringstream margin;
    union { pthread_t pt; size_t size; } convert;
    convert.pt = pthread_self();
    // Set the thread id in the margin.
    margin << std::hex << std::setw(12) << convert.size << ' ';
    Debug(libcw_do.margin().assign(margin.str()));
    threads_created = true;
  }
  first_thread = false;
}

/**
 * Initialize debugging code from main.
 *
 * This function initializes the debug code.
 */
void init()
{
#ifdef DEBUGGLOBAL
  // This has to be done at the very start of main().
  // By moving this here, the line
  //     Debug(NAMESPACE_DEBUG::init());
  // must be done at the very start of main().
  // If that is a problem then you can start main() with,
  // #ifdef DEBUGGLOBAL
  //   GlobalObjectManager::main_entered();
  // #endif
  // And call init() later when the call here will be skipped.
  if (!Singleton<GlobalObjectManager>::instantiate().is_after_global_constructors())
    GlobalObjectManager::main_entered();
#endif

  // This will warn you when you are using header files that do not belong to the
  // shared libcwd object that you linked with.
  libcwd::main_reached();

#if CWDEBUG_ALLOC && defined(USE_LIBCW)
  // Tell the memory leak detector which parts of the code are
  // expected to leak so that we won't get an alarm for those.
  {
    std::vector<std::pair<std::string, std::string> > hide_list;
    hide_list.push_back(std::pair<std::string, std::string>("libdl.so.2", "_dlerror_run"));
    hide_list.push_back(std::pair<std::string, std::string>("libstdc++.so.6", "__cxa_get_globals"));
    // The following is actually necessary because of a bug in glibc
    // (see http://sources.redhat.com/bugzilla/show_bug.cgi?id=311).
    hide_list.push_back(std::pair<std::string, std::string>("libc.so.6", "dl_open_worker"));
    memleak_filter().hide_functions_matching(hide_list);
  }
  {
    std::vector<std::string> hide_list;
    // Also because of http://sources.redhat.com/bugzilla/show_bug.cgi?id=311
    hide_list.push_back(std::string("ld-linux.so.2"));
    memleak_filter().hide_objectfiles_matching(hide_list);
  }
  memleak_filter().set_flags(libcwd::show_objectfile|libcwd::show_function);
#endif

#ifdef NO_SYNC_WITH_STDIO_FALSE
#warning "NO_SYNC_WITH_STDIO_FALSE is now the default."
#endif
#ifdef SYNC_WITH_STDIO_FALSE        // By defining this you will no longer synchronize with the standard C streams.
  // Read http://en.cppreference.com/w/cpp/io/ios_base/sync_with_stdio for more information.
  std::ios::sync_with_stdio(false);
#endif

  Debug(
    libcw_do.on();		// Show which rcfile we are reading!
    ForAllDebugChannels(
      while (debugChannel.is_on())
        debugChannel.off()	// Print as little as possible though.
    );
    read_rcfile();		// Put 'silent = on' in the rcfile to suppress most of the output here.
    libcw_do.off()
  );
  save_dc_states();

  init_thread();
}

#if CWDEBUG_LOCATION
/**
 * Return call location.
 *
 * @param return_addr The return address of the call.
 */
std::string call_location(void const* return_addr)
{
  libcwd::location_ct loc((char*)return_addr + libcwd::builtin_return_address_offset);
  std::ostringstream convert;
  convert << loc;
  return convert.str();
}
#endif

static int s_being_traced = 0;

void ignore_being_traced()
{
  s_being_traced = 1;
}

// Detect if the application is running inside a debugger.
//
// Usage:
//
// #ifdef CWDEBUG
//   if (NAMESPACE_DEBUG::being_traced())
//     DoutFatal(dc::core, "Trap point");
// #endif
//
bool being_traced()
{
  if (s_being_traced == 1)
    return false;

  std::ifstream sf("/proc/self/status");
  std::string s;
  while (sf >> s)
  {
    if (s == "TracerPid:")
    {
      int pid;
      sf >> pid;
      s_being_traced = (pid != 0) ? 2 : 1;
      return pid != 0;
    }
    std::getline(sf, s);
  }

  s_being_traced = 1;
  return false;
}

NAMESPACE_DEBUG_END

HelperPipeFDs::HelperPipeFDs()
{
  if (pipe(m_pipefd) == -1)
  {
    perror("pipe");
    exit(1);
  }
}

std::string DebugPipedOStringStream::str()
{
  std::string result{std::istreambuf_iterator<char>(ibuf()), std::istreambuf_iterator<char>()};
  if (result.back() == '\n')
    result.pop_back();
  return result;
}

NAMESPACE_DEBUG_CHANNELS_START
channel_ct tracked("TRACKED");
channel_ct system("SYSTEM");    // Intended to be used for system calls.
channel_ct restart("RESTART");   // Used by debug::Restart.
NAMESPACE_DEBUG_CHANNELS_END

#endif // CWDEBUG
