dnl Process this file with autoconf to produce a configure script.
AC_REVISION($Revision$)dnl

dnl General intialization of `autoconf' variables.

dnl Package name and version
AC_INIT(libcwd, [0.99.46])

dnl Put resulting configuration defines in this header file.
AC_CONFIG_HEADERS([config.h])

dnl Version info
dnl
dnl INTERFACE_VERSION		: Current interface version, increment whenever the interface is changed.
dnl INTERFACE_VERSION_REVISION	: Revision, increment every public release; or set to 0 when the interface is changed.
dnl INTERFACE_AGE		: Increment if interfaces have been added; or set to 0 when interfaces have been removed.

INTERFACE_VERSION=99		dnl 0.99 actually
INTERFACE_VERSION_REVISION=46
INTERFACE_AGE=99		dnl 0.99 actually

dnl Ensure that the directory specified with --srcdir was correct
dnl by demanding that it contains `debugmalloc.cc'.
AC_CONFIG_SRCDIR(debugmalloc.cc)

dnl Automake options.
AM_INIT_AUTOMAKE
dnl This project uses a lot of variables that start with CW_ (FIXME).
m4_pattern_allow(CW_)

dnl Include maintainer mode targets
AM_MAINTAINER_MODE

dnl Demand at least version 2.57 of autoconf.
AC_PREREQ(2.57)

dnl Because we use cwautomacros.
CW_AUTOMACROS

VERSION_MAJOR=`expr $INTERFACE_VERSION - $INTERFACE_AGE`
VERSION_MINOR=$INTERFACE_AGE
VERSION_REVISION=$INTERFACE_VERSION_REVISION

dnl Libtool version info
VERSIONINFO="$INTERFACE_VERSION:$INTERFACE_VERSION_REVISION:$INTERFACE_AGE"
AC_SUBST(VERSIONINFO)

dnl For version code used in check_configuration().
AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_REVISION)

dnl Workaround
true=
AC_SUBST(true)

dnl Set a decent default value in case we don't have the right libraries
case "$host" in
  *sparc*) CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET="0" ;;
  *) CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET="-1" ;;
esac
AC_SUBST(CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET)

dnl Manual configuration options of libcwd.
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

AC_ARG_ENABLE(pch,
    [  --disable-pch           disable precompiled header support @<:@auto detect for
                          g++@:>@],
    libcwd_config_pch=$enableval)

dnl Process default options.
CW_OPG_FLAGS

AC_ARG_VAR(THREADSMAX, [The maximum number of simultaneously running threads that are supported [1024].])
if test -z "$THREADSMAX"; then
  THREADSMAX=1024
fi
AC_DEFINE_UNQUOTED([CW_THREADSMAX], $THREADSMAX, [The maximum number of simultaneously running threads that are supported.])

dnl Restore the environment variables used when running `configure' when
dnl `config.status --recheck' was invoked.
dnl Always let CXX and CXXCPP override cached values
CW_ENVIRONMENT

dnl Remember the original environment variable.
CXXFLAGS_env="$CXXFLAGS"

dnl Check for compiler and preprocessor
if test -n "$CC"; then
  unset ac_cv_prog_CC
  unset am_cv_CC_dependencies_compiler_type
  unset am_cv_CXX_dependencies_compiler_type
fi
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXX
AC_PROG_CXXCPP

# __cxa_atexit not being used is always wrong, but not really our concern if we are configured with --disable-alloc.
if test x"$libcwd_config_alloc" = x"yes"; then
dnl Test if g++ was configured correctly.
CW_ATEXITTEST
fi

dnl Check if C compiler and C++ compiler versions match.
CW_COMPILER_VERSIONS

dnl Clear the cache values of all remaining tests if important
dnl environment variables have been changed since last run.
CW_CLEAN_CACHE

dnl Figure out linker characteristics
CW_RPATH_OPTION

dnl Checks for other programs.
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PATH_PROG(PS, ps, /bin/ps, $PATH)
# The PS_WIDE_PID_OPTION test is not needed when --disable-location is used.
if test x"$libcwd_config_location" = x"yes"; then
CW_SYS_PS_WIDE_PID_OPTION
fi
AC_DEFINE_UNQUOTED(CW_PATH_PROG_PS, "$PS", [Full path to the 'ps' executable.])

