include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H2>Tutorial 8: Debugging Threaded Applications</H2>

<P>Libcwd should be completely thread-safe, with the following restrictions:</P>
<UL>
<LI>All debug objects and debug channels <EM>must</EM> be global (as they should
be in non-threaded applications for that matter).</LI>
<LI>You are not allowed to create threads before all static and global objects
are initialized; in practise this means that you are not allowed to create threads
until <CODE>main()</CODE> is reached.</LI>
<LI>You cannot use <CODE>dlopen()</CODE> to load libcwd when threads have
already been created.&nbsp; Likewise you shouldn't <CODE>dlopen()</CODE>
other libraries that use libcwd when there are already running threads,
especially when those libraries define new debug objects and/or channels.</LI>
<LI>You need to provide one and only one locking mechanism per ostream device
that is also used to write debug output.&nbsp; It is preferable not to
use the same ostream with two or more different debug objects.</LI>
</UL>

<P>Essentially the debug objects and channels react towards each thread as if
that thread is the only thread.&nbsp; The only (visible) shared variable is
the final <CODE>ostream</CODE> that a given debug object writes to.&nbsp;
This means that if one thread changes the <CODE>ostream</CODE> then all other
threads also suddenly start to write to that <CODE>ostream</CODE>.&nbsp;
Basically, it is simply not supported: don't change the output stream
on the fly.</P>

<P>All other characteristics like the on/off state and the margin and
marker strings as well as the indentation are Thread Specific: Every
thread may change those without locking or worrying about the effect on
other threads.</P>

<P>Every time a new thread is created, it will start with all debug objects
and channels turned off, just as at the start of <CODE>main()</CODE>.</P>

<P>In all likelihood, you'd want to set the margin string such that it reflects which
thread is printing the output.&nbsp; For example:</P>

<PRE>
void* thread_function(void* arguments)
{
  // Set Thread Specific on/off flags of the debug channels.
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  // And for the debug object.
  Debug( libcw_do.on() );
  // Set a margin.
#ifdef CWDEBUG
  char margin[16];
  sprintf(margin, "%-10lu ", pthread_self());
#endif
  Debug( libcw_do.margin().assign(margin, 11) );

  Dout(dc::notice, "Entering thread " << pthread_self());
  // ... do stuff
  Dout(dc::notice, "Leaving thread " << pthread_self());
  return NULL;
}

int main(void)
{
  // Don't output a single character at a time (yuk)
  // (Read <A HREF ="http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8">http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8</A> for an explanation.)
  std::ios::sync_with_stdio(false);
  // Do header files and library match?
  Debug( check_configuration() );
  // Send debug output to std::cout.
#ifdef CWDEBUG
  pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
  Debug( libcw_do.set_ostream(&std::cout, &cout_mutex) );
  // Turn debug object on.
  Debug( libcw_do.on() );
  // Set a margin.
#ifdef CWDEBUG
  char margin[16];
  sprintf(margin, "%-10lu ", pthread_self());
#endif
  Debug( libcw_do.margin().assign(margin, 11) );
  // Turn all debug channels on.
  ForAllDebugChannels( if (!debugChannel.is_on()) debugChannel.on(); );
  // List all channels.
  Debug( list_channels_on(libcw_do) );

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
  return 0;
}
</PRE>

<P>Congratulations, you are now a libcwd expert.&nbsp; If you still have any
questions that you can't find answers to here, feel free to mail me.</P>

__PAGEEND
<P class="line"><IMG width=870 height=25 src="../images/lines/owl.png"></P>
<DIV class="buttons">
<A HREF="tut7.html"><IMG width=64 height=32 src="../images/buttons/lr_prev.png" border="0"></A>
<A HREF="index.html"><IMG width=64 height=32 src="../images/buttons/lr_index.png" border="0"></A>
</DIV>
__PAGEFOOTER
__HTMLFOOTER
