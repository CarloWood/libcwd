\namespace libcwd
\brief namespace for libcwd.

The d in libcwd stands for Debugging. The cw stand for the initials of the designer/developer of
this life-span project.

This namespace contains all user accessible classes, functions and variables.
Things defined in this namespace belong to the libcwd API and will be supported through-out version 2.x.

\namespace libcwd::channels
\brief The default DEBUGCHANNELS namespace.

This macro should contain the namespace where your application defines the \link preparation dc \endlink namespace;
If you want to define additional %debug %channels, next to the ones already \link libcwd::channels::dc provided \endlink
by libcwd, then the macro DEBUGCHANNELS must be defined in the custom \link preparation debug.h \endlink
header file of your application (or library) prior to including `<libcwd/debug.h>`.

\sa \ref preparation

\namespace libcwd::channels::dc
\brief This namespace contains the standard %debug %channels of libcwd.

Custom %debug %channels should be added in another namespace in order to
avoid the possibility of collisions with %channels defined in other libraries.

\sa \ref chapter_custom_debug_h
