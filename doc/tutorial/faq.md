@addtogroup tutorial-faq

Warning: The amount of information included in this FAQ is exhaustive.
Do NOT read it except as a replacement for self-torture. Instead read the
@ref tutorial and skip all the references to this FAQ unless you find yourself
banging your head into the wall asking yourself the same question as is listed
in the tutorial. In that case a link will bring you here to read just that one
question.

# 1. Won't this define make my code non-portable? {#faq_gnu_source}

No, not unless you actually use the GNU extensions in parts of your
application that need to be portable (like non-debugging code).
While debugging the application you will only benefit from using as
much compiler support as you can get, allowing the compiler to tell you
what could possibly be wrong with your code.
Once the application works, you don't have to define _GNU_SOURCE
because you won't be including the debug code anymore, nor link with
libcwd.
Note that GNU g++ 3.x already defines this macro currently itself as a hack
to get the libstdc++ headers to work properly, hence the test with `#ifndef`
is always needed (see [this gcc mailing list post](http://gcc.gnu.org/ml/gcc/2002-02/msg00996.html)).

# 4. What is defined *exactly* in `libcwd/debug.h`? {#faq_debug_h}

Everything.
Go and read the @ref reference-manual "Reference Manual" to get *all* gory details if you dare.

# 5. Why are you using macros for `Debug` and `Dout`? {#faq_macros}

Because it is the only way to easily remove debugging code from an application as a function of a macro
and because it allows for the fastest possible code even without optimisation, which is often the case
while debugging.

# 6. Why do I need to type the `Debug(  )` around it? {#faq_debug}

The macro `Debug()` is used for two things. 1) The code inside it is only included
when the macro `CWDEBUG` is defined. 2) It includes the namespace `libcwd`.

As a result, you don't have to add `#ifdef CWDEBUG ... #endif` around the code and
in most cases you don't have to type `libcwd`.
The expression `Debug(STATEMENT);` is equivalent with:

```
#ifdef CWDEBUG
  do {
    using namespace ::libcwd;
    using namespace ::LIBCWD_DEBUG_CHANNELS; // LIBCWD_DEBUG_CHANNELS as defined, by default NAMESPACE_DEBUG::NAMESPACE_CHANNELS.
    { STATEMENT; }
  } while(0);
#endif
```

Please note that definitions within a `Debug()` statement will be
restricted to their own scope.
Please read the @ref the-custom-debug-h-file "Reference Manual" for an
explanation of `LIBCWD_DEBUG_CHANNELS`.

# 7. Which Debug Channels exist? Can I make my own? {#faq_debugchannels}

This question is covered in the @ref controlling-the-output-level-debug-channels "Controlling The Output Level (Debug Channels)"
chapter of the Reference Manual.
As is described there, creating your own debug channels is best done by writing your own `debug.h`
header file. The following template is a good start for such a `debug.h` for an end application
(a library needs more work):

```
#ifndef MY_DEBUG_H
#define MY_DEBUG_H

#define LIBCWD_DEBUG_CHANNELS myapplication::debug::channels
#include <libcwd/debug.h>

namespace myapplication {
  namespace debug {
    namespace channels {
      namespace dc {
        using namespace ::libcwd::channels::dc;
        extern ::libcwd::Channel mychannel;
        // ... more channels here
      }
    }
  }
}

#endif // MY_DEBUG_H
```

Replace «`MY_DEBUG_H`»,
«`myapplication::debug`» and «`mychannel`» with your own names.

See the `example-project` that comes
with the source distribution of libcwd for a Real Life example.

# 8. Can I turn Debug Channels off again? Can I do that recursively? {#faq_recursive}

Debug channels can be switched on and off at any time. At the start of your program you should
turn on the channels of your choice by calling `Debug(dc::*channel*.on())`
*once*. Sometimes you want to temporarily turn off certain channels: you want to make
sure that no debug output is written to that particular debug channel, at that moment. This can be
achieved by calling the methods `off()` and `on()` in
*pairs* and in that order. For example:

```
  // Make sure no notice debug output is generated:
  Debug(dc::notice.off());
  do_something_noisy();
  // ... do stuff ...
  Debug(dc::notice.on());
```

This will work even when "do stuff" calls a function that also turns `dc::notice` off and on:
after the call to `on()` the debug channel can still be off: it is restored to the on/off state
that it was in before the corresponding call to `off()`. In fact, the calls to
`off()` and `on()` only respectively increment and decrement a counter.

# 9. Why do you call it a Debug *Channel*? What *is* a Debug Channel? {#faq_channel}

A Debug Channel is a fictitious "news channel". It should contain information of a certain kind that is
interesting or not interesting as a whole. A Debug Channel is not a device or stream, a single debug channel is best
viewed upon as a single bit in a bitmask. Every time you write debug output you have to specify a "bitmask"
which specifies when that message is written; like when you are cross posting to usenet news groups, specifying multiple
news groups for a single message. When any of the specified Debug Channels is turned on, then the message is written
to the output stream of the underlying debug object.

