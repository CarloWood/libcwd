// $Header$
//
// Copyright (C) 2000 - 2002, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include "sys.h"
#include <libcw/debug_config.h>

#if (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && defined(_REENTRANT) && CWDEBUG_ALLOC
// This has to be very early (must not have been included elsewhere already).
#define private public  // Ugly, I know.
#include <bits/stl_alloc.h>
#undef private
#endif // (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && defined(_REENTRANT) && CWDEBUG_ALLOC

#include <errno.h>
#include <iostream>
#include <sys/time.h>     	// Needed for setrlimit()
#include <sys/resource.h>	// Needed for setrlimit()
#include <algorithm>
#include <new>
#include "cwd_debug.h"
#include <libcw/strerrno.h>
#include "private_debug_stack.inl"

extern "C" int raise(int);

#ifdef _REENTRANT
using libcw::debug::_private_::rwlock_tct;
using libcw::debug::_private_::debug_objects_instance;
using libcw::debug::_private_::debug_channels_instance;
#define DEBUG_OBJECTS_ACQUIRE_WRITE_LOCK	rwlock_tct<debug_objects_instance>::wrlock();
#define DEBUG_OBJECTS_RELEASE_WRITE_LOCK	rwlock_tct<debug_objects_instance>::wrunlock();
#define DEBUG_OBJECTS_ACQUIRE_READ_LOCK		rwlock_tct<debug_objects_instance>::rdlock();
#define DEBUG_OBJECTS_RELEASE_READ_LOCK		rwlock_tct<debug_objects_instance>::rdunlock();
#define DEBUG_OBJECTS_ACQUIRE_READ2WRITE_LOCK	rwlock_tct<debug_objects_instance>::rd2wrlock();
#define DEBUG_OBJECTS_ACQUIRE_WRITE2READ_LOCK	rwlock_tct<debug_objects_instance>::wr2rdlock();
#define DEBUG_CHANNELS_ACQUIRE_WRITE_LOCK	rwlock_tct<debug_channels_instance>::wrlock();
#define DEBUG_CHANNELS_RELEASE_WRITE_LOCK	rwlock_tct<debug_channels_instance>::wrunlock();
#define DEBUG_CHANNELS_ACQUIRE_READ_LOCK	rwlock_tct<debug_channels_instance>::rdlock();
#define DEBUG_CHANNELS_RELEASE_READ_LOCK	rwlock_tct<debug_channels_instance>::rdunlock();
#define DEBUG_CHANNELS_ACQUIRE_READ2WRITE_LOCK	rwlock_tct<debug_channels_instance>::rd2wrlock();
#define DEBUG_CHANNELS_ACQUIRE_WRITE2READ_LOCK	rwlock_tct<debug_channels_instance>::wr2rdlock();
#else // !_REENTRANT
#define DEBUG_OBJECTS_ACQUIRE_WRITE_LOCK
#define DEBUG_OBJECTS_RELEASE_WRITE_LOCK
#define DEBUG_OBJECTS_ACQUIRE_READ_LOCK
#define DEBUG_OBJECTS_RELEASE_READ_LOCK
#define DEBUG_OBJECTS_ACQUIRE_READ2WRITE_LOCK
#define DEBUG_OBJECTS_ACQUIRE_WRITE2READ_LOCK
#define DEBUG_CHANNELS_ACQUIRE_WRITE_LOCK
#define DEBUG_CHANNELS_RELEASE_WRITE_LOCK
#define DEBUG_CHANNELS_ACQUIRE_READ_LOCK
#define DEBUG_CHANNELS_RELEASE_READ_LOCK
#define DEBUG_CHANNELS_ACQUIRE_READ2WRITE_LOCK
#define DEBUG_CHANNELS_ACQUIRE_WRITE2READ_LOCK
#endif // !_REENTRANT

namespace libcw {
  namespace debug {

namespace _private_ {

#if (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && defined(_REENTRANT) && CWDEBUG_ALLOC
// The following tries to take the "node allocator" lock -- the lock of the
// default allocator for threaded applications. The parameter is the value to
// return when no lock exist. This should probably be implemented as a macro
// test instead.
__inline__
bool allocator_trylock(void)
{
  if (!(__NODE_ALLOCATOR_THREADS)) return true;
#if !defined(__GTHREAD_MUTEX_INIT) && defined(__GTHREAD_MUTEX_INIT_FUNCTION)
  if (!std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_init_flag)
    std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_initialize();
#endif
  return (__gthread_mutex_trylock(&std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_lock) == 0);
}

// The following unlocks the node allocator.
__inline__
void allocator_unlock(void)
{
  if (!(__NODE_ALLOCATOR_THREADS)) return;
#if !defined(__GTHREAD_MUTEX_INIT) && defined(__GTHREAD_MUTEX_INIT_FUNCTION)
  if (!std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_init_flag)
    std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_initialize();
#endif
  __gthread_mutex_unlock(&std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_lock);
}
#endif // (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && defined(_REENTRANT) && CWDEBUG_ALLOC

} // namespace _private_

    using _private_::set_alloc_checking_on;
    using _private_::set_alloc_checking_off;
    using _private_::debug_message_st;

