// $Header$
//
// Copyright (C) 2001 - 2004, by
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
#include "libcwd/debug.h"
#include <libcwd/private_threading.h>
#include <libcwd/private_mutex.inl>
#include <libcwd/core_dump.h>
#include <unistd.h>
#include <alloca.h>
#include <map>
#include "macros.h"

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
  mutex_tct<static_tsd_instance>::initialize();
  mutex_tct<mutex_initialization_instance>::initialize();
  rwlock_tct<object_files_instance>::initialize();
  mutex_tct<dlopen_map_instance>::initialize();
  mutex_tct<dlclose_instance>::initialize();
  mutex_tct<set_ostream_instance>::initialize();
  mutex_tct<kill_threads_instance>::initialize();
  rwlock_tct<threadlist_instance>::initialize();
#if CWDEBUG_ALLOC
  mutex_tct<alloc_tag_desc_instance>::initialize();
  mutex_tct<memblk_map_instance>::initialize();
  rwlock_tct<location_cache_instance>::initialize();
  mutex_tct<list_allocations_instance>::initialize();
#endif
#if CWDEBUG_DEBUGT
  mutex_tct<keypair_map_instance>::initialize();
#endif
#endif // !LIBCWD_USE_LINUXTHREADS || CWDEBUG_DEBUGT
}

#if LIBCWD_USE_LINUXTHREADS
// Specialization.
template <>
  pthread_mutex_t mutex_tct<static_tsd_instance>::S_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

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
struct static_tsd_array_wat;

struct static_tsd_array_wrapper_base {
  protected:
    friend struct static_tsd_array_wat;
    TSD_st array[CW_THREADSMAX];
};

struct static_tsd_array_wrapper : public static_tsd_array_wrapper_base {
  ~static_tsd_array_wrapper() { /* Make sure no other thread touches this array while destructing it */ mutex_tct<static_tsd_instance>::lock(); memset(array, 0, sizeof(array)); }
};

// Access to these global variables is protected by the lock static_tsd_instance.
static static_tsd_array_wrapper static_tsd_array;

struct static_tsd_array_wat {
  static_tsd_array_wat() { mutex_tct<static_tsd_instance>::lock(); }
  ~static_tsd_array_wat() { mutex_tct<static_tsd_instance>::unlock(); }
  TSD_st& operator[](int index) const { return static_tsd_array.array[index]; }
};

static TSD_st* find_static_tsd(pthread_t tid, static_tsd_array_wat const& static_tsd_array_w)
{
  for (size_t i = 0; i < CW_THREADSMAX; ++i)
    if (pthread_equal(static_tsd_array_w[i].tid, tid))
      return &static_tsd_array_w[i];
  return NULL;
}

static TSD_st* allocate_static_tsd(static_tsd_array_wat const& static_tsd_array_w)
{
  int oldest_terminating = INT_MAX;
  size_t oldest_terminating_index = 0;	// Initialize to avoid 'may be used uninitialized' compiler warning.
  for (size_t i = 0; i < CW_THREADSMAX; ++i)
    if (static_tsd_array_w[i].tid == 0)
      return &static_tsd_array_w[i];
    else if (static_tsd_array_w[i].terminating && !static_tsd_array_w[i].inside_free && static_tsd_array_w[i].terminating < oldest_terminating)
    {
      oldest_terminating = static_tsd_array_w[i].terminating;
      oldest_terminating_index = i;
    }
  if (oldest_terminating == INT_MAX)	// This means that more than CW_THREADSMAX threads are either
  {					// inside free() or initializing after just being created.
    std::cerr << "\n****** More threads than THREADSMAX.  Reconfigure libcwd ******\n" << std::endl;
    core_dump();
  }
  return &static_tsd_array_w[oldest_terminating_index];
}

static void release_static_tsd(TSD_st* tsd)
{
  tsd->tid = 0;
  tsd->thread_iter_valid = false;
#ifdef _GLIBCXX_DEBUG
  std::memset(&tsd->thread_iter, 0 , sizeof(tsd->thread_iter));
#endif
}

pthread_key_t TSD_st::S_tsd_key;
pthread_once_t TSD_st::S_tsd_key_once = PTHREAD_ONCE_INIT;

extern void debug_tsd_init(LIBCWD_TSD_PARAM);
void threading_tsd_init(LIBCWD_TSD_PARAM);
static TSD_st terminating_thread_tsd;
static int terminating_count;