# 10. Can I make my own Debug Object? {#faq_owndebugobject}

@ref custom-debug-objects "Yes", you can make as many debug objects as you like.
Each debug object is associated with one ostream. However, the default debug output macros `Dout` and
`DoutFatal` use the default debug object `libcw_do`.
It isn't hard at all to define your own macros though; for example add something like the following to
@ref the-custom-debug-h-file "your own \"debug.h\"" file:

```
#ifdef CWDEBUG
extern libcwd::DebugObject my_debug_object;
#define MyDout(cntrl, ...) LibcwDout(LIBCWD_DEBUG_CHANNELS, my_debug_object, cntrl, __VA_ARGS__)
#define MyDoutFatal(cntrl, ...) LibcwDoutFatal(LIBCWD_DEBUG_CHANNELS, my_debug_object, cntrl, __VA_ARGS__)
#else // !CWDEBUG
#define MyDout(a, ...)
#define MyDoutFatal(a, ...) LibcwDoutFatal(::std, /*nothing*/, a, __VA_ARGS__)
#endif // CWDEBUG
```

# 11. Can I turn Debug Objects off again? Can I do that recursively? {#faq_recursive2}

Debug objects can be switched on and off at any time. At the start of your program you should
turn on the debug object(s) by calling `Debug(*debugobject*.on())`
*once*. Sometimes you want to temporarily turn off all debug output.
This can be achieved by calling the methods `off()` and `on()` in
*pairs* and in that order. For example:

```
  // Disable all debug output to `libcw_do':
  Debug(libcw_do.off());
  // ... do stuff ...
  Debug(libcw_do.on());
```

This will work even when "do stuff" calls a function that also turns `libcw_do` off and on:
after the call to `on()` the debug object can still be off: it is restored to the on/off state
that it was in before the corresponding call to `off()`. In fact, the calls to
`off()` and `on()` only respectively increment and decrement a counter.

# 12. How do I set a new `ostream` for a given Debug Object? {#faq_setostream}

You can change the `ostream` that is associated with a Debug Object at any time.
For example, changing the `ostream` of `libcw_do` from the
default `cerr` to `cout`:

```
  Debug(libcw_do.set_ostream(&cout)); // Does not change the mutex that is currently in use.
```

See also @ref tutorial-ostream "tutorial 3".

# 13. Why are Debug Objects turned off at creation? {#faq_whyoff}

The Debug Objects and Debug Channels are global objects. Because libcwd could not be
dependent on libc, they do not use libc's `Global<>` template.
As a result, the order in which the debug channels and objects are initialized is
unknown; moreover, other global objects whose constructors might try to write debug output could
be constructed before the debug objects are initialized! The debug objects are therefore
designed in a way that independent of their internal state of initialisation they function without
crashing. It should be obvious that the only way this could be achieved was by creating them
in the state *off*.

# 14. Why do you turn on the debug object after you enable a debug channel, why not the other way around? {#faq_order}

The order in which Debug Channels and Debug Objects are turned on does not matter at all.
At most, when you think about the Debug Object as the «main switch» then it seems to make
sense to first play with the little channel switches before finally activating the complete Debug
machinery. Others might think more in the lines of: lets start with setting the debug object
*on* before I forget it. That is a bit *too* fuzzy (logic) for me though ;)

# 15. Why do you call it a Debug *Object*? What *is* a Debug Object? {#faq_object}

Good question. It can't be because I wasn't creative, I am very creative.
Note that I didn't think of *Object* as in OOP (*that* would be uncreative)
but more along the lines of an *object*, like in science fiction stories -- objects
you can't get around. The monolith of [2001: A Space Odyssey](https://www.imdb.com/title/tt0062622/)
is a good example I guess.

Unlike the monolith however, a Debug Object is not mysterious at all. Basically it
is a pointer to an `ostream` with a few extra attributes added to
give it an internal state for 'on' (pass output on) and 'off' (don't pass output on) as well
as some formatting information of how to write the data that is passed on to its
`ostream`.

# 16. Do I need to type that semi-colon after the macro? Why isn't it part of the macro? {#faq_semicolon}

Yes, that semi-colon needs to be there.
It was chosen not to include the semi-colon in the macro because this way it looks
a bit like a function call which feels more natural.

The code `Dout(dc::notice, "Hello World");` is definitely
a *statement* and therefore needs to end on a semi-colon (after expansion). When the
macro `CWDEBUG` is not defined, the macro is replaced with
whitespace but still has to be a statement: it must be a single semi-colon then.

For example,

```
  if (error)
    Dout(dc::notice, "An error occurred");

  exit(0);
  cerr << "We should never reach this\n";
```

If the complete line `Dout(dc::notice, "An error occurred");`,
including semi-colon is removed (replaced with whitespace), then the line
`exit(0);` would be executed only when `error` is
`true`! And when the semi-colon would be included in
the macro then people could easily be tempted to add a semi-colon anyway (because it
looks so much better), which would break code like:

```
  if (error)
    Dout(dc::notice, "An error occurred");
  else
    cout << "Everything is ok\n";