    class buffer_ct : public _private_::internal_stringstream {
    private:
#if __GNUC__ < 3
      streampos position;
#else
      pos_type position;
#endif
    public:
#ifdef _REENTRANT
      void writeto(std::ostream* os, _private_::lock_interface_base_ct* mutex LIBCWD_COMMA_TSD_PARAM, debug_ct&
#if CWDEBUG_ALLOC
	  debug_object
#endif
	  )
#else
      void writeto(std::ostream* os LIBCWD_COMMA_TSD_PARAM, debug_ct&
#if CWDEBUG_ALLOC
	  debug_object
#endif
	  )
#endif // _REENTRANT
      {
#if (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && defined(_REENTRANT) && CWDEBUG_ALLOC
	typedef debug_message_st* msgbuf_t;
	msgbuf_t msgbuf;
	// Queue the message when the default (STL) allocator is locked and it could be that we
	// did that (because we got here via malloc or free).  At least as important: if
	// this is the last thread, just prior to exiting the application, and we CAN
	// get the lock - then DON'T queue the message (and thus flush all possibly queued
	// messages); this garantees that there will never messages be left in the queue
	// when the application exits.
        bool const queue_msg = __libcwd_tsd.inside_malloc_or_free && !_private_::allocator_trylock();
	if (__libcwd_tsd.inside_malloc_or_free && !queue_msg)
	  _private_::allocator_unlock();	// Always immedeately release the lock again.
	int const extra_size = sizeof(debug_message_st) - sizeof(msgbuf->buf);
#else
	typedef char* msgbuf_t;
	msgbuf_t msgbuf;
        bool const queue_msg = false;
	int const extra_size = 0;
#endif
	int curlen;
	curlen = rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out) - rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in);
	bool free_msgbuf = false;
	if (queue_msg)
	  msgbuf = (msgbuf_t)malloc(curlen + extra_size);
	else if (curlen > 512 || !(msgbuf = (msgbuf_t)__builtin_alloca(curlen + extra_size)))
	{
	  msgbuf = (msgbuf_t)malloc(curlen + extra_size);
	  free_msgbuf = true;
	}
#if (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && defined(_REENTRANT) && CWDEBUG_ALLOC
	rdbuf()->sgetn(msgbuf->buf, curlen);
#else
	rdbuf()->sgetn(msgbuf, curlen);
#endif
#if CWDEBUG_ALLOC
	// Writing to the final std::ostream (ie std::cerr) must be non-internal!
	// LIBCWD_DISABLE_CANCEL/LIBCWD_ENABLE_CANCEL must be done non-internal too.
	int saved_internal = __libcwd_tsd.internal;
	__libcwd_tsd.internal = 0;
	++__libcwd_tsd.library_call;
	++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
#endif
#ifdef _REENTRANT
	LIBCWD_DISABLE_CANCEL			// We don't want Dout() to be a cancellation point.
	if (mutex)
	  mutex->lock();
	else if (_private_::WST_multi_threaded)
	{
	  static bool WST_second_time = false;	// Break infinite loop.
	  if (!WST_second_time)
	  {
	    WST_second_time = true;
	    DoutFatal(dc::core, "When using multiple threads, you must provide a locking mechanism for the debug output stream.  "
		"You can pass a pointer to a mutex with `debug_ct::set_ostream' (see documentation/reference-manual/group__group__destination.html).");
	  }
	}
#endif // !_REENTRANT
#if (__GNUC__ >= 3 || __GNUC_MINOR__ >= 97) && defined(_REENTRANT) && CWDEBUG_ALLOC
	if (queue_msg)			// Inside a call to malloc and possibly owning lock of std::__default_alloc_template<true, 0>?
	{
	  // We don't write debug output to the final ostream when inside malloc and std::__default_alloc_template<true, 0> is locked.
	  // It is namely possible that this will again try to acquire the lock in std::__default_alloc_template<true, 0>, resulting
	  // in a dead lock.  Append it to the queue instead.
	  msgbuf->curlen = curlen;
	  msgbuf->prev = NULL;
	  msgbuf->next = debug_object.queue;
	  if (debug_object.queue)
	    debug_object.queue->prev = msgbuf;
	  else
	    debug_object.queue_top = msgbuf;
	  debug_object.queue = msgbuf;
	}
	else
	{
	  debug_message_st* message = debug_object.queue_top;
	  if (message)
	  {
	    // First empty the whole queue.
	    debug_message_st* next_message;
	    do
	    {
	      next_message = message->prev;
	      os->write(message->buf, message->curlen);
	      __libcwd_tsd.internal = 1;
	      free(message);
	      __libcwd_tsd.internal = 0;
	    }
	    while ((message = next_message));
	    debug_object.queue_top = debug_object.queue = NULL;
	  }
	  // Then write the new message.
	  os->write(msgbuf->buf, curlen);
	}
#else // !(CWDEBUG_ALLOC && defined(_REENTRANT))
	os->write(msgbuf, curlen);
#endif // !CWDEBUG_ALLOC
#ifdef _REENTRANT
	if (mutex)
	  mutex->unlock();
	LIBCWD_ENABLE_CANCEL
#endif // !_REENTRANT
#if CWDEBUG_ALLOC
	--LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
	--__libcwd_tsd.library_call;
	__libcwd_tsd.internal = saved_internal;
#endif
	if (free_msgbuf)
	  free(msgbuf);
      }
      void store_position(void) {
	position = rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out);
      }
      void restore_position(void) {
	rdbuf()->pubseekoff(position, ios_base::beg, ios_base::out);
	rdbuf()->pubseekoff(0, ios_base::beg, ios_base::in);
      }
    };

    // Configuration signature
    unsigned long const config_signature_lib_c = config_signature_header_c;

    // Put this here to decrease the code size of `check_configuration'
    void conf_check_failed(void)
    {
      DoutFatal(dc::fatal, "check_configuration: This version of libcwd was compiled with a different configuration than is currently used in libcw/debug_config.h!");
    }

    void version_check_failed(void)
    {
      DoutFatal(dc::fatal, "check_configuration: This version of libcwd does not match the version of libcw/debug_config.h!  Are your paths correct?");
    }

    /**
     * \brief The default %debug object.
     *
     * The %debug object that is used by default by \ref Dout and \ref DoutFatal, the only %debug object used by libcwd itself.
     * \sa \ref chapter_custom_do
     */

    debug_ct libcw_do;

    namespace {
      unsigned short int WST_max_len = 8;	// The length of the longest label.  Is adjusted automatically
    						// if a custom channel has a longer label.
    }

    namespace channels {
      namespace dc {

	/**
	 * \addtogroup group_default_dc Predefined Debug Channels
	 * \ingroup group_debug_channels
	 *
	 * These are the default %debug %channels pre-defined in libcwd.
	 */

	/** \{ */

	/** The DEBUG channel. */
	channel_ct debug
#ifndef HIDE_FROM_DOXYGEN
	    ("DEBUG")
#endif
	    ;

	/** The NOTICE channel. */
	channel_ct notice
#ifndef HIDE_FROM_DOXYGEN
	   ("NOTICE")
#endif
	   ;

	/** The SYSTEM channel. */
	channel_ct system
#ifndef HIDE_FROM_DOXYGEN
	    ("SYSTEM")
#endif
	    ;

	/** The MALLOC channel. */
	channel_ct malloc
#ifndef HIDE_FROM_DOXYGEN
	    ("MALLOC")
#endif
	    ;

	/** The WARNING channel.
	 *
	 * This is the only channel that
	 * is turned on by default.
	 */
	channel_ct warning
#ifndef HIDE_FROM_DOXYGEN
	    ("WARNING")
#endif
	    ;

	/** A special channel that is always turned on.
	 *
	 * This channel is <EM>%always</EM> on;
	 * it can not be turned off.&nbsp;
	 * It is not in the list of \ref ForAllDebugChannels "debug channels".&nbsp;
	 * When used with a label it will print as many '>'
	 * characters as the size of the largest real channel.
	 */
	always_channel_ct always;

	/** A special channel to continue to
	 * write a previous %debug channel.
	 *
	 * \sa \ref using_continued
	 */
	continued_channel_ct continued
#ifndef HIDE_FROM_DOXYGEN
	    (continued_maskbit)
#endif
	    ;

	/** A special channel to finish writing
	 * <EM>%continued</EM> %debug output.
	 *
	 * \sa \ref using_continued
	 */
	continued_channel_ct finish
#ifndef HIDE_FROM_DOXYGEN
	    (finish_maskbit)
#endif
	    ;

	/** The special FATAL channel.
	 *
	 * \sa DoutFatal
	 */
	fatal_channel_ct fatal
#ifndef HIDE_FROM_DOXYGEN
	    ("FATAL", fatal_maskbit)
#endif
	    ;

	/** The special COREDUMP channel.
	 *
	 * \sa DoutFatal
	 */
	fatal_channel_ct core
#ifndef HIDE_FROM_DOXYGEN
	    ("COREDUMP", coredump_maskbit)
#endif
	    ;

	/** \} */
      }
    }

#if CWDEBUG_LOCATION
    namespace cwbfd { extern bool ST_init(void); }
