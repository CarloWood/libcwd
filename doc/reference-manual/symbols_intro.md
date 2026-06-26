@addtogroup introduction-types-and-symbols

Libcwd reads the symbol table of the application and of each
of the linked object files upon initialization.&nbsp;
It then allows you to translate program counter addresses to
function names, source file names and line numbers.&nbsp;
You can also print demangled names of any symbol or type, making
the debug output better human readable.&nbsp;

**Example 1: printing the location that a function was called from:**

```cpp
#ifdef CWDEBUG
// Get the location that we were called from.
libcwd::Location location((char*)__builtin_return_address(0)
    + libcwd::builtin_return_address_offset);
// Demangle the function name of the location that we were called from.
std::string demangled_function_name;
libcwd::demangle_symbol(location.mangled_function_name(), demangled_function_name);
// Print it.
Dout(dc::notice, "This function was called from " << demangled_function_name << '(' << location << ')');
#endif
```

**Example 2: Printing the demangled name of the current (template) function:**

```cpp
// If we are in template Foo<TYPE>::f()
Dout(dc::notice, "We are in Foo<" << type_info_of<TYPE>().demangled_name() << ">::f()");
```

Note that calling @ref libcwd::demangle_symbol costs cpu every time you call it, but using
@ref libcwd::type_info_of<> does not cost any cpu: the demangling is done once, during the
initialization of libcwd; @ref libcwd::type_info_of<> merely returns a static pointer.