dnl This block cannot be replaced by a macro because the DISABLE_* and the
dnl AC_PROG_LIBTOOL macro need to be in configure.ac itself for some reason.
dnl Generate libtool (also checks shared/static prerequisites)
if expr "$host" : ".*openbsd.*" >/dev/null; then
  dnl openBSD has a buggy ld, no way that a shared library will work.
  AC_DISABLE_SHARED
else
  dnl Disable building static libraries by default
  AC_DISABLE_STATIC
fi
test -n "$enable_shared" || enable_shared=yes
test -n "$enable_static" || enable_static=yes
dnl Note: due to a bug in libtool-1.3c, the language must be C at this point!
AC_PROG_LIBTOOL
dnl Configuration needed by the testsuite, extract this from the generated 'libtool'
enable_shared=`./libtool --config | grep '^build_libtool_libs=' | sed -e 's/build_libtool_libs=//'`
enable_static=`./libtool --config | grep '^build_old_libs=' | sed -e 's/build_old_libs=//'`
if test $enable_shared = yes; then
  if expr "$host" : ".*openbsd.*" >/dev/null; then
    AC_MSG_ERROR([

The dynamic linker (ld) on openBSD is too buggy for C++ shared libraries.
Static libraries are supported however, please configure using
--enable-static --disable-shared, which is the default for openBSD.
])
  fi
fi
dnl libtool has a bug that it adds an -rpath for the $libdir, we don't want
dnl that because the only programs we have are test executables that are
dnl never installed anyway!  There is no option to turn this off other than
dnl by directly messing with the just generated libtool.  The trick used here
dnl is to add $libdir (or what it will become later) to `sys_lib_dlsearch_path_spec`,
dnl causing libtool to think that a hard coded path is not needed.
sys_lib_dlsearch_path_spec=`./libtool --config | grep '^sys_lib_dlsearch_path_spec=' | sed -e 's/sys_lib_dlsearch_path_spec="\(.*\)"$/\1/'`
tmp=$libdir
test "x$tmp" = 'x${exec_prefix}/lib' && tmp=$exec_prefix/lib
test "x$tmp" = 'xNONE/lib' && tmp=$prefix/lib
test "x$tmp" = 'xNONE/lib' && tmp=$ac_default_prefix/lib
dq='"'
dnl " <-- quote needed for vim.
sq="'"
se='|'
eval sed -e ${sq}s${se}^sys_lib_dlsearch_path_spec=.*${se}sys_lib_dlsearch_path_spec=${dq}$sys_lib_dlsearch_path_spec $tmp${dq}${se}${sq} libtool > libtool.tmp && mv libtool.tmp libtool

dnl Used in sys.h to force recompilation
CC_FINGER_PRINT="$cw_cv_sys_CC_finger_print"
AC_SUBST([CC_FINGER_PRINT])
CXX_FINGER_PRINT="$cw_cv_sys_CXX_finger_print"
AC_SUBST([CXX_FINGER_PRINT])
CXXCPP_FINGER_PRINT="$cw_cv_sys_CXXCPP_finger_print"
AC_SUBST([CXXCPP_FINGER_PRINT])

dnl Configuration defines of config.h
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

dnl Configuration needed by the testsuite
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

if test "$libcwd_config_glibcxx_debug" = yes; then
  CW_CONFIG_GLIBCXX_DEBUG=1
  CXXFLAGS="$CXXFLAGS -D_GLIBCXX_DEBUG"
fi

dnl Set language requirements for the tests below.
AC_LANG_CPLUSPLUS
AC_REQUIRE_CPP

AC_CHECK_FUNCS(__libc_malloc _libc_malloc)

if test "$libcwd_config_location" = yes -o \
    \("$ac_cv_func___libc_malloc" != yes -a "$ac_cv_func__libc_malloc" != yes\) ; then

dnl Check if we have dlopen in libc or libdl
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

dnl This test needs the full set of libraries we just determined.
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

dnl Checks for header files.
AC_HEADER_STAT
AC_CHECK_HEADERS(_G_config.h link.h pwd.h sys/resource.h malloc.h unistd.h)

dnl Check for global variable.
if test "$ac_cv_header_link_h" = yes; then
  AC_CHECK_FUNCS(_dl_loaded _rtld_global)
fi

dnl Check for typedefs.
AC_TYPE_UID_T
CW_TYPE_GETGROUPS

dnl Check for structures.

