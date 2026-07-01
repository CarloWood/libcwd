@addtogroup controlling-the-output-level-debug-channels

Whenever debug output is written, one or more *debug channels* must be specified.
The debug output is then written to the ostream of the debug object
unless the debug object is turned off or when all specified *debug channels* are off.
Each *debug channel* can be turned on and off independently.

Libcwd has defined six *debug channels* in namespace `libcwd::channels::dc` (See @ref predefined-debug-channels).
New *debug channels* can be defined by the user in namespace <span class="tt">@link LIBCWD_DEBUG_CHANNELS LIBCWD_DEBUG_CHANNELS::dc@endlink</span>,
which is as simple as creating a new <span class="tt">@link libcwd::Channel Channel@endlink</span> object.

Example,

```cpp
NAMESPACE_DEBUG_CHANNELS_START
Channel mychan("MYLABEL");
NAMESPACE_DEBUG_CHANNELS_END
```

Multiple *debug channels* can be given by using `operator|` between the channel names.
This shouldn't be read as \`or' but merely be seen as the bit-wise
OR operation on the bit-masks that these channels actually represent.

@sa control-flags

**Example:**

```cpp
Dout(dc::notice, "Libcwd is a great library");
```

gives as result

\exampleoutput <PRE class="example-output">
NOTICE: Libcwd is a great library</PRE>
\endexampleoutput

and

```cpp
#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
libcwd::Channel hello("HELLO");
NAMESPACE_DEBUG_CHANNELS_END

Dout(dc::hello, "Hello World!");
Dout(dc::kernel|dc::io, "This is written when either the dc::kernel "
    "or dc::io channel is turned on.");
```

gives as result

\exampleoutput <PRE class="example-output">
HELLO : Hello World!
KERNEL: This is written when either the kernel or io channel is turned on.</PRE>
\endexampleoutput
