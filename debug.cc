// $Header$
//
// Copyright (C) 2000 - 2001, by
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

#ifdef LIBCWD_THREAD_SAFE
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
#endif // !LIBCWD_THREAD_SAFE

namespace libcw {
  namespace debug {

    using _private_::set_alloc_checking_on;
    using _private_::set_alloc_checking_off;

    class buffer_ct : public _private_::internal_stringstream {
    private:
#if __GNUC__ < 3
      streampos position;
#else
      pos_type position;
#endif
    public:
      void writeto(std::ostream* os LIBCWD_COMMA_TSD_PARAM)
      {
	char* buf;
	bool used_malloc = false;
	int curlen = rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out) - rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in);
	if (curlen > 512 || !(buf = (char*)alloca(curlen)))
	{
	  set_alloc_checking_off(LIBCWD_TSD);
	  buf = (char*)malloc(curlen);
	  set_alloc_checking_on(LIBCWD_TSD);
	  used_malloc = true;
	}
	rdbuf()->sgetn(buf, curlen);
#ifdef DEBUGMALLOC
	// Writing to the final std::ostream (ie std::cerr) must be non-internal!
	int saved_internal = __libcwd_tsd.internal;
	__libcwd_tsd.internal = 0;
	++__libcwd_tsd.library_call;
	++LIBCWD_DO_TSD_MEMBER(libcw_do, _off);
#endif
	os->write(buf, curlen);
#ifdef DEBUGMALLOC
	--LIBCWD_DO_TSD_MEMBER(libcw_do, _off);
	--__libcwd_tsd.library_call;
	__libcwd_tsd.internal = saved_internal;
#endif
	if (used_malloc)
	{
	  set_alloc_checking_off(LIBCWD_TSD);
	  free(buf);
	  set_alloc_checking_on(LIBCWD_TSD);
	}
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
     * The %debug object that is used by default by Dout and DoutFatal, the only %debug object used by libcwd itself.
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
	 * It is not in the list of debug channels.&nbsp;
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

#ifdef DEBUGUSEBFD
    namespace cwbfd { extern bool ST_init(void); }
#endif

    void ST_initialize_globals(void)
    {
      static bool ST_already_called;
      if (ST_already_called)
	return;
      ST_already_called = true;
#ifdef DEBUGMALLOC
      init_debugmalloc();
#endif
#ifdef LIBCWD_THREAD_SAFE
      _private_::initialize_global_mutexes();
#endif
      libcw_do.NS_init();			// Initialize debug code.
#ifdef DEBUGUSEBFD
      cwbfd::ST_init();				// Initialize BFD code.
#endif
#if defined(DEBUGDEBUG) && !defined(DEBUGDEBUGOUTPUT)
      // Force allocation of a __cxa_eh_globals struct in libsupc++.
      (void)std::uncaught_exception();
#endif
#if defined(DEBUGDEBUG) && defined(LIBCWD_THREAD_SAFE)
      _private_::WST_multi_threaded = true;
#endif
    }

    namespace _private_ {

      debug_channels_ct debug_channels;		// List with all channel_ct objects.
      debug_objects_ct debug_objects;		// List with all debug devices.

