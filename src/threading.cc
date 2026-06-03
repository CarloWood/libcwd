// SPDX-FileCopyrightText: 2001-2004, 2006-2010, 2013-2014, 2018-2021, 2023, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#include "cwd_sys.h"
#include "ios_base_Init.h"
#include "macros.h"
#include "libcwd/debug.h"
#include <libcwd/core_dump.h>
#include <libcwd/private_mutex.inl>
#include <libcwd/private_threading.h>
#include "threadsafe/AIReadWriteMutex.h"
#include "threadsafe/threadsafe.h"

#include <map>
#include <alloca.h>
#include <unistd.h>
#include "cwd_debug.h"

#if LIBCWD_DEBUGDEBUGRWLOCK
pthread_mutex_t LIBCWD_DEBUGDEBUGLOCK_CERR_mutex;
unsigned int LIBCWD_DEBUGDEBUGLOCK_CERR_count;
#endif

namespace libcwd {
namespace _private_ {

bool WST_multi_threaded = false;
bool WST_first_thread_initialized = false;
bool WST_is_NPTL = false;

#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
int instance_locked[instance_locked_size];
pthread_t locked_by[instance_locked_size];
void const* locked_from[instance_locked_size];
#endif

void initialize_global_mutexes()
{
#if !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
  mutex_tct<mutex_initialization_instance>::initialize();
  mutex_tct<set_ostream_instance>::initialize();
  mutex_tct<kill_threads_instance>::initialize();
#if CWDEBUG_DEBUGT
  mutex_tct<keypair_map_instance>::initialize();
#endif
#endif // !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
}

void mutex_ct::M_initialize()
{
  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
#if CWDEBUG_DEBUGT
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
  pthread_mutex_init(&M_mutex, &mutex_attr);
  pthread_mutexattr_destroy(&mutex_attr);
  M_initialized = true;
}

void fatal_cancellation(void* arg)
{
  char* text = static_cast<char*>(arg);
  DoutFatal(dc::core, "Cancelling a thread " << text << ".  This is not supported by libcwd, sorry.");
}

//===================================================================================================
// Thread Specific Data
//

#if LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS
pthread_key_t TSD_st::S_tsd_key;
pthread_once_t TSD_st::S_tsd_key_once = PTHREAD_ONCE_INIT;

extern void debug_tsd_init(LIBCWD_TSD_PARAM);
void threading_tsd_init(LIBCWD_TSD_PARAM);

// static
TSD_st& TSD_st::instance()
{
  TSD_st* instance;
  if (!WST_tsd_key_created || !(instance = (TSD_st*)pthread_getspecific(S_tsd_key)))
    return S_create();
  return *instance;
}

TSD_st* main_thread_tsd;

// Create, install, and initialize the TSD for the current thread.
//static
TSD_st& TSD_st::S_create()
{
  int oldtype;
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

  TSD_st* real_tsd = new TSD_st;
  // clang-format off
  PRAGMA_DIAGNOSTIC_PUSH_IGNORE_class_memaccess
  std::memset(real_tsd, 0, sizeof(struct TSD_st));
  PRAGMA_DIAGNOSTIC_POP
  // clang-format on
  real_tsd->tid = pthread_self();
  real_tsd->pid = getpid();

  // Install the pthread key before initializing libcwd subsystems. If initialization re-enters
  // TSD_st::instance(), it must see and reuse this partially initialized TSD instead of recursing.
  pthread_once(&S_tsd_key_once, &S_tsd_key_alloc);
  pthread_setspecific(S_tsd_key, (void*)real_tsd);

  if (!main_thread_tsd)
    main_thread_tsd = real_tsd;

  if (!WST_first_thread_initialized) // Is this the first thread?
  {
    WST_first_thread_initialized = true;
#ifdef _CS_GNU_LIBPTHREAD_VERSION
    size_t n = confstr(_CS_GNU_LIBPTHREAD_VERSION, NULL, 0);
    if (n > 0)
    {
      char* buf = (char*)alloca(n);
      confstr(_CS_GNU_LIBPTHREAD_VERSION, buf, n);
      if (strstr(buf, "NPTL"))
        WST_is_NPTL = true;
    }
#endif
    initialize_global_mutexes();
    threading_tsd_init(*real_tsd); // Initialize the TSD of stuff that goes in threading.cc.
  }
  else
  {
    WST_multi_threaded = true;
    debug_tsd_init(*real_tsd); // Initialize the TSD of existing debug objects.
    threading_tsd_init(*real_tsd); // Initialize the TSD of stuff that goes in threading.cc.
  }

  pthread_setcanceltype(oldtype, NULL);

  return *real_tsd;
}

bool WST_tsd_key_created = false;

void TSD_st::S_tsd_key_alloc()
{
  pthread_key_create(&S_tsd_key, &TSD_st::S_cleanup_routine); // Leaks memory, we never destruct this key again.
  WST_tsd_key_created = true;
}

#define VALGRIND 0

void TSD_st::cleanup_routine()
{
#if !VALGRIND
  if (++tsd_destructor_count < PTHREAD_DESTRUCTOR_ITERATIONS)
#else
  if (1) // Valgrind doesn't iterate the key destruction routines.
#endif
  {
    // Add the key back a number of times in order to schedule our
    // deinitialization as far as possible after other key destruction
    // routines.
    pthread_setspecific(S_tsd_key, (void*)this);
#if !VALGRIND
    if (tsd_destructor_count < PTHREAD_DESTRUCTOR_ITERATIONS - 1)
      return;
#endif
    for (int i = 0; i < LIBCWD_DO_MAX; ++i)
      if (do_array[i])
      {
        debug_tsd_st* ptr = do_array[i];
        do_off_array[i] = 0; // Turn all debugging off!  Now, hopefully, we won't use do_array[i] anymore.
        do_array[i] = NULL; // So we won't free it again.
        ptr->tsd_initialized = false;
        delete ptr; // Free debug object TSD.
      }

    int oldtype;
    pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, &oldtype);
    thread_iter->terminating();
    pthread_setcanceltype(oldtype, NULL);

    pthread_setspecific(S_tsd_key, (void*)0);
    // Then we can safely delete the current TSD.
    delete this;
  }
}

void TSD_st::S_cleanup_routine(void* arg)
{
  TSD_st* obj = reinterpret_cast<TSD_st*>(arg);
  obj->cleanup_routine();
}

#endif // LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS

// End of Thread Specific Data
//===================================================================================================

int pthread_lock_interface_ct::try_lock()
{
#if CWDEBUG_DEBUGT
  bool success = pthread_mutex_trylock(ptr);
  if (success)
  {
    LIBCWD_TSD_DECLARATION;
    _private_::test_for_deadlock(pthread_lock_interface_instance, __libcwd_tsd, __builtin_return_address(0));
    __libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] += 1;
    if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 1)
    {
      __libcwd_tsd.rdlocked_by1[pthread_lock_interface_instance] = pthread_self();
      __libcwd_tsd.rdlocked_from1[pthread_lock_interface_instance] = __builtin_return_address(0);
    }
    else if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 2)
    {
      __libcwd_tsd.rdlocked_by2[pthread_lock_interface_instance] = pthread_self();
      __libcwd_tsd.rdlocked_from2[pthread_lock_interface_instance] = __builtin_return_address(0);
    }
    else
      core_dump();
  }
  return success;
