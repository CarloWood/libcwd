@addtogroup tutorial-hello-world

The smallest C++ program that prints «<span class="output">Hello World</span>» as *debug output*
to `cerr` is:

```
// This line should actually be part of a custom "debug.h" file.  See tutorial 2.
#include <libcwd/debug.h>

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  // dc::notice can be turned on in the .libcwdrc file, but lets make sure it is here.
  Debug(if (!dc::notice.is_on()) dc::notice.on()); // Turn on the NOTICE Debug Channel.

  Dout(dc::notice, "Hello World");
}
```

Each of the lines of code in this first example program are explained below.

# `#include <libcwd/debug.h>` {#tutorial_hello_world_include}

This header file contains all definitions and declarations that are needed for debug output.
For example, it defines the macros `Debug` and `Dout` and declares the debug object `libcw_do`
and the debug channel `dc::notice`.

**FAQ**

- @ref faq_debug_h "What is defined *exactly* in `libcwd/debug.h`?"

# `Debug(NAMESPACE_DEBUG::init());` {#tutorial_hello_world_main_reached}

This call checks that the libcwd header files that are being used belong to the
`libcwd.so` shared object that the application linked with.
It is also required before any debug info can be loaded of the respective loaded shared objects.

# `Debug(dc::notice.on());` {#tutorial_hello_world_turn_on_channel}

This turns on the <u>D</u>ebug <u>C</u>hannel `dc::notice`.
Without this line, the code `Dout(dc::notice, "Hello World")` would output
nothing: all *Debug Channels* are *off* by default, at start up.

**FAQ**

- @ref faq_debug "Why do I need to type the `Debug(  )` around it?"
- @ref faq_debugchannels "Which Debug Channels exist? Can I make my own?"
- @ref faq_recursive "Can I turn Debug Channels off again? Can I do that recursively?"
- @ref faq_channel "Why do you call it a Debug *Channel*? What *is* a Debug Channel?"

# `Debug(libcw_do.on());`

This turns on the <u>D</u>ebug <u>O</u>bject `libcw_do`.
Without this line, the code `Dout(dc::notice, "Hello World")` would output
nothing: all *Debug Objects* are *off* by default, at start up.

A *Debug Object* is related to exactly one `ostream`.
`libcwd` defines only one *Debug Object* by itself (being `libcw_do`),
this is enough for most applications.
The default ostream is `cerr`.
Using the macro `Dout` causes debug output to be written to `libcw_do`.

**FAQ**

- @ref faq_debug "Why do I need to type the `Debug(  )` around it?"
- @ref faq_owndebugobject "Can I make my own Debug Object?"
- @ref faq_recursive2 "Can I turn Debug Objects off again? Can I do that recursively?"
- @ref faq_setostream "How do I set a new `ostream` for a given Debug Object?"
- @ref faq_whyoff "Why are Debug Objects turned off at creation?"
- @ref faq_order "Why do you turn on the debug object after you enable a debug channel?"
- @ref faq_object "Why do you call it a Debug *Object*? What *is* a Debug Object?"

# `Dout(dc::notice, "Hello World");`

This outputs "Hello World" to the `ostream` currently related to
`libcw_do` provided that the *Debug Channel* `dc::notice` is turned on.

Output is written as if everything in the second field of the macro `Dout` is
written to an ostream; it is "equivalent" with: `cerr << "Hello World" << '\n';`

**FAQ**

- @ref faq_macros "Why is `Dout` a macro and not a template?"
- @ref faq_semicolon "Do I need to type that semi-colon after the macro? Why isn't it part of the macro?"
- @ref faq_libcwdout "I made my own Debug Object, can I still use `Dout`?"
- @ref faq_evaluation "Is the second field of the macro still evaluated when the Debug Channel and/or Debug Object are turned off?"
- @ref faq_suppress "Can I suppress that new-line character?"

Continue with @ref tutorial-channels.
