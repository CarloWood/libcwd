#include "sys.h"
#include "threads_debug.h"
#include <iostream>
#include <sstream>

#ifdef CWDEBUG
namespace debug_channels {
  namespace dc {
    libcw::debug::channel_ct hello("HELLO");
  }
}

using libcw::debug::thread_index;
#endif

int const loopsize = 1000;
int const number_of_threads = 4;
int const number_of_threads2 = 4;

void* thread_function2(void* arguments)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  Debug( libcw_do.on() );
  Dout(dc::hello, "THIS SHOULD NOT BE PRINTED! (tf2)");
  Debug(dc::hello.on());
  
  return (void*)4;
}

void* thread_function(void* arguments)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_t thread_id[number_of_threads2];
  // Set Thread Specific on/off flags of the debug channels.
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  // And for the debug object.
  Debug( libcw_do.on() );
  char margin[32];
  sprintf(margin, "%-10lu (%04lu) ", pthread_self(), thread_index(pthread_self()));
  Debug( libcw_do.margin().assign(margin, 18) );

  Dout(dc::notice, "Entering thread " << pthread_self() << " (" << thread_index(pthread_self()) << ')');
  int cnt = 0;
  //strstream ss;
  for (int i = 0; i < loopsize; ++i)
  {
    Dout(dc::notice, "Thread " << pthread_self() << " (" << thread_index(pthread_self()) << ") now starting loop " << i);
    Dout(dc::hello, pthread_self() << " (" << thread_index(pthread_self()) << "):" << cnt++ << "; This should be printed");
    Debug(dc::hello.off());
    Debug(dc::hello.off());

    Debug(dc::hello.on());
    for (int j = 0; j < number_of_threads2; ++j)
    {
      Dout(dc::notice|continued_cf, "thread_function: creating thread " << j << ", ");
      pthread_create(&thread_id[j], NULL, thread_function2, NULL);
      Dout(dc::finish, "id " << thread_id[j] << " (" << thread_index(thread_id[j]) << ").");
    }
    Dout(dc::hello, "THIS SHOULD NOT BE PRINTED!!!" << (cnt += loopsize + 1));
    Debug(dc::hello.on());
    for (int j = 0; j < number_of_threads2; ++j)
    {
      void* status;
      pthread_join(thread_id[j], &status);
      Dout(dc::notice, "thread_function: thread " << j << ", id " << thread_id[j] << " (" << thread_index(thread_id[j]) << "), returned with status " << ((bool)status ? "OK" : "ERROR") << '.');
    }
  }
  Dout(dc::notice, "Leaving thread " << pthread_self() << " (" << thread_index(pthread_self()) << ')');
  return (void *)(cnt == loopsize);
}

int main(void)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  Debug( check_configuration() );
#ifdef DEBUGMALLOC
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif
  pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
  Debug( libcw_do.set_ostream(&std::cout, &cout_mutex) );
  Debug( libcw_do.on() );
  char margin[32];
  sprintf(margin, "%-10lu (%04lu) ", pthread_self(), thread_index(pthread_self()));
  Debug( libcw_do.margin().assign(margin, 18) );

  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( list_channels_on(libcw_do) );

  pthread_t thread_id[number_of_threads];
  for (int i = 0; i < number_of_threads; ++i)
  {
    Dout(dc::notice|continued_cf, "main: creating thread " << i << ", ");
    pthread_create(&thread_id[i], NULL, thread_function, NULL);
    Dout(dc::finish, "id " << thread_id[i] << " (" << thread_index(thread_id[i]) << ").");
  }

  for (int i = 0; i < number_of_threads; ++i)
  {
    void* status;
    pthread_join(thread_id[i], &status);
    Dout(dc::notice, "main loop: thread " << i << ", id " << thread_id[i] << " (" << thread_index(thread_id[i]) << "), returned with status " << ((bool)status ? "OK" : "ERROR") << '.');
  }

  Dout(dc::notice, "Exiting from main()");
  return 0;
}
