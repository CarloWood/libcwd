// $Header$
//
// Copyright (C) 2000, by
// 
// Carlo Wood, Run on IRC <carlo@alinoe.com>
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file may be distributed under the terms of the Q Public License
// version 1.0 as appearing in the file LICENSE.QPL included in the
// packaging of this file.
//

#include <libcw/sys.h>
#ifdef DEBUG
#include <errno.h>
#include <signal.h>		// Needed for raise()
#include <sys/time.h>     	// Needed for setrlimit()
#include <sys/resource.h>	// Needed for setrlimit()
#include <iostream>		// Needed for cerr
#include <algorithm>
#include <new>
#include <libcw/h.h>
#include <libcw/debug.h>
#include <libcw/strerrno.h>
#include <libcw/no_alloc_checking_ostrstream.h>
#endif

RCSTAG_CC("$Id$")

#ifdef DEBUG
#ifndef DEBUGNONAMESPACE
namespace libcw {
  namespace debug {
#endif

    // Configuration signature
    unsigned long config_signature_lib = config_signature_header;

    // Put this here to decrease the code size of `check_configuration'
    void conf_check_failed(void)
    {
      DoutFatal(dc::fatal, "check_configuration: This version of libcwd was compiled with a different configuration than is currently used in libcw/debug_config.h!");
    }

    debug_ct libcw_do;				// The Debug Object that is used by default by Dout(), the only debug object used
    						// by libcw itself.

    namespace {
      unsigned short int max_len = 8;		// The length of the longest label. Is adjusted automatically
    						// if a custom channel has a longer label.
    };

    namespace channels {
      namespace dc {
	channel_ct const debug("DEBUG");
	channel_ct const notice("NOTICE");
	channel_ct const warning("WARNING");
	channel_ct const system("SYSTEM");
	channel_ct const malloc("MALLOC");

	continued_channel_ct const continued(continued_maskbit);
	continued_channel_ct const finish(finish_maskbit);

	fatal_channel_ct const fatal("FATAL", fatal_maskbit);
	fatal_channel_ct const core("COREDUMP", coredump_maskbit);
      };
    };

    debug_channels_ct* debug_channels = NULL;	// List with all channel_ct objects.
    debug_objects_ct* debug_objects = NULL;	// List with all debug devices.

#ifdef DEBUGDEBUG
    static long debug_object_init_magic = 0;

    static void init_debug_object_init_magic(void)
    {
      struct timeval rn;
      gettimeofday(&rn, NULL);
      debug_object_init_magic = rn.tv_usec;
      if (!debug_object_init_magic)
        debug_object_init_magic = 1;
      cerr << "DEBUGDEBUG: Set debug_object_init_magic to " << debug_object_init_magic << endl; 
    }
#endif

    class laf_ct {
    public:
      no_alloc_checking_ostrstream oss;
	// The temporary output buffer.

      int prefix_end;
	// Number of characters in oss that make up the prefix.

      size_t flushed;
	// Number of character in the buffer that are already flushed.

      control_flag_t mask;
	// The previous control bits.

      char const* label;
	// The previous label.

      ostream* saved_os;
	// The previous original ostream.

      int err;
	// The current errno.

    public:
      laf_ct(control_flag_t m, char const* l, ostream* os, int e) : flushed(0), mask(m), label(l), saved_os(os), err(e) {}
    };

    static inline void write_whitespace_to(ostream& os, unsigned int size)
    {
      for (unsigned int i = size; i > 0; --i)
        os.put(' ');
    }

