@defgroup quick-reference Quick Reference
@brief A one-page cheat sheet for the most common libcwd tasks.

@internal
A one-page cheat sheet for the most common libcwd tasks: installing the library,
wiring it into a fresh project, and writing the first debug output lines.
Every entry below links to the corresponding @ref reference-manual "Reference Manual"
section for the full story.

This is a top-level group (a sibling of `reference-manual`, not a child of it)
so that scripts/fix_navtree.py promotes it to a root-level sidebar entry, right next to
the Reference Manual. Keep it a leaf group: do not add @defgroup children here, or the
group__quick-reference.html rename in the build would need a lazy-load .js counterpart.
@endinternal

# Installation

* Read the \htmlonly<A HREF="external/INSTALL">INSTALL</A>\endhtmlonly file.

See also: @ref configuration-options-and-macros.

# Using libcwd with a new project

* Add \htmlonly<A HREF="external/sys.h">sys.h</A>\endhtmlonly and
  \htmlonly<A HREF="external/debug.h">debug.h</A>\endhtmlonly as header files to root of your application.
  (An `#include "sys.h"` / `#include "%debug.h"` must find them before any other include directory
  with those file names; the `CMakeLists.txt` of `cwds` takes care of that if you put them in the root).
* Also add the git submodule `cwds` in the root of your project by following
  the \htmlonly<A HREF="https://github.com/CarloWood/cwds#adding-the-cwds-submodule-to-a-project">cwds setup instructions</A>\endhtmlonly.
* Add `#include "sys.h"` to the top of every source file in your application.
* Add `#include "debug.h"` to every source file needing debug output/code.
* Dump the following at the start of your `main()` function:

\include main-examples.cc

# Debug Output

* Add a `.libcwdrc` file to your HOME directory. For example,

\include doxygen-examples/home_libcwdrc

Note how this turns *all* debug channels off by default.
This allows you to set an environment variable, `LIBCWD_RCFILE_OVERRIDE_NAME` to point to
a project/test specific file which then only has to bother itself with turning on
channels. For example,

```
LIBCWD_RCFILE_OVERRIDE_NAME="$REPOROOT/libcwdrc_my_project"
```
that could contains just
```
# This is an override file; just define the debug channels that we need.

# libcwd default debug channels.
#channels_off = elfutils,demangler
channels_on = warning,debug,notice,system

# cwds
#channels_off = tracked,system,usage
channels_on = restart

# utils
#channels_off = semaphore,delayloop,tracker,semaphorestats

# threadpool
#channels_off = action,threadpool,threadpoolloop

# evio
#channels_off = evio,endofmsg,decoder,io
```

Now `dc::notice` is turned on by default, e.g.
```
Dout(dc::notice, "Hello" << ' ' << "World");
Dout(dc::notice|blank_label_cf|error_cf, "Hello World");
```

* Other @ref control-flags "Control flags".
