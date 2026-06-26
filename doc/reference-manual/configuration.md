@addtogroup group_configuration

Libcwd uses CMake for configuration.
You can list cached options with \shellcommand cmake -LAH <build-dir> \endshellcommand.

This section describes the CMake options specific to libcwd.
The name of the macros that are related to the respective features are given between parenthesis after the
option. You can not define these macros yourself, you may only use them in an `#if ... #endif`
test. The macros are always defined; when the corresponding CMake option was enabled then the macro is defined
to `1`, otherwise it is defined to 0. This makes it possible for the compiler to warn you when you
made a typo in the name of a macro (add `-Wundef` to your compile flags for that).

**Example:**

```cpp
// Use '#if' not '#ifdef'.
#if CWDEBUG_LOCATION
Dout(dc::notice, "Called from " << libcwd::Location(__builtin_return_address(0)));
#endif
```

## How to configure libcwd when using gitache

If you're using [gitache](https://github.com/CarloWood/gitache) to **download/configure/compile/install**
libcwd, put the following content (relative to the root of your project) in `cmake/gitache-configs/libcwd_r.cmake`:

```
gitache_config(
  GIT_REPOSITORY
    "https://github.com/CarloWood/libcwd.git"
  GIT_TAG
    "master"
)
```

Optionally add a `CMAKE_ARGS`. For example, to toggle all default options:
```
  CMAKE_ARGS
    "-DEnableGlibCxxDebug=ON -DEnableLibcwdLocation=OFF -DEnableLibcwdDebug=ON -DEnableLibcwdDebugOutput=ON"
```