    void debug_ct::start(void)
    {
      // It's possible we get here before this debug object is initialized: The order of calling global is undefined :(.
      // Hopefully `initialized' is set to 0 anyway for all uninitialized global objects(?).
      if (!initialized)			
        init();
#ifdef DEBUGDEBUG
      else if (init_magic != debug_object_init_magic || !debug_object_init_magic)
      {
        cerr << "DEBUGDEBUG: Ack, fatal error in libcw: `initialized' true while `init_magic' not set! "
	        "Please mail the author of libcw." << endl;
	raise(3);	// Core dump
      }
#endif

      // Skip `start()' for a `continued' debug output.
      // Generating the "prefix: <continued>" is already taken care of while generating the "<unfinished>" (see next `if' block).
      if ((channel_set.mask & (continued_maskbit|finish_maskbit)))
      {
        current->mask = channel_set.mask;	// New bits might have been added
	current->err = errno;			// Always keep the last errno as set at the start of LibcwDout()
        return;
      }

      ++_off;
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Entering debug_ct::start(), _off became " << _off << "\n";
#endif

      // Is this an interrupting debug output (in the middle of a continued debug output)?
      if ((current->mask & continued_cf_maskbit) && unfinished_expected)
      {
        // Write out what is in the buffer till now.
	os->write(current->oss.str() + current->flushed, current->oss.pcount() - current->flushed);
	current->oss.freeze(0);
	// Append <unfinished> to it.
	*os << "<unfinished>\n";		// Continued debug output should end on a space by itself.
	// Truncate the buffer to its prefix and append "<continued>" to it already.
	current->oss.rdbuf()->seekoff(current->prefix_end, ios::beg);
	current->oss << "<continued> ";		// Therefore we repeat the space here.
	current->flushed = 0;
      }

      // By putting this here instead of in finish(), all nested debug output will go to cerr too.
      if ((channel_set.mask & cerr_cf))
      {
        saved_os = os;
	os = &cerr;
      }

      // Is this a nested debug output (the first of a series in the middle of another debug output)?
      if (!start_expected)
      {
	// Put current ostrstream on the stack.
	laf_stack.push(current);

	// Indent nested debug output with 4 extra spaces.
	indent += 4;
      }

      // Without a new nested Dout() call, we expect to see a finish() call: The finish belonging to *this* Dout() call.
      start_expected = false;

      // If this is a `continued' debug output, then we want to print "<unfinished>" if next we see a start().
      unfinished_expected = true;

      // Create a new laf.
#ifdef DEBUGMALLOC
      set_alloc_checking_off();
#endif
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: creating new laf_ct\n";
#endif
      current = new laf_ct(channel_set.mask, channel_set.label, saved_os, errno);
      current_oss = &current->oss;
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: laf_ct created\n";
#endif
#ifdef DEBUGMALLOC
      set_alloc_checking_on();
#endif

      // Print prefix if requested.
      // Handle most common case first: no special flags set
      if (!(channel_set.mask & (noprefix_cf|nolabel_cf|blank_margin_cf|blank_label_cf|blank_marker_cf)))
      {
	current->oss.write(margin.str, margin.len);
	current->oss.write(channel_set.label, max_len);
	current->oss.write(marker.str, marker.len);
	write_whitespace_to(current->oss, indent);
      }
      else if (!(channel_set.mask & noprefix_cf))
      {
	if ((channel_set.mask & blank_margin_cf))
	  write_whitespace_to(current->oss, margin.len);
	else
	  current->oss.write(margin.str, margin.len);
#ifndef DEBUGDEBUG
	if (!(channel_set.mask & nolabel_cf))
#endif
	{
	  if ((channel_set.mask & blank_label_cf))
	    write_whitespace_to(current->oss, max_len);
	  else
	    current->oss.write(channel_set.label, max_len);
	  if ((channel_set.mask & blank_marker_cf))
	    write_whitespace_to(current->oss, marker.len);
	  else
	    current->oss.write(marker.str, marker.len);
	  write_whitespace_to(current->oss, indent);
	}
      }

      // If this is continued debug output, then it makes sense to remember the prefix length,
      // just in case we need indeed to output <continued> data (it wouldn't hurt to do this
      // always, but it is rarely a continued debug output and the mask test is faster than
      // calling pcount().
      if ((channel_set.mask & continued_cf_maskbit))
        current->prefix_end = current->oss.pcount();

      --_off;
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Leaving debug_ct::start(), _off became " << _off << '\n';
#endif
    }

