// $Header$
//
// Copyright (C) 2000 - 2003, by
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
#include <libcwd/config.h>

#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC
// This has to be very early (must not have been included elsewhere already).
#define private public  // Ugly, I know.
#if (__GNUC__ == 3 && __GNUC_MINOR__ < 4)
#include <bits/stl_alloc.h>
#else
extern "C" char* getenv(char const* name);	// Needed before including ext/pool_allocator.h.
#include <ext/pool_allocator.h>
#endif
#undef private
#endif // LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC

#include <cerrno>
#include <iostream>
#include <algorithm>
#include <sys/time.h>     	// Needed for setrlimit()
#include <sys/resource.h>	// Needed for setrlimit()
#include <cstdlib>		// Needed for Exit() (C99)
#include <new>
#include "cwd_debug.h"
#include <libcwd/strerrno.h>
#include <libcwd/private_internal_stringbuf.h>
#include <libcwd/private_bufferstream.h>
#include "private_debug_stack.inl"

extern "C" int raise(int);

#if LIBCWD_THREAD_SAFE
using libcw::debug::_private_::rwlock_tct;
#if __GNUC_MINOR__ != 5
using libcw::debug::_private_::debug_objects_instance;
using libcw::debug::_private_::debug_channels_instance;
#endif
#define DEBUG_OBJECTS_ACQUIRE_WRITE_LOCK	rwlock_tct<libcw::debug::_private_::debug_objects_instance>::wrlock()
#define DEBUG_OBJECTS_RELEASE_WRITE_LOCK	rwlock_tct<libcw::debug::_private_::debug_objects_instance>::wrunlock()
#define DEBUG_OBJECTS_ACQUIRE_READ_LOCK		rwlock_tct<libcw::debug::_private_::debug_objects_instance>::rdlock()
#define DEBUG_OBJECTS_RELEASE_READ_LOCK		rwlock_tct<libcw::debug::_private_::debug_objects_instance>::rdunlock()
#define DEBUG_OBJECTS_ACQUIRE_READ2WRITE_LOCK	rwlock_tct<libcw::debug::_private_::debug_objects_instance>::rd2wrlock()
#define DEBUG_OBJECTS_ACQUIRE_WRITE2READ_LOCK	rwlock_tct<libcw::debug::_private_::debug_objects_instance>::wr2rdlock()
#define DEBUG_CHANNELS_ACQUIRE_WRITE_LOCK	rwlock_tct<libcw::debug::_private_::debug_channels_instance>::wrlock()
#define DEBUG_CHANNELS_RELEASE_WRITE_LOCK	rwlock_tct<libcw::debug::_private_::debug_channels_instance>::wrunlock()
#define DEBUG_CHANNELS_ACQUIRE_READ_LOCK	rwlock_tct<libcw::debug::_private_::debug_channels_instance>::rdlock()
#define DEBUG_CHANNELS_RELEASE_READ_LOCK	rwlock_tct<libcw::debug::_private_::debug_channels_instance>::rdunlock()
#define DEBUG_CHANNELS_ACQUIRE_READ2WRITE_LOCK	rwlock_tct<libcw::debug::_private_::debug_channels_instance>::rd2wrlock()
#define DEBUG_CHANNELS_ACQUIRE_WRITE2READ_LOCK	rwlock_tct<libcw::debug::_private_::debug_channels_instance>::wr2rdlock()
#define COMMA_IFTHREADS(x) ,x
#else // !LIBCWD_THREAD_SAFE
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
#define COMMA_IFTHREADS(x)
#endif // !LIBCWD_THREAD_SAFE

#define NEED_SUPRESSION_OF_MALLOC_AND_BFD (__GNUC__ == 3 && \
    ((__GNUC_MINOR__ == 2 && __VERSION__ [3] == 0) || \
     (__GNUC_MINOR__ == 1 && __VERSION__ [4] == '1') || \
     (__GNUC_MINOR__ == 0)))

namespace libcw {
  namespace debug {

namespace _private_ {

#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
__gnu_cxx::_STL_mutex_lock* _ZN9__gnu_cxx12__pool_allocILb1ELi0EE7_S_lockE_ptr;
#endif

// The following tries to take the "node allocator" lock -- the lock of the
// default allocator for threaded applications.
__inline__
bool allocator_trylock(void)
{
#if (__GNUC__ < 3 || __GNUC_MINOR__ == 0)
  if (!(__NODE_ALLOCATOR_THREADS)) return true;
#endif
#if (__GNUC__ == 3 && __GNUC_MINOR__ < 4)
#if !defined(__GTHREAD_MUTEX_INIT) && defined(__GTHREAD_MUTEX_INIT_FUNCTION)
  if (!std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_init_flag)
    std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_initialize();
#endif
  return (__gthread_mutex_trylock(&std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_lock) == 0);
#else
#if !defined(__GTHREAD_MUTEX_INIT) && defined(__GTHREAD_MUTEX_INIT_FUNCTION)
  if (!LIBCWD_POOL_ALLOC<true, 0>::_S_lock._M_init_flag)
    LIBCWD_POOL_ALLOC<true, 0>::_S_lock._M_initialize();
#endif
  return (_ZN9__gnu_cxx12__pool_allocILb1ELi0EE7_S_lockE_ptr &&
          __gthread_mutex_trylock(&_ZN9__gnu_cxx12__pool_allocILb1ELi0EE7_S_lockE_ptr->_M_lock) == 0);
#endif
}

// The following unlocks the node allocator.
__inline__
void allocator_unlock(void)
{
#if (__GNUC__ < 3 || __GNUC_MINOR__ == 0)
  if (!(__NODE_ALLOCATOR_THREADS)) return;
#endif
#if (__GNUC__ == 3 && __GNUC_MINOR__ < 4)
#if !defined(__GTHREAD_MUTEX_INIT) && defined(__GTHREAD_MUTEX_INIT_FUNCTION)
  if (!std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_init_flag)
    std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_initialize();
#endif
  __gthread_mutex_unlock(&std::__default_alloc_template<true, 0>::_S_node_allocator_lock._M_lock);
#else
#if !defined(__GTHREAD_MUTEX_INIT) && defined(__GTHREAD_MUTEX_INIT_FUNCTION)
  if (!LIBCWD_POOL_ALLOC<true, 0>::_S_lock._M_init_flag)
    LIBCWD_POOL_ALLOC<true, 0>::_S_lock._M_initialize();
#endif
  __gthread_mutex_unlock(&_ZN9__gnu_cxx12__pool_allocILb1ELi0EE7_S_lockE_ptr->_M_lock);
#endif
}
#endif // LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC

} // namespace _private_

    using _private_::set_alloc_checking_on;
    using _private_::set_alloc_checking_off;
#if CWDEBUG_ALLOC
    using _private_::debug_message_st;
#endif

