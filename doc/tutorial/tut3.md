@addtogroup tutorial-ostream

You can write the debug output to any given ostream.
The following example opens a file `log` and
uses it to write its debug output to.

Compile as: \shellcommand g++ -DCWDEBUG log_file.cc -lcwd -o log_file \endshellcommand

```
#include <fstream>
#include "debug.h"    // See tutorial 2.

int main()
{
  Debug(main_reached());
  Debug(dc::notice.on());
  Debug(libcw_do.on());

#ifdef CWDEBUG
  std::ofstream file;
  file.open("log");
#endif

  // Set the ostream related with libcw_do to `file':
  Debug(libcw_do.set_ostream(&file));

  Dout(dc::notice, "Hippopotamus are heavy");
}
```

Debug code like the definition of the debug file `file`,
should be put between `#ifdef CWDEBUG ... #endif` as usual.
This isn't needed for `Debug()` or `Dout()`
because these macros are automatically replaced with white space
when `CWDEBUG` is not defined.

Continue with @ref tutorial-cwdebug.
