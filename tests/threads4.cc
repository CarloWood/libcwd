#include "sys.h"
#include "threads_debug.h"
#include <libcw/threading.h>
#include <iostream>
#include <sstream>

#ifdef CWDEBUG
namespace debug_channels {
  namespace dc {
    libcw::debug::channel_ct hello("HELLO");
  }
}
#endif

int const number_of_threads = 4;
int const number_of_threads2 = 4;

void* thread_function2(void* arguments)
{
  Dout(dc::hello, "THIS SHOULD NOT BE PRINTED! (tf2)");
  Debug(dc::hello.on());
  
  return (void*)4;
}

void* thread_function(void* arguments)
{
  pthread_t thread_id[number_of_threads2];
  // Set Thread Specific on/off flags of the debug channels.
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );

  std::cout << "COUT:Entering thread " << pthread_self() << ":COUT\n";
  Dout(dc::notice, "Entering thread " << pthread_self());
  int cnt = 0;
  //strstream ss;
  for (int i = 0; i < 1000; ++i)
  {
    Dout(dc::hello, pthread_self() << ':' << cnt++ << "; This should be printed");
    Debug(dc::hello.off());
    Debug(dc::hello.off());

    Debug(dc::hello.on());
    for (int j = 0; j < number_of_threads2; ++j)
      pthread_create(&thread_id[i], NULL, thread_function2, NULL);
    Dout(dc::hello, "THIS SHOULD NOT BE PRINTED!!!" << (cnt += 2000));
    Debug(dc::hello.on());
    for (int i = 0; i < number_of_threads; ++i)
    {
      void* status;
      pthread_join(thread_id[i], &status);
      Dout(dc::notice, "Thread " << thread_id[i] << " returned with status " << ((bool)status ? "OK" : "ERROR") << '.');
    }
  }
  return (void *)(cnt == 1000);
}

int main(void)
{
  Debug( check_configuration() );
#ifdef DEBUGMALLOC
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif
  Debug( libcw_do.set_ostream(&std::cout) );
  Debug( libcw_do.on() );

  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( list_channels_on(libcw_do) );

  pthread_t thread_id[number_of_threads];
  for (int i = 0; i < number_of_threads; ++i)
    pthread_create(&thread_id[i], NULL, thread_function, NULL);

  for (int i = 0; i < number_of_threads; ++i)
  {
    void* status;
    pthread_join(thread_id[i], &status);
    Dout(dc::notice, "Thread " << thread_id[i] << " returned with status " << ((bool)status ? "OK" : "ERROR") << '.');
  }
  
  return 0;
}