    void debug_ct::finish(void)
    {
      // Skip `finish()' for a `continued' debug output.
      if ((current->mask & continued_cf_maskbit) && !(current->mask & finish_maskbit))
      {
        if ((current->mask & continued_maskbit))
	  unfinished_expected = true;

        // If the `flush_cf' control flag is set, flush the ostream at every `finish()' though.
	if ((current->mask & flush_cf))
	{
	  // Write buffer to ostream.
	  os->write(current->oss.str() + current->flushed, current->oss.pcount() - current->flushed);
	  current->oss.freeze(0);
	  current->flushed = current->oss.pcount();	// Remember how much was already written.
	  // Flush ostream. Note that in the case of nested debug output this `os' can be an ostrstream,
	  // in that case, no actual flushing is done until the debug output to the real ostream has
	  // finished.
	  *os << flush;
	}
        return;
      }

      ++_off;
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Entering debug_ct::finish(), _off became " << _off << '\n';
#endif

      // Write buffer to ostream.
      os->write(current->oss.str() + current->flushed, current->oss.pcount() - current->flushed);
      current->oss.freeze(0);
      channel_set.mask = current->mask;
      channel_set.label = current->label;
      saved_os = current->saved_os;
#ifdef DEBUGMALLOC
      set_alloc_checking_off();
#endif
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Deleting `current'\n";
#endif
      delete current;
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Done deleting `current'\n";
#endif
#ifdef DEBUGMALLOC
      set_alloc_checking_on();
#endif

      // Handle control flags, if any:
      if (channel_set.mask == 0)
	*os << '\n';
      else
      {
	if ((channel_set.mask & error_cf))
	  *os << ": " << strerrno(current->err) << " (" << strerror(current->err) << ')';
	if ((channel_set.mask & coredump_maskbit))
	{
	  static bool debug_internal2 = false;
	  if (!debug_internal2)
	  {
	    debug_internal2 = true;
	    *os << endl;
	  }
	  raise(3);		// Core dump
	}
	if ((channel_set.mask & fatal_maskbit))
	{
	  static bool debug_internal2 = false;
	  if (!debug_internal2)
	  {
	    debug_internal2 = true;
	    *os << endl;
	  }
	  exit(-2);
	}
	if ((channel_set.mask & wait_cf))
	{
	  *os << "\n(type return)";
	  if (interactive)
	  {
	    *os << flush;
	    while(cin.get() != '\n');
	  }
	}
	if (!(channel_set.mask & nonewline_cf))
	  *os << '\n';
	if ((channel_set.mask & flush_cf))
	  *os << flush;
	if ((channel_set.mask & cerr_cf))
	  os = saved_os;
      }

      if (start_expected)
      {
        // Ok, we're done with the last buffer.
	indent -= 4;
        laf_stack.pop();
      }

      // Restore previous buffer as being the current one.
      if (laf_stack.size())
      {
        current = laf_stack.top();
	current_oss = &current->oss;
	if ((channel_set.mask & flush_cf))
	  current->mask |= flush_cf;	// Propagate flush to real ostream.
      }

      start_expected = true;
      unfinished_expected = false;

      --_off;
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Leaving debug_ct::finish(), _off became " << _off << '\n';
#endif
    }

    void debug_ct::fatal_finish(void)
    {
      finish();
      DoutFatal( dc::core, "Don't use `DoutFatal' together with `continue_cf', use `Dout' instead" );
    }

