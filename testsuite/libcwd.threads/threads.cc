#ifdef TWDEBUG
#include <libtw/sys.h>
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

#define PREFIX_CODE \
    pthread_mutex_lock(&start_mut); \
    set_margin(); \
    int __res; \
    heartbeat[thread_index(pthread_self())] = 1; \
    ++started; \
    pthread_cond_wait(&start_cond, &start_mut); \
    pthread_mutex_unlock(&start_mut); \
    for(int __i = 0; __i < 10; ++__i) {
#define EXIT(res) \
    __res = (res); \
    ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() ); \
    ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() ); \
    { \
      ForAllDebugObjects( LIBCWD_TSD_DECLARATION; while (LIBCWD_DO_TSD_MEMBER_OFF(debugObject) >= 0) debugObject.on() ); \
      ForAllDebugObjects( LIBCWD_TSD_DECLARATION; if (LIBCWD_DO_TSD_MEMBER_OFF(debugObject) < 0) debugObject.off() ); \
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
pthread_mutex_t start_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;

size_t const heartbeat_size = 1024;
volatile int heartbeat[heartbeat_size];
int prev_heartbeat[heartbeat_size];
int bad[heartbeat_size];
bool is_NPTL = false;
int started = 0;

unsigned long thread_index(pthread_t tid)
{
  if (is_NPTL)
  {
    unsigned long res = tid >> 23;
    if (res >= heartbeat_size)
      abort();
    return res;
  }
  return tid % 1024;
}

pthread_once_t test_keys_once = PTHREAD_ONCE_INIT;
static pthread_key_t keys[32];

static void cleanup_routine(void* arg)
{
}

void test_keys_alloc(void)
{
  for (unsigned int i = 0; i < sizeof(keys) / sizeof(pthread_key_t); ++i)
   pthread_key_create(&keys[i], cleanup_routine);
}

void set_margin(void)
{
  char margin[32];
  sprintf(margin, "%-10lu ", pthread_self());
  Debug( libcw_do.margin().assign(margin, 11) );
#if CWDEBUG_DEBUGT
  pthread_once(&test_keys_once, &test_keys_alloc);
  for (unsigned int i = 0; i < sizeof(keys) / sizeof(pthread_key_t); ++i)
    pthread_setspecific(keys[i], (void*)i);
#endif
}

#ifdef TWDEBUG
// Special debug object to print memory leaks to.
libtw::debug::debug_ct leak_do;

struct timeval start;
libtw::debug::alloc_filter_ct leak_filter(libtw::debug::show_time);
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
thread_func_t progs[] = {
    alloctag_prog,
    basic_prog,
    location_prog,
    cf_prog,
    //continued_prog,
    dc_prog,
    demangler_prog,
    dlopen_prog,
    do_prog,
    flush_prog,
    leak_prog,
    lockable_auto_ptr_prog,
    //magic_prog,
    marker_prog,
    strdup_prog,
    test_delete_prog,
    type_info_prog,
    filter_prog
};
int const number_of_threads = sizeof(progs)/sizeof(thread_func_t);

#ifdef TWDEBUG
pthread_mutex_t leak_mutex;
#endif

extern int raise (int sig);

#define WRITESTR(x) ::write(2, x, sizeof(x) - 1)
#define WRITEINT(i) do { if (i == 0) ::write(2, "0", 1); else { \
    int v = i; char b[32]; char* p = b + 32; \
    while(v) { *--p = '0' + v % 10; v /= 10; }; \
    ::write(2, p, b + sizeof(b) - p); \
    } } while(0)