#endif

    void ST_initialize_globals(void)
    {
      static bool ST_already_called;
      if (ST_already_called)
	return;
      ST_already_called = true;
#if CWDEBUG_ALLOC
      init_debugmalloc();
#endif
#ifdef _REENTRANT
      _private_::initialize_global_mutexes();
#endif
      _private_::process_environment_variables();

      // Fatal channels need to be marked fatal, otherwise we get into an endless loop
      // when they are used before they are created.
      channels::dc::core.NS_initialize("COREDUMP", coredump_maskbit);
      channels::dc::fatal.NS_initialize("FATAL", fatal_maskbit);
      // Initialize other debug channels that might be used before we reach main().
      channels::dc::debug.NS_initialize("DEBUG");
      channels::dc::malloc.NS_initialize("MALLOC");
      channels::dc::continued.NS_initialize(continued_maskbit);
      channels::dc::finish.NS_initialize(finish_maskbit);
#if CWDEBUG_LOCATION
      channels::dc::bfd.NS_initialize("BFD");
#endif
      // What the heck, initialize all other debug channels too
      channels::dc::warning.NS_initialize("WARNING");
      channels::dc::notice.NS_initialize("NOTICE");
      channels::dc::system.NS_initialize("SYSTEM");

      libcw_do.NS_init();			// Initialize debug code.
#ifdef _REENTRANT
      libcw_do.keep_tsd(true);
#endif

      // Unlimit core size.
#ifdef RLIMIT_CORE
      struct rlimit corelim;
      if (getrlimit(RLIMIT_CORE, &corelim))
	DoutFatal(dc::fatal|error_cf, "getrlimit(RLIMIT_CORE, &corelim)");
      corelim.rlim_cur = corelim.rlim_max;
      if (corelim.rlim_max != RLIM_INFINITY && !_private_::suppress_startup_msgs)
      {
	debug_ct::OnOffState state;
	libcw_do.force_on(state);
	// The cast is necessary on platforms where corelim.rlim_max is long long
	// and libstdc++ was not compiled with support for long long.
	Dout(dc::warning, "core size is limited (hard limit: " << (unsigned long)(corelim.rlim_max / 1024) << " kb).  Core dumps might be truncated!");
	libcw_do.restore(state);
      }
      if (setrlimit(RLIMIT_CORE, &corelim))
	  DoutFatal(dc::fatal|error_cf, "unlimit core size failed");
#else
      if (!_private_::suppress_startup_msgs)
      {
	debug_ct::OnOffState state;
	libcw_do.force_on(state);
	Dout(dc::warning, "Please unlimit core size manually");
	libcw_do.restore(state);
      }
#endif

#if CWDEBUG_LOCATION
      cwbfd::ST_init();				// Initialize BFD code.
#endif
#if CWDEBUG_DEBUG && !CWDEBUG_DEBUGOUTPUT
      // Force allocation of a __cxa_eh_globals struct in libsupc++.
      (void)std::uncaught_exception();
#endif
    }

    namespace _private_ {

#ifndef _REENTRANT
      TSD_st __libcwd_tsd;
#endif

      debug_channels_ct debug_channels;		// List with all channel_ct objects.
      debug_objects_ct debug_objects;		// List with all debug devices.

      // _private_::
      void debug_channels_ct::init(LIBCWD_TSD_PARAM)
      {
#ifdef _REENTRANT
	_private_::rwlock_tct<_private_::debug_channels_instance>::initialize();
#endif
	DEBUG_CHANNELS_ACQUIRE_READ_LOCK
	if (!WNS_debug_channels)			// MT: `WNS_debug_channels' is only false when this object is still Non_Shared.
	{
	  DEBUG_CHANNELS_ACQUIRE_READ2WRITE_LOCK
	  set_alloc_checking_off(LIBCWD_TSD);
	  WNS_debug_channels = new debug_channels_ct::container_type;
	  set_alloc_checking_on(LIBCWD_TSD);
	  DEBUG_CHANNELS_RELEASE_WRITE_LOCK
	}
#ifdef _REENTRANT
	else
	  DEBUG_CHANNELS_RELEASE_READ_LOCK
#endif
      }

#ifdef _REENTRANT
      // _private_::
      void debug_channels_ct::init_and_rdlock(void)
      {
	_private_::rwlock_tct<_private_::debug_channels_instance>::initialize();
	DEBUG_CHANNELS_ACQUIRE_READ_LOCK
	if (!WNS_debug_channels)			// MT: `WNS_debug_channels' is only false when this object is still Non_Shared.
	{
	  LIBCWD_TSD_DECLARATION
	  set_alloc_checking_off(LIBCWD_TSD);
	  DEBUG_CHANNELS_ACQUIRE_READ2WRITE_LOCK
	  WNS_debug_channels = new debug_channels_ct::container_type;
	  DEBUG_CHANNELS_ACQUIRE_WRITE2READ_LOCK
	  set_alloc_checking_on(LIBCWD_TSD);
	}
      }
#endif

      // _private_::
      void debug_objects_ct::init(LIBCWD_TSD_PARAM)
      {
#ifdef _REENTRANT
	_private_::rwlock_tct<_private_::debug_objects_instance>::initialize();
#endif
        DEBUG_OBJECTS_ACQUIRE_READ_LOCK
	if (!WNS_debug_objects)				// MT: `WNS_debug_objects' is only false when this object is still Non_Shared.
	{
	  DEBUGDEBUG_CERR( "_debug_objects == NULL; initializing it" );
#if CWDEBUG_ALLOC
	  // It is possible that malloc is not initialized yet.
	  init_debugmalloc();
#endif
	  DEBUG_OBJECTS_ACQUIRE_READ2WRITE_LOCK
	  set_alloc_checking_off(LIBCWD_TSD);
	  WNS_debug_objects = new debug_objects_ct::container_type;
	  set_alloc_checking_on(LIBCWD_TSD);
	  DEBUG_OBJECTS_RELEASE_WRITE_LOCK
	}
#ifdef _REENTRANT
	else
	  DEBUG_OBJECTS_RELEASE_READ_LOCK
#endif
      }

#ifdef _REENTRANT
      // _private_::
      void debug_objects_ct::init_and_rdlock(void)
      {
	_private_::rwlock_tct<_private_::debug_objects_instance>::initialize();
	DEBUG_OBJECTS_ACQUIRE_READ_LOCK
	if (!WNS_debug_objects)				// MT: `WNS_debug_objects' is only false when this object is still Non_Shared.
	{
	  DEBUGDEBUG_CERR( "_debug_objects == NULL; initializing it" );
#if CWDEBUG_ALLOC
	  // It is possible that malloc is not initialized yet.
	  init_debugmalloc();
#endif
	  LIBCWD_TSD_DECLARATION
	  set_alloc_checking_off(LIBCWD_TSD);
	  DEBUG_OBJECTS_ACQUIRE_READ2WRITE_LOCK
	  WNS_debug_objects = new debug_objects_ct::container_type;
	  DEBUG_OBJECTS_ACQUIRE_WRITE2READ_LOCK
	  set_alloc_checking_on(LIBCWD_TSD);
	}
      }
#endif

      // _private_::
      void debug_objects_ct::ST_uninit(void)
      {
	if (WNS_debug_objects)
	{
	  LIBCWD_TSD_DECLARATION
	  set_alloc_checking_off(LIBCWD_TSD);
	  delete WNS_debug_objects;
	  set_alloc_checking_on(LIBCWD_TSD);
	  WNS_debug_objects = NULL;
	}
      }

    } // namespace _private_

#if CWDEBUG_DEBUG
    static long WST_debug_object_init_magic = 0;

    static void init_debug_object_init_magic(void)
    {
      struct timeval rn;
      gettimeofday(&rn, NULL);
      WST_debug_object_init_magic = rn.tv_usec;
      if (!WST_debug_object_init_magic)
        WST_debug_object_init_magic = 1;
      DEBUGDEBUG_CERR( "Set WST_debug_object_init_magic to " << WST_debug_object_init_magic );
    }