//static
TSD_st& TSD_st::instance()
{
  TSD_st* instance;
  if (!WST_tsd_key_created || !(instance = (TSD_st*)pthread_getspecific(S_tsd_key)))
    return S_create(0);
  return *instance;
}

// This function is called at the start of free().
//static
TSD_st& TSD_st::instance_free()
{
  TSD_st* instance;
  if (!WST_tsd_key_created || !(instance = (TSD_st*)pthread_getspecific(S_tsd_key)))
    return S_create(1);
  else
    instance->inside_free++;
  return *instance;
}

TSD_st* main_thread_tsd;

TSD_st& TSD_st::S_create(int from_free)
{
  int oldtype;
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

  pthread_t _tid = pthread_self();

  mutex_tct<static_tsd_instance>::initialize();
  bool done = false;
  TSD_st* static_tsd;
  threadlist_t::iterator old_thread_iter;
  bool old_thread_iter_valid = false;
  do
  {
    static_tsd_array_wat static_tsd_array_w;

    static_tsd = find_static_tsd(_tid, static_tsd_array_w);
    if (static_tsd)
    {
      if (from_free == 1)
	static_tsd->inside_free++;
      if (static_tsd->inside_free || !static_tsd->terminating)
      {
	done = true;
	break;
      }
    }
    else
      static_tsd = allocate_static_tsd(static_tsd_array_w);

    old_thread_iter_valid = static_tsd->thread_iter_valid;
    if (old_thread_iter_valid)
      old_thread_iter = static_tsd->thread_iter;

    // Fill the temporary structure with zeroes.
PRAGMA_DIAGNOSTIC_PUSH_IGNORE_class_memaccess
    std::memset(static_tsd, 0, sizeof(struct TSD_st));
PRAGMA_DIAGNOSTIC_POP
    static_tsd->tid = _tid;			// Make sure nobody else will allocate this entry.

    if (from_free == 1)
      static_tsd->inside_free = 1;
  }
  while(0);
  if (done)
  {
    pthread_setcanceltype(oldtype, NULL);
    return *static_tsd;
  }

  static_tsd->pid = getpid();

  if (!WST_first_thread_initialized)		// Is this the first thread?
  {
    WST_first_thread_initialized = true;
#ifdef _CS_GNU_LIBPTHREAD_VERSION
    size_t n = confstr (_CS_GNU_LIBPTHREAD_VERSION, NULL, 0);
    if (n > 0)
    {
      char* buf = (char*)alloca(n);
      confstr(_CS_GNU_LIBPTHREAD_VERSION, buf, n);
      if (strstr (buf, "NPTL"))
	WST_is_NPTL = true;
    }
#endif
    initialize_global_mutexes();
    threading_tsd_init(*static_tsd);		// Initialize the TSD of stuff that goes in threading.cc.
#if CWDEBUG_ALLOC
    // Initialize libcwd now, before calling pthread_setspecific() below.
    // Otherwise the call to calloc() by pthread_setspecific might cause initialization of static_tsd
    // while its contents have already been copied to real_tsd, causing this initialization to get lost.
    init_debugmalloc();
#endif
  }
  else
  {
    WST_multi_threaded = true;
    debug_tsd_init(*static_tsd);		// Initialize the TSD of existing debug objects.
    threading_tsd_init(*static_tsd);		// Initialize the TSD of stuff that goes in threading.cc.
  }

  TSD_st* real_tsd;

  if (from_free == 0)
  {
    // Time to put the real TSD into place.
    set_alloc_checking_off(*static_tsd);
    if (old_thread_iter_valid)
      old_thread_iter->terminated(old_thread_iter, *static_tsd);
    real_tsd = new TSD_st;
    set_alloc_checking_on(*static_tsd);
    std::memcpy(real_tsd, static_tsd, sizeof(TSD_st));

    pthread_once(&S_tsd_key_once, &S_tsd_key_alloc);
    pthread_setspecific(S_tsd_key, (void*)real_tsd);

    if (!main_thread_tsd)
      main_thread_tsd = real_tsd;

    // Release the static TSD - we're not using it anymore now the key is set.
    mutex_tct<static_tsd_instance>::lock();
    release_static_tsd(static_tsd);
    mutex_tct<static_tsd_instance>::unlock();
  }
  else
  {
    // This should *seldom* happen.
    // When we can't find a (static) tsd, and this thread is calling free(), then
    // that means that this thread is terminating - but that between its key
    // destruction phase (where we created the static tsd for it) and now its
    // static tsd was overwritten by another thread.  Just keep the current
    // static tsd instead of installing a pthread key.
    mutex_tct<static_tsd_instance>::lock();
    static_tsd->terminating = ++terminating_count;
    mutex_tct<static_tsd_instance>::unlock();
    real_tsd = static_tsd;
    real_tsd->thread_iter->terminating();		// FIXME - what about the old thread_ct?
  }

  pthread_setcanceltype(oldtype, NULL);

  return *real_tsd;
}