int main(void)
{
  Debug( check_configuration() );

#ifdef _CS_GNU_LIBPTHREAD_VERSION
  size_t n = confstr (_CS_GNU_LIBPTHREAD_VERSION, NULL, 0);
  if (n > 0)
  {
    char* buf = (char*)alloca(n);
    confstr(_CS_GNU_LIBPTHREAD_VERSION, buf, n);
    if (strstr (buf, "NPTL"))
      is_NPTL = true;
  }
#endif

  bool is_valgrind = false;
  if (pthread_equal(pthread_self(), 1))
    is_valgrind = true;

#if CWDEBUG_ALLOC
  new int;
  libcwd::make_all_allocations_invisible_except(NULL);
#endif

  pthread_cond_init(&start_cond, NULL);

#ifdef TWDEBUG
  std::ofstream leakout;
  leakout.open("leakout");
#endif

  Debug( libcw_do.set_ostream(&std::cerr, &cerr_mutex) );
#ifdef TWDEBUG
  Debug2( leak_do.set_ostream(&leakout, &leak_mutex) );
#endif

  set_margin();

#ifndef TWDEBUG
  Debug( dc::notice.on() );
  Debug( libcw_do.on() );
  WRITESTR("HEARTBEAT: This is the main thread");
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
    WRITESTR("HEARTBEAT: creating thread ");
    WRITEINT(i);
    WRITESTR(", ");
    pthread_create(&thread_id[i], NULL, progs[i], NULL);
    WRITESTR("id ");
    WRITEINT(thread_id[i]);
    WRITESTR(".\n");
  }

  bool signal_sent = false;
  do
  {
    pthread_mutex_lock(&start_mut);
    if (started == number_of_threads)
    {
      pthread_mutex_unlock(&start_mut);
      WRITESTR("HEARTBEAT: Sending broadcast\n");
      pthread_cond_broadcast(&start_cond);
      WRITESTR("HEARTBEAT: Returned from broadcast\n");
      signal_sent = true;
    }
    else
    {
      pthread_mutex_unlock(&start_mut);
      WRITESTR("HEARTBEAT: Number of started threads only: ");
      WRITEINT(started);
      WRITESTR("; waiting 1 second.\n");
      struct timespec rqts = { 1, 0 };
      struct timespec rmts;
      nanosleep(&rqts, &rmts);
    }
  }
  while(!signal_sent);

  WRITESTR("HEARTBEAT: All threads started. Starting main loop\n");
  for(;;)
  {
    memcpy(prev_heartbeat, (void*)(heartbeat), sizeof(heartbeat));
    struct timespec rqts = { 1, 0 };
    if (is_valgrind)
      rqts.tv_sec = 10;
    struct timespec rmts;
    nanosleep(&rqts, &rmts);
    WRITESTR("\nHEARTBEAT: --start of new check-----------------------------\n");
    int running = 0;
    for (int i = 0; i < number_of_threads; ++i)
    {
      int ti = thread_index(thread_id[i]);
      if (heartbeat[ti] == -1)
	continue;
      if (heartbeat[ti] == 0)
      {
	void* status;
	WRITESTR("HEARTBEAT: Entering pthread_join(");
	WRITEINT(thread_id[i]);
	WRITESTR(")\n");
	pthread_join(thread_id[i], &status);
	WRITESTR("HEARTBEAT: Returned from pthread_join\n");
	WRITESTR("HEARTBEAT: thread ");
	WRITEINT(i);
	WRITESTR(", id ");
	WRITEINT(thread_id[i]);
	WRITESTR(" (");
	WRITEINT(ti);
	WRITESTR("), returned with status ");
	if ((bool)status)
	  WRITESTR("OK");
	else
	  WRITESTR("ERROR");
	WRITESTR(".\n");
	++running;
	heartbeat[ti] = -1;
      }
      else if (prev_heartbeat[ti] == heartbeat[ti])
      {
	if (++(bad[i]) == 30)
	{
	  WRITESTR("HEARTBEAT: No HEARTBEAT for thread ");
	  WRITEINT(thread_id[i]);
	  WRITESTR("\n");
	  raise(6);
	}
	else
	{
	  WRITESTR("\nHEARTBEAT: NO HEARTBEAT for ");
	  WRITEINT(thread_id[i]);
	  WRITESTR(" since ");
	  WRITEINT(bad[i]);
	  WRITESTR(" seconds.  Value still: ");
	  WRITEINT(heartbeat[ti]);
	  WRITESTR("\n");
        }
      }
      else
      {
        WRITESTR("\nHEARTBEAT: Got HEARTBEAT for ");
	WRITEINT(thread_id[i]);
	WRITESTR(": ");
	WRITEINT(heartbeat[ti]);
	WRITESTR(" after ");
	WRITEINT(bad[i]);
	WRITESTR(" seconds.\n");
	bad[i] = 0;
	++running;
      }
    }
    if (running == 0)
      break;

    WRITESTR("\nHEARTBEAT: ----end of check---------------------------------\n");
  }

  WRITESTR("\nHEARTBEAT: Exiting from main(): printing memory allocations.\n");
  Debug( dc::malloc.on() );
#if CWDEBUG_ALLOC
  libcwd::alloc_filter_ct filter(libcwd::show_allthreads);
  Debug( list_allocations_on(libcw_do, filter) );
#endif
  WRITESTR("\nHEARTBEAT: Exiting from main()\n");
  return 0;
}
