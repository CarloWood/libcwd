include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H1>FAQ</H1>

<P>Warning: The ammount of information included in this FAQ is exhaustive.&nbsp;
Do NOT read it except as a replacement for self-torture.&nbsp; Instead read the
<A HREF="index.html">tutorial</A> and skip all the references to
this FAQ unless you find yourself banging your head into the wall asking yourself
the same question as is listed in the tutorial.&nbsp; In that case a link will
bring you here to read just that one question.</P>

<HR SIZE=1 NOSHADE>

<A name="GNU_SOURCE"></A>
<H3>1. Won't this define make my code non-portable?</H3>

<P>No, not unless you actually use the GNU extensions in parts of your
application that need to be portable (like non-debugging code).&nbsp;
While debugging the application you will only benefit from using as
much compiler support as you can get, allowing the compiler to tell you
what could possibly be wrong with your code.&nbsp;
Once the application works, you don't have to define _GNU_SOURCE
because you won't be including the debug code anymore, nor link with
libcwd.&nbsp;
Note that GNU g++ 3.x already defines this macro currently itself as a hack
to get the libstdc++ headers work properly, hence the test with <CODE>#ifndef</CODE>
is always needed (see <A HREF="http://gcc.gnu.org/ml/gcc/2002-02/msg00996.html">http://gcc.gnu.org/ml/gcc/2002-02/msg00996.html</A>).</P>

<A name="sysd.h"></A>
<H3>2. Why do I have to include &quot;libcw/sysd.h&quot; as first header file?</H3>

<P>This header file is used to fix operating systems bugs, including bugs
in the system header files.&nbsp; The only way it can do this is when it
is included before <EM>any</EM> other header file, including system header
files.</P>

<P>This header file defines also a few very important operating system
dependend macros like the ones used for inclusion of the ident string.
Therefore <EM>nothing</EM> will compile without this header file.&nbsp;
Because it must be included in <EM>every</EM> source file as very first
header file, it would make no sense to include it also in another
header file; so it isn't.&nbsp; As a result, forgetting this header file
or including any other libcw header file before including libcw/sysd.h,
will definitely lead to compile errors in that header file.&nbsp;</P>

<A name="libcwd"></A>
<H3>3. What is this <SPAN class="H3code">libcw</SPAN> talk?
Aren't you forgetting the <U><SPAN class="H3code">d</SPAN></U> of
<SPAN class="H3code">libcw<U>d</U></SPAN>?</H3>

<P>Libcwd is a spin off of the larger libcw project.&nbsp;
The header files of both are put in the same directory,
called <SPAN class="filename">libcw </SPAN>(for example,
<SPAN class="filename">/usr/include/libcw</SPAN>).</P>

<P>The <U><SPAN class="code">d</SPAN></U> in
<SPAN class="code">libcw<U>d</U></SPAN> stands for <U>D</U>ebugging.&nbsp;
The <U><SPAN class="code">cw</SPAN></U> in
<SPAN class="code">lib<U>cw</U></SPAN> stand for the initials of
the designer/developer of this life-span project [but I suppose you already
guessed that&nbsp;;)&nbsp;].</P>

<A name="dir"></A>
<H3>4. Why do I need to type &quot;<SPAN class="H3code">libcw/sysd.h</SPAN>&quot;
and not just &quot;<SPAN class="H3code">sysd.h</SPAN>&quot;?</H3>

<P>The header file names of libcw are not unique.&nbsp; In order to uniquely
identify which header file needs to be included the &quot;libcw/&quot; part
is needed.</P>

<P>Never use the compiler option <SPAN class="code"><SPAN class="command-line-parameter">-I</SPAN>
<SPAN class="command-line-variable">/usr/include/libcw</SPAN></SPAN> so you can skip
the &quot;libcw/&quot; part in your <SPAN class="code">#include</SPAN>
directives.&nbsp; There is no garantee that there isn't a header file name
collision in that case.</P>

<A name="debug.h"></A>
<H3>5. What is defined <EM>exactly</EM> in <SPAN class="H3code">libcw/debug.h</SPAN>?</H3>