    void debug_ct::init(void)
    {
      if (initialized)
        return;

      _off = 1;						// Turn off all debugging until initialization is completed.
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: In debug_ct::init(void), _off set to 1" << endl;
#endif

      // The first time we get here, initialize the list with debug devices:
      if (!debug_objects)
      {
#ifdef DEBUGDEBUG
        cerr << "DEBUGDEBUG: debug_objects == NULL; initializing it" << endl;
#endif
#ifdef DEBUGMALLOC
	// It is possible that malloc is not initialized yet.
	init_debugmalloc();
	set_alloc_checking_off();
#endif
        debug_objects = new debug_objects_ct;
#ifdef DEBUGMALLOC
	set_alloc_checking_on();
#endif
      }

      if (::std::find(debug_objects->begin(), debug_objects->end(), this) == debug_objects->end()) // Not added before?
	debug_objects->push_back(this);

      // Initialize this debug object:
      set_ostream(&cerr);				// Write to cerr by default.
      interactive = true;				// and thus we're interactive.
      start_expected = true;				// Of course, we start with expecting the beginning of a debug output.
      continued_channel_set.debug_object = this;	// The owner of this continued_channel_set.
      // `current' needs to be non-zero (saving us a check in start()) and
      // current.mask needs to be 0 to avoid a crash in start():
      static char dummy_laf[sizeof(laf_ct)] __attribute__((__aligned__));
#ifdef DEBUGMALLOC
      set_alloc_checking_off();
#endif
      current = new (dummy_laf) laf_ct(0, channels::dc::debug.label, NULL, 0);	// Leaks 24 bytes of memory
      current_oss = &current->oss;
#ifdef DEBUGMALLOC
      set_alloc_checking_on();
#endif
      laf_stack.init();
      continued_stack.init();
      margin.init("", 0, true);				// Note: These first time strings need to be static/const.
      marker.init(": ", 2, true);

#if 0 // We assumed it was zeroed right? :)
      indent = 0;					// Don't do indenting.
      off_count = 0;					// No unfinished channels yet, let alone switched off ones.
      unfinished_expected = false;			// We didn't print anything yet, so we certainly don't expect to
      							// interrupt a previous debug output.
#endif

#ifdef DEBUGDEBUG
      _off = 0;			// Print as much debug output as possible right away.
      first_time = true;	// Needed to ignore the first time we call on().
#else
      _off = 1;			// Don't print debug output till the REAL initialization of the debug system has been performed
      				// (ie, the _application_ start (don't confuse that with the constructor - which does nothing)).
#endif
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: After debug_ct::init(void), _off set to " << _off << "\n";
#endif

#ifdef DEBUGDEBUG
      if (!debug_object_init_magic)
	init_debug_object_init_magic();
      init_magic = debug_object_init_magic;
      cerr << "DEBUGDEBUG: Set init_magic to " << init_magic << endl;
      cerr << "DEBUGDEBUG: Setting initialized to true" << endl;
#endif
      initialized = true;

      // We want to do the following only once per application.
      static bool very_first_time = true;
      if (very_first_time)
      {
        very_first_time = false;
	// Unlimit core size.
#ifdef RLIMIT_CORE
	struct rlimit corelim;
	corelim.rlim_cur = corelim.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &corelim))
	  DoutFatal( dc::core|error_cf, "unlimit core size failed" );
#else
	Dout( dc::warning, "Please unlimit core size manually" );
#endif
      }
    }

    debug_ct::~debug_ct()
    {
      // Sanity checks:
      if (continued_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_ct with a non-empty continued_stack" );
      if (laf_stack.size())
        DoutFatal( dc::core|cerr_cf, "Destructing debug_ct with a non-empty laf_stack" );

      ++_off;		// Turn all debug output premanently off, otherwise we might re-initialize
                        // this object again when we try to write debug output to it!
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: debug_ct destructed: _off became " << _off << '\n';
#endif
      initialized = false;
#ifdef DEBUGDEBUG
      init_magic = 0;
#endif
      marker.init("", 0);	// Free allocated memory
      margin.init("", 0);
      debug_objects->erase(::std::find(debug_objects->begin(), debug_objects->end(), this));
      if (debug_objects->empty())
      {
#ifdef DEBUGMALLOC
	set_alloc_checking_off();
#endif
        delete debug_objects;
	debug_objects = NULL;
#ifdef DEBUGMALLOC
	set_alloc_checking_on();
#endif
      }
    }

    void debug_ct::set_margin(string const& s) {
      ++_off;
      margin.init(s.data(), s.size());
      --_off;
    }

    void debug_ct::set_marker(string const& s) {
      ++_off;
      marker.init(s.data(), s.size());
      --_off;
    }

    string debug_ct::get_margin(void) const {
      return string(margin.str, margin.len);
    }

    string debug_ct::get_marker(void) const
    {
      return string(marker.str, marker.len);
    }

#if 0
    void debug_ct::destroy_all_buffered_output(void)
    {
      if (fd)
      {
	if (!fd->is_linked())
	  DoutFatal( dc::core, "Old debug channel isn't linked (in kernel list) !?" );
	if (!fd->must_be_removed()) // Prevend a little debug flood
	  fd->del();
	set_ostream(&cerr);
      }
    }
#endif

    channel_ct const* find(char const* label)
    {
      channel_ct const* tmp = NULL;
      for(debug_channels_ct::iterator i(debug_channels->begin()); i != debug_channels->end(); ++i)
      {
        if (!strncasecmp(label, (*i)->get_label(), strlen(label)))
          tmp = (*i);
      }
      return tmp;
    }

    void list_channels_on(debug_ct const& debug_object)
    {
      if (!debug_object._off)
	for(debug_channels_ct::iterator i(debug_channels->begin()); i != debug_channels->end(); ++i)
	{
	  char const* txt = (*i)->is_on() ? ": Enabled" : ": Disabled";
	  debug_object.get_os().write((*i)->get_label(), max_len);
	  debug_object.get_os() << txt << '\n';
	}
    }

    channel_ct::channel_ct(char const* lbl) : off_cnt(0)
    {
       // This is pretty much identical to fatal_channel_ct::fatal_channel_ct().

#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Entering `channel_ct::channel_ct(\"" << lbl << "\")'" << endl;
#endif

      // Of course, dc::debug is off - so this won't do anything unless DEBUGDEBUG is #defined.
      Dout( dc::debug, "Initializing channel_ct(\"" << lbl << "\")" );

      size_t lbl_len = strlen(lbl);

      if (lbl_len > max_label_len)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << lbl << "\") > " << max_label_len );

      if (lbl_len > max_len)
	max_len = lbl_len;

      strncpy(label, lbl, lbl_len);
      memset(label + lbl_len, ' ', max_label_len - lbl_len);

      if (!debug_channels)
      {
#ifdef DEBUGMALLOC
        set_alloc_checking_off();
#endif
        debug_channels = new debug_channels_ct;		// Leaks 12 bytes of memory
#ifdef DEBUGMALLOC
        set_alloc_checking_on();
#endif
      }

      debug_channels->push_back(this);

#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Leaving `channel_ct::channel_ct(\"" << lbl << "\")" << endl;
#endif
    }

    fatal_channel_ct::fatal_channel_ct(char const* lbl, control_flag_t cb) : maskbit(cb)
    {
       // This is pretty much identical to channel_ct::channel_ct().

#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Entering `fatal_channel_ct::fatal_channel_ct(\"" << lbl << "\")'" << endl;
#endif

      // Of course, dc::debug is off - so this won't do anything unless DEBUGDEBUG is #defined.
      Dout( dc::debug, "Initializing fatal_channel_ct(\"" << lbl << "\")" );

      size_t lbl_len = strlen(lbl);

      if (lbl_len > max_label_len)	// Only happens for customized channels
	DoutFatal( dc::core, "strlen(\"" << lbl << "\") > " << max_label_len );

      if (lbl_len > max_len)
	max_len = lbl_len;

      strncpy(label, lbl, lbl_len);
      memset(label + lbl_len, ' ', max_label_len - lbl_len);

#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: Leaving `fatal_channel_ct::fatal_channel_ct(\"" << lbl << "\")" << endl;
#endif
    }

    void channel_ct::off(void) const
    {
      ++off_cnt;
    }

    void channel_ct::on(void) const
    {
      if (off_cnt == -1)
	DoutFatal( dc::core, "Calling channel_ct::on() more often then channel_ct::off()" );
      --off_cnt;
    }

    //----------------------------------------------------------------------------------------
    // Handle continued channel flags and update `off_count' and `continued_stack'.
    //

    continued_channel_set_st& channel_set_st::operator|(continued_cf_st cf)
    {
#ifdef DEBUGDEBUG
      cerr << "DEBUGDEBUG: continued_cf detected" << endl;
#endif
      mask |= cf.maskbit;
      if (!on)
      {
#ifdef DEBUGDEBUG
        if (!debug_object)
	  cerr << "DEBUGDEBUG: The order in which the global contructors of the debug channels are called is wrong.  Try putting debug.cc on the other end of SRC in libcw/src/debugging/Makefile\n" << endl;
#endif
	++(debug_object->off_count);
#ifdef DEBUGDEBUG
        cerr << "DEBUGDEBUG: Channel is switched off. Increased off_count to " << debug_object->off_count << endl;
#endif
      }
      else
      {
	if (!debug_object->initialized)
	  debug_object->init();
        debug_object->continued_stack.push(debug_object->off_count);
#ifdef DEBUGDEBUG
        cerr << "DEBUGDEBUG: Channel is switched on. Pushed off_count (" << debug_object->off_count << ") to stack (size now " <<
	    debug_object->continued_stack.size() << ") and set off_count to 0" << endl;
#endif
        debug_object->off_count = 0;
      }
      return *(reinterpret_cast<continued_channel_set_st*>(this));
    }

    continued_channel_set_st& debug_ct::operator|(continued_channel_ct const& cdc)
    {
#ifdef DEBUGDEBUG
      if ((cdc.maskbit & continued_maskbit))
	cerr << "DEBUGDEBUG: dc::continued detected" << endl;
      else
        cerr << "DEBUGDEBUG: dc::finish detected" << endl;
#endif
      if ((continued_channel_set.on = !off_count))
      {
#ifdef DEBUGDEBUG
        cerr << "DEBUGDEBUG: Channel is switched on (off_count is 0)" << endl;
#endif
	current->mask |= cdc.maskbit;                                 // We continue with the current channel
	continued_channel_set.mask = current->mask;
	continued_channel_set.label = current->label;
	if (cdc.maskbit == finish_maskbit)
	{
	  off_count = continued_stack.top();
	  continued_stack.pop();
#ifdef DEBUGDEBUG
	  cerr << "DEBUGDEBUG: Restoring off_count to " << off_count << ". Stack size now " << continued_stack.size() << endl;
#endif
	}
      }
      else
      {
#ifdef DEBUGDEBUG
        cerr << "DEBUGDEBUG: Channel is switched off (off_count is " << off_count << ')';
#endif
	if (cdc.maskbit == finish_maskbit)
	{
#ifdef DEBUGDEBUG
	  cerr << ", decrementing off_count with 1" << endl;
#endif
	  --off_count;
        }
#ifdef DEBUGDEBUG
        else
	  cerr << endl;
#endif
      }
      return continued_channel_set;
    }