#endif

    class laf_ct {
    public:
      buffer_ct oss;
	// The temporary output buffer.

      control_flag_t mask;
	// The previous control bits.

      char const* label;
	// The previous label.

      int err;
	// The current errno.

    public:
      laf_ct(control_flag_t m, char const* l, int e) : mask(m), label(l), err(e) { }
    };

    static inline void write_whitespace_to(std::ostream& os, unsigned int size)
    {
      for (unsigned int i = size; i > 0; --i)
        os.put(' ');
    }

    namespace _private_ {
      static char WST_dummy_laf[sizeof(laf_ct)] __attribute__((__aligned__));
    }

    /**
     * \fn void core_dump(void)
     * \ingroup group_special
     *
     * \brief Dump core of current thread.
     */
    void core_dump(void)
    {
#if defined(_REENTRANT) && defined(HAVE_PTHREAD_KILL_OTHER_THREADS_NP)
      pthread_kill_other_threads_np();
#endif
      raise(6);
      exit(6);		// Never reached.
    }

    size_t debug_string_ct::calculate_capacity(size_t size)
    {
      size_t capacity_plus_one = M_default_capacity + 1;			// For the terminating zero.
      while(size >= capacity_plus_one)
	capacity_plus_one *= 2;
      return capacity_plus_one - 1;
    }

    void debug_string_ct::NS_internal_init(char const* str, size_t len)
    {
      M_default_capacity = min_capacity_c;
      M_str = (char*)malloc((M_default_capacity = M_capacity = calculate_capacity(len)) + 1);	// Add one for the terminating zero.
      strncpy(M_str, str, len);
      M_size = len;
      M_str[M_size] = 0;
    }

    void debug_string_ct::ST_internal_deinit(void)
    {
      free(M_str);
    }

    void debug_string_ct::internal_assign(char const* str, size_t len)
    {
      if (len > M_capacity || (M_capacity > M_default_capacity && len < M_default_capacity))
        M_str = (char*)realloc(M_str, (M_capacity = calculate_capacity(len)) + 1);
      strncpy(M_str, str, len);
      M_size = len;
      M_str[M_size] = 0;
    }

    void debug_string_ct::internal_append(char const* str, size_t len)
    {
      if (M_size + len > M_capacity || (M_capacity > M_default_capacity && M_size + len < M_default_capacity))
        M_str = (char*)realloc(M_str, (M_capacity = calculate_capacity(M_size + len)) + 1);
      strncpy(M_str + M_size, str, len);
      M_size += len;
      M_str[M_size] = 0;
    }

    void debug_string_ct::internal_prepend(char const* str, size_t len)
    {
      if (M_size + len > M_capacity || (M_capacity > M_default_capacity && M_size + len < M_default_capacity))
        M_str = (char*)realloc(M_str, (M_capacity = calculate_capacity(M_size + len)) + 1);
      memmove(M_str + len, M_str, M_size + 1);
      strncpy(M_str, str, len);
      M_size += len;
    }

    /**
     * \brief Reserve memory for the string in advance.
     */
    void debug_string_ct::reserve(size_t size)
    {
      if (size < M_size)
	return;
      LIBCWD_TSD_DECLARATION
      set_alloc_checking_off(LIBCWD_TSD);
      M_default_capacity = min_capacity_c;
      M_str = (char*)realloc(M_str, (M_default_capacity = M_capacity = calculate_capacity(size)) + 1);
      set_alloc_checking_on(LIBCWD_TSD);
    }

    void debug_string_ct::internal_swallow(debug_string_ct const& ds)
    {
      // `this' and `ds' are both initialized.  `internal' is set.
      free(M_str);
      M_str = ds.M_str;
      M_size = ds.M_size;
      M_capacity = ds.M_capacity;
      M_default_capacity = ds.M_default_capacity;
    }

    /** \addtogroup group_formatting */
    /* \{ */

    /**
     * \brief Push the current margin on a stack.
     */
    void debug_ct::push_margin(void)
    {
      LIBCWD_TSD_DECLARATION
      debug_string_stack_element_ct* current_margin_stack = LIBCWD_TSD_MEMBER(M_margin_stack);
      set_alloc_checking_off(LIBCWD_TSD);
      void* new_debug_string = malloc(sizeof(debug_string_stack_element_ct));
      LIBCWD_TSD_MEMBER(M_margin_stack) = new (new_debug_string) debug_string_stack_element_ct(LIBCWD_TSD_MEMBER(margin));
      set_alloc_checking_on(LIBCWD_TSD);
      LIBCWD_TSD_MEMBER(M_margin_stack)->next = current_margin_stack;
    }

    /**
     * \brief Pop margin from the stack.
     */
    void debug_ct::pop_margin(void)
    {
      LIBCWD_TSD_DECLARATION
      if (!LIBCWD_TSD_MEMBER(M_margin_stack))
	DoutFatal(dc::core, "Calling `debug_ct::pop_margin' more often than `debug_ct::push_margin'.");
      debug_string_stack_element_ct* next = LIBCWD_TSD_MEMBER(M_margin_stack)->next;
      set_alloc_checking_off(LIBCWD_TSD);
      LIBCWD_TSD_MEMBER(margin).internal_swallow(LIBCWD_TSD_MEMBER(M_margin_stack)->debug_string);
      free(LIBCWD_TSD_MEMBER(M_margin_stack));
      set_alloc_checking_on(LIBCWD_TSD);
      LIBCWD_TSD_MEMBER(M_margin_stack) = next;
    }

    /**
     * \brief Push the current marker on a stack.
     */
    void debug_ct::push_marker(void)
    {
      LIBCWD_TSD_DECLARATION
      debug_string_stack_element_ct* current_marker_stack = LIBCWD_TSD_MEMBER(M_marker_stack);
      set_alloc_checking_off(LIBCWD_TSD);
      void* new_debug_string = malloc(sizeof(debug_string_stack_element_ct));
      LIBCWD_TSD_MEMBER(M_marker_stack) = new (new_debug_string) debug_string_stack_element_ct(LIBCWD_TSD_MEMBER(marker));
      LIBCWD_TSD_MEMBER(M_marker_stack)->next = current_marker_stack;
    }

    /**
     * \brief Pop marker from the stack.
     */
    void debug_ct::pop_marker(void)
    {
      LIBCWD_TSD_DECLARATION
      if (!LIBCWD_TSD_MEMBER(M_marker_stack))
	DoutFatal(dc::core, "Calling `debug_ct::pop_marker' more often than `debug_ct::push_marker'.");
      debug_string_stack_element_ct* next = LIBCWD_TSD_MEMBER(M_marker_stack)->next;
      set_alloc_checking_off(LIBCWD_TSD);
      LIBCWD_TSD_MEMBER(marker).internal_swallow(LIBCWD_TSD_MEMBER(M_marker_stack)->debug_string);
      free(LIBCWD_TSD_MEMBER(M_marker_stack));
      set_alloc_checking_on(LIBCWD_TSD);
      LIBCWD_TSD_MEMBER(M_marker_stack) = next;
    }

    /** \} */

#ifdef _REENTRANT
#define LIBCWD_COMMA_MUTEX ,debug_object.M_mutex
#else
#define LIBCWD_COMMA_MUTEX
#endif

    void debug_tsd_st::start(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM)
    {
#if CWDEBUG_DEBUG
#ifdef _REENTRANT
      // Initialisation of the TSD part should be done from LIBCWD_TSD_DECLARATION inside Dout et al.
      LIBCWD_ASSERT( tsd_initialized );
#else
      if (!tsd_initialized)
	init();
#endif
#endif

      set_alloc_checking_off(LIBCWD_TSD);

      // Skip `start()' for a `continued' debug output.
      // Generating the "prefix: <continued>" is already taken care of while generating the "<unfinished>" (see next `if' block).
      if ((channel_set.mask & (continued_maskbit|finish_maskbit)))
      {
	if (!(current->mask & continued_expected_maskbit))
	{
	  std::ostream* target_os = (channel_set.mask & cerr_cf) ? &std::cerr : debug_object.real_os;
#ifdef _REENTRANT
	  // Try to get the lock, but don't try too long...
	  int res;
	  struct timespec const t = { 0, 5000000 };
	  int count = 0;
	  do
	  {
	    if (!(res = debug_object.M_mutex->trylock()))
	      break;
	    nanosleep(&t, NULL);
	  }
	  while(++count < 40);
#endif
	  target_os->put('\n');
#ifdef _REENTRANT
	  if (res == 0)
	    debug_object.M_mutex->unlock();
#endif
	  char const* channame = (channel_set.mask & finish_maskbit) ? "finish" : "continued";
#if CWDEBUG_LOCATION
	  DoutFatal(dc::core, "Using `dc::" << channame << "' in " <<
	      debug::location_ct((char*)__builtin_return_address(0) + builtin_return_address_offset) <<
	      " without (first using) a matching `continued_cf'.");
#else
	  DoutFatal(dc::core, "Using `dc::" << channame <<
	      "' without (first using) a matching `continued_cf'.");
#endif
	}
#if CWDEBUG_DEBUG
	// MT: current != _private_::WST_dummy_laf, otherwise we didn't pass the previous if.
	LIBCWD_ASSERT( current != reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf) );
#endif
        current->mask = channel_set.mask;		// New bits might have been added
	if ((current->mask & finish_maskbit))
	  current->mask &= ~continued_expected_maskbit;
	current->err = errno;			// Always keep the last errno as set at the start of LibcwDout()
        return;
      }

      ++LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
      DEBUGDEBUG_CERR( "Entering debug_ct::start(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object) );

      // Is this an interrupting debug output (in the middle of a continued debug output)?
      if ((current->mask & continued_cf_maskbit) && unfinished_expected)
      {
#if CWDEBUG_DEBUG
	// MT: if current == _private_::WST_dummy_laf then
	//     (current->mask & continued_cf_maskbit) is false and this if is skipped.
	LIBCWD_ASSERT( current != reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf) );
#endif
	// Append <unfinished> to the current buffer.
	current_oss->write("<unfinished>\n", 13);		// Continued debug output should end on a space by itself,
	// And write out what is in the buffer till now.
	std::ostream* target_os = (channel_set.mask & cerr_cf) ? &std::cerr : debug_object.real_os;
	static_cast<buffer_ct*>(current_oss)->writeto(target_os LIBCWD_COMMA_MUTEX LIBCWD_COMMA_TSD, debug_object);
	// Truncate the buffer to its prefix and append "<continued>" to it already.
	static_cast<buffer_ct*>(current_oss)->restore_position();
	current_oss->write("<continued> ", 12);		// therefore we repeat the space here.
      }

      // Is this a nested debug output (the first of a series in the middle of another debug output)?
      // MT: if current == _private_::WST_dummy_laf then
      //     start_expected is true and this if is skipped.
      if (!start_expected)
      {
	// Put current stringstream on the stack.
	laf_stack.push(current);

	// Indent nested debug output with 4 extra spaces.
	indent += 4;

	// If the previous target was written to cerr, then
	// write this interrupting output to cerr too.
	channel_set.mask |= (current->mask & cerr_cf);
      }

      // Create a new laf.
      DEBUGDEBUG_CERR( "creating new laf_ct" );
      current = new laf_ct(channel_set.mask, channel_set.label, errno);
      DEBUGDEBUG_CERR( "current = " << (void*)current );
      current_oss = &current->oss;
      DEBUGDEBUG_CERR( "current_oss = " << (void*)current_oss );
      DEBUGDEBUG_CERR( "laf_ct created" );

      // Without a new nested Dout() call, we expect to see a finish() call: The finish belonging to *this* Dout() call.
      start_expected = false;

      // If this is a `continued' debug output, then we want to print "<unfinished>" if next we see a start().
      unfinished_expected = true;

      // Print prefix if requested.
      // Handle most common case first: no special flags set
      if (!(channel_set.mask & (noprefix_cf|nolabel_cf|blank_margin_cf|blank_label_cf|blank_marker_cf)))
      {
	current_oss->write(margin.c_str(), margin.size());
	current_oss->write(channel_set.label, WST_max_len);
	current_oss->write(marker.c_str(), marker.size());
	write_whitespace_to(*current_oss, indent);
      }
      else if (!(channel_set.mask & noprefix_cf))
      {
	if ((channel_set.mask & blank_margin_cf))
	  write_whitespace_to(*current_oss, margin.size());
	else
	  current_oss->write(margin.c_str(), margin.size());
#if !CWDEBUG_DEBUGOUTPUT
	if (!(channel_set.mask & nolabel_cf))
#endif
	{
	  if ((channel_set.mask & blank_label_cf))
	    write_whitespace_to(*current_oss, WST_max_len);
	  else
	    current_oss->write(channel_set.label, WST_max_len);
	  if ((channel_set.mask & blank_marker_cf))
	    write_whitespace_to(*current_oss, marker.size());
	  else
	    current_oss->write(marker.c_str(), marker.size());
	  write_whitespace_to(*current_oss, indent);
	}
      }

      if ((channel_set.mask & continued_cf_maskbit))
      {
	// If this is continued debug output, then it makes sense to remember the prefix length,
	// just in case we need indeed to output <continued> data.
        static_cast<buffer_ct*>(current_oss)->store_position();
      }

      --LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
      DEBUGDEBUG_CERR( "Leaving debug_ct::start(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object) );
    }

    void debug_tsd_st::finish(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM)
    {
#if CWDEBUG_DEBUG
      LIBCWD_ASSERT( current != reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf) );
