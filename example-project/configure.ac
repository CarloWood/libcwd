# Package version 1.0.0.
define(CW_VERSION_MAJOR, 1)
define(CW_VERSION_MINOR, 0)
define(CW_VERSION_REVISION, 0)

# Package name and bug reporting.
define(CW_PACKAGE_NAME, [project])
define(CW_BUGREPORT, [project-bugs@yourmail.org])

# CW_COMPILE_FLAGS may contain warning flags, but also default debug flags (-g*)
# and optimization flags (-O*) that which will be stripped when not required.
define(CW_COMPILE_FLAGS, [-W -Wall -Woverloaded-virtual -Wundef -Wpointer-arith -Wwrite-strings -Winline])

# CW_THREADS can be [no] (single-threaded), [yes] (multi-threaded) or [both] (single and multi-threaded applications).
define(CW_THREADS, [yes])

# CW_MAX_ERRORS is the maximum number of errors the compiler will show.
define(CW_MAX_ERRORS, [2])

# We have to specify the AUX_DIR, or else libtoolize will install ltmain.sh in the root of the parent project (..).
# This is only necessary when running ./autogen.sh inside the example-project however; not when you create your
# own project directory.
AC_CONFIG_AUX_DIR([.])

# Include cwm4 header.
m4_include([cwm4/configure_ac_top.m4])

# Require -std=c++17 (because of how debug.h defines NAMESPACE_DEBUG_START).
AX_CXX_COMPILE_STDCXX_17([noext], [mandatory])

# Comment this out if the project is using doxygen to generate documentation.
#CW_DOXYGEN([])

# Output files.
AC_CONFIG_FILES([src/Makefile])

# Include cwm4 footer.
m4_include([cwm4/configure_ac_bottom.m4])
