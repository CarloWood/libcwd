#ifdef TWDEBUG
#include <libtw/sysd.h>
#include <libtw/debug.h>

#define LIBTWD_LEAKTEST \
    struct timeval end; \
    gettimeofday(&end, 0); \
    static int last[32]; \
    int ti = thread_index(pthread_self()); \
    if (last[ti] != end.tv_sec) \
    { \
      end.tv_sec -= 10; \
      last[ti] = end.tv_sec; \
      leak_filter.set_time_interval(start, end); \
      Debug2( leak_do.on() ); \
      Debug2( TWINCHANNELS::dc::malloc.on() ); \
      Debug2( list_allocations_on(leak_do, leak_filter) ); \
      LibtwDout2(TWINCHANNELS, leak_do, TWINCHANNELS::dc::malloc|libtw::debug::flush_cf, "...flushing..."); \
      Debug2( TWINCHANNELS::dc::malloc.off() ); \
      Debug2( leak_do.off() ); \
    }
#else
#define LIBTWD_LEAKTEST
#endif

#define PREFIX_CODE set_margin(); int __res; for(int __i = 0; __i < 10; ++__i) {
#define EXIT(res) \
    __res = (res); \
    ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() ); \
    ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() ); \
    { \
      LIBCWD_TSD_DECLARATION; \
      ForAllDebugObjects( while (LIBCWD_DO_TSD_MEMBER_OFF(debugObject) >= 0) debugObject.on() ); \
      ForAllDebugObjects( if (LIBCWD_DO_TSD_MEMBER_OFF(debugObject) < 0) debugObject.off() ); \
    } \
    ++heartbeat[thread_index(pthread_self())]; \
    pthread_testcancel(); \
    LIBTWD_LEAKTEST; \
    if (__res) break; } \
    heartbeat[thread_index(pthread_self())] = 0; \
    return (void*)(__res == 0)
#define THREADED(x) x
#define COMMA_THREADED(x) , x
#define THREADTEST

#include "sys.h"
#include "debug.h"
#include <stdio.h>	// Needed for sprintf()
#include <unistd.h>
#ifdef TWDEBUG
#include <fstream>
#endif

pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cerr_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile int heartbeat[32];
int prev_heartbeat[32];
int bad[32];

unsigned long thread_index(pthread_t tid)
{
  return tid % 1024;
}

void set_margin(void)
{
  char margin[32];
  sprintf(margin, "%-10lu (%04lu) ", pthread_self(), thread_index(pthread_self()));
  Debug( libcw_do.margin().assign(margin, 18) );
}

#ifdef TWDEBUG
// Special debug object to print memory leaks to.
libtw::debug::debug_ct leak_do;

struct timeval start;
libtw::debug::ooam_filter_ct leak_filter(libtw::debug::show_time);
#endif

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* basic_prog(void*)
#include "../libcwd.tst/basic.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* alloctag_prog(void*)
#include "../libcwd.tst/alloctag.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* location_prog(void*)
#include "../libcwd.tst/location.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* cf_prog(void*)
#include "../libcwd.tst/cf.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* continued_prog(void*)
#include "../libcwd.tst/continued.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* dc_prog(void*)
#include "../libcwd.tst/dc.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* demangler_prog(void*)
#include "../libcwd.tst/demangler.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* dlopen_prog(void*)
#include "../libcwd.tst/dlopen.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* do_prog(void*)
#include "../libcwd.tst/do.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* flush_prog(void*)
#include "../libcwd.tst/flush.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* leak_prog(void*)
#include "../libcwd.tst/leak.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* lockable_auto_ptr_prog(void*)
#include "../libcwd.tst/lockable_auto_ptr.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* magic_prog(void*)
#include "../libcwd.tst/magic.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* marker_prog(void*)
#include "../libcwd.tst/marker.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* strdup_prog(void*)
#include "../libcwd.tst/strdup.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* test_delete_prog(void*)
#include "../libcwd.tst/test_delete.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* type_info_prog(void*)
#include "../libcwd.tst/type_info.cc"

#undef MAIN_FUNCTION
#define MAIN_FUNCTION void* filter_prog(void*)
#include "../libcwd.tst/filter.cc"

