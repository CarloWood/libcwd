@addtogroup group_debug_channels

Whenever debug output is written, one or more *debug channels* must be specified.
The debug output is then written to the ostream of the debug object
unless the debug object is turned off or when all specified *debug channels* are off.
Each *debug channel* can be turned on and off independently.

Libcwd has defined six *debug channels* in namespace \link libcwd::channels::dc channels::dc \endlink
(See @ref group_default_dc).
New *debug channels* can be defined by the user, which is as simple as creating
a new Channel object.

Example,

```cpp
namespace dc {
Channel mychan("MYLABEL");
}
```

This declaration must be inside the namespace @ref DEBUGCHANNELS.

Multiple *debug channels* can be given by using `operator|` between the channel names.
This shouldn't be read as \`or' but merely be seen as the bit-wise
OR operation on the bit-masks that these channels actually represent.

@sa group_control_flags

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
namespace DEBUGCHANNELS::dc {
libcwd::Channel hello("HELLO");
} // namespace DEBUGCHANNELS::dc

Dout(dc::hello, "Hello World!");
Dout(dc::kernel|dc::io, "This is written when either the dc::kernel "
    "or dc::io channel is turned on.");
```

gives as result

\exampleoutput <PRE class="example-output">
HELLO : Hello World!
KERNEL: This is written when either the kernel or io channel is turned on.</PRE>
\endexampleoutput
