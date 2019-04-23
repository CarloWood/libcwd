# Define package version here.
define(CW_VERSION_MAJOR, 1)
define(CW_VERSION_MINOR, 1)
define(CW_VERSION_REVISION, 1)
define(CW_PACKAGE_NAME, [libcwd])
define(CW_BUGREPORT, [libcwd@alinoe.com])

# Compile configuration.
define(CW_COMPILE_FLAGS, [-W -Wall -Woverloaded-virtual -Wundef -Wpointer-arith -Wwrite-strings -Wno-inline -Wno-frame-address])
define(CW_THREADS, [no])

# Version info, see http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
#
# CW_INTERFACE_VERSION           : Current interface version, increment whenever the interface is changed.
# CW_INTERFACE_VERSION_REVISION  : Revision, increment every public release; or set to 0 when the interface is changed.
# CW_INTERFACE_AGE               : Increment if interfaces have been added; or set to 0 when interfaces have been removed.

define(CW_INTERFACE_VERSION, 6)
define(CW_INTERFACE_VERSION_REVISION, 0)
define(CW_INTERFACE_AGE, 1)

# cwm4/configure_ac_top.m4 calls CW_OPG_CXXFLAGS, but we want it to call our CW_OPG_FLAGS.
define([CW_OPG_CXXFLAGS], [
  CW_OPG_FLAGS([$1], [$2])
  AC_DISABLE_STATIC
])

# Include cwm4 header.
m4_include([cwm4/configure_ac_top.m4])

# We use doxygen for documentation.
CW_DOXYGEN([])

# Libtool version info
VERSIONINFO="CW_INTERFACE_VERSION:CW_INTERFACE_VERSION_REVISION:CW_INTERFACE_AGE"
AC_SUBST(VERSIONINFO)

# For version code used in check_configuration().
VERSION_MAJOR=CW_VERSION_MAJOR
VERSION_MINOR=CW_VERSION_MINOR
VERSION_REVISION=CW_VERSION_REVISION
AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_REVISION)

# Workaround
true=
AC_SUBST(true)

# Set a decent default value in case we don't have the right libraries.
case "$host" in
  *sparc*) CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET="0" ;;
  *) CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET="-1" ;;
esac
AC_SUBST(CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET)

# Manual configuration options of libcwd.
AC_ARG_ENABLE(libcwd, [
Options of package libcwd:])

