/* Add this at the top of your main() to have a quick start.	*/
/* Then remove what you don't need.				*/

  // You want this, unless you mix streams output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  std::ios::sync_with_stdio(false);

  // This will warn you when you are using header files that do not belong to the
  // shared libcwd object that you linked with.
  Debug( check_configuration() );

#if CWDEBUG_ALLOC
  // Remove all current (pre- main) allocations from the Allocated Memory Overview.
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif

  //-------------------------------------------------------------------------------
  // THREADS:
  // Everything below needs to be repeated at the start of every
  // thread function, because every thread starts in a completely
  // reset state with all debug channels off etc.

  // Turn on specific debug channels.
  Debug( dc::custom.on() );
  // OR
  // Turn on all debug channels.
  ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() );
  // Turn off specific debug channels.
  Debug( dc::bfd.off() ); 
  Debug( dc::malloc.off() ); 

  // Turn on debug output.
  Debug( libcw_do.on() );
  // OR
  // Only turn on debug output when the environment variable SUPPRESS_DEBUG_OUTPUT is not set.
  Debug( if (getenv("SUPPRESS_DEBUG_OUTPUT") == NULL) libcw_do.on() );

#ifndef _REENTRANT
  // Write all following debug output to std::cout.
  Debug( libcw_do.set_ostream(&std::cout) );
#else // _REENTRANT
  // THREADS: OR if you are using threads
  Debug( libcw_do.set_ostream(&std::cout, &your_cout_mutex) );
  // `your_cout_mutex' is the same lock that you already use for cout,
  // if you didn't write to cout yet (and thus don't have a lock)
  // add this as _global_ mutex before main():
  // pthread_mutex_t your_cout_mutex = PTHREAD_MUTEX_INITIALIZER;

  // THREADS: If you are using threads then you want to set a margin.
#ifdef CWDEBUG
  char margin[12];
  sprintf(margin, "%-10lu ", pthread_self());
  Debug( libcw_do.margin().assign(margin, 11) );
#endif
#endif // _REENTRANT

  // Optionally, write a list of all existing debug channels to the
  // default debug device (libcw_do) (and thus to cout in this case).
  Debug( list_channels_on(libcw_do) );