void TSD_st::free_instance(TSD_st& tsd)
{
  mutex_tct<static_tsd_instance>::lock();
  tsd.inside_free--;
  if (tsd.inside_free < 0)
    core_dump();
  mutex_tct<static_tsd_instance>::unlock();
}

bool WST_tsd_key_created = false;

void TSD_st::S_tsd_key_alloc()
{
  pthread_key_create(&S_tsd_key, &TSD_st::S_cleanup_routine);	// Leaks memory, we never destruct this key again.
  WST_tsd_key_created = true;
}

#define VALGRIND 0

void TSD_st::cleanup_routine()
{
#if !VALGRIND
  if (++tsd_destructor_count < PTHREAD_DESTRUCTOR_ITERATIONS)
#else
  if (1)	// Valgrind doesn't iterate the key destruction routines.
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
	do_off_array[i] = 0;			// Turn all debugging off!  Now, hopefully, we won't use do_array[i] anymore.
	do_array[i] = NULL;			// So we won't free it again.
	ptr->tsd_initialized = false;
#if CWDEBUG_ALLOC
	internal = 1;
#endif
	delete ptr;				// Free debug object TSD.
#if CWDEBUG_ALLOC
	internal = 0;
#endif
      }

    int oldtype;
    pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, &oldtype);
    TSD_st* static_tsd;
    {
      static_tsd_array_wat static_tsd_array_w;
      static_tsd = allocate_static_tsd(static_tsd_array_w);
      std::memcpy(static_tsd, this, sizeof(TSD_st));	// Move tsd to the static array.
      release_static_tsd(this);
      static_tsd->terminating = ++terminating_count;
      static_tsd->thread_iter->terminating();
    }
    pthread_setcanceltype(oldtype, NULL);

    pthread_setspecific(S_tsd_key, (void*)0);	// Make sure that instance_free() won't use the KEY anymore!
    // Then we can savely delete the current TSD.
#if CWDEBUG_ALLOC
    static_tsd->internal = 1;			// We can't call set_alloc_checking_off, because with --enable-debugm
						// that will do a LIBCWD_TSD_DECLARATION, causing a new key
						// to be generated.
#endif
    delete this;
#if CWDEBUG_ALLOC
    static_tsd->internal = 0;
#endif
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
  set_alloc_checking_off(LIBCWD_TSD);
  if (!threadlist)
    threadlist = new threadlist_t;
  __libcwd_tsd.thread_iter = threadlist->insert(threadlist->end(), thread_ct());
  __libcwd_tsd.thread_iter_valid = true;
  __libcwd_tsd.thread_iter->initialize(LIBCWD_TSD);
  set_alloc_checking_on(LIBCWD_TSD);
  rwlock_tct<threadlist_instance>::wrunlock();
  LIBCWD_RESTORE_CANCEL;
}