#else
  return pthread_mutex_trylock(ptr);
#endif
}

void pthread_lock_interface_ct::lock()
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
  __libcwd_tsd.waiting_for_rdlock = pthread_lock_interface_instance;
#endif
  pthread_mutex_lock(ptr);
#if CWDEBUG_DEBUGT
  __libcwd_tsd.waiting_for_rdlock = 0;
  _private_::test_for_deadlock(pthread_lock_interface_instance, __libcwd_tsd, __builtin_return_address(0));
  __libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] += 1;
  if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 1)
  {
    __libcwd_tsd.rdlocked_by1[pthread_lock_interface_instance] = pthread_self();
    __libcwd_tsd.rdlocked_from1[pthread_lock_interface_instance] = __builtin_return_address(0);
  }
  else if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 2)
  {
    __libcwd_tsd.rdlocked_by2[pthread_lock_interface_instance] = pthread_self();
    __libcwd_tsd.rdlocked_from2[pthread_lock_interface_instance] = __builtin_return_address(0);
  }
  else
    core_dump();
#endif
}

void pthread_lock_interface_ct::unlock()
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
  if (__libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] == 2)
    __libcwd_tsd.rdlocked_by2[pthread_lock_interface_instance] = 0;
  else
    __libcwd_tsd.rdlocked_by1[pthread_lock_interface_instance] = 0;
  __libcwd_tsd.instance_rdlocked[pthread_lock_interface_instance] -= 1;