    class buffer_ct : public _private_::auto_internal_stringbuf {
    private:
      typedef pos_type streampos_t;
      streampos_t position;
#if LIBCWD_THREAD_SAFE
      // These two are protected by the ostream lock of a debug object.
      bool unfinished_already_printed;
      bool continued_needed;
#endif
    public:
#if LIBCWD_THREAD_SAFE
      buffer_ct(void) : unfinished_already_printed(false), continued_needed(false) { }
#endif
      void writeto(std::ostream* os LIBCWD_COMMA_TSD_PARAM, debug_ct& debug_object,
	  bool request_unfinished, bool do_flush COMMA_IFTHREADS(bool ends_on_newline)
	  COMMA_IFTHREADS(bool possible_nonewline_cf));
      void store_position(void) {
	position = this->pubseekoff(0, std::ios_base::cur, std::ios_base::out);
      }
      void restore_position(void) {
	this->pubseekpos(position, std::ios_base::out);
	this->pubseekpos(0, std::ios_base::in);
#if LIBCWD_THREAD_SAFE
	continued_needed = false;
#endif
      }
      void write_prefix_to(std::ostream* os)
      {
	streampos_t old_in_pos = this->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
	this->pubseekpos(0, std::ios_base::in);
	os->put(this->sgetc());
	int size = position - std::streampos(0);
	for (int c = 1; c < size; ++c)
	  os->put(this->snextc());
        this->pubseekpos(old_in_pos, std::ios_base::in);
      }
    };

    void buffer_ct::writeto(std::ostream* os LIBCWD_COMMA_TSD_PARAM, debug_ct& debug_object,
	bool request_unfinished, bool do_flush COMMA_IFTHREADS(bool ends_on_newline)
	COMMA_IFTHREADS(bool possible_nonewline_cf))
    {
      // os			: The ostream that we need to write to.
      // __libcwd_tsd		: The Thread Specific context.
      // debug_object		: The debug object context.
      // request_unfinished	: When set, then this thread is writing output that is interrupting
      // 			  unfinished debug output of its own.
      // do_flush		: Flush the ostream after writing to it.
      // ends_on_newline	: This output ends on a newline.
      // possible_nonewline_cf	: When `ends_on_newline' is false, then that was caused by the use of nonewline_cf.

#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC
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
      curlen = this->pubseekoff(0, std::ios_base::cur, std::ios_base::out) - this->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
      bool free_msgbuf = false;
      if (queue_msg)
	msgbuf = (msgbuf_t)malloc(curlen + extra_size);
      else if (curlen > 512 || !(msgbuf = (msgbuf_t)__builtin_alloca(curlen + extra_size)))
      {
	msgbuf = (msgbuf_t)malloc(curlen + extra_size);
	free_msgbuf = true;
      }
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC
      this->sgetn(msgbuf->buf, curlen);
#else
      this->sgetn(msgbuf, curlen);
#endif
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC
      if (queue_msg)	// Inside a call to malloc and possibly owning lock of std::LIBCWD_POOL_ALLOC<true, 0>?
      {
	// We don't write debug output to the final ostream when inside malloc and std::LIBCWD_POOL_ALLOC<true, 0> is locked.
	// It is namely possible that this will again try to acquire the lock in std::LIBCWD_POOL_ALLOC<true, 0>, resulting
	// in a deadlock.  Append it to the queue instead.
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
#endif
#if CWDEBUG_ALLOC
	// Writing to the final std::ostream (ie std::cerr) must be non-internal!
	// LIBCWD_DISABLE_CANCEL/LIBCWD_ENABLE_CANCEL must be done non-internal too.
	int saved_internal = _private_::set_library_call_on(LIBCWD_TSD);
	++LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
#endif
#if LIBCWD_THREAD_SAFE
	LIBCWD_DISABLE_CANCEL;			// We don't want Dout() to be a cancellation point.
	_private_::mutex_tct<_private_::set_ostream_instance>::lock();
	bool got_lock = debug_object.M_mutex;
	if (got_lock)
	{
	  debug_object.M_mutex->lock();
	  __libcwd_tsd.pthread_lock_interface_is_locked = true;
	}
	std::ostream* locked_os = os;
	_private_::mutex_tct<_private_::set_ostream_instance>::unlock();
	if (!got_lock && _private_::WST_multi_threaded)
	{
	  static bool WST_second_time = false;	// Break infinite loop.
	  if (!WST_second_time)
	  {
	    WST_second_time = true;
	    DoutFatal(dc::core, "When using multiple threads, you must provide a locking mechanism for the debug output stream.  "
		"You can pass a pointer to a mutex with `debug_ct::set_ostream' (see documentation/reference-manual/group__group__destination.html).");
	  }
	}
#endif
#if LIBCWD_THREAD_SAFE
	if (debug_object.newlineless_tsd && debug_object.newlineless_tsd != &__libcwd_tsd)
	{
	  if (debug_object.unfinished_oss)
	  {
	    if (debug_object.unfinished_oss != this)
	    {
	      locked_os->write("<unfinished>\n", 13);
	      debug_object.unfinished_oss->unfinished_already_printed = true;
	      debug_object.unfinished_oss->continued_needed = true;
	    }
	  }
	  else
	    locked_os->write("<no newline>\n", 13);
	}
	if (continued_needed && curlen > 0)
	{
	  continued_needed = false;
	  write_prefix_to(locked_os);
	  locked_os->write("<continued> ", 12);
	}
#endif // LIBCWD_THREAD_SAFE
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC
	debug_message_st* message = debug_object.queue_top;
	if (message)
	{
	  // First empty the whole queue.
	  debug_message_st* next_message;
	  do
	  {
	    next_message = message->prev;
	    locked_os->write(message->buf, message->curlen);
	    __libcwd_tsd.internal = 1;
	    free(message);
	    __libcwd_tsd.internal = 0;
	  }
	  while ((message = next_message));
	  debug_object.queue_top = debug_object.queue = NULL;
	}
	// Then write the new message.
	locked_os->write(msgbuf->buf, curlen);
#else // !(CWDEBUG_ALLOC && LIBCWD_THREAD_SAFE)
#if LIBCWD_THREAD_SAFE
	locked_os->write(msgbuf, curlen);
#else // !LIBCWD_THREAD_SAFE
	os->write(msgbuf, curlen);
#endif // !LIBCWD_THREAD_SAFE
#endif // !(CWDEBUG_ALLOC && LIBCWD_THREAD_SAFE)
#if LIBCWD_THREAD_SAFE
	if (request_unfinished && !unfinished_already_printed)
	  locked_os->write("<unfinished>\n", 13);
#else
	if (request_unfinished)
	  os->write("<unfinished>\n", 13);
#endif
	if (do_flush)
#if LIBCWD_THREAD_SAFE
	  locked_os->flush();
#else
	  os->flush();
#endif
#if LIBCWD_THREAD_SAFE
	unfinished_already_printed = ends_on_newline;
	if (ends_on_newline)
	{
	  debug_object.unfinished_oss = NULL;
	  debug_object.newlineless_tsd = NULL;
	}
	else if (curlen > 0)
	{
	  debug_object.newlineless_tsd = &__libcwd_tsd;
	  if (possible_nonewline_cf)
	    debug_object.unfinished_oss = NULL;
	  else
	    debug_object.unfinished_oss = this;
        }
	if (got_lock)
	{
	  __libcwd_tsd.pthread_lock_interface_is_locked = false;
	  debug_object.M_mutex->unlock();
	}
	LIBCWD_ENABLE_CANCEL;
#endif // !LIBCWD_THREAD_SAFE
#if CWDEBUG_ALLOC
	--LIBCWD_DO_TSD_MEMBER_OFF(libcw_do);
	_private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
#endif
#if LIBCWD_THREAD_SAFE && CWDEBUG_ALLOC
      }
#endif
      if (free_msgbuf)
	free(msgbuf);
    }

