@addtogroup the-custom-debug-h-file

@section debug_channels_and_namespace Debug channels and namespace

@subsection applications Applications

User applications have less strict requirements than libraries, because nobody else will link with it.
The developer of an application directly controls and checks and resolves name collisions when needed.
If you are writing an end-application then you are still urged to create a header file
called `%debug.h` and use *that* in your source files, instead of including `<libcwd/debug.h>` directly.
You will benefit greatly from this in terms on flexibility.

This custom `%debug.h` normally lives in the root of your project; what really matters is that its
include directory is searched *before* the `cwds` directory, so that a plain `#include "%debug.h"`
from any source file picks up your version, if it exists, and only includes `cwds/debug.h` if your
project doesn't define its own `%debug.h`.

The custom `%debug.h` should include `cwds/debug.h` which sets up the surrounding machinery
(the `Debug`, `Dout` and `ASSERT` macros, the `dc` namespace) for both the `CWDEBUG`
*and* non-`CWDEBUG` case: turning most macros into no-ops. Furthermore, your `%debug.h` is the place
where you get to influence that setup *before* it happens.

Having your own `%debug.h` is handy because it is the natural, single point where you can:

- Pick the namespace in which libcwd exposes its application-facing debug utilities by defining
  `NAMESPACE_DEBUG` (it defaults to `debug` when left undefined).
  Using a project-specific namespace keeps your debug code cleanly separated from that of any
  library you link with.
- Declare project-wide debug channels, so that every translation unit can gain access to them
  with a simple `#include "%debug.h"`.
- Add project-specific debug macros, ostream formatters for your own types, or any other helper
  that you want available wherever you write debug output.

A recommended skeleton defines `NAMESPACE_DEBUG`, includes `cwds/debug.h` to piggy-back on its
boilerplate, and then declares the project-wide debug channels.
Note that each declared channel still has to be *defined* in exactly one `.cpp` file:

@include "external/debug.h"
\htmlonly &laquo;<A HREF="external/debug.h">download</A>&raquo;\endhtmlonly

@subsection libraries Libraries

If you are developing a library that uses libcwd then do not put your debug channels in the
libcwd::channels::dc namespace.
The correct way to declare new debug channels is by putting them in a namespace of the library itself.
Also end-applications will benefit by using this method (in terms of flexibility).

The following code would define a debug channel `warp` in the namespace `libexample`:

```cpp
// This is some .cpp file of your library.
#define LIBCWD_DEBUG_CHANNELS libexample::channels
#include "debug.h"
// ...
#ifdef CWDEBUG
namespace LIBCWD_DEBUG_CHANNELS::dc {
libcwd::Channel warp("WARP");
// Add new channels here...
} // namespace LIBCWD_DEBUG_CHANNELS::dc
#endif
```

Next provide two debug header files (both named %debug.h), one for installation
with the library headers (ie libexample/debug.h) and one in an include directory
that is only used while compiling your library itself - this one would not be installed.

The first one (the %debug.h that will be installed) would look something like this:

```cpp
// This is for example <libexample/debug.h>
#ifndef LIBEXAMPLE_DEBUG_H
#define LIBEXAMPLE_DEBUG_H

#ifdef CWDEBUG
#include <libcwd/libraries_debug.h>

namespace libexample::channels::dc {
using namespace libcwd::channels::dc;
extern libcwd::Channel warp;
// Add new channels here...
} // namespace libexample::channels::dc

// Define private debug output macros for use in header files of the library,
// there is no reason to do this for normal applications.
// We use a literal libexample::channels here and not LIBCWD_DEBUG_CHANNELS!
#define LibexampleDebug(...) LibcwDebug(libexample::channels, __VA_ARGS__)
#define LibexampleDout(cntrl, ...) LibcwDout(libexample::channels, libcwd::libcw_do, cntrl, __VA_ARGS__)
#define LibexampleDoutFatal(cntrl, ...) LibcwDoutFatal(libexample::channels, libcwd::libcw_do, cntrl, __VA_ARGS__)
#define LibexampleForAllDebugChannels(...) LibcwdForAllDebugChannels(libexample::channels, __VA_ARGS__)
#define LibexampleForAllDebugObjects(...) LibcwdForAllDebugObjects(libexample::channels, __VA_ARGS__)

// All other macros might be used in header files of libexample, but need to be
// defined by the debug.h of the application that uses it.
// LIBEXAMPLE_INTERNAL is defined when the library itself is being compiled (see below).
#if !defined(Debug) && !defined(LIBEXAMPLE_INTERNAL)
#error The application source file (e.g. .cpp) must use '#include "debug.h"' _before_ including the header file that it includes now, that led to this error.
#endif

#else

#define LibexampleDebug(...) do { } while(0)
#define LibexampleDout(cntrl, ...) do { } while(0)
#define LibexampleDoutFatal(cntrl, ...) do { ::std::cerr << __VA_ARGS__ << ::std::endl; ::std::exit(EXIT_FAILURE); } while(1)
#define LibexampleForAllDebugChannels(...) do { } while(0)
#define LibexampleForAllDebugObjects(...) do { } while(0)

#endif // CWDEBUG

#endif // LIBEXAMPLE_DEBUG_H
```