#endif
  pthread_mutex_unlock(ptr);
}

//---------------------------------------------------------------------------------------------------
// Below is the implementation of a list with thread specific objects
// that are kept even after the destruction of a thread, and even
// after the TSD_st structure is reused for another thread.
//
// thread_ct stores per-thread state that must remain address-stable while
// libcwd observes thread lifetime and, in fatal debug-output paths, cancels
// all known peer threads.
//

// Store the global list of thread_ct objects behind a typed read/write lock.
//
// The list is reached through ThreadList::instance() instead of a global object so that it can be
// constructed on first use, including during early TSD initialization before normal global construction
// has completed. The contained std::list keeps thread_iter values address-stable across later insertions.
struct ThreadList::impl_ct
{
  using threads_ts = threadsafe::Unlocked<ThreadList::list_type, threadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  threads_ts threads_;
};

// Return the process-wide list that tracks all threads known to libcwd.
//static
ThreadList const& ThreadList::instance()
{
  static ThreadList const threadlist_registry{new ThreadList::impl_ct};
  return threadlist_registry;
}

// Insert a thread_ct for __libcwd_tsd and initialize it while the list write lock is held.
//
// The resulting iterator is stored in __libcwd_tsd before initialization finishes so later TSD cleanup can
// mark this exact list entry as terminating. Holding the write access until after thread_ct::initialize
// ensures that concurrent traversals only observe fully initialized thread objects.
void ThreadList::add_current_thread(LIBCWD_TSD_PARAM) const
{
  impl_ct::threads_ts::wat threads_w(impl_->threads_);
  __libcwd_tsd.thread_iter = threads_w->insert(threads_w->end(), thread_ct());
  __libcwd_tsd.thread_iter_valid = true;
  __libcwd_tsd.thread_iter->initialize(LIBCWD_TSD);
}

struct ThreadsWat
{
  ThreadList::impl_ct::threads_ts::wat& ref_;
  ThreadsWat(ThreadList::impl_ct::threads_ts::wat& wat) : ref_(wat) { }
};

// Mark a known thread-list entry as terminated while serializing with list traversals and insertions.
//
// The current thread_iter implementation does not erase the entry, but this wrapper keeps the iterator
// operation under the same write access that would be required if terminated ever starts recycling nodes.
void ThreadList::mark_thread_terminated(ThreadList::list_type::iterator thread_iter LIBCWD_COMMA_TSD_PARAM) const
{
  impl_ct::threads_ts::wat threads_w(impl_->threads_);
  thread_iter->terminated(threads_w LIBCWD_COMMA_TSD);
}

// Cancel every known peer thread while a read access prevents concurrent modification of the list.
//
// current_tid is never cancelled. On old LinuxThreads builds the thread-manager pseudo-thread is skipped
// for the same reason as the previous raw traversal: cancelling it would interfere with process teardown.
void ThreadList::cancel_all_other_threads(pthread_t current_tid) const
{
  impl_ct::threads_ts::crat threads_r(impl_->threads_);
  for (thread_ct const& thread : *threads_r)
    if (!pthread_equal(thread.tid, current_tid)
#ifdef __linux
        && (WST_is_NPTL || thread.tid != (pthread_t)1024)
#endif
    )
      pthread_cancel(thread.tid);
}

// Called when a new thread is detected.
// Adds a new thread_ct to threadlist and initializes it.
void threading_tsd_init(LIBCWD_TSD_PARAM)
{
  LIBCWD_DEFER_CANCEL;
  ThreadList::instance().add_current_thread(LIBCWD_TSD);
  LIBCWD_RESTORE_CANCEL;
}

// The default constructor of a thread_ct object.
// No real initialization is done yet, for that thread_ct::initialize
// needs to be called after this object is added to threadlist.
// The thread list write access needs to be held before insertion of the thread_ct takes place and not be
// released until initialization of the object has finished.
void thread_ct::initialize(LIBCWD_TSD_PARAM)
{
  thread_mutex.initialize();
  tid = __libcwd_tsd.tid;
}

// This member function is called when the TSD_st structure
// is being reused, which is currently our only way to know
// for sure that the corresponding thread has terminated.
void thread_ct::terminated(ThreadsWat wat LIBCWD_COMMA_TSD_PARAM_UNUSED)
{
}

