#include "sys.h"
#include <pthread.h>
#include "threads_debug.h"

RCSTAG_CC("$Id$")

int const number_of_threads = 4;
static int volatile state_thread[number_of_threads];

char const* in_the_middle(int my_id)
{
  if (my_id != number_of_threads - 1)
  {
    Dout(dc::notice, my_id << ": waiting for thread " << my_id + 1);
    while(state_thread[my_id + 1] == 0);
  }
  state_thread[my_id] = 1;
  return "in the middle of a Dout.";
}


void* thread_function(void* arguments)
{
  static pthread_mutex_t	thread_counter_mutex	= PTHREAD_MUTEX_INITIALIZER;
  static int 			thread_counter		= -1;

  // Serialize incrementation.
  pthread_mutex_lock(&thread_counter_mutex);
  int my_id = ++thread_counter;
  pthread_mutex_unlock(&thread_counter_mutex);

  state_thread[my_id] = 0;
  Dout(dc::notice, my_id << ": Entering thread " << pthread_self());
  Dout(dc::notice, my_id << ": Here we are " << in_the_middle(my_id));
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