// The default constructor of a thread_ct object.
// No real initialization is done yet, for that thread_ct::initialize
// needs to be called.  The reason for that is because 'new_memblk_map'
// (see thread_ct::initialize) will use allocator_adaptor<> which will
// try to dereference TSD_st::thread_iter which is not initialized yet because
// this object isn't yet added to threadlist at the moment of its construction.
// The threadlist_instance mutex needs to be locked
// before insertion of the thread_ct takes place and
// not be unlocked until initialization of the object
// has finished.
#if CWDEBUG_ALLOC
extern void* new_memblk_map(LIBCWD_TSD_PARAM);
#endif
void thread_ct::initialize(LIBCWD_TSD_PARAM)
{
#if CWDEBUG_ALLOC
#if CWDEBUG_DEBUGT
  if (!__libcwd_tsd.internal || !is_locked(threadlist_instance))
    core_dump();
#endif
  std::memset(this, 0 , sizeof(thread_ct));		// This is ok: we have no virtual table or base classes.
  current_alloc_list =  &base_alloc_list;		// This is why we may only initialize thread_ct after
							// it reached it final place, and may not move the object
							// anymore after that!
#endif
  thread_mutex.initialize();
#if CWDEBUG_ALLOC
  thread_mutex.lock();		// Need to be locked because otherwise sanity_check() of the maps allocator will fail.
				// Its not really necessary to lock of course, because threadlist_instance is locked as
				// well and thus this is the only thread that could possibly access the map.
  // new_memblk_map allocates a new map<> for the current thread.
  // The implementation of new_memblk_map resides in debugmalloc.cc.
  memblk_map = new_memblk_map(LIBCWD_TSD);
  DEBUGDEBUG_CERR("My memblk_map is " << (void*)memblk_map << " (thread_ct at " << (void*)this << ", thread_iter at " << (void*)&__libcwd_tsd.thread_iter << ", __libcwd_tsd at " << (void*)&__libcwd_tsd << ')');
  thread_mutex.unlock();
#endif
  tid = __libcwd_tsd.tid;
}

// This member function is called when the TSD_st structure
// is being reused, which is currently our only way to know
// for sure that the corresponding thread has terminated.
extern bool delete_memblk_map(void* memblk_map LIBCWD_COMMA_TSD_PARAM);
void thread_ct::terminated(threadlist_t::iterator
#if CWDEBUG_ALLOC
    thread_iter LIBCWD_COMMA_TSD_PARAM
#elif LIBCWD_THREAD_SAFE
    , ::libcwd::_private_::TSD_st&
#endif
    )
{
#if CWDEBUG_ALLOC
  set_alloc_checking_off(LIBCWD_TSD);
#if CWDEBUG_DEBUGT
  // Cancel is already defered (we're called from TSD_st::S_create).
  __libcwd_tsd.cancel_explicitely_deferred++;
#endif
  // Must lock the threadlist because we might delete the map (if it is empty)
  // at which point another thread shouldn't be trying to search that map,
  // looping over all elements of threadlist.
  rwlock_tct<threadlist_instance>::wrlock();
  // delete_memblk_map will delete memblk_map (which is actually a
  // pointer to the type memblk_map_ct) if the map is empty and
  // return true, or it does nothing and returns false.
  // Also this function is implemented in debugmalloc.cc because
  // memblk_map_ct is undefined here.
  if (delete_memblk_map(memblk_map, LIBCWD_TSD))	// Returns true if memblk_map was deleted.
  {
    DEBUGDEBUG_CERR("Erasing from threadlist memblk_map " << (void*)thread_iter->memblk_map << " (thread_ct at " << (void*)&(*thread_iter) << " which should be equal to " << (void*)this << ", [old_]thread_iter at " << (void*)&thread_iter << ')');
    memblk_map = NULL;
    threadlist->erase(thread_iter);			// We're done with this thread object.
  }
  else
    M_zombie = true;				// This causes the memblk_map to be deleted as soon as the last
						// allocation belonging to this thread is freed.
  rwlock_tct<threadlist_instance>::wrunlock();
#if CWDEBUG_DEBUGT
  __libcwd_tsd.cancel_explicitely_deferred--;
#endif
  set_alloc_checking_on(LIBCWD_TSD);
#endif
}

#if CWDEBUG_DEBUGT
#include <inttypes.h>

// These are the different groups that are allowed together.
uint16_t const group1 = 0x002;	// Instance 1 locked first.
uint16_t const group2 = 0x004;	// Instance 2 locked first.
uint16_t const group3 = 0x020;	// Instance 1 read only and high_priority when second.
uint16_t const group4 = 0x040;	// Instance 2 read only and high_priority when second.
uint16_t const group5 = 0x200;	// (Instance 1 locked first and (either one read only and high_priority when second))
				// or (both read only and high_priority when second).
uint16_t const group6 = 0x400;	// (Instance 2 locked first and (either one read only and high_priority when second))
				// or (both read only and high_priority when second).

struct keypair_key_st {
  size_t instance1;
  size_t instance2;
};

struct keypair_info_st {
  uint16_t state;
  int limited;
  void const* from_first[5];
  void const* from_second[5];
};