#ifdef DEBUGMALLOC
    void* no_alloc_checking_alloc(size_t size)
    {
      set_alloc_checking_off();
      channels::dc::malloc.off();
      void* ptr = (void*)new char[size];
      channels::dc::malloc.on();
      set_alloc_checking_on();
      return ptr;
    }

    void no_alloc_checking_free(void* ptr)
    {
      set_alloc_checking_off();
      channels::dc::malloc.off();
      delete [] (char*)ptr;
      channels::dc::malloc.on();
      set_alloc_checking_on();
    }
#endif

    void buf_st::init(char const* s, size_t l, bool first_time)
    {
      if (first_time)
      {
        str = orig_str = s;
	len = l;
        str_is_allocated = false;
	return;
      }
#ifdef DEBUGMALLOC
      set_alloc_checking_off();
#endif
      if (str_is_allocated)
	delete [] const_cast<char*>(str);
      if ((len = l) > 0)
      {
        char* p = new char[l];
	memcpy(p, s, l);
	str = p;
	str_is_allocated = true;
      }
      else
      {
        str = orig_str;
	str_is_allocated = false;
	len = strlen(str);
      }
#ifdef DEBUGMALLOC
      set_alloc_checking_on();
#endif
    }

#ifdef DEBUGMALLOC
    no_alloc_checking_ostrstream::no_alloc_checking_ostrstream(void)
    {
      set_alloc_checking_off();
      my_sb = new strstreambuf(no_alloc_checking_alloc, no_alloc_checking_free);
      set_alloc_checking_on();
      ios::init(my_sb);					// Add the real buffer
    }

    no_alloc_checking_ostrstream::~no_alloc_checking_ostrstream()
    {
      set_alloc_checking_off();
      delete my_sb;
      set_alloc_checking_on();
    }
#endif

#ifndef DEBUGNONAMESPACE
  };	// namespace debug
};	// namespace libcw
#endif

#endif // DEBUG
