include(definitions.m4)dnl
__HTMLHEADER
__PAGEHEADER
__PAGESTART

<H2>Tutorial 4: Management of <CODE>CWDEBUG</CODE></H2>

<P>In a production environment you most likely want to remove all debugging code from
the application because it makes the application slower, of course.&nbsp;
You can do this by compiling the final application <EM>without</EM> defining the macro <CODE>CWDEBUG</CODE>.&nbsp;
You also don't need to link with libcwd in that case.</P>

<P>In example, with debugging code you'd compile a program as</P>

<P class="shell-command">g++ -g -DCWDEBUG application.cc -lcwd -o application</P>

<P>and without debugging code you'd use</P>

<P class="shell-command">g++ -O3 application.cc -o application</P>

<P>Because the final application doesn't need libcwd, we can afford to use features of g++ and
third party libraries that developers are able to install but that would be impractical as demand for
the end-user of the application.&nbsp; The <EM>developer</EM> that uses libcwd will have to use
g++ as compiler and perhaps use a linux box for the development of the application, but gets a lot
of developing advantages in return.&nbsp; Afterwards it is relatively easy to port the bug-free
code to other platforms/compilers.</P>

<P>As a developer you need to know two things:</P>
<OL TYPE="1">
<LI>How to compile the application you are developing, while you're still developing it.</LI>
<LI>How to compile a final version of your application, without debugging support.</LI>
</OL>

<P>In the first case, all libraries and your application need to be compiled with <CODE>CWDEBUG</CODE> defined.&nbsp;
In the second case all libraries, except libcwd (because you won't link with it) and your application, need to be compiled
with <CODE>CWDEBUG</CODE> undefined.</P>

<P>Libraries that do not use libcwd (or otherwise depend on the macro
<CODE>CWDEBUG</CODE>) do not need to be recompiled of course.&nbsp;
If you are writing an end-application and do not have other libraries that depend on
libcwd (or otherwise depend on the macro <CODE>CWDEBUG</CODE>)
then it is as easy as defining <CODE>CWDEBUG</CODE> and linking
with libcwd, or not defining it and not linking with libcwd.&nbsp;
However, if you are using other libraries that depend on or use libcwd, then these need to
be recompiled too before linking with them.&nbsp;
If you are writing a library yourself then at least distribute a version without debug support, but consider
to also release a developers version with debug support turned on.&nbsp;
Realize however that the meaning of the macro <CODE>CWDEBUG</CODE> is: debugging the
application that is developed <EM>with</EM> a library, not debugging the library
itself! Testing a library is best done with seperate test applications.&nbsp;
If extra debugging code is needed to test the library itself then use a library
specific define like <CODE>DEBUG_YOURLIB</CODE>.</P>

<P>Finally, in order for others to be able to compile your application without having
libcwd installed, you need to include a custom "debug.h" and "sys.h" as is described
in chapter <A HREF="../html/preparation.html#preparation">Preparation</A> of
the <A HREF="../html/reference.html">Reference Manual</A> and in <A HREF="tut2.html">tutorial 2</A>.</P>

__PAGEEND
<P class="line"><IMG width=870 height=23 src="../images/lines/snail.png"></P>
<DIV class="buttons">
<A HREF="tut3.html"><IMG width=64 height=32 src="../images/buttons/lr_prev.png" border="0"></A>
<A HREF="index.html"><IMG width=64 height=32 src="../images/buttons/lr_index.png" border="0"></A>
<A HREF="tut5.html"><IMG width=64 height=32 src="../images/buttons/lr_next.png" border="0"></A>
</DIV>
__PAGEFOOTER
__HTMLFOOTER