#if CWDEBUG_DEBUGT
#include <inttypes.h>

// These are the different groups that are allowed together.
uint16_t const group1 = 0x002; // Instance 1 locked first.
uint16_t const group2 = 0x004; // Instance 2 locked first.
uint16_t const group3 = 0x020; // Instance 1 read only and high_priority when second.
uint16_t const group4 = 0x040; // Instance 2 read only and high_priority when second.
uint16_t const group5 = 0x200; // (Instance 1 locked first and (either one read only and high_priority when second))
                               // or (both read only and high_priority when second).
uint16_t const group6 = 0x400; // (Instance 2 locked first and (either one read only and high_priority when second))
                               // or (both read only and high_priority when second).

struct keypair_key_st
{
  size_t instance1;
  size_t instance2;
};

struct keypair_info_st
{
  uint16_t state;
  int limited;
  void const* from_first[5];
  void const* from_second[5];
};

struct keypair_compare_st
{
  bool operator()(keypair_key_st const& a, keypair_key_st const& b) const;
};

// Definition of the map<> that holds the key instance pairs that are ever locked simultaneously by the same thread.
typedef std::pair<keypair_key_st const, keypair_info_st> keypair_map_value_t;
typedef std::map<keypair_key_st, keypair_info_st, keypair_compare_st> keypair_map_t;
static keypair_map_t* keypair_map;

// Bring some arbitrary ordering into the map with key pairs.
bool keypair_compare_st::operator()(keypair_key_st const& a, keypair_key_st const& b) const
{
  return (a.instance1 == b.instance1) ? (a.instance2 < b.instance2) : (a.instance1 < b.instance1);
}

extern "C" int raise(int);
void test_lock_pair(size_t instance_first, void const* from_first, size_t instance_second, void const* from_second)
{
  if (instance_first == instance_second)
    return; // Must have been a recursive lock.

  keypair_key_st keypair_key;
  keypair_info_st keypair_info;

  // Do some decoding and get rid of the 'read_lock_offset' and 'high_priority_read_lock_offset' flags.

  // During the decoding, we assume that the first instance is the smallest (instance 1).
  keypair_info.state = group1;
  bool first_is_readonly = false;
#if CWDEBUG_DEBUGOUTPUT
  bool second_is_high_priority = false;
#endif
  if (instance_first < 0x10000)
  {
    if (instance_first >= read_lock_offset)
    {
      first_is_readonly = true;
      keypair_info.state |= (group3 | group5);
    }
    instance_first %= read_lock_offset;
  }
  if (instance_second < 0x10000)
  {
    if (instance_second >= high_priority_read_lock_offset)
    {
#if CWDEBUG_DEBUGOUTPUT
      second_is_high_priority = true;
#endif
      keypair_info.state |= (group4 | group5);
      if (first_is_readonly)
        keypair_info.state |= group6;
    }
    instance_second %= read_lock_offset;
  }

  // Put the smallest instance in instance1.
  if (instance_first < instance_second)
  {
    keypair_key.instance1 = instance_first;
    keypair_key.instance2 = instance_second;
  }
  else
  {
    keypair_key.instance1 = instance_second;
    keypair_key.instance2 = instance_first;
    // Correct the state by swapping groups 1 <-> 2, 3 <-> 4, and 5 <-> 6.
    keypair_info.state =
        ((keypair_info.state << 1) | (keypair_info.state >> 1)) & (group1 | group2 | group3 | group4 | group5 | group6);
  }

  // Store the locations where the locks were set.
  keypair_info.limited = 0;
  keypair_info.from_first[0] = from_first;
  keypair_info.from_second[0] = from_second;

  mutex_tct<keypair_map_instance>::lock();
  std::pair<keypair_map_t::iterator, bool> result = keypair_map->insert(keypair_map_value_t(keypair_key, keypair_info));
  if (!result.second)
  {
    keypair_info_st& stored_info((*result.first).second);
    uint16_t prev_state = stored_info.state;
    stored_info.state &= keypair_info.state; // Limit possible groups.
    if (prev_state != stored_info.state)
    {
      stored_info.limited++;
      stored_info.from_first[stored_info.limited] = from_first;
      stored_info.from_second[stored_info.limited] = from_second;
      DEBUGDEBUG_CERR("\nKEYPAIR: first: " << instance_first << (first_is_readonly ? " (read-only lock)" : "")
                                           << "; second: " << instance_second
                                           << (second_is_high_priority ? " (high priority lock)" : "")
                                           << "; groups: " << (void*)(unsigned long)stored_info.state << '\n');
    }
    if (stored_info.state == 0) // No group left?
    {
      FATALDEBUGDEBUG_CERR("\nKEYPAIR: There is a potential deadlock between lock " << keypair_key.instance1 << " and "
                                                                                    << keypair_key.instance2 << '.');
      FATALDEBUGDEBUG_CERR("\nKEYPAIR: Previously, these locks were locked in the following locations:");
      for (int cnt = 0; cnt <= stored_info.limited; ++cnt)
        FATALDEBUGDEBUG_CERR("\nKEYPAIR: First at " << stored_info.from_first[cnt] << " and then at "
                                                    << stored_info.from_second[cnt]);
      mutex_tct<keypair_map_instance>::unlock();
      core_dump();
    }
  }
  else
  {
    DEBUGDEBUG_CERR("\nKEYPAIR: first: " << instance_first << (first_is_readonly ? " (read-only lock)" : "")
                                         << "; second: " << instance_second
                                         << (second_is_high_priority ? " (high priority lock)" : "")
                                         << "; groups: " << (void*)(unsigned long)keypair_info.state << '\n');
#if 0
    // X1,W/R2 + Y2,Z3 imply X1,Z3.
    // First assume the new pair is a possible X1,W/R2:
    if (!second_is_high_priority)
    {
      for (keypair_map_t::iterator iter = keypair_map->begin(); iter != keypair_map->end(); ++iter)
      {
      }
    }
#endif
  }
  mutex_tct<keypair_map_instance>::unlock();
}

