@page preparation Preparation

This page describes the preparations that you need to perform
before starting to use libcwd in your applications source code.

# Step 1: Installing libcwd {#installation}

Binary distributions should be installed the usual way.

If you are installing libcwd from source then please read the
\htmlonly<A HREF="external/INSTALL">\endhtmlonly
INSTALL\htmlonly</A> \endhtmlonly
file that is included in the source distribution.<BR>
See also: \ref group_configuration

\anchor preparation_step2
# Step 2: Creating the custom header files {#header_files}

You need to add two custom header files to your application.<BR>
The recommended names are `"%debug.h"` and `"%sys.h"`.

You can use the following templates for a quick start:

\par sys.h example template
\include "external/sys.h"
\htmlonly &laquo;<A HREF="external/sys.h">download</A>&raquo;\endhtmlonly

\par debug.h example template
\include "external/debug.h"
\htmlonly &laquo;<A HREF="external/debug.h">download</A>&raquo;\endhtmlonly

This %debug.h file is for applications; for more detailed information and for information
for library developers who want to use libcwd, please also read \ref chapter_custom_debug_h.

# Step 3: Adding the cwds submodule {#cwds_submodule}

Add the git submodule `cwds` in the root of your project.&nbsp;
See the \htmlonly<A HREF="https://github.com/CarloWood/cwds#adding-the-cwds-submodule-to-a-project">cwds setup instructions</A>\endhtmlonly
\latexonly cwds setup instructions\endlatexonly for the exact commands.

If you added one or more custom %debug %channels to your namespace
`DEBUGCHANNELS` in your custom "debug.h", then define those
channels in one of your project source files.&nbsp; The initialization code
at the top of main then looks like:

```
int main()
{
#ifdef CWDEBUG
  myproject::debug::init();
#endif
// ...
```

And every thread would call `myproject::debug::init_thread();` when started.
