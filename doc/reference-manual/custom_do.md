@addtogroup chapter_custom_do

Each %debug object is associated with one `ostream`.
The default %debug output macros @ref Dout and @ref DoutFatal use the \em default debug object libcwd::libcw_do.
Other %debug objects may be created as global objects;
it is convenient to define new macros for each (custom) %debug object using the generic macros
@ref LibcwDout and @ref LibcwDoutFatal.

For example, add something like the following to your own @link chapter_custom_debug_h "debug.h" @endlink file:

```cpp
#ifdef CWDEBUG
extern libcwd::DebugObject my_debug_object;
#define MyDout(cntrl, data) LibcwDout(DEBUGCHANNELS, my_debug_object, cntrl, data)
#define MyDoutFatal(cntrl, data) LibcwDoutFatal(DEBUGCHANNELS, my_debug_object, cntrl, data)
#else // !CWDEBUG
#define MyDout(a, b)
#define MyDoutFatal(a, b) LibcwDoutFatal(::std,, a, b)
#endif // !CWDEBUG
```

@sa @ref libcwd::libcw_do