void test_for_deadlock(size_t instance, struct TSD_st& __libcwd_tsd, void const* from)
{
  if (!WST_multi_threaded)
    return; // Give libcwd the time to get initialized.

  if (instance == keypair_map_instance)
    return;

  // Initialization.
  if (!keypair_map)
    keypair_map = new keypair_map_t; // LEAK 28 bytes.  This is never freed anymore.

  // Hold typed read access while scanning the per-thread mutexes. The AIReadWriteMutex used for this
  // registry is not part of the libcwd mutex bookkeeping that is being inspected here, so the diagnostic
  // remains focused on application-visible libcwd locks while avoiding an unlocked traversal of a list that
  // another thread may extend during TSD initialization.
  ThreadList::impl_ct::threads_ts::crat threads_r(ThreadList::instance().impl_->threads_);
  for (threadlist_t::const_iterator iter = threads_r->begin(); iter != threads_r->end(); ++iter)
  {
    mutex_ct const* mutex = &((*iter).thread_mutex);
    if (mutex->M_locked_by == __libcwd_tsd.tid && mutex->M_instance_locked >= 1)
    {
      assert(reinterpret_cast<size_t>(mutex) >= 0x10000);
      test_lock_pair(reinterpret_cast<size_t>(mutex), mutex->M_locked_from, instance, from);
    }
  }
  for (int inst = 0; inst < instance_locked_size; ++inst)
  {
    // Check for read locks that are already set.  Because it was never stored wether or not
    // it was a high priority lock, this information is lost.  This is not a problem though
    // because we treat high priority and normal read locks the same when they are set first.
    if (inst < instance_rdlocked_size && __libcwd_tsd.rdlocked_by1[inst] == __libcwd_tsd.tid &&
        __libcwd_tsd.instance_rdlocked[inst] >= 1)
      test_lock_pair(inst + read_lock_offset, __libcwd_tsd.rdlocked_from1[inst], instance, from);
    if (inst < instance_rdlocked_size && __libcwd_tsd.rdlocked_by2[inst] == __libcwd_tsd.tid &&
        __libcwd_tsd.instance_rdlocked[inst] >= 2)
      test_lock_pair(inst + read_lock_offset, __libcwd_tsd.rdlocked_from2[inst], instance, from);
    // Check for write locks and normal mutexes.
    if (locked_by[inst] == __libcwd_tsd.tid && instance_locked[inst] >= 1 && inst != keypair_map_instance)
      test_lock_pair(inst, locked_from[inst], instance, from);
  }
}

pthread_mutex_t raw_write_mutex = PTHREAD_MUTEX_INITIALIZER;

#endif // CWDEBUG_DEBUGT

} // namespace _private_
} // namespace libcwd