    // Configuration signature
    unsigned long const config_signature_lib_c = config_signature_header_c;

    // Return the configuration signature of the .so file.
    unsigned long get_config_signature_lib_c(void)
    {
      return config_signature_lib_c;
    }

    // Put this here to decrease the code size of `check_configuration'
    void conf_check_failed(void)
    {
      DoutFatal(dc::fatal, "check_configuration: This version of libcwd was compiled with a different configuration than is currently used in libcwd/config.h!");
    }

    void version_check_failed(void)
    {
      DoutFatal(dc::fatal, "check_configuration: This version of libcwd does not match the version of libcwd/config.h!  Are your paths correct?");
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
    } // namespace

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
      } // namespace dc
    } // namespace channels

#if CWDEBUG_LOCATION
    namespace cwbfd {
      extern bool ST_init(LIBCWD_TSD_PARAM);
    } // namespace cwbfd
#endif

    void ST_initialize_globals(LIBCWD_TSD_PARAM)
    {
      static bool ST_already_called;
      if (ST_already_called)
	return;
      ST_already_called = true;
#if CWDEBUG_ALLOC
      init_debugmalloc();
#endif
#if LIBCWD_THREAD_SAFE
      _private_::initialize_global_mutexes();
#endif
      _private_::process_environment_variables();

      // Fatal channels need to be marked fatal, otherwise we get into an endless loop
      // when they are used before they are created.
      channels::dc::core.NS_initialize("COREDUMP", coredump_maskbit LIBCWD_COMMA_TSD);
      channels::dc::fatal.NS_initialize("FATAL", fatal_maskbit LIBCWD_COMMA_TSD);
      // Initialize other debug channels that might be used before we reach main().
      channels::dc::debug.NS_initialize("DEBUG" LIBCWD_COMMA_TSD, true);
      channels::dc::malloc.NS_initialize("MALLOC" LIBCWD_COMMA_TSD, true);
      channels::dc::continued.NS_initialize(continued_maskbit);
      channels::dc::finish.NS_initialize(finish_maskbit);
#if CWDEBUG_LOCATION
      channels::dc::bfd.NS_initialize("BFD" LIBCWD_COMMA_TSD, true);
#endif
      // What the heck, initialize all other debug channels too
      channels::dc::warning.NS_initialize("WARNING" LIBCWD_COMMA_TSD, true);
      channels::dc::notice.NS_initialize("NOTICE" LIBCWD_COMMA_TSD, true);
      channels::dc::system.NS_initialize("SYSTEM" LIBCWD_COMMA_TSD, true);

      if (!libcw_do.NS_init(LIBCWD_TSD))	// Initialize debug code.
	DoutFatal(dc::core, "Calling debug_ct::NS_init recursively from ST_initialize_globals");

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
      cwbfd::ST_init(LIBCWD_TSD);		// Initialize BFD code.
#endif
#if CWDEBUG_DEBUG && !CWDEBUG_DEBUGOUTPUT
      // Force allocation of a __cxa_eh_globals struct in libsupc++.
      (void)std::uncaught_exception();		// Leaks memory.
#endif
    }

    namespace _private_ {

#if !LIBCWD_THREAD_SAFE
      TSD_st __libcwd_tsd;
#endif
#if LIBCWD_THREAD_SAFE
      extern bool WST_is_NPTL;
#endif

      debug_channels_ct debug_channels;		// List with all channel_ct objects.
      debug_objects_ct debug_objects;		// List with all debug devices.

      // _private_::
      void debug_channels_ct::init(LIBCWD_TSD_PARAM)
      {
#if LIBCWD_THREAD_SAFE
	_private_::rwlock_tct<libcw::debug::_private_::debug_channels_instance>::initialize();
#endif
	DEBUG_CHANNELS_ACQUIRE_READ_LOCK;
	if (!WNS_debug_channels)			// MT: `WNS_debug_channels' is only false when this object is still Non_Shared.
	{
	  DEBUG_CHANNELS_ACQUIRE_READ2WRITE_LOCK;
	  set_alloc_checking_off(LIBCWD_TSD);
	  WNS_debug_channels = new debug_channels_ct::container_type;
	  set_alloc_checking_on(LIBCWD_TSD);
	  DEBUG_CHANNELS_RELEASE_WRITE_LOCK;
	}
#if LIBCWD_THREAD_SAFE
	else
	  DEBUG_CHANNELS_RELEASE_READ_LOCK;
#endif
      }

#if LIBCWD_THREAD_SAFE
      // _private_::
      void debug_channels_ct::init_and_rdlock(void)
      {
	_private_::rwlock_tct<libcw::debug::_private_::debug_channels_instance>::initialize();
	DEBUG_CHANNELS_ACQUIRE_READ_LOCK;
	if (!WNS_debug_channels)			// MT: `WNS_debug_channels' is only false when this object is still Non_Shared.
	{
	  LIBCWD_TSD_DECLARATION;
	  set_alloc_checking_off(LIBCWD_TSD);
	  DEBUG_CHANNELS_ACQUIRE_READ2WRITE_LOCK;
	  WNS_debug_channels = new debug_channels_ct::container_type;
	  DEBUG_CHANNELS_ACQUIRE_WRITE2READ_LOCK;
	  set_alloc_checking_on(LIBCWD_TSD);
	}
      }
#endif

      // _private_::
      void debug_objects_ct::init(LIBCWD_TSD_PARAM)
      {
#if LIBCWD_THREAD_SAFE
	_private_::rwlock_tct<libcw::debug::_private_::debug_objects_instance>::initialize();
#endif
        DEBUG_OBJECTS_ACQUIRE_READ_LOCK;
	if (!WNS_debug_objects)				// MT: `WNS_debug_objects' is only false when this object is still Non_Shared.
	{
	  DEBUGDEBUG_CERR( "_debug_objects == NULL; initializing it" );
#if CWDEBUG_ALLOC
	  // It is possible that malloc is not initialized yet.
	  init_debugmalloc();
#endif
	  DEBUG_OBJECTS_ACQUIRE_READ2WRITE_LOCK;
	  set_alloc_checking_off(LIBCWD_TSD);
	  WNS_debug_objects = new debug_objects_ct::container_type;
	  set_alloc_checking_on(LIBCWD_TSD);
	  DEBUG_OBJECTS_RELEASE_WRITE_LOCK;
	}
#if LIBCWD_THREAD_SAFE
	else
	  DEBUG_OBJECTS_RELEASE_READ_LOCK;
#endif
      }

#if LIBCWD_THREAD_SAFE
      // _private_::
      void debug_objects_ct::init_and_rdlock(void)
      {
	_private_::rwlock_tct<libcw::debug::_private_::debug_objects_instance>::initialize();
	DEBUG_OBJECTS_ACQUIRE_READ_LOCK;
	if (!WNS_debug_objects)				// MT: `WNS_debug_objects' is only false when this object is still Non_Shared.
	{
	  DEBUGDEBUG_CERR( "_debug_objects == NULL; initializing it" );
#if CWDEBUG_ALLOC
	  // It is possible that malloc is not initialized yet.
	  init_debugmalloc();
#endif
	  LIBCWD_TSD_DECLARATION;
	  set_alloc_checking_off(LIBCWD_TSD);
	  DEBUG_OBJECTS_ACQUIRE_READ2WRITE_LOCK;
	  WNS_debug_objects = new debug_objects_ct::container_type;
	  DEBUG_OBJECTS_ACQUIRE_WRITE2READ_LOCK;
	  set_alloc_checking_on(LIBCWD_TSD);
	}
      }
#endif

      // _private_::
      void debug_objects_ct::ST_uninit(void)
      {
	if (WNS_debug_objects)
	{
	  LIBCWD_TSD_DECLARATION;
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
      buffer_ct buffer;
	// The temporary output buffer.

      _private_::bufferstream_ct bufferstream;
        // Our 'stringstream'.

      control_flag_t mask;
	// The previous control bits.

      char const* label;
	// The previous label.

      int err;
	// The current errno.

    public:
      laf_ct(control_flag_t m, char const* l, int e) : bufferstream(&buffer), mask(m), label(l), err(e) { }
    };

    static inline void write_whitespace_to(std::ostream& os, unsigned int size)
    {
      for (unsigned int i = size; i > 0; --i)
        os.put(' ');
    }

    namespace _private_ {
      static char WST_dummy_laf[sizeof(laf_ct)] __attribute__((__aligned__));
    } // namespace _private_

    /**
     * \fn void core_dump(void)
     * \ingroup chapter_core_dump
     *
     * \brief Dump core of current thread.
     *
     * <b>Example:</b>
     *
     * \code
     * Debug( core_dump() );
     * \endcode
     *
     * Normally you don't call <code>core_dump()</code> directly though.
     * Instead you'd do for example:
     *
     * \code
     * if (error_condition)
     *   DoutFatal(dc::core, "Something went wrong");
     * \endcode
     */
    void core_dump(void)
    {
#if LIBCWD_THREAD_SAFE
      // Are we the first thread that tries to generate a core?
#if CWDEBUG_DEBUGT || CWDEBUG_ALLOC
      LIBCWD_TSD_DECLARATION;
#endif
      LIBCWD_DISABLE_CANCEL;
      if (!_private_::mutex_tct<_private_::kill_threads_instance>::trylock())
      {
#if CWDEBUG_ALLOC
	__libcwd_tsd.internal = 0;	// Dunno if this is needed, but it looks consistant.
	++__libcwd_tsd.library_call;;	// So our sanity checks allow us to call free() again in
					// pthread_exit when we get here from malloc et al.
#endif
	// Another thread is already trying to generate a core dump.
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_exit(PTHREAD_CANCELED); 
      }
      // Leave cancelation disabled because otherwise it might be that another thread is generating the core.
#ifdef HAVE_PTHREAD_KILL_OTHER_THREADS_NP
      // For the same reason, kill other threads prior to generating the core.
      //pthread_kill_other_threads_np(); // Only causes a deadlock.
#endif
#endif
#if LIBCWD_THREAD_SAFE && CWDEBUG_DEBUG
      if (!_private_::WST_is_NPTL && pthread_self() == (pthread_t)2049)
      {
	::write(1, "WARNING: Thread manager core dumped.  Going into infinite loop.  Please detach process with gdb.\n", 97);
	while(1);
      }
#endif
      raise(6);
#if LIBCWD_THREAD_SAFE
      LIBCWD_ENABLE_CANCEL;
#endif
      _Exit(6);		// Never reached.
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

    // This is called with alloc checking off.
    void debug_string_ct::deinitialize(void)
    {
      free(M_str);
      M_str = NULL;
    }

    // This is called with alloc checking on (or off).
    debug_string_ct::~debug_string_ct(void)
    {
#if CWDEBUG_DEBUG
      LIBCWD_ASSERT(M_str == NULL);	// Need to call debug_string_ct::deinitialize() before destructor.
#endif
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
      LIBCWD_TSD_DECLARATION;
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
      LIBCWD_TSD_DECLARATION;
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
      LIBCWD_TSD_DECLARATION;
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
      LIBCWD_TSD_DECLARATION;
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
      LIBCWD_TSD_DECLARATION;
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

    void debug_tsd_st::start(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM)
    {
#if CWDEBUG_DEBUG || CWDEBUG_DEBUGT
#if LIBCWD_THREAD_SAFE
      // Initialisation of the TSD part should be done from LIBCWD_TSD_DECLARATION inside Dout et al.
      LIBCWD_ASSERT( tsd_initialized );
#else
      if (!tsd_initialized)
	init();
#endif
#endif

      if (NEED_SUPRESSION_OF_MALLOC_AND_BFD)
      {
        channels::dc::malloc.off();
        channels::dc::bfd.off();
      }

      // Skip `start()' for a `continued' debug output.
      // Generating the "prefix: <continued>" is already taken care
      // of while generating the "<unfinished>" (see next `if' block).
      if ((channel_set.mask & (continued_maskbit|finish_maskbit)))
      {
	current->err = errno;				// Always keep the last errno as set at the start of LibcwDout()
	if (!(current->mask & continued_expected_maskbit))
	{
	  std::ostream* target_os = (channel_set.mask & cerr_cf) ? &std::cerr : debug_object.real_os;
#if LIBCWD_THREAD_SAFE
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
#if LIBCWD_THREAD_SAFE
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
        return;
      }

      set_alloc_checking_off(LIBCWD_TSD);

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
        int saved_errno = errno;				// The writeto below changes errno.
	// And write out what is in the buffer till now.
	std::ostream* target_os = (channel_set.mask & cerr_cf) ? &std::cerr : debug_object.real_os;
	current->buffer.writeto(target_os LIBCWD_COMMA_TSD, debug_object,
	    true,			// This thread requests an <unfinished> because of previous, unfinished 'continued' output.
	    false			// Don't flush.
	    COMMA_IFTHREADS(true)	// This output ends on a newline by itself.
	    COMMA_IFTHREADS(false));	// The newline is not missing as a result of nonewline_cf.
	// Truncate the buffer to its prefix and append "<continued>" to it already.
	current->buffer.restore_position();
	current_bufferstream->write("<continued> ", 12);	// therefore we repeat the space here.
        errno = saved_errno;
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
      int saved_internal = _private_::set_library_call_on(LIBCWD_TSD);
      _private_::set_invisible_on(LIBCWD_TSD);
      current = new laf_ct(channel_set.mask, channel_set.label, errno);
      _private_::set_invisible_off(LIBCWD_TSD);
      _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
      DEBUGDEBUG_CERR( "current = " << (void*)current );
      current_bufferstream = &current->bufferstream;
      DEBUGDEBUG_CERR( "laf_ct created" );

      // Without a new nested Dout() call, we expect to see a finish() call: The finish belonging to *this* Dout() call.
      start_expected = false;

      // If this is a `continued' debug output, then we want to print "<unfinished>" if next we see a start().
      unfinished_expected = true;

      // Print prefix if requested.
      // Handle most common case first: no special flags set
      if (!(channel_set.mask & (noprefix_cf|nolabel_cf|blank_margin_cf|blank_label_cf|blank_marker_cf)))
      {
	current_bufferstream->write(margin.c_str(), margin.size());
	current_bufferstream->write(channel_set.label, WST_max_len);
	current_bufferstream->write(marker.c_str(), marker.size());
	write_whitespace_to(*current_bufferstream, indent);
      }
      else if (!(channel_set.mask & noprefix_cf))
      {
	if ((channel_set.mask & blank_margin_cf))
	  write_whitespace_to(*current_bufferstream, margin.size());
	else
	  current_bufferstream->write(margin.c_str(), margin.size());
#if !CWDEBUG_DEBUGOUTPUT
	if (!(channel_set.mask & nolabel_cf))
#endif
	{
	  if ((channel_set.mask & blank_label_cf))
	    write_whitespace_to(*current_bufferstream, WST_max_len);
	  else
	    current_bufferstream->write(channel_set.label, WST_max_len);
	  if ((channel_set.mask & blank_marker_cf))
	    write_whitespace_to(*current_bufferstream, marker.size());
	  else
	    current_bufferstream->write(marker.c_str(), marker.size());
	  write_whitespace_to(*current_bufferstream, indent);
	}
      }

      if ((channel_set.mask & continued_cf_maskbit))
      {
	// If this is continued debug output, then it makes sense to remember the prefix length,
	// just in case we need indeed to output <continued> data.
        current->buffer.store_position();
      }

      --LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
      DEBUGDEBUG_CERR( "Leaving debug_ct::start(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object) );

      set_alloc_checking_on(LIBCWD_TSD);
    }

    void debug_tsd_st::finish(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM)
    {
#if CWDEBUG_DEBUG
      LIBCWD_ASSERT( current != reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf) );
#endif
      std::ostream* target_os = (current->mask & cerr_cf) ? &std::cerr : debug_object.real_os;

      set_alloc_checking_off(LIBCWD_TSD);

      if (NEED_SUPRESSION_OF_MALLOC_AND_BFD)
      {
        channels::dc::malloc.on();
        channels::dc::bfd.on();
      }

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
	  // Flush ostream.  Note that in the case of nested debug output this `os' can be an stringstream,
	  // in that case, no actual flushing is done until the debug output to the real ostream has
	  // finished.
	  current->buffer.writeto(target_os LIBCWD_COMMA_TSD, debug_object,
	      false,			// This thread requests <unfinished> because of previous, unfinished 'continued' output.
	      true			// Flush ostream after printing this.
	      COMMA_IFTHREADS(false)	// This output does not end on a newline.
	      COMMA_IFTHREADS(false));	// The newline is not missing as a result of nonewline_cf.
	}
	set_alloc_checking_on(LIBCWD_TSD);
        return;
      }

      ++LIBCWD_DO_TSD_MEMBER_OFF(debug_object);
      DEBUGDEBUG_CERR( "Entering debug_ct::finish(), _off became " << LIBCWD_DO_TSD_MEMBER_OFF(debug_object) );

      // Handle control flags, if any:
      if ((current->mask & error_cf))
      {
	// strerror[_r] can call malloc (in gettext()).
#if CWDEBUG_ALLOC
	int saved_internal = _private_::set_library_call_on(LIBCWD_TSD);
#endif
#if !LIBCWD_THREAD_SAFE
	char const* error_text = strerror(current->err);
#else // LIBCWD_THREAD_SAFE
	char error_text_buf[512];
	char const* error_text = strerror_r(current->err, error_text_buf, sizeof(error_text_buf));
#endif
#if CWDEBUG_ALLOC
	_private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
#endif
	*current_bufferstream << ": " << strerrno(current->err) << " (" << error_text << ')';
      }
      if (!(current->mask & nonewline_cf))
	current_bufferstream->put('\n');

      // Handle control flags, if any:
      if (current->mask != 0)
      {
	if ((current->mask & (coredump_maskbit|fatal_maskbit)))
	{
	  current->buffer.writeto(target_os LIBCWD_COMMA_TSD, debug_object,
	      false,			// This thread requests <unfinished> because of previous, unfinished 'continued' output.
	      !__libcwd_tsd.recursive_fatal	// Flush ostream after printing this when there is no recursive loop yet.
	      COMMA_IFTHREADS(!(current->mask & nonewline_cf))	// Whether or not this output ends on a newline.
	      COMMA_IFTHREADS(true));	// If the newline is missing, then it is missing because of the use of nonewline_cf.
	  __libcwd_tsd.recursive_fatal = true;
	  if ((current->mask & coredump_maskbit))
	    core_dump();
	  DEBUGDEBUG_CERR( "Deleting `current' " << (void*)current );
	  int saved_internal = _private_::set_library_call_on(LIBCWD_TSD);
	  _private_::set_invisible_on(LIBCWD_TSD);
	  delete current;
	  _private_::set_invisible_off(LIBCWD_TSD);
	  _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
	  DEBUGDEBUG_CERR( "Done deleting `current'" );
	  set_alloc_checking_on(LIBCWD_TSD);
#if CWDEBUG_ALLOC
	  if (__libcwd_tsd.internal)			// Still internal?  Are we ever calling DoutFatal while internal?
	    _private_::set_library_call_on(LIBCWD_TSD);	// Indefinetely turn library call on before terminating threads.
#endif
#if LIBCWD_THREAD_SAFE
	  LIBCWD_DISABLE_CANCEL;
	  if (!_private_::mutex_tct<_private_::kill_threads_instance>::trylock())
	  {
	    // Another thread is already trying to generate a core dump.
	    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	    pthread_exit(PTHREAD_CANCELED); 
	  }
	  _private_::rwlock_tct<_private_::threadlist_instance>::rdlock(true);
          // Terminate all threads that I know of, so that no locks will remain.
	  for(_private_::threadlist_t::iterator thread_iter = _private_::threadlist->begin(); thread_iter != _private_::threadlist->end(); ++thread_iter)
	    if (!pthread_equal((*thread_iter).tid, pthread_self()) && (_private_::WST_is_NPTL || (*thread_iter).tid != 1024))
              pthread_cancel((*thread_iter).tid);
	  _private_::rwlock_tct<_private_::threadlist_instance>::rdunlock();
	  LIBCWD_ENABLE_CANCEL;
#endif
	  _Exit(254);	// Exit without calling global destructors.
	}
	if ((current->mask & wait_cf))
	{
	  current->buffer.writeto(target_os LIBCWD_COMMA_TSD, debug_object,
	      false, debug_object.interactive COMMA_IFTHREADS(!(current->mask & nonewline_cf))
	      COMMA_IFTHREADS(true));
#if LIBCWD_THREAD_SAFE
	  debug_object.M_mutex->lock();
#endif
	  *target_os << "(type return)";
	  if (debug_object.interactive)
	  {
	    *target_os << std::flush;
	    while(std::cin.get() != '\n');
	  }
#if LIBCWD_THREAD_SAFE
	  debug_object.M_mutex->unlock();
#endif
	}
	else
	  current->buffer.writeto(target_os LIBCWD_COMMA_TSD, debug_object,
	      false, (current->mask & flush_cf) COMMA_IFTHREADS(!(current->mask & nonewline_cf))
	      COMMA_IFTHREADS(true));
      }
      else
	current->buffer.writeto(target_os LIBCWD_COMMA_TSD, debug_object,
	    false, false COMMA_IFTHREADS(!(current->mask & nonewline_cf)) COMMA_IFTHREADS(true));

      DEBUGDEBUG_CERR( "Deleting `current' " << (void*)current );
      int saved_internal = _private_::set_library_call_on(LIBCWD_TSD);
      _private_::set_invisible_on(LIBCWD_TSD);
      control_flag_t mask = current->mask;	// Keep this.
      delete current;
      _private_::set_invisible_off(LIBCWD_TSD);
      _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
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
        current = laf_stack.top();
	DEBUGDEBUG_CERR( "current = " << (void*)current );
	current_bufferstream = &current->bufferstream;
	if ((mask & flush_cf))
	  current->mask |= flush_cf;	// Propagate flush to real ostream.
      }
      else
      {
        current = reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf);	// Used (MT: read-only!) in next debug_ct::start().
	DEBUGDEBUG_CERR( "current = " << (void*)current );
	current_bufferstream = NULL;
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
      _Exit(1);	// This is never reached, but g++ 3.3.x -O2 thinks it is.
    }

#if LIBCWD_THREAD_SAFE
    int debug_ct::S_index_count = 0;
#endif

    bool debug_ct::NS_init(LIBCWD_TSD_PARAM)
    {
      if (NS_being_initialized)
        return false;

      ST_initialize_globals(LIBCWD_TSD);// Because all allocations for global objects are internal these days, we use
      					// the constructor of debug_ct to initiate the global initialization of libcwd
					// instead of relying on malloc().

      if (WNS_initialized)
        return true;

      NS_being_initialized = true;

#if LIBCWD_THREAD_SAFE
      M_mutex = NULL;
      unfinished_oss = NULL;
#endif

#if CWDEBUG_DEBUG
      if (!WST_debug_object_init_magic)
	init_debug_object_init_magic();
      init_magic = WST_debug_object_init_magic;
      DEBUGDEBUG_CERR( "Set init_magic to " << init_magic );
      DEBUGDEBUG_CERR( "Setting WNS_initialized to true" );
#endif

      LIBCWD_DEFER_CANCEL;
      _private_::debug_objects.init(LIBCWD_TSD);
      set_alloc_checking_off(LIBCWD_TSD);		// debug_objects is internal.
      DEBUG_OBJECTS_ACQUIRE_WRITE_LOCK;
      if (find(_private_::debug_objects.write_locked().begin(),
	       _private_::debug_objects.write_locked().end(), this)
	  == _private_::debug_objects.write_locked().end()) // Not added before?
	_private_::debug_objects.write_locked().push_back(this);
      DEBUG_OBJECTS_RELEASE_WRITE_LOCK;
#if LIBCWD_THREAD_SAFE
      set_alloc_checking_on(LIBCWD_TSD);
      LIBCWD_RESTORE_CANCEL;
      set_alloc_checking_off(LIBCWD_TSD);		// debug_objects is internal.
#endif
      int saved_internal = _private_::set_library_call_on(LIBCWD_TSD);
      _private_::set_invisible_on(LIBCWD_TSD);
      new (_private_::WST_dummy_laf) laf_ct(0, channels::dc::debug.get_label(), 0);	// Leaks 24 bytes of memory
      _private_::set_invisible_off(LIBCWD_TSD);
      _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
#if LIBCWD_THREAD_SAFE
      WNS_index = S_index_count++;
#if CWDEBUG_DEBUGT
      LIBCWD_ASSERT( !_private_::WST_multi_threaded ); // Only the first thread should be initializing debug_ct objects.
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
      DEBUGDEBUG_CERR( "debug_ct::NS_init(), _off set to " << LIBCWD_TSD_MEMBER_OFF );

      set_ostream(&std::cerr);			// Write to std::cerr by default.
      interactive = true;			// and thus we're interactive.

      NS_being_initialized = false;
      WNS_initialized = true;
      return true;				// Success.
    }

    void debug_tsd_st::init(void)
    {
#if CWDEBUG_DEBUGM
      LIBCWD_TSD_DECLARATION;
      LIBCWD_ASSERT( __libcwd_tsd.internal );
#endif
      start_expected = true;			// Of course, we start with expecting the beginning of a debug output.
      unfinished_expected = false;

      // `current' needs to be non-zero (saving us a check in start()) and
      // current.mask needs to be 0 to avoid a crash in start():
      current = reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf);
      DEBUGDEBUG_CERR( "current = " << (void*)current );
      current_bufferstream = NULL;
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

#if LIBCWD_THREAD_SAFE
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
    } // namespace _private_
#endif

    debug_tsd_st::~debug_tsd_st()
    {
#if !LIBCWD_THREAD_SAFE
      // In the threaded case, we are called with `internal' set already.
      set_alloc_checking_off();
#endif
      margin.deinitialize();
      marker.deinitialize();
#if !LIBCWD_THREAD_SAFE
      set_alloc_checking_on();
#endif
      if (!tsd_initialized)	// Skip the rest when it wasn't initialized.
	return;
      // Sanity checks:
      if (continued_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_tsd_st with a non-empty continued_stack (missing dc::finish?)" );
      if (laf_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_tsd_st with a non-empty laf_stack" );
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
      LIBCWD_TSD_DECLARATION;
      LIBCWD_DEFER_CANCEL;
      _private_::debug_channels.init(LIBCWD_TSD);
      DEBUG_CHANNELS_ACQUIRE_READ_LOCK;
      for(_private_::debug_channels_ct::container_type::const_iterator i(_private_::debug_channels.read_locked().begin());
	  i != _private_::debug_channels.read_locked().end(); ++i)
      {
        if (!strncasecmp(label, (*i)->get_label(), strlen(label)))
          tmp = (*i);
      }
      DEBUG_CHANNELS_RELEASE_READ_LOCK;
      LIBCWD_RESTORE_CANCEL;
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
      LIBCWD_TSD_DECLARATION;
      if (LIBCWD_DO_TSD_MEMBER_OFF(debug_object) < 0)
      {
	LIBCWD_DEFER_CANCEL;
        _private_::debug_channels.init(LIBCWD_TSD);
	LIBCWD_RESTORE_CANCEL;
	LIBCWD_DEFER_CLEANUP_PUSH(&rwlock_tct<libcw::debug::_private_::debug_channels_instance>::cleanup, NULL);
	DEBUG_CHANNELS_ACQUIRE_READ_LOCK;
	for(_private_::debug_channels_ct::container_type::const_iterator i(_private_::debug_channels.read_locked().begin());
	    i != _private_::debug_channels.read_locked().end(); ++i)
	{
	  LibcwDoutScopeBegin(DEBUGCHANNELS, debug_object, dc::always|noprefix_cf);
	  if (NEED_SUPRESSION_OF_MALLOC_AND_BFD)
	  {
	    channels::dc::malloc.on();
	    channels::dc::bfd.on();
	  }
	  LibcwDoutStream.write(LIBCWD_DO_TSD_MEMBER(debug_object, margin).c_str(), LIBCWD_DO_TSD_MEMBER(debug_object, margin).size());
	  LibcwDoutStream.write((*i)->get_label(), WST_max_len);
	  if ((*i)->is_on(LIBCWD_TSD))
	    LibcwDoutStream.write(": Enabled", 9);
	  else
	    LibcwDoutStream.write(": Disabled", 10);
	  if (NEED_SUPRESSION_OF_MALLOC_AND_BFD)
	  {
	    channels::dc::malloc.off();
	    channels::dc::bfd.off();
	  }
	  LibcwDoutScopeEnd;
	}
        DEBUG_CHANNELS_RELEASE_READ_LOCK;
	LIBCWD_CLEANUP_POP_RESTORE(false);
      }
    }

    void channel_ct::NS_initialize(char const* label LIBCWD_COMMA_TSD_PARAM, bool add_to_channel_list)
    {
      // This is pretty much identical to fatal_channel_ct::fatal_channel_ct().

      if (WNS_initialized)
        return;		// Already initialized.

      DEBUGDEBUG_CERR( "Entering `channel_ct::NS_initialize(\"" << label << "\")'" );

      size_t label_len = strlen(label);

      if (label_len > max_label_len_c)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << label << "\") > " << max_label_len_c );

      LIBCWD_DEFER_CANCEL;
      _private_::debug_channels.init(LIBCWD_TSD);
      DEBUG_CHANNELS_ACQUIRE_WRITE_LOCK;

      set_alloc_checking_off(LIBCWD_TSD);	// debug_channels is internal.
      _private_::debug_channels_ct::container_type& channels(_private_::debug_channels.write_locked());
      for(_private_::debug_channels_ct::container_type::iterator i(channels.begin()); i != channels.end(); ++i)
	const_cast<char*>((*i)->get_label())[WST_max_len] = ' ';

      // MT: This is not strict thread safe because it is possible that after threads are already
      //     running, a new shared library is dlopen-ed with a new global channel_ct with a larger
      //     label.  However, it makes no sense to lock the *reading* of WST_max_len for the other
      //     threads because this assignment is an atomic operation.  The lock above is only needed
      //     to prefend two such simultaneously loaded libraries from causing WST_max_len to end up
      //     not maximal.
      //     When this thread is cancelled later, there is no need to take action: the debug channel
      //     is global and can be used by any thread.  We want to keep the maximum label length as
      //     set by this channel.  Even when this debug channel is part of shared library that is
      //     being loaded by dlopen() then it won't get removed when this thread is cancelled.
      //     (Actually, a downwards update of WST_max_len should be done by dlclose if at all).
      if (label_len > WST_max_len)
	WST_max_len = label_len;

      for(_private_::debug_channels_ct::container_type::iterator i(channels.begin()); i != channels.end(); ++i)
	const_cast<char*>((*i)->get_label())[WST_max_len] = '\0';
      set_alloc_checking_on(LIBCWD_TSD);

#if LIBCWD_THREAD_SAFE
      // MT: Take advantage of the `libcw::debug::_private_::debug_channels_instance' lock to prefend simultaneous access
      //     to `next_index' in the case of simultaneously dlopen-loaded libraries.
      static int next_index;
      WNS_index = ++next_index;		// Don't use index 0, it is used to make sure that uninitialized channels appear to be off.
       
      __libcwd_tsd.off_cnt_array[WNS_index] = 0;
#else
      off_cnt = 0;
#endif

      strncpy(WNS_label, label, label_len);
      std::memset(WNS_label + label_len, ' ', max_label_len_c - label_len);
      WNS_label[WST_max_len] = '\0';

      // We store debug channels in some organized order, so that the
      // order in which they appear in the ForAllDebugChannels is not
      // dependent on the order in which these global objects are
      // initialized.
      if (add_to_channel_list)
      {
	set_alloc_checking_off(LIBCWD_TSD);	// debug_channels is internal.
	_private_::debug_channels_ct::container_type::iterator i(channels.begin());
	for(; i != channels.end(); ++i)
	  if (strncmp((*i)->get_label(), WNS_label, WST_max_len) > 0)
	    break;
	channels.insert(i, this);
	set_alloc_checking_on(LIBCWD_TSD);
      }

      DEBUG_CHANNELS_RELEASE_WRITE_LOCK;
      LIBCWD_RESTORE_CANCEL;

      // Turn debug channel "WARNING" on by default.
      if (strncmp(WNS_label, "WARNING", label_len) == 0)
#if LIBCWD_THREAD_SAFE
	__libcwd_tsd.off_cnt_array[WNS_index] = -1;
#else
        off_cnt = -1;
#endif

      DEBUGDEBUG_CERR( "Leaving `channel_ct::NS_initialize(\"" << label << "\")" );

      WNS_initialized = true;
    }

    void fatal_channel_ct::NS_initialize(char const* label, control_flag_t maskbit LIBCWD_COMMA_TSD_PARAM)
    {
       // This is pretty much identical to channel_ct::NS_initialize().

      if (WNS_maskbit)
        return;		// Already initialized.

      WNS_maskbit = maskbit;

      DEBUGDEBUG_CERR( "Entering `fatal_channel_ct::NS_initialize(\"" << label << "\")'" );

      size_t label_len = strlen(label);

      if (label_len > max_label_len_c)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << label << "\") > " << max_label_len_c );

      LIBCWD_DEFER_CANCEL;
      _private_::debug_channels.init(LIBCWD_TSD);
      DEBUG_CHANNELS_ACQUIRE_WRITE_LOCK;

      set_alloc_checking_off(LIBCWD_TSD);       // debug_channels is internal.
      _private_::debug_channels_ct::container_type& channels(_private_::debug_channels.write_locked());
      for(_private_::debug_channels_ct::container_type::iterator i(channels.begin()); i != channels.end(); ++i)
        const_cast<char*>((*i)->get_label())[WST_max_len] = ' ';

      // MT: See comments in channel_ct::NS_initialize above.
      if (label_len > WST_max_len)
	WST_max_len = label_len;

      for(_private_::debug_channels_ct::container_type::iterator i(channels.begin()); i != channels.end(); ++i)
        const_cast<char*>((*i)->get_label())[WST_max_len] = '\0';
      set_alloc_checking_on(LIBCWD_TSD);

      strncpy(WNS_label, label, label_len);
      std::memset(WNS_label + label_len, ' ', max_label_len_c - label_len);
      WNS_label[WST_max_len] = '\0';

      DEBUG_CHANNELS_RELEASE_WRITE_LOCK;
      LIBCWD_RESTORE_CANCEL;

      DEBUGDEBUG_CERR( "Leaving `fatal_channel_ct::NS_initialize(\"" << label << "\")" );
    }

    void continued_channel_ct::NS_initialize(control_flag_t maskbit)
    {
      if (!WNS_maskbit)
	WNS_maskbit = maskbit;
    }

    char const always_channel_ct::label[max_label_len_c + 1] = { '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', 0 };

    /**
     * \brief Turn this channel off.
     *
     * \sa on()
     */
    void channel_ct::off(void)
    {
#if LIBCWD_THREAD_SAFE
      LIBCWD_TSD_DECLARATION;
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
#if LIBCWD_THREAD_SAFE
      LIBCWD_TSD_DECLARATION;
      if (__libcwd_tsd.off_cnt_array[WNS_index] == -1)
#else
      if (off_cnt == -1)
#endif
	DoutFatal( dc::core, "Calling channel_ct::on() more often than channel_ct::off()" );
#if LIBCWD_THREAD_SAFE
      __libcwd_tsd.off_cnt_array[WNS_index] -= 1;
#else
      --off_cnt;
#endif
    }

    //----------------------------------------------------------------------------------------
    // Handle continued channel flags and update `off_count' and `continued_stack'.
    //

    namespace _private_ {
      void print_pop_error(void) {
        DoutFatal(dc::core, "Using \"dc::finish\" without corresponding \"continued_cf\" or "
	    "calling the Dout(dc::finish, ...) more often than its corresponding "
	    "Dout(dc::channel|continued_cf, ...).  Note that the wrong \"dc::finish\" doesn't "
	    "have to be the one that we core dumped on, if two or more are nested.");
      }
    }

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
	LIBCWD_TSD_DECLARATION;
	if (__libcwd_tsd.recursive_assert
#if CWDEBUG_DEBUGM
	    || __libcwd_tsd.inside_malloc_or_free
#endif
	    ) 
	{
	  if (!__libcwd_tsd.recursive_assert && __libcwd_tsd.library_call < 6)
	  {
	    int saved_internal;
	    bool is_internal = __libcwd_tsd.internal;
	    if (is_internal)
	      saved_internal = _private_::set_library_call_on(LIBCWD_TSD);	// flush is a library call.
            else
	      ++__libcwd_tsd.library_call;
	    Debug( libcw_do.get_ostream()->flush() );
	    if (is_internal)
	      _private_::set_library_call_off(saved_internal LIBCWD_COMMA_TSD);
            else
	      --__libcwd_tsd.library_call;
	  }
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
      LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUG
      if (!NS_init(LIBCWD_TSD))
        DoutFatal(dc::core, "Calling debug_ct::NS_init recursively from debug_ct::force_on");
#else
      (void)NS_init(LIBCWD_TSD);
#endif
      state._off = LIBCWD_TSD_MEMBER_OFF;
#if CWDEBUG_DEBUGOUTPUT
      state.first_time = LIBCWD_TSD_MEMBER(first_time);
#endif
      LIBCWD_TSD_MEMBER_OFF = -1;					// Turn object on.
    }

    void debug_ct::restore(debug_ct::OnOffState const& state)
    {
      LIBCWD_TSD_DECLARATION;
#if CWDEBUG_DEBUGOUTPUT
      if (state.first_time != LIBCWD_TSD_MEMBER(first_time))		// state.first_time && !first_time.
	core_dump();							// on() was called without first a call to off().
#endif
      if (LIBCWD_TSD_MEMBER_OFF != -1)
	core_dump();							// off() and on() where called and not in equal pairs.
      LIBCWD_TSD_MEMBER_OFF = state._off;				// Restore.
    }

    void channel_ct::force_on(channel_ct::OnOffState& state, char const* label)
    {
      LIBCWD_TSD_DECLARATION;
      NS_initialize(label LIBCWD_COMMA_TSD, true);
#if LIBCWD_THREAD_SAFE
      int& off_cnt(__libcwd_tsd.off_cnt_array[WNS_index]);
#endif
      state.off_cnt = off_cnt;
      off_cnt = -1;					// Turn channel on.
    }

    void channel_ct::restore(channel_ct::OnOffState const& state)
    {
#if LIBCWD_THREAD_SAFE
      LIBCWD_TSD_DECLARATION;
      int& off_cnt(__libcwd_tsd.off_cnt_array[WNS_index]);
#endif
      if (off_cnt != -1)
	core_dump();					// off() and on() where called and not in equal pairs.
      off_cnt = state.off_cnt;				// Restore.
    }

#if LIBCWD_THREAD_SAFE
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
    LIBCWD_TSD_DECLARATION;
    _private_::set_alloc_checking_off(LIBCWD_TSD);
    _private_::lock_interface_base_ct* new_mutex = new _private_::pthread_lock_interface_ct(mutex);	// A single LEAK of 20 bytes per debug object from here is ok.
    _private_::set_alloc_checking_on(LIBCWD_TSD);
    LIBCWD_DEFER_CANCEL;
    _private_::mutex_tct<_private_::set_ostream_instance>::lock();
    _private_::lock_interface_base_ct* old_mutex = M_mutex;
    if (old_mutex)
      old_mutex->lock();		// Make sure all other threads left this critical area.
    M_mutex = new_mutex;
    if (old_mutex)
    {
      old_mutex->unlock();
      _private_::set_alloc_checking_off(LIBCWD_TSD);
      delete old_mutex;
      _private_::set_alloc_checking_on(LIBCWD_TSD);
    }
    private_set_ostream(os);
    _private_::mutex_tct<_private_::set_ostream_instance>::unlock();
    LIBCWD_RESTORE_CANCEL;
  }