<P>Everything.&nbsp;
Go and read the <A HREF="../html/reference.html">Reference Manual</A> to get <EM>all</EM> gory details if you dare.</P>

<A name="macros"></A>
<H3>6. Why are you using macros for <SPAN class="H3code">Debug</SPAN> and <SPAN class="H3code">Dout</SPAN>?</H3>

<P>Because it is the only way to easy remove debugging code from an application as function of a macro
and because it allows for the fastest possible code even without optimisation, which is often the case
while debugging.&nbsp; A more detailed explanation is given in the <A HREF="../html/page_why_macro.html">Reference Manual</A>.</P>

<A name="Debug"></A>
<H3>7. Why do I need to type the <SPAN class="H3code">Debug(&nbsp;&nbsp;)</SPAN> around it?</H3>

<P>The macro <SPAN class="code">Debug()</SPAN> is used for two things. 1) The code inside it is only included
when the macro <SPAN class="code">CWDEBUG</SPAN> is defined. 2) It includes the namespace <SPAN class="code">libcw::debug</SPAN>.</P>

<P>As a result, you don't have to add <SPAN class="code">#ifdef CWDEBUG ... #endif</SPAN> around the code and
in most cases you don't have to type <SPAN class="code">libcw::debug</SPAN>.&nbsp;
The expression <SPAN class="code">Debug( STATEMENT );</SPAN> is equivalent with:</P>

<PRE class="code">
#ifdef CWDEBUG
  do {
    using namespace ::libcw::debug;
    using namespace DEBUGCHANNELS;
    { STATEMENT; }
  } while(0);
#endif
</PRE>

<P>Please note that definitions within a <SPAN class="code">Debug()</SPAN> statement will be
restricted to their own scope.&nbsp;
Please read the <A HREF="../html/group__chapter__custom__debug__h.html">Reference Manual</A> for an
explanation of <SPAN class="code">DEBUGCHANNELS</SPAN>.</P>

<A name="DebugChannels"></A>
<H3>8. Which Debug Channels exist?&nbsp; Can I make my own?</H3>

<P>This question is covered in chapter
<A HREF="../html/group__group__debug__channels.html">Controlling The Output Level (Debug Channels)</A>
of the Reference Manual.&nbsp;
As is described there, creating your own debug channels is best done by writing your own <SPAN class="code">debug.h</SPAN>
header file.&nbsp; The following template is a good start for such a <SPAN class="code">debug.h</SPAN> for an end application
(a library needs more work):</P>

<PRE class="code">
#ifndef MY_DEBUG_H
#define MY_DEBUG_H

#define DEBUGCHANNELS ::myapplication::debug::channels
#include &lt;libcw/debug.h&gt;

namespace myapplication {
  namespace debug {
    namespace channels {
      namespace dc {
        using namespace ::libcw::debug::channels::dc;
        extern ::libcw::debug::channel_ct mychannel;
	// ... more channels here
      }
    }
  }
}

#endif // MY_DEBUG_H
</PRE>

<P>Replace &laquo;<SPAN class="code">MY_DEBUG_H</SPAN>&raquo;,
&laquo;<SPAN class="code">myapplication::debug</SPAN>&raquo; and &laquo;<SPAN class="code">mychannel</SPAN>&raquo; with your own names.

<P>See the <SPAN class="filename">example-project</SPAN> that comes
with the source distribution of libcwd for a Real Life example.</P>

<A name="recursive"></A>
<H3>9. Can I turn Debug Channels off again?&nbsp; Can I do that recursively?</H3>

<P>Debug channels can be switched on and off at any time.&nbsp; At the start of your program you should
turn on the channels of your choice by calling <SPAN class="code">Debug(dc::<EM>channel</EM>.on())</SPAN>
<EM>once</EM>.&nbsp; Sometimes you want to temporally turn off certain channels: you want to make
sure that no debug output is written to that particular debug channel, at that moment.&nbsp; This can be
achieved by calling the methods <SPAN class="code">off()</SPAN> and <SPAN class="code">on()</SPAN> in
<EM>pairs</EM> and in that order.&nbsp; For example:</P>

