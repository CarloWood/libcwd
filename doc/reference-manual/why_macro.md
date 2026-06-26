@addtogroup page_why_macro

This page describes why we use a macro instead of an inline
function for `Dout()`.

Good C++ code whenever possible, should not use macros but `inline` functions which
have the advantage of type checking, overloading and easier debugging with a debugger.
It is therefore a very relevant question to ask why a macro was used for `Dout()`.

Using a macro has its advantages however:

* It inlines the debug code even when we don't use optimization.
* It allows us to use tricks that make the code run faster when debug code is included.
* It allows us to omit variables in the program that are only used for debugging and which are written to a debug `ostream`.
* It compiles faster when debugging is omitted.
* No optimization is needed to really get rid of the debug code when debugging is omitted.

Points 1, 2 and 3 are the most important reasons that lead to the decision to use a macro.
Please note that  the author of %libcwd used the alternative for **two years** before finally deciding
to **rewrite** the debug facility, being convinced that it was better to do it the way it is done now.
While points 4 and 5 are trivial, the first three advantages might need some explanation:

1. Usually a developer won't use compiler optimization because that makes debugging harder.
In most cases the debug code will be compiled and used <em>without</em> compiler optimization; implying
the fact that no inlining is done.
Moreover, we expect to use a lot of inserter operators and without
optimization, each of these will be called.
[ Note that when omitting debug code we can't get rid of the content of all
`%operator<<` functions because they might be used for
non-debug code too, so we'd need to use a trick and write to
something else than an `ostream` (let's say to a class `no_dstream`).
Each call to `template<class T> no_dstream %operator<<(T) { };` would actually be done(!),
without inlining (point&nbsp;5&nbsp;above)&nbsp;].

2. Let's develop step by step an example where we write debug output.

Let's start with writing some example debug output to `cerr`.

```cpp
std::cerr << "i = " << i << "; j = " << j << "; s = " << s << std::endl;
```

This line calls seven functions. If we want to save CPU time when we
**don't** want to write this output, then we need to test
whether the debugging output (for this specific channel) is turned
on or off **before** calling the `%operator<<()`'s.
After all, such an operator call can use a lot of CPU time for arbitrary objects.

We cannot pass `"i = " << i << "; j = " << j << "; s = " << s << std::endl`
to an inline function without causing all `%operator<<`
functions to be called.
The only way, not using a macro, to achieve that no `%operator<<` is called is by not calling them
in the first place: We can't write to an `ostream`.
It is necessary to write to a new class (let's call that class `dstream`) which
checks if the debug channel we are writing to is turned on before
calling the `ostream %operator<<()`:

```cpp
template<class T>
inline dstream&
operator<<(dstream& ds, T const& data)
{
  if (on)
    std::cerr << data;
}
```

Nevertheless, even with inlining (often requiring the highest level of optimization), most compilers would turn that into:

```cpp
if (on)
  std::cerr << "i = ";
if (on)
  std::cerr << i;
if (on)
  std::cerr << "; j = ";
if (on)
  std::cerr << j;
if (on)
  std::cerr << "; s = ";
if (on)
  std::cerr << s;
if (on)
  std::cerr << std::endl;
```

checking `on` seven times.

With a macro we can easily achieve the best result, even without any optimization:

```cpp
if (on)
  std::cerr << "i = " << i << "; j = " << j << "; s = " << s << std::endl;
```

3. Sometimes a variable is specific to debug code and only being used for writing to a debug ostream.
When the debug is omitted and inline functions are used for the `Dout()` calls, then these
variables still need to exist: they are passed to the `Dout()`
function (or actually, to the `no_dstream %operator<<()`'s).
Using a macro allows one to really get rid of such variables by
surrounding them with `#ifdef CWDEBUG ... #endif` preprocessor directives.

Example:

```cpp
if (need_close)
{
  Dout(dc::system|continued_cf, "close(" << __fd << ") = ");
  [[maybe_unused]] int ret = ::close(__fd);
  Dout(dc::finish|cond_error_cf(ret < 0), ret);
  return false;
}
```
