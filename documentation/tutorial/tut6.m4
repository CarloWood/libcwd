include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H2>Tutorial 6: The debugging of dynamic memory allocations</H2>

<H3>Introduction</H3>

<P>For an introduction please read chapter
<A HREF="../html/group__book__allocations__intro.html">Memory Allocation Debug Support: Introduction</A>
of the <A HREF="../html/reference.html">Reference Manual</A>.</P>

<H3>Conventions</H3>

<P>This tutorial will give a quick overview of the conventions that you
have to follow to fully benefit from the support of libcwd for the
debugging of dynamic memory allocations.</P>

<H3>Environment</H3>

<P>First make sure that support is compiled in.&nbsp;
Check the headerfile <SPAN class="filename">libcw/debug_config.h</SPAN>
and make sure that the macro <CODE>CWDEBUG_ALLOC</CODE>
is set to 1.&nbsp;
If it is not defined, then you'll have to reconfigure, recompile and install libcwd.&nbsp;
Use <SPAN class="shell-command">./configure --enable-alloc</SPAN> during configure.</P>

<P>You also want the macros <CODE>CWDEBUG_LOCATION</CODE>
and <CODE>CWDEBUG_MAGIC</CODE> to be defined to 1.</P>

<H3>Header files</H3>

<P>There is no special header file needed.&nbsp;
You will need to include <CODE>&quot;debug.h&quot;</CODE> in the same
way as is described in <A HREF="tut5.html#debug.h">tutorial 5</A>
and everything needed will be included from <CODE>libcw/debug.h</CODE>.&nbsp;
In the remainder of this tutorial we will simple include <CODE>libcw/debug.h</CODE> directly
and not create any custom debug channels.</P>

<H3>The Allocated memory Overview</H3>

<P>Libcw does some basic checks on de-allocations and memory leaks (at the end
of the program), but there is nothing to teach about that: you'll see what
happens when you make a mistake.</P>

<P>However, at any moment in your program you can ask libcwd to print an <I>overview</I>
of all memory allocations to a debug object (to channel <CODE>dc::malloc</CODE>)
or to an arbitrary ostream.&nbsp;
All details of the Allocated memory Overview are described in chapter
<A HREF="../html/group__group__overview.html">Overview Of Allocated Memory</A> of
the <A HREF="../html/reference.html">Reference Manual</A>.</P>

<P>The following example program writes the Allocated memory Overview to
the default debug object <CODE>libcw_do</CODE>:</P>

<P>Compile as: <CODE>g++ -DCWDEBUG amo.cc -lcwd -o amo</CODE></P>
<PRE>
#include &quot;sys.h&quot;		// See tutorial 2.
#include &quot;debug.h&quot;

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );

  Debug( <SPAN class="highlight">list_allocations_on(libcw_do)</SPAN> );

  return 0;
}
</PRE>

<P>The output of this program is very simple,</P>

<PRE class="output">
MALLOC  : Allocated memory: 0 bytes in 0 blocks.
</PRE>

<P>because we didn't allocate any memory.</P>

<P>Now let us actually allocate some memory:</P>

<PRE>
#include &quot;sys.h&quot;		// See tutorial 2.
#include &quot;debug.h&quot;

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );

  <SPAN class="highlight">int* p = new int [100];</SPAN>

  Debug( list_allocations_on(libcw_do) );

  return 0;
}
</PRE>

<P>Also the call to <CODE>operator new[]</CODE> is written to debug channel MALLOC:</P>

<PRE class="output">
MALLOC  : operator new[] (size = 400) = 0x804f310
MALLOC  : Allocated memory: 400 bytes in 1 blocks.
new[]     0x804f310             &lt;unknown type&gt;; (sz = 400) 
</PRE>

<P>The call to <CODE>list_allocations_on()</CODE> is responsible for the last two lines.</P>

<P>There is something missing however!&nbsp; When we use <CODE>CWDEBUG_LOCATION</CODE>
we expect source file and line number information of every memory allocation, and there is none.&nbsp;
In order to find out what is wrong, we <EM>also turn on debug channel</EM><CODE> dc::bfd</CODE>:</P>

<PRE>
#include &quot;sys.h&quot;		// See tutorial 2.
#include &quot;debug.h&quot;

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );
  <SPAN class="highlight">Debug( dc::bfd.on() );</SPAN>

  int* p = new int [100];

  Debug( list_allocations_on(libcw_do) );

  return 0;
}
</PRE>

<P>Which results in the following output:</P>