dnl Check for compiler/system characteristics.
CW_BUG_REDEFINES([sys/resource.h])
CW_SYS_MALLOC_OVERHEAD
CW_BUG_G_CONFIG_H
CW_SYS_RECURSIVE_BUILTIN_RETURN_ADDRESS
CW_SYS_FRAME_ADDRESS_OFFSET
AC_C_BIGENDIAN

dnl Check if compiler supports PCH.
AC_CACHE_CHECK([for compiler with PCH support],
  [cw_cv_prog_CXX_pch],
  [cw_save_CXXFLAGS="$CXXFLAGS"
   CXXFLAGS="$CXXFLAGS -Werror -Winvalid-pch -Wno-deprecated"
   AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   echo '#include <math.h>' > conftest.h
   rm -f conftest.h.gch
   if $CXX $CXXFLAGS $CPPFLAGS -x c++-header conftest.h \
	  -c -o conftest.h.gch 1>&5 2>&1 &&
      echo '#error "pch failed"' > conftest.h &&
      echo '#include "conftest.h"' > conftest.cc &&
     $CXX -c $CXXFLAGS $CPPFLAGS conftest.cc 1>&5 2>&1 ;
   then
     if test x"$libcwd_config_pch" != x"no"; then
       cw_cv_prog_CXX_pch=yes
     else
       cw_cv_prog_CXX_pch=disabled
     fi
   else
     cw_cv_prog_CXX_pch=no
   fi
   rm -f conftest*
   CXXFLAGS=$cw_save_CXXFLAGS
   AC_LANG_RESTORE
  ])
AM_CONDITIONAL(BUILD_PCH, test "$cw_cv_prog_CXX_pch" = "yes")

dnl Check for library functions.
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

dnl Check for system services.

dnl Set up build directory
test ! -d include && mkdir include

dnl Set up rpm directory when in maintainer mode
CW_SETUP_RPM_DIRS

dnl The install prefix is hardcoded in the library in order to find the default rc file.
libcwd_datarootdir=`eval echo "$datadir"`
libcwd_datadir=`eval echo "$libcwd_datarootdir/$PACKAGE" | sed -e 's|NONE|'$ac_default_prefix'|'`
AC_DEFINE_UNQUOTED([CW_DATADIR], "$libcwd_datadir", [Data directory where the default rcfile is read from.])

dnl These options are needed, but cannot be used during the configure tests.
PCHOPTS=
if test "$cw_cv_prog_CXX_pch" = "yes"; then
  PCHOPTS="-include pch.h"
fi
AC_SUBST(PCHOPTS)

dnl Generate include/libcwd/config.h, include/libcwd/sys.h and include/sys.h, preserving timestamp when they are not changed.
AC_CONFIG_FILES(include/libcwd/timestamp-config.h:include/libcwd/config.h.in, [
cd include/libcwd
if cmp -s config.h timestamp-config.h 2> /dev/null; then
  echo "config.status: config.h is unchanged"
else
  echo "config.status: creating config.h"
  cp timestamp-config.h config.h
fi
touch timestamp-config.h
cd ../..
])
AC_CONFIG_FILES(include/libcwd/timestamp-sys.h:include/libcwd/sys.h.in, [
cd include/libcwd
if cmp -s sys.h timestamp-sys.h 2> /dev/null; then
  echo "config.status: sys.h is unchanged"
else
  echo "config.status: creating sys.h"
  cp timestamp-sys.h sys.h
fi
touch timestamp-sys.h
cd ../..
])
AC_CONFIG_FILES(include/timestamp-sys.h:include/sys.h.in, [
cd include
if cmp -s sys.h timestamp-sys.h 2> /dev/null; then
  echo "config.status: sys.h is unchanged"
else
  echo "config.status: creating sys.h"
  cp timestamp-sys.h sys.h
fi
touch timestamp-sys.h
cd ..
])
dnl Output the other configuration files.
AC_CONFIG_FILES([libcwd.spec] [libcwd.lsm] [maintMakefile] [Makefile] [utils/Makefile] [include/Makefile]
		[include/libcwd/Makefile] [testsuite/Makefile] [cutee/Makefile] [tests/Makefile]
		[documentation/doxygen.config] [documentation/Makefile] [documentation/styles/Makefile]
		[documentation/tutorial/Makefile] [libcwd.pc] [libcwd_r.pc])

AC_OUTPUT