#endif
      std::ostream* target_os = (current->mask & cerr_cf) ? &std::cerr : debug_object.real_os;

      // Skip `finish()' for a `continued' debug output.
      if ((current->mask & continued_cf_maskbit) && !(current->mask & finish_maskbit))
      {
	// Allow a subsequent Dout(dc::continued (or dc::finish), ...).
	current->mask |= continued_expected_maskbit;
        if ((current->mask & continued_maskbit))
	  unfinished_expected = true;
        // If the `flush_cf' control flag is set, flush the ostream at every `finish()' though.
	if ((current->mask & flush_cf))
	{
	  // Write buffer to ostream.
	  static_cast<buffer_ct*>(current_oss)->writeto(target_os LIBCWD_COMMA_MUTEX LIBCWD_COMMA_TSD, debug_object);
	  // Flush ostream.  Note that in the case of nested debug output this `os' can be an stringstream,
	  // in that case, no actual flushing is done until the debug output to the real ostream has
	  // finished.
	  *target_os << std::flush;
	}
	set_alloc_checking_on(LIBCWD_TSD);
        return;
      }

      ++LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
      DEBUGDEBUG_CERR( "Entering debug_ct::finish(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object) );

      // Handle control flags, if any:
      if ((current->mask & error_cf))
	*current_oss << ": " << strerrno(current->err) << " (" << strerror(current->err) << ')';
      if (!(current->mask & nonewline_cf))
	current_oss->put('\n');

      // Write buffer to ostream.
      static_cast<buffer_ct*>(current_oss)->writeto(target_os LIBCWD_COMMA_MUTEX LIBCWD_COMMA_TSD, debug_object);

      // Handle control flags, if any:
      if (current->mask != 0)
      {
	if ((current->mask & (coredump_maskbit|fatal_maskbit)))
	{
	  set_alloc_checking_on(LIBCWD_TSD);
	  if (!__libcwd_tsd.recursive_fatal)
	  {
	    __libcwd_tsd.recursive_fatal = true;
	    LIBCWD_DISABLE_CANCEL
	    *target_os << std::flush;	// First time, try to flush.
	    LIBCWD_ENABLE_CANCEL
	  }
	  if ((current->mask & coredump_maskbit))
	    core_dump();
	  DEBUGDEBUG_CERR( "Deleting `current' " << (void*)current );
	  delete current;
	  DEBUGDEBUG_CERR( "Done deleting `current'" );
	  set_alloc_checking_on(LIBCWD_TSD);
	  exit(254);
	}
	if ((current->mask & wait_cf))
	{
#ifdef _REENTRANT
	  debug_object.M_mutex->lock();
#endif
	  *target_os << "(type return)";
	  if (debug_object.interactive)
	  {
	    *target_os << std::flush;
	    while(std::cin.get() != '\n');
	  }
#ifdef _REENTRANT
	  debug_object.M_mutex->unlock();
#endif
	}
	if ((current->mask & flush_cf))
	{
	  set_alloc_checking_on(LIBCWD_TSD);
	  LIBCWD_DISABLE_CANCEL
	  *target_os << std::flush;
	  LIBCWD_ENABLE_CANCEL
	  set_alloc_checking_off(LIBCWD_TSD);
	}
      }

      DEBUGDEBUG_CERR( "Deleting `current' " << (void*)current );
      delete current;
      DEBUGDEBUG_CERR( "Done deleting `current'" );

      if (start_expected)
      {
        // Ok, we're done with the last buffer.
	indent -= 4;
        laf_stack.pop();
      }

      // Restore previous buffer as being the current one, if any.
      if (laf_stack.size())
      {
	control_flag_t mask = current->mask;
        current = laf_stack.top();
	DEBUGDEBUG_CERR( "current = " << (void*)current );
	current_oss = &current->oss;
	DEBUGDEBUG_CERR( "current_oss = " << (void*)current_oss );
	if ((mask & flush_cf))
	  current->mask |= flush_cf;	// Propagate flush to real ostream.
      }
      else
      {
        current = reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf);	// Used (MT: read-only!) in next debug_ct::start().
	DEBUGDEBUG_CERR( "current = " << (void*)current );
#if CWDEBUG_DEBUG
	current_oss = NULL;
	DEBUGDEBUG_CERR( "current_oss = " << (void*)current_oss );
#endif
      }

      start_expected = true;
      unfinished_expected = false;

      --LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
      DEBUGDEBUG_CERR( "Leaving debug_ct::finish(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object) );

      set_alloc_checking_on(LIBCWD_TSD);
    }

    void debug_tsd_st::fatal_finish(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM)
    {
      finish(debug_object, channel_set LIBCWD_COMMA_TSD);
      DoutFatal( dc::core, "Don't use `DoutFatal' together with `continued_cf', use `Dout' instead.  (This message can also occur when using DoutFatal correctly but from the constructor of a global object)." );
    }

#ifdef _REENTRANT
    int debug_ct::S_index_count = 0;