<PRE class="output">
MALLOC  : operator new[] (size = 400) = &lt;unfinished&gt;
BFD     :     Loading debug symbols from /home/carlo/c++/libcw/www/tutorial/amo... done (153 symbols)
BFD     :     Loading debug symbols from /usr/local/lib/libcwd.so.0 (0x4001a000) ... done (1529 symbols)
BFD     :     Loading debug symbols from /usr/lib/libstdc++-libc6.1-2.so.3 (0x4006b000) ... done (1578 symbols)
BFD     :     Loading debug symbols from /lib/libm.so.6 (0x400b2000) ... done (1295 symbols)
BFD     :     Loading debug symbols from /lib/libc.so.6 (0x400cf000) ... done (4025 symbols)
BFD     :     Loading debug symbols from /usr/lib/libbfd-2.10.0.18.so (0x401c5000) ... done (1191 symbols)
BFD     :     Loading debug symbols from /lib/libdl.so.2 (0x4021c000) ... done (88 symbols)
BFD     :     Loading debug symbols from /lib/ld-linux.so.2 (0x40000000) ... done (293 symbols)
BFD     :     Warning: Address 0x804aa8e in section .text does not have a line number, perhaps the unit containing the function
              `main' wasn't compiled with flag -g
MALLOC  : &lt;continued&gt; 0x804e898
MALLOC  : Allocated memory: 400 bytes in 1 blocks.
new[]     0x804e898             &lt;unknown type&gt;; (sz = 400) 
</PRE>

<P>Please note the following</P>
<UL>
<LI>The debugging symbols are loaded <EM>the first time</EM> an address is resolved,
<A HREF="tut5.html#interrupted">interrupting</A> the debug output of the first allocation.</LI>
<LI>The allocation is done somewhere inside a function <CODE>main</CODE></A> but
no <A HREF="../html/group__group__locations.html">Source-file:Line-number Information</A> information is found.</LI>
<LI>Likely the 'Loading debug symbols from..' is done <EM>before</EM> the application reaches <CODE>main()</CODE>
and is hence invisible (because the debug object, <CODE>libcw_do</CODE>, is still turned off).&nbsp;
You can force libcwd to print it nevertheless by setting the environment variable <CODE>LIBCWD_PRINT_LOADING</CODE>.</LI>
</UL>

<TABLE>
<TR>
<TD>And indeed, we forgot to compile amo.cc with -g</TD>
<TD><IMG width=60 height=45 src="../images/wink.gif" border=0></TD>
</TR>
</TABLE>

<P>When we compile correctly, using <CODE>g++ -g -DCWDEBUG amo.cc -lcwd -o amo</CODE>, the output of the program becomes:</P>

<PRE class="output">
MALLOC  : operator new[] (size = 400) = 0x804f208
MALLOC  : Allocated memory: 400 bytes in 1 blocks.
new[]     0x804f208               <SPAN class="highlight">amo.cc:9</SPAN>    &lt;unknown type&gt;; (sz = 400) 
</PRE>

<P>As you can see, the <EM>type</EM> of the object for which the memory was
allocated is still unknown.&nbsp; You can make the Allocated memory Overview
better surveyable by adding a &laquo;tag&raquo; for every allocation that
your program is doing:</P>

<PRE>
#include &quot;sys.h&quot;		// See tutorial 2.
#include &quot;debug.h&quot;

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );

  int* p = new int [100];
  <SPAN class="highlight">AllocTag(p, "A test array");</SPAN>

  Debug( list_allocations_on(libcw_do) );

  return 0;
}
</PRE>

<P>This results in the output</P>

<PRE class="output">
MALLOC  : operator new[] (size = 400) = 0x804f7a8
MALLOC  : Allocated memory: 400 bytes in 1 blocks.
new[]     0x804f7a8               amo.cc:9    <SPAN class="highlight">int[100]</SPAN>; (sz = 400)  <SPAN class="highlight">A test array</SPAN>
</PRE>

<P>The second parameter of <CODE>AllocTag()</CODE> may be
anything that can be written to an ostream (like is the case for <CODE>Dout()</CODE>).&nbsp;
However, it is only processed <EM>once</EM>: the first time it is called.&nbsp;
This allows to even add an <CODE>AllocTag()</CODE> at places where
a low cpu usage is important and/or in loops that allocate a very large number
of memory blocks (the comment is only stored once).</P>

<P>Consider the following code:</P>

<PRE>
#include &quot;sys.h&quot;		// See tutorial 2.
#include &quot;debug.h&quot;

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );

  int* p[4];
  <SPAN class="highlight">for(int i = 0; i &lt; 4; ++i)</SPAN>
  {
    p[i] = new int [100];
    AllocTag(p[i], "Test array number " <SPAN class="highlight">&lt;&lt; i</SPAN>);	// This won't work
  }

  Debug( list_allocations_on(libcw_do) );

  return 0;
}
</PRE>

<P>The Allocated memory Overview will show four times the same tag:</P>

<PRE class="output">
MALLOC  : operator new[] (size = 400) = 0x804f8d0
MALLOC  : operator new[] (size = 400) = 0x8137c80
MALLOC  : operator new[] (size = 400) = 0x8138028
MALLOC  : operator new[] (size = 400) = 0x81383d0
MALLOC  : Allocated memory: 1600 bytes in 4 blocks.
new[]     0x81383d0               amo.cc:12   int[100]; (sz = 400)  Test array number <SPAN class="highlight">0</SPAN>
new[]     0x8138028               amo.cc:12   int[100]; (sz = 400)  Test array number <SPAN class="highlight">0</SPAN>
new[]     0x8137c80               amo.cc:12   int[100]; (sz = 400)  Test array number <SPAN class="highlight">0</SPAN>
new[]     0x804f8d0               amo.cc:12   int[100]; (sz = 400)  Test array number <SPAN class="highlight">0</SPAN>
</PRE>

<P>If you don't care about the extra memory and cpu usage, you can also use
<CODE>AllocTag_dynamic_description()</CODE>, which <EM>will</EM> work.</P>

<PRE>
#include &quot;sys.h&quot;		// See tutorial 2.
#include &quot;debug.h&quot;

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );

  int* p[4];
  for(int i = 0; i &lt; 4; ++i)
  {
    p[i] = new int [100];
    <SPAN class="highlight">AllocTag_dynamic_description</SPAN>(p[i], "Test array number " &lt;&lt; i);	// This will work
  }

  Debug( list_allocations_on(libcw_do) );

  return 0;
}
</PRE>

<P>gives as result</P>

<PRE class="output">
MALLOC  : operator new[] (size = 400) = 0x804f968
MALLOC  : operator new[] (size = 400) = 0x8137d70
MALLOC  : operator new[] (size = 400) = 0x8138290
MALLOC  : operator new[] (size = 400) = 0x804f6f8
MALLOC  : Allocated memory: 1600 bytes in 4 blocks.
new[]     0x804f6f8               amo.cc:12   int[100]; (sz = 400)  Test array number <SPAN class="highlight">3</SPAN>
new[]     0x8138290               amo.cc:12   int[100]; (sz = 400)  Test array number <SPAN class="highlight">2</SPAN>
new[]     0x8137d70               amo.cc:12   int[100]; (sz = 400)  Test array number <SPAN class="highlight">1</SPAN>
new[]     0x804f968               amo.cc:12   int[100]; (sz = 400)  Test array number <SPAN class="highlight">0</SPAN>
</PRE>

<P>Often just the type of an object tells you enough about what it is.&nbsp
In that case you can omit the comment completely and simply use
<CODE>AllocTag1(p)</CODE>.&nbsp;
Or, if you don't need any <CODE>operator&lt;&lt;</CODE>, you
can use <CODE>AllocTag2(p, &quot;Some string constant here&quot;)</CODE>.&nbsp;
Finally, you can use the macro <CODE>NEW</CODE> to catch the type of
an allocation as if you did a <CODE>new</CODE> followed by a
<CODE>AllocTag1(p)</CODE>:</P>

<PRE>
#include &quot;sys.h&quot;		// See tutorial 2.
#include &quot;debug.h&quot;

int main(void)
{
  Debug( libcw_do.on() );
  Debug( dc::malloc.on() );

  int* p = <SPAN class="highlight">NEW(</SPAN> int [100] <SPAN class="highlight">)</SPAN>;

  Debug( list_allocations_on(libcw_do) );

  return 0;
}
</PRE>

<P>would output</P>

<PRE class="output">
MALLOC  : operator new[] (size = 400) = 0x804f4f0
MALLOC  : Allocated memory: 400 bytes in 1 blocks.
new[]     0x804f4f0               amo.cc:9    <SPAN class="highlight">int[100]</SPAN>; (sz = 400) 
</PRE>

__PAGEEND
<P class="line"><IMG width=870 height=33 src="../images/lines/caterpil.png"></P>
<DIV class="buttons">
<A HREF="tut5.html"><IMG width=64 height=32 src="../images/buttons/lr_prev.png" border="0"></A>
<A HREF="index.html"><IMG width=64 height=32 src="../images/buttons/lr_index.png" border="0"></A>
<A HREF="tut7.html"><IMG width=64 height=32 src="../images/buttons/lr_next.png" border="0"></A>
</DIV>
__PAGEFOOTER
__HTMLFOOTER