```

because after macro expansion that would become:

```
  if (error)
    ;
    ;
  else  // <-- syntax error
    cout << "Everything is ok\n";
```

# 17. I made my own Debug Object, can I still use `Dout`? {#faq_libcwdout}

No, macro `Dout` et al. use exclusively the debug object that
comes with libcwd. It is easy to define your own macros however (see @ref faq_owndebugobject "above").
You are free to *redefine* the `Dout` macros however, just realize that libcwd
will continue to use its own debug object (`libcw_do`), debug output written by libcwd
in its header files do not use the `Dout` macro (especially in order to allow you
to redefine it).

# 18. Is the second field of the macro still evaluated when the Debug Channel and/or Debug Object are turned off? {#faq_evaluation}

No! And that is a direct result of the fact that `Dout` et al. are *macros*.
Indeed this fact could therefore be a little confusing.
In pseudo-code the macro expansion looks something like

```
  if (debug object and any of the debug channels are turned on)
    the_ostream << your message;
```

and so, "your message" is *not* evaluated when it isn't also
actually written. This fact is also covered in the @ref the-output-device-debug-object "Reference Manual".

Note that debug code should never have an effect on any of your variables (and thus on the application) anyway.
In the production version of your application all debug code will be removed and you don't want it to behave differently then!

# 19. Can I suppress that new-line character? {#faq_suppress}

Yes, and a lot more. See @ref tutorial_advanced_formatting "tutorial 5.4".

# 20. What is the maximum length of a label? {#faq_label}

The maximum length of the label of a new Debug Channel is given
by the constant `libcwd::max_label_len_c`.
At this moment that is 16.

# 21. Why do I have to use the `dc::` prefix? {#faq_prefix}

This is a complex reason. Basically because of a flaw in the design of namespaces in C++.
Namespaces have been introduced in order to avoid name collisions, which was a good thing.
It doesn't make much sense if you constantly have to type `::somelibrary::debug::channel::notice`
of course, then you could as well have avoided the name space problem by using
`somelibrary_debug_channel_notice` right?
Therefore you don't have to type the name of the namespace that is "current".
There can be only *one* namespace current at a time however. The result is that
this cannot be used to solve our problem: We want to avoid both, name collisions between debug channels
and any other variable or function name, but *also* between debug channels defined in
different libraries. That means we need more than one namespace: A namespace for each of the
libraries. We cannot make all of them current however. Worse, we cannot
make any namespace current because it must be possible to add code that writes debug output
*everywhere*. We can only use the `using namespace`
directive. Now here is the real flaw: A `using namespace` directive
gives no priority whatsoever to names when resolving them, for example, you can't do this:

```
namespace base {
  int base1;
  int base2;
}

namespace derived {
  using namespace base;
  int derived1;
  char base1;
}

  // ...
  using namespace derived;
  base1 = 'a';
```

because C++ will make absolutely no difference between variables defined in
`derived` and variables defined in `base`
but will complain that `base1` is ambiguous.

The only opening that the ANSI/ISO C++ Standard allows us here is in the
following phrase:

> Given `X::m` (where `X` is a user-declared namespace),
> or given `::m` (where `X` is the global namespace),
> let `S` be the set of all declarations of `m` in `X`
> and in the transitive closure of all namespaces nominated by *using-directive*s in X and its used namespaces,
> except that *using-directive*s are ignored in any namespace, including `X`,
> directly containing one or more declarations of `m`.
> No namespace is searched more than once in the lookup of a name. If `S` is the empty set,
> the program is ill-formed. Otherwise, if `S` has exactly one member,
> or if the context of the reference is a using-declaration, `S` is
> the required set of declarations of `m`.
> Otherwise if the use of `m` is not one that allows a unique
> declaration to be chosen from `S`, the program is ill-formed.

Replace `X` with `dc::`
(obviously we don't want to put the debug channels in global namespace)
and we can use this rule to at least select a specific channel by using
the trick that the used `dc` namespace is *not the same*
namespace for the different libraries. Then we can use debug channels with
the same name in `dc` namespaces in *different*
namespaces in different libraries and use the namespaces of *one* library
at a time to select the current `dc` namespace.

If this is over your head then that is probably because I can't explain :).
Don't worry however, you only need to know *how* to introduce new debug
channels and not understand how it works. The correct procedure is described
in the @ref controlling-the-output-level-debug-channels "Reference Manual".

# 22. Can I put my debug channels in my own name space? {#faq_ownnamespace}

Yes. How, is described in the @ref controlling-the-output-level-debug-channels "Reference Manual".
For some background information on why this has to be so complex, please read the @ref faq_prefix "previous question".

# 23. Why does it print spaces between the label and the colon? How is the field width of the label determined? {#faq_labelwidth}

The colon is indented so it ends up in the same column for all existing debug channels.
Hence, the longest label of all existing/created debug channels determines the number of spaces.
This value can be less than the @ref faq_label "maximum allowed label size" of course.
