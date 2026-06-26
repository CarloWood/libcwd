@addtogroup function-objects

## Defining and initializing `Function` objects

### Defining

`Function` objects need to be defined as static or global objects
otherwise they will be re-initialized every time you call the
`Function::init()` method.

```cpp
Function f1;            // Global `Function' instance.

void f()
{
  static Function f2;   // Static `Function' instance.
  f2.init();
}
```

Each `Function` instance needs to be initialized at least once before
it can be used.  There are several ways to do this all of which
use the overloaded `Function::init()` method.

When you try to use an uninitialized `Function` object,
the program will fail by default (exit with an error
message).  If you don't want that then you can add a
flag to its constructor:

```cpp
Function f1(Function::nofail);
```

### Initializing

First of all, it is possible to assign the *current function* to
a `Function` object like so:

```cpp
void f()
{
  f1.init();            // f1 represents the current function, f().
  // ...
}
```

If this function is a template function (or a method in
a templated class) then it is possible that it covers
more than one instance.  Therefore, in the case of
a template you need to make the function object a
static instance inside the template function: this
will lead to one instance per template function
instantiation.

In order to be able to refer to it later in that case
you must use another `Function` to alias for
the static instance:

```cpp
template<typename T>
void f(T const& a)
{
  static Function sf;   // Do NOT call sf.init()!
  f1.init(sf);          // f1 represents all `void f<T>(T const&)'
                        //   template function instantiations that
  // ...                //   have been called so far.
}
```

Note that in this case the *static* object must **not** be initialized!

### Searches

If you need to refer to functions that are not called yet, or when
those functions are part of third party libraries, then you'll need
to use search routines.  These are much slower, especially
the ones that use the demangled names; but each lookup will be done
only once.

The following examples demonstrate several ways to search
for function symbols.  These initialization calls are best
put early in main() of course.  All of them will print
debug output about what they are looking up and what they
found.  By default they will fail when nothing is found.

```cpp
f1.init(Function::regexp("^int g(.*)$")); // f1 represents all functions 'g' returning an int.

f1.init(Function::exactmatch("int h()")); // f1 represents the function 'int h()'.

f1.init(Function::mangled("_ZTv0_n12_NSoD0Ev"); // Look up by mangled name (exact matches only).
```

The `mangled` lookup is the fastest.
It looks for both, C++ as well as C functions, so you could use it to look for C functions skipping the cpu intensive demangling that way.
However, you can also specifically specify that the function you are looking for has C linkage by passing a flag to the search.

Flags are always the right-most parameter.

```cpp
f1.init(Function::regexp("malloc"), Function::c_linkage);
```

`regexp` and `exactmatch` look only for C++ functions by default.

### Labels

It's possible to link an arbitrary *label* (an unsigned integer constant) to a `Function` object.
This can be useful when you want to specifically mark a `Function` temporarily;
you are allowed to reassign labels and/or remove the labels again.

For example,

```cpp
// Add this to debug.h.
enum function_labels {
  f_x,
  f_y
};

// Elsewhere...
void f()
{
  f1.init();            // f1 represents the current function
  f1.label(f_x);        // and assign the label f_x to it.

  x();

  f1.label(f_y);        // Change the label, setting it to f_y.

  y();

  f1.rmlabel();
}
```

Functions `x()` and `y()` can retrieve the label set on `f1`,
and as such find out whether this function (`f1`) called `x()` or `y()`.
This might be needed when for example `x()` also calls `y()`.

**Important**: The label is not removed when you exit function `f()`!
You'll have to explicitely do so yourself if you want to keep track of the fact that the current function exited
(otherwise subsequential calls to `y()` could think that it still was called from `f()`).