      // _private_::
      void debug_channels_ct::init(void)
      {
#ifdef LIBCWD_THREAD_SAFE
	_private_::rwlock_tct<_private_::debug_channels_instance>::initialize();
#endif
	DEBUG_CHANNELS_ACQUIRE_READ_LOCK
	if (!WNS_debug_channels)			// MT: `WNS_debug_channels' is only false when this object is still Non_Shared.
	{
	  LIBCWD_TSD_DECLARATION
	  set_alloc_checking_off(LIBCWD_TSD);
	  DEBUG_CHANNELS_ACQUIRE_READ2WRITE_LOCK
	  WNS_debug_channels = new debug_channels_ct::container_type;
	  DEBUG_CHANNELS_RELEASE_WRITE_LOCK
	  set_alloc_checking_on(LIBCWD_TSD);
	}
#ifdef LIBCWD_THREAD_SAFE
	else
	  DEBUG_CHANNELS_RELEASE_READ_LOCK
#endif
      }

#ifdef LIBCWD_THREAD_SAFE
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
      void debug_objects_ct::init(void)
      {
#ifdef LIBCWD_THREAD_SAFE
	_private_::rwlock_tct<_private_::debug_objects_instance>::initialize();
#endif
        DEBUG_OBJECTS_ACQUIRE_READ_LOCK
	if (!WNS_debug_objects)				// MT: `WNS_debug_objects' is only false when this object is still Non_Shared.
	{
	  DEBUGDEBUG_CERR( "_debug_objects == NULL; initializing it" );
#ifdef DEBUGMALLOC
	  // It is possible that malloc is not initialized yet.
	  init_debugmalloc();
#endif
	  LIBCWD_TSD_DECLARATION
	  set_alloc_checking_off(LIBCWD_TSD);
	  DEBUG_OBJECTS_ACQUIRE_READ2WRITE_LOCK
	  WNS_debug_objects = new debug_objects_ct::container_type;
	  DEBUG_OBJECTS_RELEASE_WRITE_LOCK
	  set_alloc_checking_on(LIBCWD_TSD);
	}
#ifdef LIBCWD_THREAD_SAFE
	else
	  DEBUG_OBJECTS_RELEASE_READ_LOCK
#endif
      }

#ifdef LIBCWD_THREAD_SAFE
      // _private_::
      void debug_objects_ct::init_and_rdlock(void)
      {
	_private_::rwlock_tct<_private_::debug_objects_instance>::initialize();
	DEBUG_OBJECTS_ACQUIRE_READ_LOCK
	if (!WNS_debug_objects)				// MT: `WNS_debug_objects' is only false when this object is still Non_Shared.
	{
	  DEBUGDEBUG_CERR( "_debug_objects == NULL; initializing it" );
#ifdef DEBUGMALLOC
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

#ifdef DEBUGDEBUG
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
#if defined(LIBCWD_THREAD_SAFE) && defined(HAVE_PTHREAD_KILL_OTHER_THREADS_NP)
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

    void debug_tsd_st::start(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM)
    {
#ifdef DEBUGDEBUG
#ifdef LIBCWD_THREAD_SAFE
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
	  target_os->put('\n');
	  char const* channame = (channel_set.mask & finish_maskbit) ? "finish" : "continued";
#ifdef DEBUGUSEBFD
	  DoutFatal(dc::core, "Using `dc::" << channame << "' in " <<
	      debug::location_ct((char*)__builtin_return_address(0) + builtin_return_address_offset) <<
	      " without (first using) a matching `continued_cf'.");
#else
	  DoutFatal(dc::core, "Using `dc::" << channame <<
	      "' without (first using) a matching `continued_cf'.");
#endif
	}
#ifdef DEBUGDEBUG
	// MT: current != _private_::WST_dummy_laf, otherwise we didn't pass the previous if.
	LIBCWD_ASSERT( current != reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf) );
#endif
        current->mask = channel_set.mask;		// New bits might have been added
	if ((current->mask & finish_maskbit))
	  current->mask &= ~continued_expected_maskbit;
	current->err = errno;			// Always keep the last errno as set at the start of LibcwDout()
        return;
      }

      ++_off;
      DEBUGDEBUG_CERR( "Entering debug_ct::start(), _off became " << _off );

      // Is this an interrupting debug output (in the middle of a continued debug output)?
      if ((current->mask & continued_cf_maskbit) && unfinished_expected)
      {
#ifdef DEBUGDEBUG
	// MT: if current == _private_::WST_dummy_laf then
	//     (current->mask & continued_cf_maskbit) is false and this if is skipped.
	LIBCWD_ASSERT( current != reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf) );
#endif
	// Write out what is in the buffer till now.
	std::ostream* target_os = (channel_set.mask & cerr_cf) ? &std::cerr : debug_object.real_os;
	static_cast<buffer_ct*>(current_oss)->writeto(target_os LIBCWD_COMMA_TSD);
	// Append <unfinished> to it.
	target_os->write("<unfinished>\n", 13);		// Continued debug output should end on a space by itself,
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
#ifndef DEBUGDEBUGOUTPUT
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