<PRE class="code">
  // Make sure no allocation debug output is generated:
  Debug( dc::malloc.off() );
  char* temporal_buffer = new char [1024];
  // ... do stuff ...
  delete [] temporal_buffer;
  Debug( dc::malloc.on() );
</PRE>

<P>This will work even when `do stuff' calls a function that also turns <SPAN class="code">dc::malloc</SPAN> off and on:
after the call to <SPAN class="code">on()</SPAN> the debug channel can still be off: it is restored to the on/off state
that it was in before the corresponding call to <SPAN class="code">off()</SPAN>.&nbsp; In fact, the calls to
<SPAN class="code">off()</SPAN> and <SPAN class="code">on()</SPAN> only respectively increment and decrement a counter.</P>

<A name="Channel"></A>
<H3>10. Why do you call it a Debug <EM>Channel</EM>?&nbsp; What <EM>is</EM> a Debug Channel?</H3>

<P>A Debug Channel is a fictious &quot;news channel&quot;.&nbsp; It should contain information of a certain kind that is
interesting or not interesting as a whole.&nbsp; A Debug Channel is not a device or stream, a single debug channel is best
viewed upon as a single bit in a bitmask.&nbsp; Every time you write debug output you have to specify a &quot;bitmask&quot;
which specifies when that message is written; like when you are cross posting to usenet news groups, specifying multiple
news groups for a single message.&nbsp; When any of the specified Debug Channels is turned on, then the message is written
to the output stream of the underlaying debug object.</P>

<A name="OwnDebugObject"></A>
<H3>11. Can I make my own Debug Object?</H3>

<P><A HREF="../html/group__chapter__custom__do.html">Yes</A>, you can make as many debug objects as you like.&nbsp;
Each debug object is associated with one ostream.&nbsp; However, the default debug output macros <CODE>Dout</CODE> and
<CODE>DoutFatal</CODE> use the default debug object <CODE>libcw_do</CODE>.&nbsp;
It isn't hard at all to define your own macros though; for example add something like the following to
<A HREF="../html/group__chapter__custom__debug__h.html">your own &quot;debug.h&quot;</A> file:</P>

<PRE class="code">
#ifdef CWDEBUG
extern libcw::debug::debug_ct <SPAN class="highlight">my_debug_object</SPAN>;
#define <SPAN class="highlight">MyDout</SPAN>(cntrl, data) LibcwDout(DEBUGCHANNELS, <SPAN class="highlight">my_debug_object</SPAN>, cntrl, data)
#define <SPAN class="highlight">MyDoutFatal</SPAN>(cntrl, data) LibcwDoutFatal(DEBUGCHANNELS, <SPAN class="highlight">my_debug_object</SPAN>, cntrl, data)
#else // !CWDEBUG
#define <SPAN class="highlight">MyDout</SPAN>(a, b)
#define <SPAN class="highlight">MyDoutFatal</SPAN>(a, b) LibcwDoutFatal(::std, /*nothing*/, a, b)
#endif // !CWDEBUG
</PRE>

<A name="recursive2"></A>
<H3>12. Can I turn Debug Objects off again? Can I do that recursively?</H3>

<P>Debug objects can be switched on and off at any time.&nbsp; At the start of your program you should
turn on the debug object(s) by calling <SPAN class="code">Debug(<EM>debugobject</EM>.on())</SPAN>
<EM>once</EM>.&nbsp; Sometimes you want to temporally turn off all debug output.&nbsp;
This can be achieved by calling the methods <SPAN class="code">off()</SPAN> and <SPAN class="code">on()</SPAN> in
<EM>pairs</EM> and in that order.&nbsp; For example:</P>

<PRE class="code">
  // Disable all debug output to `libcw_do':
  Debug( libcw_do.off() );
  // ... do stuff ...
  Debug( libcw_do.on() );
</PRE>

