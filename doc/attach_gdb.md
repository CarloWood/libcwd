@addtogroup chapter_attach_gdb

A running program can initiate a gdb session by calling the function <code>attach_gdb()</code>.

For example,

```cpp
Debug(
  if (counter == 31523 && ptr == (void*)0x40013fa0)	// When to start debugging?
    attach_gdb();
);

ptr->foobar++;	// The debugging will start at this line.
```

The function `attach_gdb()` opens an xterm (or whatever is configured in the rcfile)
and starts the gdb session inside it.

After exiting gdb the application will continue running.

In order for `attach_gdb()` to work, you will need to call
<span class="tt">\link chapter_rcfile read_rcfile() \endlink</span>
at the start of your application.