#endif

    void debug_ct::NS_init(void)
    {
      ST_initialize_globals();		// Because all allocations for global objects are internal these days, we use
      					// the constructor of debug_ct to initiate the global initialization of libcwd
					// instead of relying on malloc().

      if (WNS_initialized)
        return;

#ifdef _REENTRANT
      M_mutex = NULL;
#endif

#if CWDEBUG_DEBUG
      if (!WST_debug_object_init_magic)
	init_debug_object_init_magic();
      init_magic = WST_debug_object_init_magic;
      DEBUGDEBUG_CERR( "Set init_magic to " << init_magic );
      DEBUGDEBUG_CERR( "Setting WNS_initialized to true" );
#endif

      LIBCWD_TSD_DECLARATION
      LIBCWD_DEFER_CANCEL
      _private_::debug_objects.init(LIBCWD_TSD);
      set_alloc_checking_off(LIBCWD_TSD);		// debug_objects is internal.
      DEBUG_OBJECTS_ACQUIRE_WRITE_LOCK
      if (find(_private_::debug_objects.write_locked().begin(),
	       _private_::debug_objects.write_locked().end(), this)
	  == _private_::debug_objects.write_locked().end()) // Not added before?
	_private_::debug_objects.write_locked().push_back(this);
      DEBUG_OBJECTS_RELEASE_WRITE_LOCK
#ifdef _REENTRANT
      set_alloc_checking_on(LIBCWD_TSD);
      LIBCWD_RESTORE_CANCEL
      set_alloc_checking_off(LIBCWD_TSD);		// debug_objects is internal.
#endif
      new (_private_::WST_dummy_laf) laf_ct(0, channels::dc::debug.get_label(), 0);	// Leaks 24 bytes of memory
#ifdef _REENTRANT
      WNS_index = S_index_count++;
#if CWDEBUG_DEBUGT
      LIBCWD_ASSERT( pthread_self() == PTHREAD_THREADS_MAX );	// Only the initial thread should be initializing debug_ct objects.
#endif
      LIBCWD_ASSERT( __libcwd_tsd.do_array[WNS_index] == NULL );
      debug_tsd_st& tsd(*(__libcwd_tsd.do_array[WNS_index] =  new debug_tsd_st));
#endif
      tsd.init();
      set_alloc_checking_on(LIBCWD_TSD);

#if CWDEBUG_DEBUGOUTPUT
      LIBCWD_TSD_MEMBER_OFF = -1;		// Print as much debug output as possible right away.
#else
      LIBCWD_TSD_MEMBER_OFF = 0;		// Don't print debug output till the REAL initialization of the debug system
      						// has been performed (ie, the _application_ start (don't confuse that with
						// the constructor - which does nothing)).
#endif
      DEBUGDEBUG_CERR( "debug_ct::NS_init(void), _off set to " << LIBCWD_TSD_MEMBER_OFF );

      // This sets current_oss and must be called after tsd.init().
      set_ostream(&std::cerr);				// Write to std::cerr by default.
      interactive = true;				// and thus we're interactive.

      WNS_initialized = true;
    }

    void debug_tsd_st::init(void)
    {
#if CWDEBUG_DEBUGM
      LIBCWD_TSD_DECLARATION
      LIBCWD_ASSERT( __libcwd_tsd.internal );
#endif
      start_expected = true;				// Of course, we start with expecting the beginning of a debug output.
      unfinished_expected = false;

      // `current' needs to be non-zero (saving us a check in start()) and
      // current.mask needs to be 0 to avoid a crash in start():
      current = reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf);
      DEBUGDEBUG_CERR( "current = " << (void*)current );
#if CWDEBUG_DEBUG
      current_oss = NULL;
      DEBUGDEBUG_CERR( "current_oss = " << (void*)current_oss );
#endif
      laf_stack.init();
      continued_stack.init();
      margin.NS_internal_init("", 0);
      marker.NS_internal_init(": ", 2);

#if CWDEBUG_DEBUGOUTPUT
      first_time = true;
#endif
      off_count = 0;
      M_margin_stack = NULL;
      M_marker_stack = NULL;
      indent = 0;


      tsd_initialized = true;
    }

#ifdef _REENTRANT
    namespace _private_ {
      void debug_tsd_init(LIBCWD_TSD_PARAM)
      {
	ForAllDebugObjects(
	  set_alloc_checking_off(LIBCWD_TSD);
	  LIBCWD_ASSERT( __libcwd_tsd.do_array[(debugObject).WNS_index] == NULL );
	  debug_tsd_st& tsd(*(__libcwd_tsd.do_array[(debugObject).WNS_index] =  new debug_tsd_st));
	  tsd.init();
	  set_alloc_checking_on(LIBCWD_TSD);
	  LIBCWD_DO_TSD_MEMBER_OFF(debugObject) = 0;
	);
      }
    }
