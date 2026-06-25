@addtogroup chapter_rcfile

In order to use <span class="tt">@link chapter_attach_gdb attach_gdb()@endlink</span> and/or to read debug channel states
from an external file, you should call `read_rcfile()`,
for example,

```cpp
int main()
{
  Debug(reached_main());
  Debug(libcw_do.on());     // In order to get RCFILE messages.
  Debug(read_rcfile());
```

`read_rcfile()` can be used to turn on or off debug *channels*, but you still have to turn on the
debug *object*(s); in particular the debug object <span class="tt">@link libcwd::libcw_do libcw_do@endlink</span>
that is used to print WARNING messages in case something is wrong with your rcfile.

The default rcfile is
\filename .libcwdrc \endfilename (you can change that by setting
the environment variable <span class="tt">@link chapter_environment LIBCWD_RCFILE_NAME@endlink</span>
).
The application will first attempt to read this file from the *current directory*.
If that fails then it will try to read the rcfile from the users *home directory*.
If that fails too then it will fall back to reading \filename ${CMAKE_INSTALL_DATADIR}/libcwd \endfilename.
You can use the latter as a template and/or
\htmlonly<A HREF="../external/libcwdrc">\endhtmlonly example\htmlonly</A>\endhtmlonly
file for writing your own rcfile.

If the environment variable <span class="tt">@link chapter_environment LIBCWD_RCFILE_OVERRIDE_NAME@endlink</span>
is set, it will be read after the above file where any setting overwrites a previous one.
Using `channels_on` even once, in this file, will reset all previously turned on channels,
although additional `channels_on` lines accumulate again.