struct keypair_compare_st {
  bool operator()(keypair_key_st const& a, keypair_key_st const& b) const;
};

// Definition of the map<> that holds the key instance pairs that are ever locked simultaneously by the same thread.
typedef std::pair<keypair_key_st const, keypair_info_st> keypair_map_value_t;
#if CWDEBUG_ALLOC
typedef std::map<keypair_key_st, keypair_info_st, keypair_compare_st, internal_allocator::rebind<keypair_map_value_t>::other> keypair_map_t;
#else
typedef std::map<keypair_key_st, keypair_info_st, keypair_compare_st> keypair_map_t;
#endif
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
    return;	// Must have been a recursive lock.

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
    keypair_info.state = ((keypair_info.state << 1) | (keypair_info.state >> 1)) & (group1|group2|group3|group4|group5|group6);
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
    stored_info.state &= keypair_info.state;			// Limit possible groups.
    if (prev_state != stored_info.state)
    {
      stored_info.limited++;
      stored_info.from_first[stored_info.limited] = from_first;
      stored_info.from_second[stored_info.limited] = from_second;
      DEBUGDEBUG_CERR("\nKEYPAIR: first: " << instance_first << (first_is_readonly ? " (read-only lock)" : "")
          << "; second: " << instance_second << (second_is_high_priority ? " (high priority lock)" : "")
          << "; groups: " << (void*)(unsigned long)stored_info.state << '\n');
    }
    if (stored_info.state == 0)					// No group left?
    {
      FATALDEBUGDEBUG_CERR("\nKEYPAIR: There is a potential deadlock between lock " << keypair_key.instance1 << " and " << keypair_key.instance2 << '.');
      FATALDEBUGDEBUG_CERR("\nKEYPAIR: Previously, these locks were locked in the following locations:");
      for (int cnt = 0; cnt <= stored_info.limited; ++cnt)
	FATALDEBUGDEBUG_CERR("\nKEYPAIR: First at " << stored_info.from_first[cnt] << " and then at " << stored_info.from_second[cnt]);
      mutex_tct<keypair_map_instance>::unlock();
      core_dump();
    }
  }
  else
  {
    DEBUGDEBUG_CERR("\nKEYPAIR: first: " << instance_first << (first_is_readonly ? " (read-only lock)" : "")
	<< "; second: " << instance_second << (second_is_high_priority ? " (high priority lock)" : "")
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
    return;			// Give libcwd the time to get initialized.

  if (instance == keypair_map_instance)
    return;

  set_alloc_checking_off(__libcwd_tsd);

  // Initialization.
  if (!keypair_map)
    keypair_map = new keypair_map_t;	// LEAK 28 bytes.  This is never freed anymore.

  // We don't use a lock here because we can't.  I hope that in fact this is
  // not a problem because threadlist is a list<> and new elements will be added
  // to the end.  None of the new items will be of interest to us.  It is possible
  // that an item is *removed* from the list (see thread_ct::terminated above)
  // but that is very unlikely in the cases where this test is actually being
  // used (we don't test with more than 1024 threads).
  for (threadlist_t::iterator iter = threadlist->begin(); iter != threadlist->end(); ++iter)
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
    if (inst < instance_rdlocked_size && __libcwd_tsd.rdlocked_by1[inst] == __libcwd_tsd.tid && __libcwd_tsd.instance_rdlocked[inst] >= 1)
      test_lock_pair(inst + read_lock_offset, __libcwd_tsd.rdlocked_from1[inst], instance, from);
    if (inst < instance_rdlocked_size && __libcwd_tsd.rdlocked_by2[inst] == __libcwd_tsd.tid && __libcwd_tsd.instance_rdlocked[inst] >= 2)
      test_lock_pair(inst + read_lock_offset, __libcwd_tsd.rdlocked_from2[inst], instance, from);
    // Check for write locks and normal mutexes.
    if (locked_by[inst] == __libcwd_tsd.tid && instance_locked[inst] >= 1 && inst != keypair_map_instance)
      test_lock_pair(inst, locked_from[inst], instance, from);
  }

  set_alloc_checking_on(__libcwd_tsd);
}

pthread_mutex_t raw_write_mutex = PTHREAD_MUTEX_INITIALIZER;

#endif // CWDEBUG_DEBUGT

  } // namespace _private_
} // namespace libcwd
