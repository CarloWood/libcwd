@addtogroup tutorial-channels

You can easily create your own debug channels.
In the example below we create a debug channel `dc::ghost`
that will use the string "<span class="output">GHOST</span>" as label.

Create a file `"debug.h"` that is part of your application and put in it:

```
#ifndef DEBUG_H
#define DEBUG_H

#ifndef CWDEBUG

#include <iostream>
#include <cstdlib>

#define Debug(...) do { } while(0)
#define Dout(cntrl, ...)
#define DoutFatal(cntrl, ...) LibcwDoutFatal(, , cntrl, __VA_ARGS__)
#define ForAllDebugChannels(...)
#define ForAllDebugObjects(...)
#define LibcwDebug(dc_namespace, ...) do { } while(0)
#define LibcwDout(dc_namespace, d, cntrl, ...)
#define LibcwDoutFatal(dc_namespace, d, cntrl, ...) \
  do                                                \
  {                                                 \
    ::std::cerr << __VA_ARGS__ << ::std::endl;      \
    ::std::exit(EXIT_FAILURE);                      \
  } while (1)
#define LibcwdForAllDebugChannels(dc_namespace, ...)
#define LibcwdForAllDebugObjects(dc_namespace, ...)
#define CWDEBUG_LOCATION 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGT 0

#else // CWDEBUG

// This must be defined before <libcwd/debug.h> is included and must
// be the name of the namespace containing your `dc' namespace (see below).
// You can use any namespace(s) you like, except existing namespaces
// (like std or libcwd). It may not start with two colons because it is used
// for both `namespace LIBCWD_DEBUG_CHANNELS {` as well as
// `using namespace ::LIBCWD_DEBUG_CHANNELS`: the first namespace is implied
// to be a global namespace already.
#define LIBCWD_DEBUG_CHANNELS myproject::debug::channels
#include <libcwd/debug.h>

NAMESPACE_DEBUG_CHANNELS_START

// Add the declaration of new debug channels here
// and their definition in a custom debug.cpp file.
extern Channel custom;

NAMESPACE_DEBUG_CHANNELS_END

#endif // CWDEBUG
#endif // DEBUG_H
```

Finally write the program:

```
#include "debug.h"

NAMESPACE_DEBUG_CHANNELS_START          // In this case this is example::channels
Channel ghost("GHOST");                 // Create our own Debug Channel.
NAMESPACE_DEBUG_CHANNELS_END

int main()
{
  Debug(NAMESPACE_DEBUG::init());       // Mandatory call to notify the library that
                                        // main() was reached, check that the correct
                                        // headers are being used and to read the rcfile.

  // Lets not forget to turn the debug Channel and Object on!
  Debug(if (!dc::ghost.is_on()) dc::ghost.on()); // Might already be on due to rcfile.
  Debug(libcw_do.on());

  for (int i = 0; i < 4; ++i)
    Dout(dc::ghost, "i = " << i);       // We can write more than just
                                        // "Hello World" to the ostream :)
}
```

This program outputs:

```
GHOST   : i = 0
GHOST   : i = 1
GHOST   : i = 2
GHOST   : i = 3
```

Note that when writing a *library* you are highly advised to follow the namespace guideline
as set forth in the @ref the-custom-debug-h-file "Reference Manual" (in particular its
*Libraries* section).

**FAQ**

- @ref faq_label "What is the maximum length of a label?"
- @ref faq_prefix "Why do I have to use the `dc::` prefix?"
- @ref faq_labelwidth "Why does it print spaces between the label and the colon? How is the field width of the label determined?"

Continue with @ref tutorial-ostream.
