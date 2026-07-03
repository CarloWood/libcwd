@defgroup tutorial Tutorial
@brief A step-by-step introduction to using libcwd.

Hello, my name is [Carlo Wood](https://carlowood.github.io/) and I will guide you through a few example
programs in order to help familiarize you with `libcwd`.

Hopefully you have already installed the library and the header files.
If not, then do so now - see the \htmlonly<A HREF="external/INSTALL">INSTALL\endhtmlonly file that is
\htmlonly</A>\endhtmlonly included in the \htmlonly<A HREF="https://github.com/CarloWood/libcwd/releases">distribution package</A>\endhtmlonly
for detailed instructions, or the @ref preparation "Preparation" chapter of the @ref reference-manual.

This tutorial is mixed with a lot of FAQ-style questions, which are linked to a more detailed explanation
in the @ref tutorial-faq.
For a correct understanding of the tutorial it is *not* needed to read these FAQs; they are only provided
for your convenience, only read them if you really want to know the answer.

The tutorial is a six-part learning sequence (each part builds on the previous one):

- @ref tutorial-hello-world — print your first line of debug output.
- @ref tutorial-channels — create your own debug channels.
- @ref tutorial-ostream — write debug output to any ostream.
- @ref tutorial-cwdebug — compile with and without debugging support.
- @ref tutorial-advanced — loop over channels, combine them, and format output.
- @ref tutorial-threads — debug multi-threaded applications.

@internal This file defines the navigation-tree structure and the order in which the tutorial
parts appear in it. Each part is an @ingroup of `tutorial`; tut5 is generated into the build tree
(tut5.md) by the moo.awk output-capture pipeline, the rest are static markdown authored here. @endinternal

@defgroup tutorial-hello-world Tutorial 1: Hello World
@ingroup tutorial
@brief Print your first line of debug output.

@defgroup tutorial-channels Tutorial 2: Creating Your Own Debug Channels
@ingroup tutorial
@brief Define project-specific debug channels.

@defgroup tutorial-ostream Tutorial 3: Setting The ostream
@ingroup tutorial
@brief Redirect debug output to any ostream.

@defgroup tutorial-cwdebug Tutorial 4: Management Of CWDEBUG
@ingroup tutorial
@brief Compile with and without debugging support.

@defgroup tutorial-advanced Tutorial 5: Advanced Examples
@ingroup tutorial
@brief Loop over channels, combine them, and format and interrupt debug output.

@defgroup tutorial-threads Tutorial 6: Debugging Threaded Applications
@ingroup tutorial
@brief Thread-safety rules and multi-threaded debug output.

@defgroup tutorial-faq FAQ
@ingroup tutorial
@brief Exhaustive answers to the questions raised by the tutorial.
