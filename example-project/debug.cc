#include "sys.h"
#include "debug.h"

#ifdef CWDEBUG

namespace myproject {		// >
  namespace debug {		//  >--> This part must match DEBUGCHANNELS, see debug.h
    namespace channels {	// >

      namespace dc {

        // Add new debug channels here.
	channel_ct custom("CUSTOM");

      } // namespace dc

    } // namespace DEBUGCHANNELS

// Initialize debugging code from new threads.
void init_thread(void)
{
  // Everything below needs to be repeated at the start of every
  // thread function, because every thread starts in a completely
  // reset state with all debug channels off etc.

#if LIBCWD_THREAD_SAFE		// For the non-threaded case this is set by the rcfile.
  // Turn on all debug channels by default.
  ForAllDebugChannels(while(!debugChannel.is_on()) debugChannel.on());
  // Turn off specific debug channels.
  Debug(dc::bfd.off());
  Debug(dc::malloc.off());
#endif

  // Turn on debug output.
  // Only turn on debug output when the environment variable SUPPRESS_DEBUG_OUTPUT is not set.
  Debug(if (getenv("SUPPRESS_DEBUG_OUTPUT") == NULL) libcw_do.on());
#if LIBCWD_THREAD_SAFE
  Debug(libcw_do.set_ostream(&std::cout, &cout_mutex));

  // Set the thread id in the margin.
  char margin[12];
  sprintf(margin, "%-10lu ", pthread_self());
  Debug(libcw_do.margin().assign(margin, 11));
#else
  Debug(libcw_do.set_ostream(&std::cout));
#endif

  // Write a list of all existing debug channels to the default debug device.
  Debug(list_channels_on(libcw_do));
}

// Initialize debugging code from main().
void init(void)
{
  // You want this, unless you mix streams output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  // We can't use it, because other code uses printf to write to the console.
#if 0
  Debug(set_invisible_on());
  std::ios::sync_with_stdio(false);	// Cause "memory leaks" ([w]cin, [w]cout and [w]cerr filebuf allocations).
  Debug(set_invisible_off());
#endif

  // This will warn you when you are using header files that do not belong to the
  // shared libcwd object that you linked with.
  Debug(check_configuration());

#if CWDEBUG_ALLOC
  // Remove all current (pre- main) allocations from the Allocated Memory Overview.
  libcwd::make_all_allocations_invisible_except(NULL);
#endif

  Debug(read_rcfile());

  init_thread();
}

  } // namespace debug
} // namespace myproject

#endif // CWDEBUG
