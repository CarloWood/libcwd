// This code was originally retrieved from
// http://gcc.gnu.org/ml/libstdc++/2001-05/msg00384.html
//
// ChangeLog:
// 2001/11/07	Added libcwd specific includes and Debug() lines.
//
// This multi-threading C++/STL/POSIX code adheres to rules outlined here:
// http://www.sgi.com/tech/stl/thread_safety.html
//
// It is believed to exercise the allocation code in a manner that
// should reveal memory leaks (and, under rare cases, race conditions,
// if the STL threading support is fubar'd).
//
// In addition to memory leak detection, which requires some human
// observation, this test also looks for memory corruption of the data
// passed between threads using an STL container.
//
// To manually inspect code generation, use:
// /usr/local/beta-gcc/bin/g++ -E STL-pthread-example.C|grep _Lock
// /usr/local/beta-gcc/bin/g++ -E STL-pthread-example.C|grep _M_acquire_lock

#ifndef _REENTRANT
#error "Recompile libcwd with --enable-threading"
#endif
#include "sys.h"
#include <list>
#include <pthread.h>
#include <libcw/debug.h>

using namespace std;

const int thread_cycles = 10;
const int thread_pairs = 10;
const unsigned max_size = 100;
const int iters = 10000;

class task_queue
{
public:
  task_queue ()
  {
    pthread_mutex_init (&fooLock, NULL);
    pthread_cond_init (&fooCond, NULL);
  }
  ~task_queue ()
  {
    pthread_mutex_destroy (&fooLock);
    pthread_cond_destroy (&fooCond);
  }
  list<int> foo;
  pthread_mutex_t fooLock;
  // This code uses a special case that allows us to use just one
  // condition variable - in general, don't use this idiom unless you
  // know what you are doing. ;-)
  pthread_cond_t fooCond;
};

void*
produce (void* t)
{
  task_queue& tq = *(static_cast<task_queue*> (t));
  int num = 0;
  while (num < iters)
    {
      pthread_mutex_lock (&tq.fooLock);
      while (tq.foo.size () >= max_size)
        pthread_cond_wait (&tq.fooCond, &tq.fooLock);
      tq.foo.push_back (num++);
      pthread_cond_signal (&tq.fooCond);
      pthread_mutex_unlock (&tq.fooLock);
    }
  return 0;
}

void*
consume (void* t)
{
  task_queue& tq = *(static_cast<task_queue*> (t));
  int num = 0;
  while (num < iters)
    {
      pthread_mutex_lock (&tq.fooLock);
      while (tq.foo.size () == 0)
        pthread_cond_wait (&tq.fooCond, &tq.fooLock);
      if (tq.foo.front () != num++)
        abort ();
      tq.foo.pop_front ();
      pthread_cond_signal (&tq.fooCond);
      pthread_mutex_unlock (&tq.fooLock);
    }
  return 0;
}

int
main (int argc, char** argv)
{
  Debug( check_configuration() );
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );

  pthread_t prod[thread_pairs];
  pthread_t cons[thread_pairs];

  task_queue* tq[thread_pairs];

  for (int j = 0; j < thread_cycles; j++)
    {
      for (int i = 0; i < thread_pairs; i++)
        {
          tq[i] = NEW( task_queue );
          pthread_create (&prod[i], NULL, produce, static_cast<void*> (tq[i]));
          pthread_create (&cons[i], NULL, consume, static_cast<void*> (tq[i]));
        }

      for (int i = 0; i < thread_pairs; i++)
        {
          pthread_join (prod[i], NULL);
          pthread_join (cons[i], NULL);
#if defined(__FreeBSD__)
          // These lines are not required by POSIX since a successful
          // join is suppose to detach as well...
          pthread_detach (prod[i]);
          pthread_detach (cons[i]);
          // ...but they are according to the FreeBSD 4.X code base
          // or else you get a memory leak.
#endif
          delete tq[i];
        }
    }

  Debug( list_allocations_on(libcw_do) );

  return 0;
}