typedef void* (*thread_func_t)(void*);
thread_func_t progs[] = { alloctag_prog, basic_prog, location_prog, cf_prog, /*continued_prog,*/ dc_prog,
    demangler_prog, dlopen_prog, do_prog, flush_prog, leak_prog, lockable_auto_ptr_prog, /*magic_prog,*/
    marker_prog, strdup_prog, test_delete_prog, type_info_prog, filter_prog };
int const number_of_threads = sizeof(progs)/sizeof(thread_func_t);

#ifdef TWDEBUG
pthread_mutex_t leak_mutex;
#endif

extern int raise (int sig);

int main(void)
{
  Debug( check_configuration() );

#if CWDEBUG_ALLOC
  new int;
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif

#ifdef TWDEBUG
  std::ofstream leakout;
  leakout.open("leakout");
#endif

  Debug( libcw_do.set_ostream(&std::cout, &cout_mutex) );
#ifdef TWDEBUG
  Debug2( leak_do.set_ostream(&leakout, &leak_mutex) );
#endif

  set_margin();

#ifndef TWDEBUG
  Debug( dc::notice.on() );
  Debug( libcw_do.on() );
#endif

  for (int i = 0; i < number_of_threads; ++i)
    ++heartbeat[i + 2];

#ifdef TWDEBUG
  gettimeofday(&start, 0);
  start.tv_sec += 10;

  progs[6](0);
  exit(0);
#endif

  pthread_t thread_id[number_of_threads];
  for (int i = 0; i < number_of_threads; ++i)
  {
    Dout(dc::notice|continued_cf, "main: creating thread " << i << ", ");
    pthread_create(&thread_id[i], NULL, progs[i], NULL);
    Dout(dc::finish, "id " << thread_id[i] << " (" << thread_index(thread_id[i]) << ").");
  }

  for(;;)
  {
    memcpy(prev_heartbeat, (void*)(heartbeat), sizeof(heartbeat));
    struct timespec rqts = { 1, 0 };
    struct timespec rmts;
    nanosleep(&rqts, &rmts);
    std::cerr << "\nHEARTBEAT --start of new check-----------------------------\n";
    int running = 0;
    for (int i = 0; i < number_of_threads; ++i)
    {
      int ti = thread_index(thread_id[i]);
      if (heartbeat[ti] == -1)
	continue;
      if (heartbeat[ti] == 0)
      {
	void* status;
	pthread_join(thread_id[i], &status);
	Dout(dc::notice, "main loop: thread " << i << ", id " << thread_id[i] << " (" << ti << "), returned with status " << ((bool)status ? "OK" : "ERROR") << '.');
	++running;
	heartbeat[ti] = -1;
      }
      else if (prev_heartbeat[ti] == heartbeat[ti])
      {
	if (++(bad[i]) == 30)
	{
	  std::cerr << "No heartbeat for thread " << thread_id[i] << '/' <<
	    libcw::debug::_private_::__libcwd_tsd_array[ti].pid << '\n';
	  raise(6);
	}
	else
	  std::cerr << "\nNO HEARTBEAT for " << thread_id[i] << '/' <<
	    libcw::debug::_private_::__libcwd_tsd_array[ti].pid <<
	    " since " << bad[i] << " seconds.  Value still: " << heartbeat[ti] << "\n";
      }
      else
      {
	std::cerr << "\nGot HEARTBEAT for " << thread_id[i] << '/' <<
	  libcw::debug::_private_::__libcwd_tsd_array[ti].pid <<
	  ": " << heartbeat[ti] << " after " << bad[i] << " seconds.\n";
	bad[i] = 0;
	++running;
      }
    }
    if (running == 0)
      break;

    std::cerr << "\nHEARTBEAT ----end of check---------------------------------\n";
  }

  Debug( dc::malloc.on() );
#if CWDEBUG_ALLOC
  libcw::debug::ooam_filter_ct filter(libcw::debug::show_allthreads);
  Debug( list_allocations_on(libcw_do, filter) );
#endif
  Dout(dc::notice, "Exiting from main()");
  return 0;
}
