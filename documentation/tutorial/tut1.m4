include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H2>Tutorial 1: Hello World</H2>

<P>The smallest C++ program that prints &laquo;<SPAN class="output">Hello World</SPAN>&raquo; as <I>debug output</I>
to <CODE>cerr</CODE> is:</P>

<P class="download">[<A HREF="hello_world.cc">download</A>]</P>

<P>Compile as: <SPAN class="shell-command">g++ -g -DCWDEBUG hello_world.cc -lcwd -o hello_world</SPAN></P>

<PRE>
#define _GNU_SOURCE			// This must be defined before including &lt;libcw/sysd.h&gt;
#include &lt;libcw/sysd.h&gt;           // This must be the first header file&nbsp;&nbsp;
#include &lt;libcw/debug.h&gt;

int main(void)
{
  Debug( dc::notice.on() );             // Turn on the NOTICE Debug Channel
  Debug( libcw_do.on() );               // Turn on the default Debug Object

  Dout( dc::notice, "Hello World" );

  return 0;
}
</PRE>

<P>Each of the lines of code in this first example program are explained below:</P>

<H3><CODE>#define _GNU_SOURCE</CODE></H3>

<P>This define is necessary to tell the system headers that you
want to use the GNU extensions (see /usr/include/features.h).&nbsp;
In order to make you explicitely aware of the fact that it is
defined, libcwd does not define this macro itself (which it could do inside &lt;libcw/sysd.h&gt;),
but forces you to define it when using libcwd.&nbsp;
Note you only really have to define it when you compiled libcwd with
threading support.&nbsp;
If you do not define this macro and libcwd needs it, then you will get
a compile error in &lt;libcw/sysd.h&gt; telling you so.</P>

<DIV class="faq-frame"><H4>FAQ</H4><UL class="faq">
<LI><A HREF="faq.html#GNU_SOURCE">Won't this define make my code non-portable?</A></LI>
</UL></DIV>

<H3><CODE>#include &lt;libcw/sysd.h&gt;</CODE></H3>

<P>This must be the very first header file that is included; even before system header files.&nbsp;
Every source file that includes other libcw headers must include it.</P>

<DIV class="faq-frame"><H4>FAQ</H4><UL class="faq">
<LI><A HREF="faq.html#sysd.h">Why?</A></LI>
<LI><A HREF="faq.html#libcwd">What is this <TT>libcw</TT> talk?
Aren't you forgetting the <U><TT>d</TT></U> of
<TT>libcw<U>d</U></TT>?</A></LI>
<LI><A HREF="faq.html#dir">Why do I need to type "<CODE>libcw/sysd.h</CODE>"
and not just "<CODE>sysd.h</CODE>"?</LI></A>
</UL></DIV>

<H3><CODE>#include &lt;libcw/debug.h&gt;</CODE></H3>

<P>This header file contains all definitions and declarations that are needed for debug output.&nbsp;
For example, it defines the macros <CODE>Debug</CODE> and <CODE>Dout</CODE> and declares
the debug object <CODE>libcw_do</CODE> and the debug channel <CODE>dc::notice</CODE>.</P>

<DIV class="faq-frame"><H4>FAQ</H4><UL class="faq">
<LI><A HREF="faq.html#debug.h">What is defined <EM>exactly</EM> in <CODE>libcw/debug.h</CODE>?</A></LI>
<LI><A HREF="faq.html#macros">Why are you using macros for <CODE>Debug</CODE> and <CODE>Dout</CODE>?</A></LI>
</UL></DIV>

<A NAME="turn_on_channel"></A>
<H3></CODE>Debug( dc::notice.on() );</CODE></H3>

<P>This turns on the <I><U>D</U>ebug <U>C</U>hannel</I>&nbsp; <CODE> <U>dc</U>::notice</CODE>.&nbsp;
Without this line, the code <CODE>Dout(&nbsp;dc::notice, "Hello World"&nbsp;)</CODE> would output
nothing: all <I>Debug Channels</I> are <EM>off</EM> by default, at start up.</P>

