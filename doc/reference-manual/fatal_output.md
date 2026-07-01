@addtogroup fatal-debug-output

Often an application needs to be terminated when a fatal error occurs (whether or not `CWDEBUG` is defined).
Libcwd defines for these cases the macro DoutFatal.

This allows you to write

```cpp
if (error)
  DoutFatal(dc::core|error_cf, "An error occurred");
```

instead of the equivalent

```cpp
if (error)
{
  Dout(dc::core|error_cf, "An error occurred");
  std::cerr << "An error occurred" << std::endl;
  _Exit(180);
}
```

The big difference with Dout is that DoutFatal is not replaced with white space when
the macro `CWDEBUG` is not defined.

There are two %debug %channels that can be used together with DoutFatal:
<span class="tt">@link libcwd::channels::dc::fatal dc::fatal@endlink</span> and
<span class="tt">@link libcwd::channels::dc::core dc::core@endlink</span>.
The first terminates by calling `_Exit(180)`,
the second terminates by `std::abort()`, causing the application to core dump.

@sa @ref predefined-debug-channels
 \n @ref control-flags
 \n @link custom-debug-objects Defining your own debug objects @endlink
