#include <libcwd/init_functions.h>
#include <libcwd/debug.h>
#include <map>
#include <mutex>

namespace libcwd {
std::mutex cout_mutex;
namespace _private_ {
// Non-const pointer - but do NOT write to it.
// main_thread_tsd is defined in libcwd v1.1.1 and higher (libcwd.so.5.1), or see libcwd github (added 23 apr 2019).
extern ThreadSpecificData* main_thread_tsd;
} // namespace _private_
} // namespace libcwd

namespace libcwd::init_functions {

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
    __Dout(dc::warning, "Calling set_state() more than once for the same label!");
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
    __Dout(dc::warning, "Calling save_dc_states() more than once!");
    return;
  }
  second_time = true;
  ForAllDebugChannels( set_state(debugChannel.get_label(), debugChannel.is_on()) );
}

/**
 * Returns the the original state of a debug channel.
 *
 * @internal
 *
 * For a given @a dc_label, which must be the exact name (<tt>channel_ct::get_label</tt>) of an
 * existing debug channel, this function returns `true` when the corresponding debug channel was
 * <em>on</em> at the startup of the application, directly after reading the libcwd runtime
 * configuration file (.libcwdrc).
 *
 * If the label/channel did not exist at the start of the application, it will return `false`
 * (note that libcwd disallows adding debug channels to modules - so this would probably
 * a bug).
 */
bool is_on_in_rcfile(char const* dc_label)
{
  rcfile_dc_states_type::const_iterator iter = rcfile_dc_states.find(std::string(dc_label));
  if (iter == rcfile_dc_states.end())
  {
    __Dout(dc::warning, "is_on_in_rcfile(\"" << dc_label << "\"): \"" << dc_label << "\" is an unknown label!");
    return false;
  }
  return iter->second;
}

} // namespace

// Initialize this in main once, before starting other threads.
thread_init_t default_thread_init = from_rcfile;
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
void init_thread(std::string thread_name, thread_init_t thread_init)
{
  if (thread_init == thread_init_default)
    thread_init = default_thread_init;
  if (thread_init == from_rcfile)
  {
    // Turn on all debug channels that are turned on as per rcfile configuration.
    ForAllDebugChannels(
        if (!debugChannel.is_on() && is_on_in_rcfile(debugChannel.get_label()))
          debugChannel.on();
    );
  }
  else if (thread_init == copy_from_main)
  {
    // Turn on all debug channels that are turned on in the main thread.
    ForAllDebugChannels(
        if (!debugChannel.is_on() && debugChannel.is_on(*libcwd::_private_::main_thread_tsd))
          debugChannel.on();
    );
    if (!libcwd::libcw_do.is_on(*libcwd::_private_::main_thread_tsd))
      thread_init = debug_off;
  }

  if (thread_init != debug_off)
  {
    // Turn on debug output.
    __Debug(libcw_do.on());
  }

  if (!libcwd::libcw_do.has_mutex())
    libcwd::libcw_do.set_ostream(libcwd::libcw_do.get_ostream(), &libcwd::cout_mutex);

  static bool first_thread = true;
  if (!thread_name.empty())
  {
    std::string margin = thread_name.substr(0, 15) + std::string(16 - std::min(15UL, thread_name.length()), ' ');
    __Debug(libcw_do.margin().assign(margin));
    __Dout(dc::notice, "Thread started. Set debug margin to \"" << margin << "\".");
#ifdef TRACY_ENABLE
    tracy::SetThreadName(thread_name.c_str());
#else
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
    __Debug(libcw_do.margin().assign(margin.str()));
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

#ifdef NO_SYNC_WITH_STDIO_FALSE
#warning "NO_SYNC_WITH_STDIO_FALSE is now the default."
#endif
#ifdef SYNC_WITH_STDIO_FALSE        // By defining this you will no longer synchronize with the standard C streams.
  // Read http://en.cppreference.com/w/cpp/io/ios_base/sync_with_stdio for more information.
  std::ios::sync_with_stdio(false);
#endif

  // This will warn you when you are using header files that do not belong to the
  // shared libcwd object that you linked with.
  __Debug(main_reached());

  __Debug(
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

} // namespace libcwd::init_functions