<DIV class="faq-frame"><H4>FAQ</H4><UL class="faq">
<LI><A HREF="faq.html#Debug">Why do I need to type the <CODE>Debug(&nbsp;&nbsp;)</CODE> around it?</A></LI>
<LI><A HREF="faq.html#DebugChannels">Which Debug Channels exist? Can I make my own?</A></LI>
<LI><A HREF="faq.html#recursive">Can I turn Debug Channels off again? Can I do that recursively?</A></LI>
<LI><A HREF="faq.html#Channel">Why do you call it a Debug <EM>Channel</EM>? What <EM>is</EM> a Debug Channel?</A></LI>
</UL></DIV>

<H3><CODE>Debug( libcw_do.on() );</CODE></H3>

<P>This turns on the <I><U>D</U>ebug <U>O</U>bject</I> <CODE>libcw_<U>do</U></CODE>.&nbsp;
Without this line, the code <CODE>Dout( dc::notice, "Hello World" )</CODE> would output
nothing: all <I>Debug Objects</I> are <EM>off</EM> by default, at start up.</P>

<P>A <I>Debug Object</I> is related to exactly one <CODE>ostream</CODE>.&nbsp;
<CODE>Libcwd</CODE> defines only one <I>Debug Object</I> by itself (being <CODE>libcw_do</CODE>),
this is enough for most applications.&nbsp;
The default ostream is <CODE>cerr</CODE>.&nbsp;
Using the macro <CODE>Dout</CODE> causes debug output to be written to <CODE>libcw_do</CODE>.</P>

<DIV class="faq-frame"><H4>FAQ</H4><UL class="faq">
<LI><A HREF="faq.html#Debug">Why do I need to type the <CODE>Debug(&nbsp;&nbsp;)</CODE> around it?</A></LI>
<LI><A HREF="faq.html#OwnDebugObject">Can I make my own Debug Object?</A></LI>
<LI><A HREF="faq.html#recursive2">Can I turn Debug Objects off again? Can I do that recursively?</A></LI>
<LI><A HREF="faq.html#SetOstream">How do I set a new <CODE>ostream</CODE> for a given Debug Object?</A></LI>
<LI><A HREF="faq.html#WhyOff">Why are Debug Objects turned off at creation?</A></LI>
<LI><A HREF="faq.html#Order">Why do you turn on the debug object after you enable a debug channel, why not the other way around?</A></LI>
<LI><A HREF="faq.html#Object">Why do you call it a Debug <EM>Object</EM>? What <EM>is</EM> a Debug Object?</A></LI>
</UL></DIV>

<H3><CODE>Dout( dc::notice, "Hello World" );</CODE></H3>

<P>This outputs "Hello World" to the <CODE>ostream</CODE> currently related to
<CODE>libcw_do</CODE> provided that the <I>Debug Channel</I>
<CODE>dc::notice</CODE> is turned on.</P>

<P>Output is written as if everything in the second field of the macro <CODE>Dout</CODE> is
written to an ostream; It is "equivalent" with: <CODE>cerr &lt;&lt; "Hello World" &lt;&lt; '\n';</CODE></P>

<DIV class="faq-frame"><H4>FAQ</H4><UL class="faq">
<LI><A HREF="faq.html#macros">Why is <CODE>Dout</CODE> a macro and not a template?</A></LI>
<LI><A HREF="faq.html#semicolon">Do I need to type that semi-colon after the macro? Why isn't it part of the macro?</A></LI>
<LI><A HREF="faq.html#LibcwDout">I made my own Debug Object, can I still use <CODE>Dout</CODE>?</A></LI>
<LI><A HREF="faq.html#evaluation">Is the second field of the macro still evaluated when the Debug Channel and/or Debug Object are turned off?</A></LI>
<LI><A HREF="faq.html#suppress">Can I suppress that new-line character?</A></LI>
</UL></DIV>

__PAGEEND
<P class="line"><IMG width=870 height=18 src="../images/lines/cat.png"></P>
<DIV class="buttons">
<A HREF="intro.html"><IMG width=64 height=32 src="../images/buttons/lr_prev.png" border=0></A>
<A HREF="index.html"><IMG width=64 height=32 src="../images/buttons/lr_index.png" border="0"></A>
<A HREF="tut2.html"><IMG width=64 height=32 src="../images/buttons/lr_next.png" border=0></A>
</DIV>
__PAGEFOOTER
__HTMLFOOTER

