@addtogroup book_writing_intro

Libcwd is an `ostream` oriented debug output facility.
The class libcwd::DebugObject is associated with a single `ostream`.

Libcwd defines and internally uses only one object of that class, called a @ref the-output-device-debug-object "debug object",
being `libcwd::libcw_do`.

Debug output is written using macros (@ref Dout and @ref DoutFatal),
both of which are defined to use `libcwd::libcw_do`.
More general macros exist (@ref LibcwDout and @ref LibcwDoutFatal) that allow you
to use a different (@ref custom-debug-objects "custom") debug object.

@ref Dout and @ref DoutFatal take two arguments: the first argument is used to specify
@ref controlling-the-output-level-debug-channels "debug channels" and @ref control-flags "control flags"
while the second argument should be a series of objects separated by `<<` that you want to write to the `ostream`.

For example,

```cpp
Dout(dc::notice|blank_label_cf|flush_cf, "Total count: " << count << "; Average: " << average);
```

In this example `dc::notice` is one of the @ref predefined-debug-channels "predefined" debug channels.
Debug channels are intended to control the amount of output of your application:
you can switch the channels on and off.