      --_off;
      DEBUGDEBUG_CERR( "Leaving debug_ct::start(), _off became " << _off );
    }

    void debug_tsd_st::finish(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM)
    {
#ifdef DEBUGDEBUG
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
	  static_cast<buffer_ct*>(current_oss)->writeto(target_os LIBCWD_COMMA_TSD);
	  // Flush ostream.  Note that in the case of nested debug output this `os' can be an stringstream,
	  // in that case, no actual flushing is done until the debug output to the real ostream has
	  // finished.
	  *target_os << std::flush;
	}
	set_alloc_checking_on(LIBCWD_TSD);
        return;
      }

      ++_off;
      DEBUGDEBUG_CERR( "Entering debug_ct::finish(), _off became " << _off );

      // Write buffer to ostream.
      static_cast<buffer_ct*>(current_oss)->writeto(target_os LIBCWD_COMMA_TSD);
      channel_set.mask = current->mask;
      channel_set.label = current->label;

      // Handle control flags, if any:
      if (channel_set.mask == 0)
	target_os->put('\n');
      else
      {
	if ((channel_set.mask & error_cf))
	  *target_os << ": " << strerrno(current->err) << " (" << strerror(current->err) << ')';
	if ((channel_set.mask & coredump_maskbit))
	{
	  if (!__libcwd_tsd.recursive_fatal)
	  {
	    __libcwd_tsd.recursive_fatal = true;
	    *target_os << std::endl;	// First time, try to write a new-line and flush.
	  }
	  core_dump();
	}
	if ((channel_set.mask & fatal_maskbit))
	{
	  if (!__libcwd_tsd.recursive_fatal)
	  {
	    __libcwd_tsd.recursive_fatal = true;
	    *target_os << std::endl;
	  }
	  DEBUGDEBUG_CERR( "Deleting `current' " << (void*)current );
	  delete current;
	  DEBUGDEBUG_CERR( "Done deleting `current'" );
	  set_alloc_checking_on(LIBCWD_TSD);
	  exit(254);
	}
	if ((channel_set.mask & wait_cf))
	{
	  *target_os << "\n(type return)";
	  if (debug_object.interactive)
	  {
	    *target_os << std::flush;
	    while(std::cin.get() != '\n');
	  }
	}
	if (!(channel_set.mask & nonewline_cf))
	  *target_os << '\n';
	if ((channel_set.mask & flush_cf))
	  *target_os << std::flush;
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
        current = laf_stack.top();
	DEBUGDEBUG_CERR( "current = " << (void*)current );
	current_oss = &current->oss;
	DEBUGDEBUG_CERR( "current_oss = " << (void*)current_oss );
	if ((channel_set.mask & flush_cf))
	  current->mask |= flush_cf;	// Propagate flush to real ostream.
      }
      else
      {
        current = reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf);	// Used (MT: read-only!) in next debug_ct::start().
	DEBUGDEBUG_CERR( "current = " << (void*)current );
#ifdef DEBUGDEBUG
	current_oss = NULL;
	DEBUGDEBUG_CERR( "current_oss = " << (void*)current_oss );
#endif
      }

      start_expected = true;
      unfinished_expected = false;

      --_off;
      DEBUGDEBUG_CERR( "Leaving debug_ct::finish(), _off became " << _off );

      set_alloc_checking_on(LIBCWD_TSD);
    }

    void debug_tsd_st::fatal_finish(debug_ct& debug_object, channel_set_data_st& channel_set LIBCWD_COMMA_TSD_PARAM)
    {
      finish(debug_object, channel_set LIBCWD_COMMA_TSD);
      DoutFatal( dc::core, "Don't use `DoutFatal' together with `continued_cf', use `Dout' instead.  (This message can also occur when using DoutFatal correctly but from the constructor of a global object)." );
    }

    void debug_ct::NS_init(void)
    {
      ST_initialize_globals();		// Because all allocations for global objects are internal these days, we use
      					// the constructor of debug_ct to initiate the global initialization of libcwd
					// instead of relying on malloc().

      if (WNS_initialized)
        return;

#ifdef DEBUGDEBUG
      if (!WST_debug_object_init_magic)
	init_debug_object_init_magic();
      init_magic = WST_debug_object_init_magic;
      DEBUGDEBUG_CERR( "Set init_magic to " << init_magic );
      DEBUGDEBUG_CERR( "Setting WNS_initialized to true" );
#endif

      LIBCWD_TSD_DECLARATION
      set_alloc_checking_off(LIBCWD_TSD);		// debug_objects is internal.
      _private_::debug_objects.init();
      DEBUG_OBJECTS_ACQUIRE_WRITE_LOCK
      if (find(_private_::debug_objects.write_locked().begin(),
	       _private_::debug_objects.write_locked().end(), this)
	  == _private_::debug_objects.write_locked().end()) // Not added before?
	_private_::debug_objects.write_locked().push_back(this);
      DEBUG_OBJECTS_RELEASE_WRITE_LOCK
      new (_private_::WST_dummy_laf) laf_ct(0, channels::dc::debug.get_label(), 0);	// Leaks 24 bytes of memory
#ifndef LIBCWD_THREAD_SAFE
      tsd.init();
#endif
      set_alloc_checking_on(LIBCWD_TSD);

      // This set current_oss and must be called after tsd.init().
      set_ostream(&std::cerr);				// Write to std::cerr by default.
      interactive = true;				// and thus we're interactive.

      WNS_initialized = true;

      // Unlimit core size.
#ifdef RLIMIT_CORE
      struct rlimit corelim;
      if (getrlimit(RLIMIT_CORE, &corelim))
	DoutFatal(dc::fatal|error_cf, "getrlimit(RLIMIT_CORE, &corelim)");
      corelim.rlim_cur = corelim.rlim_max;
      if (corelim.rlim_max != RLIM_INFINITY)
      {
	OnOffState state;
	force_on(state);
	// The cast is necessary on platforms where corelim.rlim_max is long long
	// and libstdc++ was not compiled with support for long long.
	Dout(dc::warning, "core size is limited (hard limit: " << (unsigned long)(corelim.rlim_max / 1024) << " kb).  Core dumps might be truncated!");
	restore(state);
      }
      if (setrlimit(RLIMIT_CORE, &corelim))
	  DoutFatal(dc::fatal|error_cf, "unlimit core size failed");
#else
      OnOffState state;
      force_on(state);
      Dout(dc::warning, "Please unlimit core size manually");
      restore(state);
#endif
    }

    void debug_tsd_st::init(void)
    {
#ifdef DEBUGDEBUGMALLOC
      LIBCWD_TSD_DECLARATION
      LIBCWD_ASSERT( __libcwd_tsd.internal );
#endif
      _off = 0;						// Turn off all debugging until initialization is completed.
      DEBUGDEBUG_CERR( "In debug_tsd_st::NS_init(void), _off set to 0" );

      start_expected = true;				// Of course, we start with expecting the beginning of a debug output.
      // Fatal channels need to be marked fatal, otherwise we get into an endless loop
      // when they are used before they are created.
      channels::dc::core.NS_initialize("COREDUMP", coredump_maskbit);
      channels::dc::fatal.NS_initialize("FATAL", fatal_maskbit);
      // Initialize other debug channels that might be used before we reach main().
      channels::dc::debug.NS_initialize("DEBUG");
      channels::dc::malloc.NS_initialize("MALLOC");
      channels::dc::continued.NS_initialize(continued_maskbit);
      channels::dc::finish.NS_initialize(finish_maskbit);
#ifdef DEBUGUSEBFD
      channels::dc::bfd.NS_initialize("BFD");
#endif
      // What the heck, initialize all other debug channels too
      channels::dc::warning.NS_initialize("WARNING");
      channels::dc::notice.NS_initialize("NOTICE");
      channels::dc::system.NS_initialize("SYSTEM");
      // `current' needs to be non-zero (saving us a check in start()) and
      // current.mask needs to be 0 to avoid a crash in start():
      current = reinterpret_cast<laf_ct*>(_private_::WST_dummy_laf);
      DEBUGDEBUG_CERR( "current = " << (void*)current );
#ifdef DEBUGDEBUG
      current_oss = NULL;
      DEBUGDEBUG_CERR( "current_oss = " << (void*)current_oss );
#endif
      laf_stack.init();
      continued_stack.init();
      margin.NS_internal_init("", 0);
      marker.NS_internal_init(": ", 2);

#ifdef DEBUGDEBUGOUTPUT
      _off = -1;		// Print as much debug output as possible right away.
      first_time = true;	// Needed to ignore the first time we call on().
#else
      _off = 0;			// Don't print debug output till the REAL initialization of the debug system has been performed
      				// (ie, the _application_ start (don't confuse that with the constructor - which does nothing)).
#endif
      DEBUGDEBUG_CERR( "After debug_tsd_st::NS_init(void), _off set to " << _off );
      tsd_initialized = true;
    }

    debug_tsd_st::~debug_tsd_st()
    {
      // Sanity checks:
      if (continued_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_tsd_st with a non-empty continued_stack (missing dc::finish?)" );
      if (laf_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_tsd_st with a non-empty laf_stack" );

      ++_off;		// Turn all debug output premanently off, otherwise we might re-initialize
                        // this object again when we try to write debug output to it!
      DEBUGDEBUG_CERR( "debug_tsd_st destructed: _off became " << _off );
      LIBCWD_TSD_DECLARATION
      set_alloc_checking_off(LIBCWD_TSD);
      marker.ST_internal_deinit();
      margin.ST_internal_deinit();
      set_alloc_checking_on(LIBCWD_TSD);
      tsd_initialized = false;
    }

    /**
     * \brief Destructor
     */
    debug_ct::~debug_ct()
    {
      WNS_initialized = false;
#ifdef DEBUGDEBUG
      init_magic = 0;
#endif
      LIBCWD_TSD_DECLARATION
      set_alloc_checking_off(LIBCWD_TSD);
      DEBUG_OBJECTS_ACQUIRE_WRITE_LOCK
      {
        _private_::debug_objects_ct::container_type& objects(_private_::debug_objects.write_locked());
        objects.erase(find(objects.begin(), objects.end(), this));
        if (objects.empty())
	  _private_::debug_objects.ST_uninit();
      }
      DEBUG_OBJECTS_RELEASE_WRITE_LOCK
      set_alloc_checking_on(LIBCWD_TSD);
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
      _private_::debug_channels.init();
      DEBUG_CHANNELS_ACQUIRE_READ_LOCK
      for(_private_::debug_channels_ct::container_type::const_iterator i(_private_::debug_channels.read_locked().begin());
	  i != _private_::debug_channels.read_locked().end(); ++i)
      {
        if (!strncasecmp(label, (*i)->get_label(), strlen(label)))
          tmp = (*i);
      }
      DEBUG_CHANNELS_RELEASE_READ_LOCK
      return tmp;
    }

    /**
     * \fn void list_channels_on(debug_ct const& debug_object)
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
      if (LIBCWD_DO_TSD_MEMBER(debug_object, _off) < 0)
      {
        _private_::debug_channels.init();
	DEBUG_CHANNELS_ACQUIRE_READ_LOCK
	for(_private_::debug_channels_ct::container_type::const_iterator i(_private_::debug_channels.read_locked().begin());
	    i != _private_::debug_channels.read_locked().end(); ++i)
	{
	  LibcwDoutScopeBegin(DEBUGCHANNELS, debug_object, dc::always|noprefix_cf);
	  LibcwDoutStream.write((*i)->get_label(), WST_max_len);
	  if ((*i)->is_on())
	    LibcwDoutStream.write(": Enabled", 9);
	  else
	    LibcwDoutStream.write(": Disabled", 10);
	  LibcwDoutScopeEnd;
	}
	DEBUG_CHANNELS_RELEASE_READ_LOCK
      }
    }

    void channel_ct::NS_initialize(char const* label)		// Single Threaded function (or just Non-Shared if you take dlopen into account).
    {
      // This is pretty much identical to fatal_channel_ct::fatal_channel_ct().

      if (WNS_initialized)
        return;		// Already initialized.

      DEBUGDEBUG_CERR( "Entering `channel_ct::NS_initialize(\"" << label << "\")'" );

      // Of course, dc::debug is off - so this won't do anything unless DEBUGDEBUG is #defined.
      Dout( dc::debug, "Initializing channel_ct(\"" << label << "\")" );

      size_t label_len = strlen(label);

      if (label_len > max_label_len_c)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << label << "\") > " << max_label_len_c );

#ifdef LIBCWD_THREAD_SAFE
      _private_::mutex_tct<_private_::write_max_len_instance>::initialize();
      _private_::mutex_tct<_private_::write_max_len_instance>::lock();
#endif
      // MT: This is not strict thread safe because it is possible that after threads are already
      //     running, a new shared library is dlopen-ed with a new global channel_ct with a larger
      //     label.  However, it makes no sense to lock the *reading* of max_len for the other threads
      //     because this assignment is an atomic operation.  The lock above is only needed to
      //     prefend two such simultaneously loaded libraries from causing WST_max_len to end up
      //     not maximal.
      if (label_len > WST_max_len)
	WST_max_len = label_len;

      LIBCWD_TSD_DECLARATION
#ifdef LIBCWD_THREAD_SAFE
      // MT: Take advantage of the `write_max_len_instance' lock to prefend simultaneous access
      //     to `next_index' in the case of simultaneously dlopen-loaded libraries.
      static int next_index;
      WNS_index = ++next_index;		// Don't use index 0, it is used to make sure that uninitialized channels appear to be off.
       
      _private_::mutex_tct<_private_::write_max_len_instance>::unlock();

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
      set_alloc_checking_off(LIBCWD_TSD);	// debug_channels is internal.
      _private_::debug_channels.init();
      DEBUG_CHANNELS_ACQUIRE_WRITE_LOCK
      {
	_private_::debug_channels_ct::container_type& channels(_private_::debug_channels.write_locked());
	_private_::debug_channels_ct::container_type::iterator i(channels.begin());
	for(; i != channels.end(); ++i)
	  if (strncmp((*i)->get_label(), WNS_label, max_label_len_c) > 0)
	    break;
        channels.insert(i, this);
      }
      DEBUG_CHANNELS_RELEASE_WRITE_LOCK
      set_alloc_checking_on(LIBCWD_TSD);

      // Turn debug channel "WARNING" on by default.
      if (strncmp(WNS_label, "WARNING", label_len) == 0)
#ifdef LIBCWD_THREAD_SAFE
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

      // Of course, dc::debug is off - so this won't do anything unless DEBUGDEBUG is #defined.
      Dout( dc::debug, "Initializing fatal_channel_ct(\"" << label << "\")" );

      size_t label_len = strlen(label);

      if (label_len > max_label_len_c)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << label << "\") > " << max_label_len_c );

#ifdef LIBCWD_THREAD_SAFE
      _private_::mutex_tct<_private_::write_max_len_instance>::initialize();
      _private_::mutex_tct<_private_::write_max_len_instance>::lock();
#endif
      // MT: See comment in channel_ct::NS_initialize above.
      if (label_len > WST_max_len)
	WST_max_len = label_len;
#ifdef LIBCWD_THREAD_SAFE
      _private_::mutex_tct<_private_::write_max_len_instance>::unlock();
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
#ifdef LIBCWD_THREAD_SAFE
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
#ifdef LIBCWD_THREAD_SAFE
      LIBCWD_TSD_DECLARATION
      if (__libcwd_tsd.off_cnt_array[WNS_index] == -1)
#else
      if (off_cnt == -1)
#endif
	DoutFatal( dc::core, "Calling channel_ct::on() more often then channel_ct::off()" );
#ifdef LIBCWD_THREAD_SAFE
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
#ifdef DEBUGDEBUG
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
#ifdef DEBUGDEBUG
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
#ifdef DEBUGUSEBFD
      DoutFatal(dc::fatal, location_ct((char*)__builtin_return_address(0) + libcw::debug::builtin_return_address_offset) <<
          " : Don't use Dout together with dc::core or dc::fatal!  Use DoutFatal instead.");
#else
      DoutFatal(dc::core,
          "Don't use Dout together with dc::core or dc::fatal!  Use DoutFatal instead.");
#endif
    }

    channel_set_st& channel_set_bootstrap_st::operator&(channel_ct const&)
    {
#ifdef DEBUGUSEBFD
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
#ifdef DEBUGDEBUGMALLOC
	LIBCWD_TSD_DECLARATION
	if (__libcwd_tsd.recursive)
	{
	  set_alloc_checking_off(LIBCWD_TSD);
	  FATALDEBUGDEBUG_CERR(file << ':' << line << ": " << function << ": Assertion `" << expr << "' failed.\n");
	  set_alloc_checking_on(LIBCWD_TSD);
	  core_dump();
	}
#endif
	DoutFatal(dc::core, file << ':' << line << ": " << function << ": Assertion `" << expr << "' failed.\n");
      }

    } // namespace _private_

    void debug_ct::force_on(debug_ct::OnOffState& state)
    {
      NS_init();
      LIBCWD_TSD_DECLARATION
      state._off = LIBCWD_TSD_MEMBER(_off);
#ifdef DEBUGDEBUGOUTPUT
      state.first_time = LIBCWD_TSD_MEMBER(first_time);
#endif
      LIBCWD_TSD_MEMBER(_off) = -1;					// Turn object on.
    }

    void debug_ct::restore(debug_ct::OnOffState const& state)
    {
      LIBCWD_TSD_DECLARATION
#ifdef DEBUGDEBUGOUTPUT
      if (state.first_time != LIBCWD_TSD_MEMBER(first_time))		// state.first_time && !first_time.
	core_dump();							// on() was called without first a call to off().
#endif
      if (LIBCWD_TSD_MEMBER(_off) != -1)
	core_dump();							// off() and on() where called and not in equal pairs.
      LIBCWD_TSD_MEMBER(_off) = state._off;				// Restore.
    }

    void channel_ct::force_on(channel_ct::OnOffState& state, char const* label)
    {
      NS_initialize(label);
#ifdef LIBCWD_THREAD_SAFE
      LIBCWD_TSD_DECLARATION
      int& off_cnt(__libcwd_tsd.off_cnt_array[WNS_index]);
#endif
      state.off_cnt = off_cnt;
      off_cnt = -1;					// Turn channel on.
    }

    void channel_ct::restore(channel_ct::OnOffState const& state)
    {
#ifdef LIBCWD_THREAD_SAFE
      LIBCWD_TSD_DECLARATION
      int& off_cnt(__libcwd_tsd.off_cnt_array[WNS_index]);
#endif
      if (off_cnt != -1)
	core_dump();					// off() and on() where called and not in equal pairs.
      off_cnt = state.off_cnt;				// Restore.
    }

  }	// namespace debug
}	// namespace libcw

// This can be used in configure to see if libcwd exists.
extern "C" char const* const __libcwd_version = VERSION;
