#define PREFIX_CODE set_margin(); int __res; for(int __i = 0; __i < 100; ++__i) {
#define EXIT(res) \
    __res = (res); \
    ForAllDebugChannels( while (!debugChannel.is_on()) debugChannel.on() ); \
    ForAllDebugChannels( if (debugChannel.is_on()) debugChannel.off() ); \
    ForAllDebugObjects( debugObject.set_ostream(&std::cerr, &cerr_mutex) ); \
    { \
      LIBCWD_TSD_DECLARATION; \
      ForAllDebugObjects( while (LIBCWD_DO_TSD_MEMBER_OFF(debugObject) >= 0) debugObject.on() ); \
      ForAllDebugObjects( if (LIBCWD_DO_TSD_MEMBER_OFF(debugObject) < 0) debugObject.off() ); \
    } \
    if (__res) break; } return (void*)(__res == 0)
#define THREADED(x) x
#define COMMA_THREADED(x) , x
#define THREADTEST

#include "sys.h"
#include "debug.h"
#include <stdio.h>	// Needed for sprintf()
#include <unistd.h>

pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cerr_mutex = PTHREAD_MUTEX_INITIALIZER;

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

typedef void* (*thread_func_t)(void*);
thread_func_t progs[] = { alloctag_prog, basic_prog, location_prog, cf_prog, /*continued_prog,*/ dc_prog,
    demangler_prog, dlopen_prog, do_prog, flush_prog, leak_prog, lockable_auto_ptr_prog, /*magic_prog,*/
    marker_prog, strdup_prog, test_delete_prog, type_info_prog };
int const number_of_threads = sizeof(progs)/sizeof(thread_func_t);

int main(void)
{
  Debug( check_configuration() );

#if CWDEBUG_ALLOC
  new int;
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif

  Debug( libcw_do.set_ostream(&std::cout, &cout_mutex) );

  set_margin();

  Debug( dc::notice.on() );
  Debug( libcw_do.on() );

  pthread_t thread_id[number_of_threads];
  for (int i = 0; i < number_of_threads; ++i)
  {
    Dout(dc::notice|continued_cf, "main: creating thread " << i << ", ");
    pthread_create(&thread_id[i], NULL, progs[i], NULL);
    Dout(dc::finish, "id " << thread_id[i] << " (" << thread_index(thread_id[i]) << ").");
  }

  for (int i = 0; i < number_of_threads; ++i)
  {
    void* status;
    pthread_join(thread_id[i], &status);
    Dout(dc::notice, "main loop: thread " << i << ", id " << thread_id[i] << " (" << thread_index(thread_id[i]) << "), returned with status " << ((bool)status ? "OK" : "ERROR") << '.');
  }

  Debug( dc::malloc.on() );
  libcw::debug::ooam_filter_ct filter(libcw::debug::show_allthreads);
  Debug( list_allocations_on(libcw_do, filter) );
  Dout(dc::notice, "Exiting from main()");
  return 0;
}