#endif

    debug_tsd_st::~debug_tsd_st()
    {
      if (!tsd_initialized)	// Skip when it wasn't initialized.
	return;
      // Sanity checks:
      if (continued_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_tsd_st with a non-empty continued_stack (missing dc::finish?)" );
      if (laf_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_tsd_st with a non-empty laf_stack" );
      // Don't actually deinitialize anything.
    }

    /**
     * \fn channel_ct* find_channel(char const* label)
     * \ingroup group_special
     *
     * \brief Find %debug channel with label \a label.
     *
     * \return A pointer to the %debug channel object whose name starts with \a label.
     * &nbsp;If there is more than one such %debug %channel, the object with the lexicographically
     * largest name is returned.&nbsp; When no %debug channel could be found, NULL is returned.
     */
    channel_ct* find_channel(char const* label)
    {
      channel_ct* tmp = NULL;
      LIBCWD_TSD_DECLARATION
      LIBCWD_DEFER_CANCEL
      _private_::debug_channels.init(LIBCWD_TSD);
      DEBUG_CHANNELS_ACQUIRE_READ_LOCK
      for(_private_::debug_channels_ct::container_type::const_iterator i(_private_::debug_channels.read_locked().begin());
	  i != _private_::debug_channels.read_locked().end(); ++i)
      {
        if (!strncasecmp(label, (*i)->get_label(), strlen(label)))
          tmp = (*i);
      }
      DEBUG_CHANNELS_RELEASE_READ_LOCK
      LIBCWD_RESTORE_CANCEL
      return tmp;
    }

    /**
     * \fn void list_channels_on(debug_ct& debug_object)
     * \ingroup group_special
     *
     * \brief List all %debug %channels to a given %debug object.
     *
     * <b>Example:</b>
     *
     * \code
     * Dout( list_channels_on(libcw_do) );   // libcw_do is the (default) debug object of libcwd.
     * \endcode
     *
     * Example of output:
     *
     * \exampleoutput <PRE>
     * DEBUG   : Disabled
     * NOTICE  : Enabled
     * WARNING : Enabled
     * SYSTEM  : Enabled
     * MALLOC  : Disabled
     * LLISTS  : Disabled
     * KERNEL  : Disabled
     * IO      : Disabled
     * FOO     : Enabled
     * BAR     : Enabled</PRE>
     * \endexampleoutput
     *
     * Where FOO and BAR are \link preparation user defined channels \endlink in this example.
     */
    void list_channels_on(debug_ct& debug_object)
    {
      LIBCWD_TSD_DECLARATION
      if (LIBCWD_DO_TSD_MEMBER_OFF(debug_object) < 0)
      {
	LIBCWD_DEFER_CANCEL
        _private_::debug_channels.init(LIBCWD_TSD);
	LIBCWD_RESTORE_CANCEL
	LIBCWD_DEFER_CLEANUP_PUSH(&rwlock_tct<debug_channels_instance>::cleanup, NULL)
	DEBUG_CHANNELS_ACQUIRE_READ_LOCK
	for(_private_::debug_channels_ct::container_type::const_iterator i(_private_::debug_channels.read_locked().begin());
	    i != _private_::debug_channels.read_locked().end(); ++i)
	{
	  LibcwDoutScopeBegin(DEBUGCHANNELS, debug_object, dc::always|noprefix_cf);
	  LibcwDoutStream.write(LIBCWD_DO_TSD_MEMBER(debug_object, margin).c_str(), LIBCWD_DO_TSD_MEMBER(debug_object, margin).size());
	  LibcwDoutStream.write((*i)->get_label(), WST_max_len);
	  if ((*i)->is_on())
	    LibcwDoutStream.write(": Enabled", 9);
	  else
	    LibcwDoutStream.write(": Disabled", 10);
	  LibcwDoutScopeEnd;
	}
        DEBUG_CHANNELS_RELEASE_READ_LOCK
	LIBCWD_CLEANUP_POP_RESTORE(false)
      }
    }

    void channel_ct::NS_initialize(char const* label)		// Single Threaded function (or just Non-Shared if you take dlopen into account).
    {
      // This is pretty much identical to fatal_channel_ct::fatal_channel_ct().

      if (WNS_initialized)
        return;		// Already initialized.

      DEBUGDEBUG_CERR( "Entering `channel_ct::NS_initialize(\"" << label << "\")'" );

      size_t label_len = strlen(label);

      if (label_len > max_label_len_c)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << label << "\") > " << max_label_len_c );

      LIBCWD_TSD_DECLARATION

#ifdef _REENTRANT
      _private_::mutex_tct<_private_::write_max_len_instance>::initialize();
      LIBCWD_DEFER_CANCEL
      _private_::mutex_tct<_private_::write_max_len_instance>::lock();
      // MT: This critical area does not contain cancellation points.
      // When this thread is cancelled later, there is no need to take action:
      // the debug channel is global and can be used by any thread.  We want
      // to keep the maximum label length as set by this channel.  Even when
      // this debug channel is part of shared library that is being loaded
      // by dlopen() then it won't get removed when this thread is cancelled.
      // (Actually, a downwards update of WST_max_len should be done by
      // dlclose if at all).
#endif
      // MT: This is not strict thread safe because it is possible that after threads are already
      //     running, a new shared library is dlopen-ed with a new global channel_ct with a larger
      //     label.  However, it makes no sense to lock the *reading* of max_len for the other threads
      //     because this assignment is an atomic operation.  The lock above is only needed to
      //     prefend two such simultaneously loaded libraries from causing WST_max_len to end up
      //     not maximal.
      if (label_len > WST_max_len)
	WST_max_len = label_len;

#ifdef _REENTRANT
      // MT: Take advantage of the `write_max_len_instance' lock to prefend simultaneous access
      //     to `next_index' in the case of simultaneously dlopen-loaded libraries.
      static int next_index;
      WNS_index = ++next_index;		// Don't use index 0, it is used to make sure that uninitialized channels appear to be off.
       
      _private_::mutex_tct<_private_::write_max_len_instance>::unlock();
      LIBCWD_RESTORE_CANCEL

      __libcwd_tsd.off_cnt_array[WNS_index] = 0;
#else
      off_cnt = 0;
#endif

      strncpy(WNS_label, label, label_len);
      memset(WNS_label + label_len, ' ', max_label_len_c - label_len);

      // We store debug channels in some organized order, so that the
      // order in which they appear in the ForAllDebugChannels is not
      // dependent on the order in which these global objects are
      // initialized.
      LIBCWD_DEFER_CANCEL
      _private_::debug_channels.init(LIBCWD_TSD);
      DEBUG_CHANNELS_ACQUIRE_WRITE_LOCK
      {
	set_alloc_checking_off(LIBCWD_TSD);	// debug_channels is internal.
	_private_::debug_channels_ct::container_type& channels(_private_::debug_channels.write_locked());
	_private_::debug_channels_ct::container_type::iterator i(channels.begin());
	for(; i != channels.end(); ++i)
	  if (strncmp((*i)->get_label(), WNS_label, max_label_len_c) > 0)
	    break;
        channels.insert(i, this);
	set_alloc_checking_on(LIBCWD_TSD);
      }
      DEBUG_CHANNELS_RELEASE_WRITE_LOCK
      LIBCWD_RESTORE_CANCEL

      // Turn debug channel "WARNING" on by default.
      if (strncmp(WNS_label, "WARNING", label_len) == 0)
#ifdef _REENTRANT
	__libcwd_tsd.off_cnt_array[WNS_index] = -1;
#else
        off_cnt = -1;
#endif

      DEBUGDEBUG_CERR( "Leaving `channel_ct::NS_initialize(\"" << label << "\")" );

      WNS_initialized = true;
    }

    void fatal_channel_ct::NS_initialize(char const* label, control_flag_t maskbit)
    {
       // This is pretty much identical to channel_ct::NS_initialize().

      if (WNS_maskbit)
        return;		// Already initialized.

      WNS_maskbit = maskbit;

      DEBUGDEBUG_CERR( "Entering `fatal_channel_ct::NS_initialize(\"" << label << "\")'" );

      size_t label_len = strlen(label);

      if (label_len > max_label_len_c)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << label << "\") > " << max_label_len_c );

#ifdef _REENTRANT
      _private_::mutex_tct<_private_::write_max_len_instance>::initialize();
      LIBCWD_DEFER_CANCEL
      _private_::mutex_tct<_private_::write_max_len_instance>::lock();
#endif
      // MT: See comments in channel_ct::NS_initialize above.
      if (label_len > WST_max_len)
	WST_max_len = label_len;
#ifdef _REENTRANT
      _private_::mutex_tct<_private_::write_max_len_instance>::unlock();
      LIBCWD_RESTORE_CANCEL
#endif

      strncpy(WNS_label, label, label_len);
      memset(WNS_label + label_len, ' ', max_label_len_c - label_len);

      DEBUGDEBUG_CERR( "Leaving `fatal_channel_ct::NS_initialize(\"" << label << "\")" );
    }

    void continued_channel_ct::NS_initialize(control_flag_t maskbit)
    {
      if (!WNS_maskbit)
	WNS_maskbit = maskbit;
    }

    char const always_channel_ct::label[max_label_len_c] = { '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' };

    /**
     * \brief Turn this channel off.
     *
     * \sa on()
     */
    void channel_ct::off(void)
    {
#ifdef _REENTRANT
      LIBCWD_TSD_DECLARATION
      __libcwd_tsd.off_cnt_array[WNS_index] += 1;
#else
      ++off_cnt;
#endif
    }

    /**
     * \brief Cancel one call to `off()'.
     *
     * The channel is turned on when on() is called as often as off() was called before.
     */
    void channel_ct::on(void)
    {
#ifdef _REENTRANT
      LIBCWD_TSD_DECLARATION
      if (__libcwd_tsd.off_cnt_array[WNS_index] == -1)
#else
      if (off_cnt == -1)
#endif
	DoutFatal( dc::core, "Calling channel_ct::on() more often then channel_ct::off()" );
#ifdef _REENTRANT
      __libcwd_tsd.off_cnt_array[WNS_index] -= 1;
#else
      --off_cnt;
#endif
    }

    //----------------------------------------------------------------------------------------
    // Handle continued channel flags and update `off_count' and `continued_stack'.
    //

    continued_channel_set_st& channel_set_st::operator|(continued_cf_nt)
    {
#if CWDEBUG_DEBUG
      DEBUGDEBUG_CERR( "continued_cf detected" );
      if (!do_tsd_ptr || !do_tsd_ptr->tsd_initialized)
      {
        FATALDEBUGDEBUG_CERR( "Don't use DoutFatal together with continued_cf, use Dout instead." );
	core_dump();
      }
#endif
      mask |= continued_cf_maskbit;
      if (!on)
      {
	++(do_tsd_ptr->off_count);
	DEBUGDEBUG_CERR( "Channel is switched off. Increased off_count to " << do_tsd_ptr->off_count );
      }
      else
      {
        do_tsd_ptr->continued_stack.push(do_tsd_ptr->off_count);
        DEBUGDEBUG_CERR( "Channel is switched on. Pushed off_count (" << do_tsd_ptr->off_count << ") to stack (size now " <<
	    do_tsd_ptr->continued_stack.size() << ") and set off_count to 0" );
        do_tsd_ptr->off_count = 0;
      }
      return *(reinterpret_cast<continued_channel_set_st*>(this));
    }

    continued_channel_set_st& channel_set_bootstrap_st::operator|(continued_channel_ct const& cdc)
    {
#if CWDEBUG_DEBUG
      if ((cdc.get_maskbit() & continued_maskbit))
	DEBUGDEBUG_CERR( "dc::continued detected" );
      else
        DEBUGDEBUG_CERR( "dc::finish detected" );
#endif

      if ((on = !do_tsd_ptr->off_count))
      {
        DEBUGDEBUG_CERR( "Channel is switched on (off_count is 0)" );
	do_tsd_ptr->current->mask |= cdc.get_maskbit();					// We continue with the current channel
	mask = do_tsd_ptr->current->mask;
	label = do_tsd_ptr->current->label;
	if (cdc.get_maskbit() == finish_maskbit)
	{
	  do_tsd_ptr->off_count = do_tsd_ptr->continued_stack.top();
	  do_tsd_ptr->continued_stack.pop();
	  DEBUGDEBUG_CERR( "Restoring off_count to " << do_tsd_ptr->off_count << ". Stack size now " << do_tsd_ptr->continued_stack.size() );
	}
      }
      else
      {
        DEBUGDEBUG_CERR( "Channel is switched off (off_count is " << do_tsd_ptr->off_count << ')' );
	if (cdc.get_maskbit() == finish_maskbit)
	{
	  DEBUGDEBUG_CERR( "` decrementing off_count with 1" );
	  --(do_tsd_ptr->off_count);
        }
      }
      return *reinterpret_cast<continued_channel_set_st*>(this);
    }

    channel_set_st& channel_set_bootstrap_st::operator|(fatal_channel_ct const&)
    {
#if CWDEBUG_LOCATION
      DoutFatal(dc::fatal, location_ct((char*)__builtin_return_address(0) + libcw::debug::builtin_return_address_offset) <<
          " : Don't use Dout together with dc::core or dc::fatal!  Use DoutFatal instead.");
#else
      DoutFatal(dc::core,
          "Don't use Dout together with dc::core or dc::fatal!  Use DoutFatal instead.");
#endif
    }

    channel_set_st& channel_set_bootstrap_st::operator&(channel_ct const&)
    {
#if CWDEBUG_LOCATION
      DoutFatal(dc::fatal, location_ct((char*)__builtin_return_address(0) + libcw::debug::builtin_return_address_offset) <<
	  " : Use dc::core or dc::fatal together with DoutFatal.");
#else
      DoutFatal(dc::core,
	  "Use dc::core or dc::fatal together with DoutFatal.");
#endif
    }

    namespace _private_ {

      void assert_fail(char const* expr, char const* file, int line, char const* function)
      {
#if CWDEBUG_DEBUG
	LIBCWD_TSD_DECLARATION
	if (__libcwd_tsd.recursive_assert
#if CWDEBUG_DEBUGM
	    || __libcwd_tsd.inside_malloc_or_free
#endif
	    ) 
	{
	  set_alloc_checking_off(LIBCWD_TSD);
	  FATALDEBUGDEBUG_CERR(file << ':' << line << ": " << function << ": Assertion `" << expr << "' failed.\n");
	  set_alloc_checking_on(LIBCWD_TSD);
	  core_dump();
	}
	__libcwd_tsd.recursive_assert = true;
#if CWDEBUG_DEBUGT
	__libcwd_tsd.internal_debugging_code = true;
#endif
#endif
	DoutFatal(dc::core, file << ':' << line << ": " << function << ": Assertion `" << expr << "' failed.\n");
      }

    } // namespace _private_

    void debug_ct::force_on(debug_ct::OnOffState& state)
    {
      NS_init();
      LIBCWD_TSD_DECLARATION
      state._off = LIBCWD_TSD_MEMBER_OFF;
#if CWDEBUG_DEBUGOUTPUT
      state.first_time = LIBCWD_TSD_MEMBER(first_time);
#endif
      LIBCWD_TSD_MEMBER_OFF = -1;					// Turn object on.
    }

    void debug_ct::restore(debug_ct::OnOffState const& state)
    {
      LIBCWD_TSD_DECLARATION
#if CWDEBUG_DEBUGOUTPUT
      if (state.first_time != LIBCWD_TSD_MEMBER(first_time))		// state.first_time && !first_time.
	core_dump();							// on() was called without first a call to off().
#endif
      if (LIBCWD_TSD_MEMBER_OFF != -1)
	core_dump();							// off() and on() where called and not in equal pairs.
      LIBCWD_TSD_MEMBER_OFF = state._off;				// Restore.
    }

