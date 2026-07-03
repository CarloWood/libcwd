@addtogroup tutorial-threads

`libcwd` is always built with thread support. For example:

```
g++ -pthread -DCWDEBUG program.cc -lpthread -lcwd
```

Best practice is to use the tool `pkg-config` to retrieve the
flags that one needs to pass to the compiler and linker. That is, the output
of `pkg-config --cflags libcwd` and `pkg-config --libs libcwd`.

libcwd should be completely thread-safe, with the following restrictions:

- All debug objects and debug channels *must* be global (as they should
  be in non-threaded applications for that matter).
- You are not allowed to create threads before all static and global objects
  are initialized; in practice this means that you are not allowed to create threads
  until `main()` is reached.
- You cannot use `dlopen()` to load libcwd when threads have
  already been created. Likewise you shouldn't `dlopen()`
  other libraries that use libcwd when there are already running threads,
  especially when those libraries define new debug objects and/or channels.
- You need to provide one and only one locking mechanism per ostream device
  that is also used to write debug output. It is preferable not to
  use the same ostream with two or more different debug objects.

Essentially the debug objects and channels react towards each thread as if
that thread is the only thread. The only (visible) shared variable is
the final `ostream` that a given debug object writes to.
This means that if one thread changes the `ostream` then all other
threads also suddenly start to write to that `ostream`.
Basically, it is simply not supported: don't change the output stream
on the fly.

All other characteristics like the on/off state and the margin and
marker strings as well as the indentation are Thread Specific: Every
thread may change those without locking or worrying about the effect on
other threads.

Every time a new thread is created, it will start with all debug objects
and channels turned off, just as at the start of `main()`.

In all likelihood, you'd want to set the margin string such that it reflects which
thread is printing the output. For example:

```
#include "debug.h"
#include <iostream>
#include <cstdio>
#include <pthread.h>

void* thread_function(void* arguments)
{
  // Set Thread Specific on/off flags of the debug channels.
  ForAllDebugChannels(if (!debugChannel.is_on()) debugChannel.on(););
  // And for the debug object.
  Debug(libcw_do.on());
  // Set a margin.
#ifdef CWDEBUG
  char margin[16];
  sprintf(margin, "%-10lu ", pthread_self());
#endif
  Debug(libcw_do.margin().assign(margin, 11));

  Dout(dc::notice, "Entering thread " << pthread_self());
  // ... do stuff
  Dout(dc::notice, "Leaving thread " << pthread_self());
  return (void*)true;
}

#ifdef CWDEBUG
std::mutex cout_mutex;
#endif

int main()
{
  // Don't output a single character at a time (yuk)
  std::ios::sync_with_stdio(false);
  // Do header files and library match?
  Debug(main_reached());
  // Send debug output to std::cout.
  Debug(libcw_do.set_ostream(&std::cout, &cout_mutex));
  // Turn debug object on.
  Debug(libcw_do.on());
  // Set a margin.
#ifdef CWDEBUG
  char margin[16];
  sprintf(margin, "%-10lu ", pthread_self());
#endif
  Debug(libcw_do.margin().assign(margin, 11));
  // Turn all debug channels on.
  ForAllDebugChannels(if (!debugChannel.is_on()) debugChannel.on());
  // List all channels.
  Debug(list_channels_on(libcw_do));

  // Create and join a few threads...
  int const number_of_threads = 4;
  pthread_t thread_id[number_of_threads];
  for (int i = 0; i < number_of_threads; ++i)
  {
    Dout(dc::notice|continued_cf, "main: creating thread " << i << ", ");
    pthread_create(&thread_id[i], NULL, thread_function, NULL);
    Dout(dc::finish, "id " << thread_id[i] << '.');
  }

  for (int i = 0; i < number_of_threads; ++i)
  {
    void* status;
    pthread_join(thread_id[i], &status);
    Dout(dc::notice, "main loop: thread " << i << ", id " << thread_id[i] <<
         ", returned with status " << ((bool)status ? "OK" : "ERROR") << '.');
  }

  Dout(dc::notice, "Exiting from main()");
}
```

Which outputs something like:

```
1024       DEBUG   : Enabled
1024       ELFUTILS: Enabled
1024       NOTICE  : Enabled
1024       SYSTEM  : Enabled
1024       WARNING : Enabled
1024       NOTICE  : main: creating thread 0, <unfinished>
1024       NOTICE  : <continued> id 1026.
1026       NOTICE  : Entering thread 1026
1026       NOTICE  : Leaving thread 1026
1024       NOTICE  : main: creating thread 1, id 2051.
2051       NOTICE  : Entering thread 2051
2051       NOTICE  : Leaving thread 2051
1024       NOTICE  : main: creating thread 2, id 3076.
3076       NOTICE  : Entering thread 3076
3076       NOTICE  : Leaving thread 3076
1024       NOTICE  : main: creating thread 3, id 4101.
1024       NOTICE  : main loop: thread 0, id 1026, returned with status OK.
1024       NOTICE  : main loop: thread 1, id 2051, returned with status OK.
1024       NOTICE  : main loop: thread 2, id 3076, returned with status OK.
4101       NOTICE  : Entering thread 4101
4101       NOTICE  : Leaving thread 4101
1024       NOTICE  : main loop: thread 3, id 4101, returned with status OK.
1024       NOTICE  : Exiting from main()
```

Congratulations, you are now a libcwd expert.
If you still have any questions that you can't find answers to here, feel free to mail me.
