@addtogroup tutorial-cwdebug

In a production environment you most likely want to remove all debugging code from
the application because it makes the application slower, of course.
You can do this by compiling the final application *without* defining the macro `CWDEBUG`.
You also don't need to link with libcwd in that case.

In example, with debugging code you'd compile a program as

\shellcommand g++ -g -DCWDEBUG application.cc -lcwd -o application \endshellcommand

and without debugging code you'd use

\shellcommand g++ -O3 application.cc -o application \endshellcommand

Because the final application doesn't need libcwd, we can afford to use features of g++ and
third party libraries that developers are able to install but that would be impractical as demand for
the end-user of the application.
The *developer* that uses libcwd will have to use g++ as compiler and perhaps use a linux box for
the development of the application, but gets a lot of developing advantages in return.
Afterwards it is relatively easy to port the bug-free code to other platforms/compilers.

As a developer you need to know two things:

1. How to compile the application you are developing, while you're still developing it.
2. How to compile a final version of your application, without debugging support.

In the first case, all libraries and your application need to be compiled with `CWDEBUG` defined.
In the second case all libraries, except libcwd (because you won't link with it) and your application, need to be compiled
with `CWDEBUG` undefined.

Libraries that do not use libcwd (or otherwise depend on the macro `CWDEBUG`) do not need to be recompiled of course.
If you are writing an end-application and do not have other libraries that depend on
libcwd (or otherwise depend on the macro `CWDEBUG`)
then it is as easy as defining `CWDEBUG` and linking
with libcwd, or not defining it and not linking with libcwd.
However, if you are using other libraries that depend on or use libcwd, then these need to
be recompiled too before linking with them.
If you are writing a library yourself then at least distribute a version without debug support, but consider
to also release a developers version with debug support turned on.
Realize however that the meaning of the macro `CWDEBUG` is: debugging the
application that is developed *with* a library, not debugging the library
itself! Testing a library is best done with separate test applications.
If extra debugging code is needed to test the library itself then use a library
specific define like `DEBUG_YOURLIB`.

Finally, in order for others to be able to compile your application without having
libcwd installed, you need to include a custom "debug.h" as is described
in the @ref preparation "Preparation" chapter of
the @ref reference-manual "Reference Manual" and in @ref tutorial-channels "tutorial 2".

Continue with @ref tutorial-advanced.