<P>This will work even when `do stuff' calls a function that also turns <SPAN class="code">libcw_do</SPAN> off and on:
after the call to <SPAN class="code">on()</SPAN> the debug object can still be off: it is restored to the on/off state
that it was in before the corresponding call to <SPAN class="code">off()</SPAN>.&nbsp; In fact, the calls to
<SPAN class="code">off()</SPAN> and <SPAN class="code">on()</SPAN> only respectively increment and decrement a counter.</P>

<A name="SetOstream"></A>
<H3>13. How do I set a new <SPAN class="H3code">ostream</SPAN> for a given Debug Object?</H3>

<P>You can change the <SPAN class="code">ostream</SPAN> that is associated with a Debug Object at any time.&nbsp;
For example, changing the <SPAN class="code">ostream</SPAN> of <SPAN class="code">libcw_do</SPAN> from the
default <SPAN class="code">cerr</SPAN> to <SPAN class="code">cout</SPAN>:</P>

<PRE class="code">
  Debug( libcw_do.set_ostream(&cout) );
</PRE>

<P>See also <A HREF="tut3.html">tutorial 3</A>.</P>

<A name="WhyOff"></A>
<H3>14. Why are Debug Objects turned off at creation?</H3>

<P>The Debug Objects and Debug Channels are global objects.&nbsp; Because libcwd could not be
dependant of libcw, they do not use libcw's <CODE>Global&lt;&gt;</CODE> template.&nbsp;
As a result, the order in which the debug channels and objects are initialized is
unknown; moreover, other global objects whose constructors might try to write debug output could
be constructed before the debug objects are initialized!&nbsp; The debug objects are therefore
designed in a way that independent of there internal state of initialisation they function without
crashing.&nbsp; It should be obvious that the only way this could be achieved was by creating them
in the state <EM>off</EM>.</P>

<A name="Order"></A>
<H3>15. Why do you turn on the debug object after you enable a debug channel, why not the other way around?</H3>

<P>The order in which Debug Channels and Debug Objects are turned on does not matter at all.&nbsp;
At most, when you think about the Debug Object as the &laquo;main switch&raquo; then it seems to make
sense to first play with the little channel switches before finally activating the complete Debug
machinery.&nbsp; Others might think more in the lines of: lets start with setting the debug object
<EM>on</EM> before I forget it.&nbsp; That is a bit <EM>too</EM> fuzzy (logic) for me though ;)</P>

<A name="Object"></A>
<H3>16. Why do you call it a Debug <EM>Object</EM>?&nbsp; What <EM>is</EM> a Debug Object?</H3>

<P>Good question.&nbsp; It can't be because I wasn't creative, I am very creative.&nbsp;
Note that I didn't think of <EM>Object</EM> as in OOP (<EM>that</EM> would be uncreative)
but more along the lines of an <EM>object</EM>, like in science fiction stories -- objects
you can't get around.&nbsp; The monolith of <A HREF="http://uk.imdb.com/Title?0062622" target="_blank">2001: A Space Odyssey</A>
is a good example I guess.</P>

<P>Unlike the monolith however, a Debug Object is not mysterious at all.&nbsp; Basically it
is a pointer to an <SPAN class="code">ostream</SPAN> with a few extra attributes added to
give it an internal state for 'on' (pass output on) and 'off' (don't pass output on) as well
as some formatting information of how to write the data that is passed on to its
<SPAN class="code">ostream</SPAN>.</P>

<A name="semicolon"></A>
<H3>17. Do I need to type that semi-colon after the macro?&nbsp; Why isn't it part of the macro?</H3>

<P>Yes, that colon needs to be there.&nbsp;
It was chosen not to include the semi-colon in the macro because this way it looks
a bit like a function call which feels more natural.</P>

<P>The code <SPAN class="code">Dout(dc::notice,&nbsp;"Hello World");</SPAN> is definitely
a <EM>statement</EM> and therefore needs to end on a semi-colon (after expansion).&nbsp; When the
macro <SPAN class="code">CWDEBUG</SPAN> is not defined, the macro is replaced with
whitespace but still has to be a statement: it must be a single semi-colon then.</P>

<P>For example,</P>

<PRE class="code">
  if (error)
    Dout(dc::notice, "An error occured");

  exit(0);
  cerr &lt;&lt; "We should never reach this\n";
</PRE>

<P>If the complete line <SPAN class="code">Dout(dc::notice, "An error occured");</SPAN>,
including semi-colon is removed (replaced with whitespace), then the line
<SPAN class="code">exit(0);</SPAN> would be executed only when <SPAN class="code">error</SPAN>
is <SPAN class="code">true</SPAN>!&nbsp; And when the semi-colon would be included in
the macro then people could easily be tempted to add a semi-colon anyway (because it
looks so much better), which would break code like:</P>

<PRE class="code">
  if (error)
    Dout(dc::notice, "An error occured");
  else
    cout &lt;&lt; "Everything is ok\n";
</PRE>

<P>because after macro expansion that would become:</P>

<PRE class="code">
  if (error)
    ;
    ;
  else		// &lt;-- syntax error
    cout &lt;&lt; "Everything is ok\n";
</PRE>

<A name="LibcwDout"></A>
<H3>18. I made my own Debug Object, can I still use <SPAN class="H3code">Dout</SPAN>?</H3>

<P>No, macro <SPAN class="code">Dout</SPAN> et al. use exclusively the debug object that
comes with libcwd.&nbsp; It is easy to define your own macros however (see <A HREF="#OwnDebugObject">above</A>).&nbsp;
You are free to <EM>redefine</EM> the <SPAN class="code">Dout</SPAN> macros however, just realize that libcwd
will continue to use its own debug object (<SPAN class="code">libcw_do</SPAN>), debug output written by libcwd
in its header files do not use the <SPAN class="code">Dout</SPAN> macro (especially in order to allow you
to redefine it).</P>

<A name="evaluation"></A>
<H3>19. Is the second field of the macro still evaluated when the Debug Channel and/or Debug Object are turned off?</H3>

<P>No!&nbsp; And that is a direct result of the fact that <SPAN class="code">Dout</SPAN> et al. are <EM>macros</EM>.&nbsp;
Indeed this fact could therefore be a little confusing.&nbsp;
In pseudo-code the macro expansion looks something like</P>

<PRE class="code">
  if (debug object and any of the debug channels are turned on)
    the_ostream &lt;&lt; your message;
</PRE>

<P>and so, &quot;your message&quot; is <EM>not</EM> evaluated when it isn't also
actually written.&nbsp; This fact is also covered in the
<A HREF="../html/classlibcw_1_1debug_1_1debug__ct.html#eval_example">Reference Manual</A>.</P>

<P>Note that debug code should never have an effect on any of your variables (and thus on the application) anyway.&nbsp;
In the production version of your application all debug code will be removed and you don't want it to behave differently then!</P>

<A name="suppress"></A>
<H3>20. Can I suppress that new-line character?</H3>

<P>Yes, and a lot more.&nbsp; See <A HREF="tut5.html#Formatting">tutorial 5.4</A>.</P>

<A name="label"></A>
<H3>21. What is the maximum length of a label?</H3>

<P>The maximum length of the label of a new Debug Channel is given
by the constant<SPAN class="code"> libcw::debug::max_label_len_c</SPAN>.&nbsp;
At this moment that is 16.</P>

<A name="prefix"></A>
<H3>22. Why do I have to use the <SPAN class="H3code">dc::</SPAN> prefix?</H3>

<P>This is a complex reason.&nbsp; Basically because of a flaw in the design of namespaces in C++.&nbsp;
Namespaces have been introduced in order to avoid name collisions, which was a good thing.&nbsp;
It doesn't make much sense if you constantly have to type <SPAN class="code">::somelibrary::debug::channel::notice</SPAN>
of course, then you could as well have avoided the name space problem by using
<SPAN class="code">somelibrary_debug_channel_notice</SPAN> right?&nbsp;
Therefore you don't have to type the name of the namespace that is &quot;current&quot;.&nbsp;
There can be only <EM>one</EM> namespace current at a time however.&nbsp; The result is that
this can not be used to solve our problem: We want to avoid both, name collisions between debug channels
and any other variable or function name, but <EM>also</EM> between debug channels defined in
different libraries.&nbsp; That means we need more than one namespace: A namespace for each of
the libraries.&nbsp; We can not make all of them current however.&nbsp; Worse, we can not
make any namespace current because it must be possible to add code that writes debug output
<EM>everywhere</EM>.&nbsp; We can only use the <SPAN class="code">using namespace</SPAN>
directive.&nbsp; Now here is the real flaw: A <SPAN class="code">using namespace</SPAN> directive
gives no priority whatsoever to names when resolving them, for example, you can't do this:</P>

<PRE class="code">
namespace base {
  int base1;
  int base2;
}

namespace derived {
  using namespace base;
  int derived1;
  char base1;
}

  // ...
  using namespace derived;
  base1 = 'a';
</PRE>

<P>because C++ will make absolutely no difference between variables defined in
<SPAN class="code">derived</SPAN> and variables defined in <SPAN class="code">base</SPAN>
but will complain that <SPAN class="code">base1</SPAN> is ambigious.</P>

<P>The only opening that the ANSI/ISO C++ Standard allows us here is in the
following phrase:</P>

<QUOTE>
Given <SPAN class="code">X::m</SPAN> (where <SPAN class="code">X</SPAN> is a user-declared namespace),
or given <SPAN class="code">::m</SPAN> (where <SPAN class="code">X</SPAN> is the global namespace),
let <SPAN class="code">S</SPAN> be the set of all declarations of <SPAN class="code">m</SPAN> in <SPAN class="code">X</SPAN>
and in the transitive closure of all namespaces nominated by <I>using-directive</I>s in X and its used namespaces,
except that <I>using-directive</I>s are ignored in any namespace, including <SPAN class="code">X</SPAN>,
directly containing one or more declarations of <SPAN class="code">m</SPAN>.&nbsp;
No namespace is searched  more than once in the lookup of a name.&nbsp; If <SPAN class="code">S</SPAN> is the empty set,
the program is ill-formed.&nbsp; Otherwise, if <SPAN class="code">S</SPAN> has exactly one member,
or if the context of the reference is a using-declaration, <SPAN class="code">S</SPAN> is
the required set of declarations of <SPAN class="code">m</SPAN>.&nbsp;
Otherwise if the use of <SPAN class="code">m</SPAN> is not one that allows a unique
declaration to be chosen from <SPAN class="code">S</SPAN>, the program is ill-formed.
</QUOTE>

<P>Replace <SPAN class="code">X</SPAN> with <SPAN class="code">dc::</SPAN>
(obviously we don't want to put the debug channels in global namespace)
and we can use this rule to at least select a specific channel by using
the trick that the used <SPAN class="code">dc</SPAN> namespace is <EM>not the same</EM>
namespace for the different libraries.&nbsp; Then we can use debug channels with
the same name in <SPAN class="code">dc</SPAN> namespaces in <EM>different</EM>
namespaces in different libraries and use the namespaces of <EM>one</EM> library
at a time to select the current <SPAN class="code">dc</SPAN> namespace.</P>

<P>If this is over your head then that is probably because I can't explain&nbsp;:).&nbsp;
Don't worry however, you only need to know <EM>how</EM> to introduce new debug
channels and not understand how it works.&nbsp; The correct procedure is described
in the <A HREF="../html/group__group__debug__channels.html">Reference Manual</A>.</P>

<A name="ownnamespace"></A>
<H3>23. Can I put my debug channels in my own name space?</H3>

<P>Yes.&nbsp; How, is described in the <A HREF="../html/group__group__debug__channels.html">Reference Manual</A>.&nbsp;
For some background information on why this has to be so complex, please read the <A HREF="#prefix">previous question</A>.</P>

<A name="labelwidth"></A>
<H3>24. Why does it print spaces between the label and the colon?&nbsp; How is the field width of the label determined?</H3>

<P>The colon is indented so it ends up in the same column for all existing debug channels.&nbsp;
Hence, the longest label of all existing/created debug channels determines the number of spaces.&nbsp;
This value can be less than the <A HREF="#label">maximum allowed label size</A> of course.</P>

__PAGEEND
__PAGEFOOTER
__HTMLFOOTER