libcwd_config_glibcxx_debug=no
AC_ARG_ENABLE(glibcxx_debug,
    [  --enable-glibcxx_debug  compile libcwd with -D_GLIBCXX_DEBUG.
                          See http://gcc.gnu.org/onlinedocs/libstdc++/debug.html.],
    libcwd_config_glibcxx_debug=$enableval)

libcwd_config_alloc=yes
AC_ARG_ENABLE(alloc,
    [  --disable-alloc         disable memory allocation debugging support.],
    libcwd_config_alloc=$enableval)

libcwd_config_magic=yes
AC_ARG_ENABLE(magic,
    [  --disable-magic         disable magic numbers in memory allocation debugging.],
    libcwd_config_magic=$enableval)

AC_ARG_WITH(redzone,
    [  --with-redzone@<:@=ARG@:>@    The minimum size of a redzone in bytes @<:@default=0@:>@],
    [libcwd_config_redzone=$withval], [libcwd_config_redzone=0])
AC_DEFINE_UNQUOTED([LIBCWD_REDZONE], $libcwd_config_redzone, [The minimum size of a redzone in bytes.])

libcwd_config_marker=yes
AC_ARG_ENABLE(marker,
    [  --disable-marker        disable memory allocation markers.],
    libcwd_config_marker=$enableval)

libcwd_config_location=yes
AC_ARG_ENABLE(location,
    [  --disable-location      disable the ability to lookup source file and line
                          number, you might need this on a non-ELF32 system.],
    libcwd_config_location=$enableval)

libcwd_config_debug=no
AC_ARG_ENABLE(debug,
    [  --enable-debug          enable debugging of libcwd itself.],
    libcwd_config_debug=$enableval)

libcwd_config_debug_output=yes
AC_ARG_ENABLE(debug-output,
    [  --disable-debug-output  disable debug output of --enable-debug.],
    libcwd_config_debug_output=$enableval)

libcwd_config_debugm=no
AC_ARG_ENABLE(debugm,
    [  --enable-debugm         enable debugging of memory allocation debugging code
                          in libcwd itself.],
    libcwd_config_debugm=$enableval)

libcwd_config_threading=yes
AC_ARG_ENABLE(threading,
    [  --disable-threading     don't compile libcwd_r, the thread-safe version of
                          libcwd.],
    libcwd_config_threading=$enableval)

libcwd_config_nonthreading=yes
AC_ARG_ENABLE(nonthreading,
    [  --disable-nonthreading  don't compile libcwd, the thread-unsafe version of
                          libcwd.],
    libcwd_config_nonthreading=$enableval)

libcwd_config_debugt=no
AC_ARG_ENABLE(debugt,
    [  --enable-debugt         enable debugging of threading debugging code
                          in libcwd itself.],
    libcwd_config_debugt=$enableval)

AC_ARG_VAR(THREADSMAX, [The maximum number of simultaneously running threads that are supported [1024].])
if test -z "$THREADSMAX"; then
  THREADSMAX=1024
fi
AC_DEFINE_UNQUOTED([CW_THREADSMAX], $THREADSMAX, [The maximum number of simultaneously running threads that are supported.])

# Restore the environment variables used when running `configure' when
# `config.status --recheck' was invoked.
# Always let CXX and CXXCPP override cached values
CW_ENVIRONMENT

# Remember the original environment variable.
CXXFLAGS_env="$CXXFLAGS"

# Check for compiler and preprocessor.
if test -n "$CC"; then
  unset ac_cv_prog_CC
  unset am_cv_CC_dependencies_compiler_type
  unset am_cv_CXX_dependencies_compiler_type
fi
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXXCPP

# __cxa_atexit not being used is always wrong, but not really our concern if we are configured with --disable-alloc.
if test x"$libcwd_config_alloc" = x"yes"; then
dnl Test if g++ was configured correctly.
CW_ATEXITTEST
fi

# Check if C compiler and C++ compiler versions match.
CW_COMPILER_VERSIONS

# Clear the cache values of all remaining tests if important
# environment variables have been changed since last run.
CW_CLEAN_CACHE

# Figure out linker characteristics
CW_RPATH_OPTION

# Checks for other programs.
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PATH_PROG(PS, ps, /bin/ps, $PATH)
# The PS_WIDE_PID_OPTION test is not needed when --disable-location is used.
if test x"$libcwd_config_location" = x"yes"; then
CW_SYS_PS_WIDE_PID_OPTION
fi
AC_DEFINE_UNQUOTED(CW_PATH_PROG_PS, "$PS", [Full path to the 'ps' executable.])

if test -e ./libtool -a "X$enable_shared" = "X"; then
dnl Configuration needed by the testsuite, extract this from the generated 'libtool'
enable_shared=`./libtool --config | grep '^build_libtool_libs=' | sed -e 's/build_libtool_libs=//'`
enable_static=`./libtool --config | grep '^build_old_libs=' | sed -e 's/build_old_libs=//'`
fi

# Used in sys.h to force recompilation.
CC_FINGER_PRINT="$cw_cv_sys_CC_finger_print"
AC_SUBST([CC_FINGER_PRINT])
CXX_FINGER_PRINT="$cw_cv_sys_CXX_finger_print"
AC_SUBST([CXX_FINGER_PRINT])
CXXCPP_FINGER_PRINT="$cw_cv_sys_CXXCPP_finger_print"
AC_SUBST([CXXCPP_FINGER_PRINT])

# Configuration defines of config.h.
AC_SUBST(CW_CONFIG_ALLOC)
CW_CONFIG_ALLOC=0
AC_SUBST(CW_CONFIG_MAGIC)
CW_CONFIG_MAGIC=0
AC_SUBST(CW_CONFIG_GLIBCXX_DEBUG)
CW_CONFIG_GLIBCXX_DEBUG=0
AC_SUBST(CW_CONFIG_LOCATION)
CW_CONFIG_LOCATION=0
AC_SUBST(CW_CONFIG_LIBBFD)
CW_CONFIG_LIBBFD=0
AC_SUBST(CW_CONFIG_DEBUG)
CW_CONFIG_DEBUG=0
AC_SUBST(CW_CONFIG_DEBUGOUTPUT)
CW_CONFIG_DEBUGOUTPUT=0
AC_SUBST(CW_CONFIG_DEBUGM)
CW_CONFIG_DEBUGM=0
AC_SUBST(CW_CONFIG_DEBUGT)
CW_CONFIG_DEBUGT=0
AC_SUBST(CW_CONFIG_MARKER)
CW_CONFIG_MARKER=0
AC_SUBST(CW_CONFIG_HAVE___LIBC_MALLOC)
CW_CONFIG_HAVE___LIBC_MALLOC=undef
AC_SUBST(CW_CONFIG_HAVE__LIBC_MALLOC)
CW_CONFIG_HAVE__LIBC_MALLOC=undef
AC_SUBST(CW_CONFIG_HAVE_DLOPEN)
CW_CONFIG_HAVE_DLOPEN=undef
AC_SUBST(CW_CONFIG_HAVE_POSIX_MEMALIGN)
CW_CONFIG_HAVE_POSIX_MEMALIGN=undef
AC_SUBST(CW_CONFIG_HAVE_MEMALIGN)
CW_CONFIG_HAVE_MEMALIGN=undef
AC_SUBST(CW_CONFIG_HAVE_VALLOC)
CW_CONFIG_HAVE_VALLOC=undef

# Configuration needed by the testsuite.
AC_SUBST(enable_shared)
AC_SUBST(enable_static)
AC_SUBST(libcwd_config_threading)
AC_SUBST(libcwd_config_nonthreading)

if test "$libcwd_config_alloc" = yes; then
  CW_CONFIG_ALLOC=1
fi

if test "$libcwd_config_magic" = yes; then
  CW_CONFIG_MAGIC=1
fi

PCCFLAGS="-DCWDEBUG"
AC_SUBST(PCCFLAGS)
if test "$libcwd_config_glibcxx_debug" = yes; then
  CW_CONFIG_GLIBCXX_DEBUG=1
  CXXFLAGS="$CXXFLAGS -D_GLIBCXX_DEBUG"
  PCCFLAGS="$PCCFLAGS -D_GLIBCXX_DEBUG"
fi

# Set language requirements for the tests below.
AC_LANG_CPLUSPLUS
AC_REQUIRE_CPP

AC_CHECK_FUNCS(__libc_malloc _libc_malloc)

if test "$libcwd_config_location" = yes -o \
    \("$ac_cv_func___libc_malloc" != yes -a "$ac_cv_func__libc_malloc" != yes\) ; then

# Check if we have dlopen in libc or libdl.
AC_CHECK_LIB(c, dlopen, [AC_DEFINE_UNQUOTED(HAVE_DLOPEN, 1, [Defined if dlopen is available.])],
    [AC_CHECK_LIB(dl, dlopen, [LIBS="-ldl $LIBS"; AC_DEFINE_UNQUOTED(HAVE_DLOPEN)])])

fi

if test "$libcwd_config_location" = yes ; then
  CW_CONFIG_LOCATION=1
fi

AC_SUBST(CW_CONFIG_HAVE_PTHREADS)
CW_CONFIG_HAVE_PTHREADS=undef
AC_SUBST(LIB_THREADS)
AC_SUBST(LIB_THREADS_SHARED)
if test "$libcwd_config_threading" = yes; then

  dnl Check if we have POSIX threads
  AC_LANG_SAVE
  AC_LANG_C
  CFLAGS="-pthread"
  AC_SEARCH_LIBS(pthread_create, pthread,
    [AC_CHECK_HEADER(pthread.h,
      [CW_CONFIG_HAVE_PTHREADS=define],
      [AC_MSG_WARN([

**************************************************************************
* Sorry but libcwd only supports POSIX Threads at the moment and I can't *
* find the header file pthread.h.  Only the thread unsafe library libcwd *
* will be compiled.  Use --disable-threading to suppress this warning.   *
**************************************************************************
])
      libcwd_config_threading=no])],
    [AC_MSG_WARN([

**************************************************************************
* Sorry but libcwd only supports POSIX Threads at the moment.  Your libc *
* doesn't include posix threads by itself and I can't find -lpthread.    *
* Use --disable-threading to suppress this warning.                      *
**************************************************************************
])
     libcwd_config_threading=no])
  if test "$libcwd_config_threading" = yes; then
    if test "$ac_cv_search_pthread_create" = "none required"; then
      dnl Even if nothing is required, we still need to use -lpthread explicitly
      dnl while linking a shared library, otherwise the versioning info of
      dnl versioned symbols will be missing!  This is needed for pthread_create
      dnl itself even!.
      AC_CHECK_LIB(pthread, pthread_create, [LIB_THREADS_SHARED=-lpthread])
    fi
  fi

  AC_LANG_RESTORE

  if test "$libcwd_config_threading" = yes; then
    dnl pthread_kill_other_threads_np is a GNU extension, therefore we need to
    dnl define _GNU_SOURCE.  The extension also needs -lpthread to be available.
    AC_DEFINE_UNQUOTED(CW_NEED__GNU_SOURCE, 1, [This is needed when using threading, for example to get 'pthread_kill_other_threads_np'.])
    SAVE_LIBS="$LIBS"
    LIBS="$LIBS $LIB_THREADS $LIB_THREADS_SHARED"
    AC_CHECK_FUNCS(pthread_kill_other_threads_np)
    LIBS="$SAVE_LIBS"
  fi

fi

# This test needs the full set of libraries we just determined.
CW_SYS_BUILTIN_RETURN_ADDRESS_OFFSET

if test "$libcwd_config_debug" = yes; then
  CW_CONFIG_DEBUG=1
fi

if test "$libcwd_config_debug_output" = yes; then
  CW_CONFIG_DEBUGOUTPUT=1
fi

if test "$libcwd_config_debugm" = yes; then
  CW_CONFIG_DEBUGM=1
fi

if test "$libcwd_config_debugt" = yes; then
  CW_CONFIG_DEBUGT=1
fi

if test "$libcwd_config_marker" = yes; then
  CW_CONFIG_MARKER=1
fi

AM_CONDITIONAL(ENABLETHREADING, test x$libcwd_config_threading = xyes)
AM_CONDITIONAL(ENABLENONTHREADING, test x$libcwd_config_nonthreading = xyes)
AM_CONDITIONAL(ENABLESHARED, test x$enable_shared = xyes)
AM_CONDITIONAL(ENABLESTATIC, test x$enable_static = xyes)

# Checks for header files.
AC_HEADER_STAT
AC_CHECK_HEADERS(_G_config.h link.h pwd.h sys/resource.h malloc.h unistd.h)

# Check for global variable.
if test "$ac_cv_header_link_h" = yes; then
  AC_CHECK_FUNCS(_dl_loaded _rtld_global)
fi

# Check for typedefs.
AC_TYPE_UID_T
CW_TYPE_GETGROUPS

# Check for structures.

# Check for compiler/system characteristics.
CW_BUG_REDEFINES([sys/resource.h])
CW_SYS_MALLOC_OVERHEAD
CW_BUG_G_CONFIG_H
CW_SYS_RECURSIVE_BUILTIN_RETURN_ADDRESS
CW_SYS_FRAME_ADDRESS_OFFSET
AC_C_BIGENDIAN

# Check for library functions.
AC_CHECK_FUNCS(getgid getegid getgroups wmemcpy sysconf valloc memalign)
AC_CHECK_DECL([_SC_PAGESIZE],
  AC_DEFINE_UNQUOTED([HAVE__SC_PAGESIZE], 1, [Defined if we have _SC_PAGESIZE.]),
  [], [#include <unistd.h>])
AC_CHECK_DECL([posix_memalign],
  AC_DEFINE_UNQUOTED([HAVE_POSIX_MEMALIGN], 1, [Defined if we have posix_memalign.]), [],
[#define _XOPEN_SOURCE 600
#include <cstdlib>])

if test "$ac_cv_func___libc_malloc" = "yes"; then
  CW_CONFIG_HAVE___LIBC_MALLOC=define
else if test "$ac_cv_func__libc_malloc" = "yes"; then
  CW_CONFIG_HAVE__LIBC_MALLOC=define
fi fi
if test "$ac_cv_lib_c_dlopen" = "yes" -o "$ac_cv_lib_dl_dlopen" = "yes"; then
  CW_CONFIG_HAVE_DLOPEN=define
fi
if test "$ac_cv_func_valloc" = "yes"; then
  CW_CONFIG_HAVE_VALLOC=define
fi
if test "$ac_cv_func_memalign" = "yes"; then
  CW_CONFIG_HAVE_MEMALIGN=define
fi
if test "$ac_cv_have_decl_posix_memalign" = "yes"; then
  CW_CONFIG_HAVE_POSIX_MEMALIGN=define
fi

# Check for system services.

# Set up build directory
test ! -d include && mkdir include

# The install prefix is hardcoded in the library in order to find the default rc file.
libcwd_datarootdir=`eval echo "$datadir"`
libcwd_datadir=`eval echo "$libcwd_datarootdir/$PACKAGE" | sed -e 's|NONE|'$ac_default_prefix'|'`
AC_DEFINE_UNQUOTED([CW_DATADIR], "$libcwd_datadir", [Data directory where the default rcfile is read from.])

# Generate include/libcwd/config.h, include/libcwd/sys.h and include/sys.h, preserving timestamp when they are not changed.
CW_CONFIG_FILE([include/libcwd], [config.h])
CW_CONFIG_FILE([include/libcwd], [sys.h])
CW_CONFIG_FILE([include], [sys.h])

# Output the other configuration files.
AC_CONFIG_FILES([libcwd.spec] [libcwd.lsm] [maintMakefile] [utils/Makefile] [include/Makefile]
		[include/libcwd/Makefile] [testsuite/Makefile] [cutee/Makefile] [tests/Makefile]
		[doc/styles/Makefile] [doc/tutorial/Makefile] [libcwd.pc] [libcwd_r.pc])

# Include cwm4 footer.
m4_include([cwm4/configure_ac_bottom.m4])
