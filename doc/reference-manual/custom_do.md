@addtogroup custom-debug-objects

Each %debug object is associated with one `ostream`.
The default %debug output macros @ref Dout and @ref DoutFatal use the \em default debug object libcwd::libcw_do.
Other %debug objects may be created as global objects;
it is convenient to define new macros for each (custom) %debug object using the generic macros
@ref LibcwDout and @ref LibcwDoutFatal.

For example, add something like the following to your own @link the-custom-debug-h-file "debug.h" @endlink file:

```cpp
#ifdef CWDEBUG
extern libcwd::DebugObject my_debug_object;
#define MyDout(cntrl, ...) LibcwDout(LIBCWD_DEBUG_CHANNELS, my_debug_object, cntrl, __VA_ARGS__)
#define MyDoutFatal(cntrl, ...) LibcwDoutFatal(LIBCWD_DEBUG_CHANNELS, my_debug_object, cntrl, __VA_ARGS__)
#else // !CWDEBUG
#define MyDout(a, ...)
#define MyDoutFatal(a, ...) LibcwDoutFatal(std,, a, __VA_ARGS__)
#endif // !CWDEBUG
```

@sa @ref libcwd::libcw_do
