// $Header$
//
// Copyright (C) 2001 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// Copyright (C) 2001, by Eric Lesage.
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include "cwd_debug.h"
#include <libcw/private_threading.h>
#include <libcw/private_mutex.inl>
#include <unistd.h>
#include <map>

namespace libcw {
  namespace debug {
    namespace _private_ {

bool WST_multi_threaded = false;
bool WST_first_thread_initialized = false;
#if CWDEBUG_DEBUG
int instance_locked[instance_locked_size];
#if CWDEBUG_DEBUGT
pthread_t locked_by[instance_locked_size];
void const* locked_from[instance_locked_size];
#endif
#endif

void initialize_global_mutexes(void) throw()
{
#if !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
  mutex_tct<mutex_initialization_instance>::initialize();
  rwlock_tct<object_files_instance>::initialize();
  mutex_tct<dlopen_map_instance>::initialize();
  mutex_tct<set_ostream_instance>::initialize();
  mutex_tct<kill_threads_instance>::initialize();
  rwlock_tct<threadlist_instance>::initialize();
#if CWDEBUG_ALLOC
  mutex_tct<alloc_tag_desc_instance>::initialize();
  mutex_tct<memblk_map_instance>::initialize();
  rwlock_tct<location_cache_instance>::initialize();
  mutex_tct<list_allocations_instance>::initialize();
#endif
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
  mutex_tct<type_info_of_instance>::initialize();
#endif
#endif // !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
}

#if LIBCWD_USE_LINUXTHREADS
// Specialization.
template <>
  pthread_mutex_t mutex_tct<tsd_initialization_instance>::S_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

void mutex_ct::M_initialize(void) throw()
{
  pthread_mutexattr_t mutex_attr;
#if CWDEBUG_DEBUGT
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
#else
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
#endif
  pthread_mutex_init(&M_mutex, &mutex_attr);
  M_initialized = true;
}

void fatal_cancellation(void* arg) throw()
{
  char* text = static_cast<char*>(arg);
  Dout(dc::core, "Cancelling a thread " << text << ".  This is not supported by libcwd, sorry.");
}

//===================================================================================================
// Thread Specific Data
//

TSD_st __libcwd_tsd_array[PTHREAD_THREADS_MAX];

#if LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS
pthread_key_t TSD_st::S_exit_key;
pthread_once_t TSD_st::S_exit_key_once = PTHREAD_ONCE_INIT;

extern void debug_tsd_init(LIBCWD_TSD_PARAM);
void threading_tsd_init(LIBCWD_TSD_PARAM);

void TSD_st::S_initialize(void) throw()
{
  int oldtype;
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
  mutex_tct<tsd_initialization_instance>::initialize();
  mutex_tct<tsd_initialization_instance>::lock();
  debug_tsd_st* old_array[LIBCWD_DO_MAX];
  std::memcpy(old_array, do_array, sizeof(old_array));
  threadlist_t::iterator old_thread_iter = thread_iter;
  bool old_thread_iter_valid = thread_iter_valid;
  std::memset(this, 0, sizeof(struct TSD_st));	// This structure might be reused and therefore already contain data.
  tid = pthread_self();
  pid = getpid();
  mutex_tct<tsd_initialization_instance>::unlock();
  // We assume that the main() thread will call malloc() at least
  // once before it reaches main() and thus before any other thread is created.
  // When it does we get here; and thus are still single threaded.
  if (!WST_first_thread_initialized)		// Is this the first thread?
  {
    WST_first_thread_initialized = true;
    initialize_global_mutexes();		// This is a good moment to initialize all pthread mutexes.
  }
  else
  {
    WST_multi_threaded = true;
    set_alloc_checking_off(*this);
    for (int i = 0; i < LIBCWD_DO_MAX; ++i)
      if (old_array[i])
	delete old_array[i];			// Free old objects when this structure is being reused.
    set_alloc_checking_on(*this);
    debug_tsd_init(*this);			// Initialize the TSD of existing debug objects.
  }
  if (old_thread_iter_valid)
    (*old_thread_iter).tsd_destroyed();
  threading_tsd_init(*this);			// Initialize the TSD of stuff that goes in threading.cc.
  pthread_once(&S_exit_key_once, &TSD_st::S_exit_key_alloc);
  pthread_setspecific(S_exit_key, (void*)this);
  pthread_setcanceltype(oldtype, NULL);
}

void TSD_st::S_exit_key_alloc(void) throw()
{
  pthread_key_create(&S_exit_key, &TSD_st::S_cleanup_routine);
}

void TSD_st::cleanup_routine(void) throw()
{
  set_alloc_checking_off(*this);
  for (int i = 0; i < LIBCWD_DO_MAX; ++i)
    if (do_array[i])
    {
      debug_tsd_st* ptr = do_array[i];
      if (ptr->tsd_keep)
	continue;
      do_off_array[i] = 0;			// Turn all debugging off!  Now, hopefully, we won't use do_array[i] anymore.
      do_array[i] = NULL;			// So we won't free it again.
      ptr->tsd_initialized = false;
      delete ptr;				// Free debug object TSD.
    }
  set_alloc_checking_on(*this);
  terminated = true;
}

void TSD_st::S_cleanup_routine(void* arg) throw()
{
  TSD_st* obj = reinterpret_cast<TSD_st*>(arg);
  obj->cleanup_routine();
}

#endif // LIBCWD_USE_POSIX_THREADS || LIBCWD_USE_LINUXTHREADS

// End of Thread Specific Data
//===================================================================================================

int pthread_lock_interface_ct::trylock(void)
{
#if CWDEBUG_DEBUGT
  bool success = pthread_mutex_trylock(ptr);
  if (success)
  {
    LIBCWD_TSD_DECLARATION
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

void pthread_lock_interface_ct::lock(void)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION
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

void pthread_lock_interface_ct::unlock(void)
{
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION
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
// At this moment the only use of this object (thread_ct) is the
// memblk_map with memory allocations per thread.  Allocations
// done by a thread are stored in this map.  If the thread
// exists before all the memory is freed, then the thread_ct
// object is kept (until all memory is finally freed by other
// threads).
//

// A pointer to the global list with thread_ct objects.
// This must be a pointer -and not a global object- because
// it is being used before the global objects are initialized.
// Access to this list is locked with threadlist_instance.
threadlist_t* threadlist;

// Called when a new thread is detected.
// Adds a new thread_ct to threadlist and initializes it.
void threading_tsd_init(LIBCWD_TSD_PARAM)
{
  LIBCWD_DEFER_CANCEL;
  rwlock_tct<threadlist_instance>::wrlock();
  set_alloc_checking_off(__libcwd_tsd);
  if (!threadlist)
    threadlist = new threadlist_t;
  __libcwd_tsd.thread_iter = threadlist->insert(threadlist->end(), thread_ct(&__libcwd_tsd));
  __libcwd_tsd.thread_iter_valid = true;
  (*__libcwd_tsd.thread_iter).initialize(&__libcwd_tsd);
  set_alloc_checking_on(__libcwd_tsd);
  rwlock_tct<threadlist_instance>::wrunlock();
  LIBCWD_RESTORE_CANCEL;
}

// The default constructor of a thread_ct object.
// No real initialization is done yet, for that thread_ct::initialize
// needs to be called.  The reason for that is because 'new_memblk_map'
// (see thread_ct::initialize) will use allocator_adaptor<> which will
// try to dereference TSD_st::thread_iter which is not initialized yet because
// this object isn't yet added to threadlist at the moment of its construction.
thread_ct::thread_ct(TSD_st* tsd_ptr) throw()
{
  std::memset(this, 0 , sizeof(thread_ct));		// This is ok: we have no virtual table or base classes.
  tsd = tsd_ptr;
  tid = tsd->tid;
}

// Initialization of a freshly added thread_ct.
// The threadlist_instance mutex needs to be locked
// before insertion of the thread_ct takes place and
// not be unlocked untill initialization of the object
// has finished.
extern void* new_memblk_map(LIBCWD_TSD_PARAM);
void thread_ct::initialize(TSD_st* tsd_ptr) throw()
{
#if CWDEBUG_DEBUGT
  if (!tsd_ptr->internal || !is_locked(threadlist_instance))
    core_dump();
#endif
  current_alloc_list =  &base_alloc_list;
  thread_mutex.initialize();
  thread_mutex.lock();			// Need to be lock because the othewise sanity_check() of the maps allocator will fail.
  						// Its not really necessary to lock of course, because threadlist_instance is locked as
						// well and thus this is the only thread that could possibly access the map.
  // new_memblk_map allocates a new map<> for the current thread.
  // The implementation of new_memblk_map resides in debugmalloc.cc.
  memblk_map = new_memblk_map(*tsd_ptr);
  thread_mutex.unlock();
}

// This member function is called when the TSD_st structure
// is being reused, which is currently our only way to know
// for sure that the corresponding thread has terminated.
extern bool delete_memblk_map(void* memblk_map LIBCWD_COMMA_TSD_PARAM);
void thread_ct::tsd_destroyed(void) throw()
{
#if CWDEBUG_DEBUGT
  if (!tsd->internal)
    core_dump();
#endif
  // Must lock the threadlist because we might delete the map (if it is empty)
  // at which point another thread shouldn't be trying to search that map,
  // looping over all elements of threadlist.
  // Cancel is already defered (we're called from TSD_st::S_initialize).
  rwlock_tct<threadlist_instance>::wrlock();
  // delete_memblk_map will delete memblk_map (which is actually a
  // pointer to the type memblk_map_ct) if the map is empty and
  // return true, or it does nothing and returns false.
  // Also this function is implemented in debugmalloc.cc because
  // memblk_map_ct is undefined here.
  if (delete_memblk_map(memblk_map, *tsd))	// Returns true if memblk_map was deleted.
  {
    threadlist->erase(tsd->thread_iter);	// We're done with this thread object.
    tsd->thread_iter_valid = false;
  }
  rwlock_tct<threadlist_instance>::wrunlock();
  tsd = NULL;					// This causes the memblk_map to be deleted as soon as the last
  						// allocation belonging to this thread is freed.
}

#if CWDEBUG_DEBUGT
struct keypair_key_st {
  int smallest_instance;
  int largest_instance;
};

struct keypair_info_st {
  bool smallest_locked_first;
  void const* from_first;
  void const* from_second;
};

struct keypair_compare_st {
  bool operator()(keypair_key_st const& a, keypair_key_st const& b) const;
};

bool keypair_compare_st::operator()(keypair_key_st const& a, keypair_key_st const& b) const
{
  return (a.smallest_instance == b.smallest_instance) ? (a.largest_instance < b.largest_instance) : (a.smallest_instance < b.smallest_instance);
}

typedef std::pair<keypair_key_st const, keypair_info_st> keypair_map_value_t;
typedef std::map<keypair_key_st, keypair_info_st, keypair_compare_st, internal_allocator::rebind<keypair_map_value_t>::other> keypair_map_t;
static keypair_map_t* keypair_map;

void test_lock_pair(int instance_first, void const* from_first, int instance_second, void const* from_second)
{
  if (instance_first == instance_second)
    return;	// Must have been a recursive lock.
  if (!keypair_map)
    keypair_map = new keypair_map_t;
  keypair_key_st keypair_key;
  keypair_info_st keypair_info;
  if (instance_first < instance_second)
  {
    keypair_key.smallest_instance = instance_first;
    keypair_key.largest_instance = instance_second;
    keypair_info.smallest_locked_first = true;
  }
  else
  {
    keypair_key.smallest_instance = instance_second;
    keypair_key.largest_instance = instance_first;
    keypair_info.smallest_locked_first = false;
  }
  keypair_info.from_first = from_first;
  keypair_info.from_second = from_second;
  std::pair<keypair_map_t::iterator, bool> result = keypair_map->insert(keypair_map_value_t(keypair_key, keypair_info));
  if (!result.second)
  {
    if ((*result.first).second.smallest_locked_first != keypair_info.smallest_locked_first)
    {
      if (instance_first > 1000)
	FATALDEBUGDEBUG_CERR("Lock " << (void*)instance_first << " locked at " << from_first);
      else
	FATALDEBUGDEBUG_CERR("Lock " << instance_first << " locked at " << from_first);
      if (instance_second > 1000)
        FATALDEBUGDEBUG_CERR(" followed by lock " << (void*)instance_second << " at " << from_second << ".");
      else
        FATALDEBUGDEBUG_CERR(" followed by lock " << instance_second << " at " << from_second << ".");
      FATALDEBUGDEBUG_CERR("Previously, these locks were locked in reversed order at " << (*result.first).second.from_first <<
	  " and " << (*result.first).second.from_second << " respectively though!");
      core_dump();
    }
  }
  else
  {
    FATALDEBUGDEBUG_CERR("\nKEYPAIR: first: " << instance_first << "; second: " << instance_second);
  }
}

void test_for_deadlock(int instance, struct TSD_st& __libcwd_tsd, void const* from)
{
  if (!WST_multi_threaded)
    return;			// Take the time to get initialized.

  Debug( libcw_do.off() );
  set_alloc_checking_off(__libcwd_tsd);

  // We don't use a lock here because we can't.  I hope that in fact this is
  // not a problem because threadlist is a list<> and new elements will be added
  // to the end.  None of the new items will be of interest to us.  It is possible
  // that an item is *removed* from the list (see thread_ct::tsd_destroyed above)
  // but that is very unlikely in the cases where this test is actually being
  // used (we don't test with more than 1024 threads).
  for (threadlist_t::iterator iter = threadlist->begin(); iter != threadlist->end(); ++iter)
  {
    mutex_ct const* mutex = &((*iter).thread_mutex);
    if (mutex->M_locked_by == __libcwd_tsd.tid && mutex->M_instance_locked >= 1)
      test_lock_pair(reinterpret_cast<int>(mutex), mutex->M_locked_from, instance, from);
  }
  for (int inst = 0; inst < instance_locked_size; ++inst)
  {
    if (inst < instance_rdlocked_size && __libcwd_tsd.rdlocked_by1[inst] == __libcwd_tsd.tid && __libcwd_tsd.instance_rdlocked[inst] >= 1)
      test_lock_pair(inst, __libcwd_tsd.rdlocked_from1[inst], instance, from);
    if (inst < instance_rdlocked_size && __libcwd_tsd.rdlocked_by2[inst] == __libcwd_tsd.tid && __libcwd_tsd.instance_rdlocked[inst] >= 2)
      test_lock_pair(inst, __libcwd_tsd.rdlocked_from2[inst], instance, from);
    if (locked_by[inst] == __libcwd_tsd.tid && instance_locked[inst] >= 1)
      test_lock_pair(inst, locked_from[inst], instance, from);
  }

  set_alloc_checking_on(__libcwd_tsd);
  Debug( libcw_do.on() );
}
#endif

    } // namespace _private_
  } // namespace debug
} // namespace libcw
