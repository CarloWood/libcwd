#ifndef _REENTRANT
#error "Recompile libcwd with --enable-threading"
#endif
//#include "sys.h"
#include "../include/sys.h"
#include "threads_debug.h"
#include <vector>
#include <iostream>

using libcw::debug::_private_::mutex_tct;
using libcw::debug::_private_::rwlock_tct;
using libcw::debug::_private_::test_instance0;

template<class TYPE>
  class foo_tct {
  public:
    foo_tct(void)
    {
      Dout(dc::notice, "Creating " << type_info_of<foo_tct<TYPE> >().demangled_name()
          << " with pthread_self() = " << pthread_self());
    }
    ~foo_tct(void)
    {
      Dout(dc::notice, "Destroying " << type_info_of<foo_tct<TYPE> >().demangled_name()
          << " with pthread_self() = " << pthread_self());
    }
  };

template<class TYPE>
  inline typename TYPE::foo_ct test(TYPE&)
  {
    static foo_tct<std::vector<typename TYPE::foo_ct> > foo;
    static typename TYPE::foo_ct dummy;
    return dummy;
  }

int const number_of_threads = 4;
static int volatile state_thread[number_of_threads];

class A {
public:
  typedef int foo_ct;
};

class B {
public:
  typedef float foo_ct;
};

class C {
public:
  typedef A const* foo_ct;
};

char const* in_the_middle(int my_id)
{
  if (my_id != number_of_threads - 1)
  {
    Dout(dc::notice, my_id << ": waiting for thread " << my_id + 1);
    while(state_thread[my_id + 1] == 0);
  }
  state_thread[my_id] = 1;
  A a; B b; C c;
  if (my_id == 1)
    test(a);
  else if (my_id == 2)
    test(b);
  else
    test(c);
  return "in the middle of a Dout.";
}

class TSD {
public:
  TSD(void);
  ~TSD();
};

TSD::TSD(void)
{
  Dout(dc::notice, "Calling TSD(), this = " << (void*)this << '.');
}

TSD::~TSD()
{
  Dout(dc::notice, "Calling ~TSD(), this is " << (void*)this << ".  From "
      << location_ct((char*)__builtin_return_address(0) + builtin_return_address_offset));
}

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static void destroy(void* arg) { TSD* tsd = reinterpret_cast<TSD*>(arg); delete tsd; }
static void key_alloc() { pthread_key_create(&key, destroy); }

void* thread_function(void* arguments)
{
  static int thread_counter = -1;

  pthread_once(&key_once, key_alloc);
  pthread_setspecific(key, new TSD);

  // Set Thread Specific on/off flags of the debug channels.
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  // And for the debug object.
  Debug( libcw_do.on() );

  // Serialize incrementation.
  int my_id;
  LIBCWD_DEFER_CANCEL
  mutex_tct<test_instance0>::lock();
  my_id = ++thread_counter;
  mutex_tct<test_instance0>::unlock();
  LIBCWD_RESTORE_CANCEL

  state_thread[my_id] = 0;
  Dout(dc::notice, my_id << ": Entering thread " << pthread_self());

  // Get TSD.
  TSD& tsd(*reinterpret_cast<TSD*>(pthread_getspecific(key)));
  Dout(dc::notice, my_id << ": tsd is " << (void*)&tsd << '.');

  Dout(dc::notice, my_id << ": Here we are " << in_the_middle(my_id));

  return NULL;
}

int main(void)
{
  Debug( check_configuration() );
#if CWDEBUG_ALLOC
  libcw::debug::make_all_allocations_invisible_except(NULL);
#endif
  Debug( libcw_do.on() );
  Debug( libcw_do.set_ostream(&std::cout) );

  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  Debug( list_channels_on(libcw_do) );

  mutex_tct<test_instance0>::initialize();
  rwlock_tct<test_instance0>::initialize();

  // Test if rwlocks allows multiple read locks but only one write lock.
  LIBCWD_DEFER_CANCEL
  rwlock_tct<test_instance0>::wrlock();
    LIBCWD_ASSERT( !rwlock_tct<test_instance0>::trywrlock() );
    LIBCWD_ASSERT( !rwlock_tct<test_instance0>::tryrdlock() );
  rwlock_tct<test_instance0>::wrunlock();
  LIBCWD_RESTORE_CANCEL

  LIBCWD_DEFER_CANCEL
  rwlock_tct<test_instance0>::rdlock();
    LIBCWD_ASSERT( !rwlock_tct<test_instance0>::trywrlock() );
    LIBCWD_ASSERT( rwlock_tct<test_instance0>::tryrdlock() && (rwlock_tct<test_instance0>::rdunlock(), true) );
    rwlock_tct<test_instance0>::rdlock();
      LIBCWD_ASSERT( !rwlock_tct<test_instance0>::trywrlock() );
      LIBCWD_ASSERT( rwlock_tct<test_instance0>::tryrdlock() && (rwlock_tct<test_instance0>::rdunlock(), true) );
    rwlock_tct<test_instance0>::rdunlock();
    LIBCWD_ASSERT( !rwlock_tct<test_instance0>::trywrlock() );
    LIBCWD_ASSERT( rwlock_tct<test_instance0>::tryrdlock() && (rwlock_tct<test_instance0>::rdunlock(), true) );
  rwlock_tct<test_instance0>::rdunlock();
  LIBCWD_ASSERT( rwlock_tct<test_instance0>::tryrdlock() && (rwlock_tct<test_instance0>::rdunlock(), true) );
  LIBCWD_ASSERT( rwlock_tct<test_instance0>::trywrlock() && (rwlock_tct<test_instance0>::wrunlock(), true) );
  LIBCWD_RESTORE_CANCEL

  // Now test that a mutex allows only one lock.
  LIBCWD_DEFER_CANCEL
  mutex_tct<test_instance0>::lock();
  LIBCWD_ASSERT( !mutex_tct<test_instance0>::trylock() );
  mutex_tct<test_instance0>::unlock();
  LIBCWD_RESTORE_CANCEL

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
