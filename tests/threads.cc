#include "sys.h"
#include <pthread.h>
#include "threads_debug.h"

RCSTAG_CC("$Id$")

int const number_of_threads = 4;
static int thread_counter = -1;
static int volatile state_thread[number_of_threads];

char const* in_the_middle(void)
{
  if (thread_counter != number_of_threads - 1)
  {
    Dout(dc::notice, thread_counter << ": waiting for thread " << thread_counter + 1);
    while(state_thread[thread_counter + 1] == 0);
  }
  state_thread[thread_counter] = 1;
  return "in the middle of a Dout.";
}

void* thread_function(void* arguments)
{
  ++thread_counter;
  state_thread[thread_counter] = 0;
  Dout(dc::notice, thread_counter << ": Entering thread " << pthread_self());
  Dout(dc::notice, thread_counter << ": Here we are " << in_the_middle());
  return NULL;
}
 
int main(void)
{
  Debug( check_configuration() );
#ifdef DEBUGMALLOC
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( libcw_do.set_ostream(&std::cout) );
  Debug( libcw_do.on() );
  Debug( list_channels_on(libcw_do) );

  pthread_t thread_id[number_of_threads];
  for (int i = 0; i < number_of_threads; ++i)
    pthread_create(&thread_id[i], NULL, thread_function, NULL);

  for (int i = 0; i < number_of_threads; ++i)
  {
    void* status;
    pthread_join(thread_id[i], &status);
    Dout(dc::notice, "Thread " << thread_id[i] << " returned with status " << status << '.');
  }
  
  return 0;
}