#ifdef _REENTRANT
    bool debug_ct::keep_tsd(bool keep)
    {
      LIBCWD_TSD_DECLARATION
      bool old = LIBCWD_TSD_MEMBER(tsd_keep);
      LIBCWD_TSD_MEMBER(tsd_keep) = keep;
      return old;
    }
#endif

    void channel_ct::force_on(channel_ct::OnOffState& state, char const* label)
    {
      NS_initialize(label);
#ifdef _REENTRANT
      LIBCWD_TSD_DECLARATION
      int& off_cnt(__libcwd_tsd.off_cnt_array[WNS_index]);
#endif
      state.off_cnt = off_cnt;
      off_cnt = -1;					// Turn channel on.
    }

    void channel_ct::restore(channel_ct::OnOffState const& state)
    {
#ifdef _REENTRANT
      LIBCWD_TSD_DECLARATION
      int& off_cnt(__libcwd_tsd.off_cnt_array[WNS_index]);
#endif
      if (off_cnt != -1)
	core_dump();					// off() and on() where called and not in equal pairs.
      off_cnt = state.off_cnt;				// Restore.
    }

#ifdef _REENTRANT
/**
 * \brief Set output device and provide external pthread mutex.
 * \ingroup group_destination
 *
 * Assign a new \c ostream to this %debug object.
 * The \c ostream will only be written to after acquiring the
 * pthread mutex passed in the second argument.
 *
 * <b>Example:</b>
 *
 * \code
 * pthread_mutex_t lock;
 *
 * // Use the same lock as you use in your application for std::cerr.
 * Debug( libcw_do.set_ostream(&std::cerr, &lock) );
 *
 * pthread_mutex_lock(&lock);
 * std::cerr << "The application uses cerr too\n";
 * pthread_mutex_unlock(&lock);
 * \endcode
 */
// Specialization
template<>
  void debug_ct::set_ostream(std::ostream* os, pthread_mutex_t* mutex)
  {
    _private_::lock_interface_base_ct* new_mutex = new _private_::pthread_lock_interface_ct(mutex);
    LIBCWD_DEFER_CANCEL
    _private_::mutex_tct<_private_::set_ostream_instance>::lock();
    _private_::lock_interface_base_ct* old_mutex = M_mutex;
    if (old_mutex)
      old_mutex->lock();		// Make sure all other threads left this critical area.
    M_mutex = new_mutex;
    if (old_mutex)
    {
      old_mutex->unlock();
      delete old_mutex;
    }
    private_set_ostream(os);
    _private_::mutex_tct<_private_::set_ostream_instance>::unlock();
    LIBCWD_RESTORE_CANCEL
  }
#endif // _REENTRANT

/**
 * \brief Set output device (single threaded applications).
 * \ingroup group_destination
 *
 * Assign a new \c ostream to this %debug object (default is <CODE>std::cerr</CODE>).
 * For use in single threaded applications only.
 */
void debug_ct::set_ostream(std::ostream* os)
{
#ifdef _REENTRANT
  if (_private_::WST_multi_threaded)
    Dout(dc::warning, location_ct((char*)__builtin_return_address(0) + builtin_return_address_offset) << ": You should passing a locking mechanism to `set_ostream' for the ostream (see documentation/reference-manual/group__group__destination.html)");
  LIBCWD_DEFER_CANCEL
  _private_::mutex_tct<_private_::set_ostream_instance>::lock();
#endif
  private_set_ostream(os);
#ifdef _REENTRANT
  _private_::mutex_tct<_private_::set_ostream_instance>::unlock();
  LIBCWD_RESTORE_CANCEL
#endif
}

  }	// namespace debug
}	// namespace libcw

// This can be used in configure to see if libcwd exists.
extern "C" char const* const __libcwd_version = VERSION;