The second "debug.h", which would not be installed but only be included when compiling
the .cpp files (that `#include "%debug.h"`) of your library itself, then looks like this:

\htmlonly
<div class="doxygen-awesome-fragment-wrapper">
\endhtmlonly
<PRE class="fragment">\#ifndef DEBUG_H
\#define DEBUG_H
&nbsp;
\#ifndef CWDEBUG
&nbsp;
\#include &lt;iostream&gt;           // std::cerr
\#include &lt;cstdlib&gt;            // std::exit, EXIT_FAILURE
\verbinclude "nodebug.h"\#endif // CWDEBUG
&nbsp;
\#define LIBEXAMPLE_INTERNAL   // See above.
\#include &lt;libexample/debug.h&gt; // The %debug.h shown above.
\#define LIBCWD_DEBUG_CHANNELS libexample::channels
\#ifdef CWDEBUG
\#include &lt;libcwd/debug.h&gt;
\#endif
&nbsp;
\#endif // DEBUG_H
</PRE>
\htmlonly
</div>
\endhtmlonly

@subsection header_files_of_libraries Header files of libraries

Don't use Dout etc. in header files of libraries, instead use (for example) LibExampleDout etc., as shown above.
If you want to use Dout etc. in your *source* files then you can do so
after first including the `"%debug.h"` as shown above.

@subsection debug_channel_name_collisions Debug channel name collisions

The reason that libcwd uses the convention to put debug channels in the namespace dc
is to avoid collisions between debug channel names of libraries.
There are two types of name collisions possible: you upgrade or start to use a library which uses a debug channel
that you had already defined, in that case you might need to change the name of your own channel,
or you link with two or more libraries that both use libcwd and that defined the same debug channel,
in that case you will have to make your own debug namespace as shown above and choose a new name for one of the channels.

For example, suppose you link with two libraries: lib1 and lib2 who use the above convention and defined their
own namespaces called lib1 and lib2, but both defined a debug channel called foobar.
Then you can rename these channels as follows.
Make a debug header file that contains:

```cpp
#define LIBCWD_DEBUG_CHANNELS application::channels
#include <lib1/debug.h>
#include <lib2/debug.h>
namespace LIBCWD_DEBUG_CHANNELS::dc {
using namespace lib1::channels::dc;
using namespace lib2::channels::dc;
static libcwd::Channel& foobar1(lib1::channels::dc::foobar);
static libcwd::Channel& foobar2(lib2::channels::dc::foobar);
} // namespace LIBCWD_DEBUG_CHANNELS::dc
```

\htmlonly
<DIV class="normal">
\endhtmlonly
The hiding mechanism of the above "cascading" of debug channel declarations of libraries works as follows.
The debug macros use a using-directive to include the scope `LIBCWD_DEBUG_CHANNELS`, which is
what you defined that or to `libcwd::channels` otherwise.
All debug channels are specified as `dc::channelname` in the source code and the `namespace dc` will
be uniquely defined.
For instance, in the case of the above example, when writing `dc::%notice` the `dc` will be unambiguously
resolved to `application::debug::channels::dc`, because it is the only `dc` namespace in `LIBCWD_DEBUG_CHANNELS`
(`application::debug::channels`).
The C++ standard states: "During the lookup of a name qualified by a namespace name, declarations that would otherwise be made
visible by a using-directive can be hidden by declarations with the same name in the namespace containing the using-directive;".
This allows us to put a list of using-directives in `application::debug::channels::dc` and then hide any collision by
redefining it in `application::debug::channels::dc` itself, either as new debug channel, or as reference to one of the
%debug %channels of the library of choice.
\htmlonly
</DIV>
\endhtmlonly