#endif // LIBCWD_THREAD_SAFE

/**
 * \brief Set output device (single threaded applications).
 * \ingroup group_destination
 *
 * Assign a new \c ostream to this %debug object (default is <CODE>std::cerr</CODE>).
 * For use in single threaded applications only.
 */
void debug_ct::set_ostream(std::ostream* os)
{
#if LIBCWD_THREAD_SAFE
  if (_private_::WST_multi_threaded)
#if CWDEBUG_LOCATION
    Dout(dc::warning, location_ct((char*)__builtin_return_address(0) + builtin_return_address_offset) << ": You should passing a locking mechanism to `set_ostream' for the ostream (see documentation/reference-manual/group__group__destination.html)");
#else
    Dout(dc::core, "You must passing a locking mechanism to `set_ostream' for the ostream (see documentation/reference-manual/group__group__destination.html)");
#endif
#if CWDEBUG_DEBUGT
  LIBCWD_TSD_DECLARATION;
#endif
  LIBCWD_DEFER_CANCEL;
  _private_::mutex_tct<_private_::set_ostream_instance>::lock();
#endif
  private_set_ostream(os);
#if LIBCWD_THREAD_SAFE
  _private_::mutex_tct<_private_::set_ostream_instance>::unlock();
  LIBCWD_RESTORE_CANCEL;
#endif
}

  } // namespace debug
} // namespace libcw

// This can be used in configure to see if libcwd exists.
extern "C" char const* const __libcwd_version = VERSION;

// The following functions can be invoked from gdb directly.

namespace libcw {
  namespace debug {
    namespace _private_ {
      extern void demangle_symbol(char const* in, _private_::internal_string& out);
    } // namespace _private_
  } // namespace debug
} // namespace libcw
