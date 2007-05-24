dnl CW_CHECK_STATICLIB(LIBRARY, FUNCTION [, ACTION-IF-FOUND
dnl		 [, ACTION-IF-NOT-FOUND  [, OTHER-LIBRARIES]]])
dnl Like AC_CHECK_LIB but looking for static libraries, however
dnl unlike AC_CHECK_LIB this macro adds OTHER-LIBRARIES to LIBS
dnl when successful.  LIBRARY must be of the form libxxx.a.
AC_DEFUN([CW_CHECK_STATICLIB],
[AC_MSG_CHECKING([for $2 in $1 $5])
dnl Use a cache variable name containing both the library and function name,
dnl because the test really is for library $1 defining function $2, not
dnl just for library $1.  Separate tests with the same $1 and different $2s
dnl may have different results.
ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ac_cv_lib_static_$ac_lib_var,
[if test -r /etc/ld.so.conf ; then
  ld_so_paths="/lib /usr/lib `cat /etc/ld.so.conf`"
else
  ld_so_paths="/lib /usr/lib"
fi
for path in x $LDFLAGS; do
  case "$path" in
  -L*) ld_so_paths="`echo $path | sed -e 's/^-L//'` $ld_so_paths" ;;
  esac
done
for path in $ld_so_paths; do
  ac_save_LIBS="$LIBS"
  LIBS="$path/$1 $5 $LIBS"
  AC_TRY_LINK(dnl
  ifelse(AC_LANG, [FORTRAN77], ,
  ifelse([$2], [main], , dnl Avoid conflicting decl of main.
  [/* Override any gcc2 internal prototype to avoid an error.  */
  ]ifelse(AC_LANG, CPLUSPLUS, [#ifdef __cplusplus
  extern "C"
  #endif
  ])dnl
  [/* We use char because int might match the return type of a gcc2
      builtin and then its argument prototype would still apply.  */
  char $2();
  ])),
	      [$2()],
	      eval "ac_cv_lib_static_$ac_lib_var=\"$path/$1 $5\"",
	      eval "ac_cv_lib_static_$ac_lib_var=no")
  LIBS="$ac_save_LIBS"
  if eval "test \"`echo '$ac_cv_lib_static_'$ac_lib_var`\" != no"; then
    break
  fi
done
])dnl
eval result=\"`echo '$ac_cv_lib_static_'$ac_lib_var`\"
if test "$result" != no; then
  AC_MSG_RESULT([$result])
  ifelse([$3], ,
[changequote(, )dnl
  ac_tr_lib=HAVE_`echo "$1" | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED([$ac_tr_lib])
  LIBS="$result $LIBS"
], [$3])
else
  AC_MSG_RESULT(no)
ifelse([$4], , , [$4
])dnl
fi
])

dnl CW_SYS_BUILTIN_RETURN_ADDRESS_OFFSET
dnl
dnl Determines the size of a call, in other words: what needs to be substracted
dnl from __builtin_return_address(0) to get the start of the assembly instruction
dnl that did the call.
AC_DEFUN([CW_SYS_BUILTIN_RETURN_ADDRESS_OFFSET],
[AC_CACHE_CHECK(needed offset to __builtin_return_address(), cw_cv_sys_builtin_return_address_offset,
[save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS=""
AC_TRY_RUN(
changequote(<<, >>)dnl
<<int size;

void f(void)
{
  size = (unsigned int)__builtin_return_address(0) & 255;
}

int main(int argc, char* argv[])
{
  asm( ".align 256" );
  f();
   return (size > 4) ? 1 : 0;
}
>>,
changequote([, ])dnl
cw_cv_sys_builtin_return_address_offset=0,
cw_cv_sys_builtin_return_address_offset=-1,
cw_cv_sys_builtin_return_address_offset=-1)
CXXFLAGS="$save_CXXFLAGS"])
eval "CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET=\"$cw_cv_sys_builtin_return_address_offset\""
AC_SUBST(CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET)
])

dnl CW_SYS_FRAME_ADDRESS_OFFSET
dnl
dnl Defines CW_FRAME_ADDRESS_OFFSET_C to be the number of void*
dnl between a frame pointer and the next frame pointer.
dnl
AC_DEFUN([CW_SYS_FRAME_ADDRESS_OFFSET],
[AC_REQUIRE([CW_SYS_RECURSIVE_BUILTIN_RETURN_ADDRESS])
CW_CONFIG_FRAME_ADDRESS_OFFSET=undef
if test "$cw_cv_sys_recursive_builtin_return_address" != "no"; then
AC_CACHE_CHECK(frame pointer offset in frame structure, cw_sys_frame_address_offset,
[save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS=""
AC_TRY_RUN([
extern "C" void exit(int status);
int func4(int offset)
{
  void* f0 = __builtin_frame_address(0);
  void* f1 = __builtin_frame_address(1);
  void* f2 = __builtin_frame_address(2);
  void* f3 = __builtin_frame_address(3);
  void* f4 = __builtin_frame_address(4);
  return (f1 == ((void**)f0)[offset] && f2 == ((void**)f1)[offset] && f3 == ((void**)f2)[offset] && f4 == ((void**)f3)[offset]);
}
void func3(void)
{
  for (int offset = 0; offset < 128; ++offset)
    if (func4(offset))
      exit(offset);
}
void func2(int i)
{
  char a[22];
  func3();
  if (i == 1)		// Some arbitrary code
    return;
  i = 0;
}
void func1(void)
{
  char a[13];
  func2(1);
  for (int i = 1; i < 10; ++i);	// Some arbitrary code
}
void calculate_offset(void)
{
  char a[100];
  func1();
}
int main(int argc, char* argv[])
{
  if (argc == 1)
    return 0;	// This wasn't the real test yet
  calculate_offset();
  return 255;	// Failure
}],
./conftest run
cw_sys_frame_address_offset=$?
if test "$cw_sys_frame_address_offset" != "255"; then
CW_CONFIG_FRAME_ADDRESS_OFFSET=define
fi,
[AC_MSG_ERROR(Failed to compile a test program!?)],
cw_sys_frame_address_offset=0 dnl Guess a default for cross compiling
)
CXXFLAGS="$save_CXXFLAGS"])
fi
eval "CW_FRAME_ADDRESS_OFFSET_C=$cw_sys_frame_address_offset"
AC_SUBST(CW_FRAME_ADDRESS_OFFSET_C)
AC_SUBST(CW_CONFIG_FRAME_ADDRESS_OFFSET)
])

dnl CW_RPM
dnl Generate directories and files needed for an rpm target.
dnl For this to work, the Makefile.am needs to contain the string @RPM_TARGET@
AC_DEFUN([CW_RPM],
[if test "$USE_MAINTAINER_MODE" = yes; then
AC_CHECK_PROGS(rpm)
if test "$ac_cv_prog_rpm" = yes; then

fi
fi])

dnl CW_SYS_PS_WIDE_PID_OPTION
dnl Determines the options needed for `ps' to print the full command of a specified PID
AC_DEFUN([CW_SYS_PS_WIDE_PID_OPTION],
[AC_CACHE_CHECK([for option of ps to print the full command of a specified PID], cw_cv_sys_ps_wide_pid_option,
[if $PS -ww >/dev/null 2>/dev/null; then
cw_cv_sys_ps_wide_pid_option="-ww"
elif $PS -w >/dev/null 2>/dev/null; then
cw_cv_sys_ps_wide_pid_option="-w"
else
cw_cv_sys_ps_wide_pid_option="-f"
fi
$PS $cw_cv_sys_ps_wide_pid_option 1 > ./ps.out.$$ 2>/dev/null
if grep init ./ps.out.$$ >/dev/null; then
  :
else
$PS $cw_cv_sys_ps_wide_pid_option\p 1 > ./ps.out.$$ 2>/dev/null
if grep init ./ps.out.$$ >/dev/null; then
  cw_cv_sys_ps_wide_pid_option="$cw_cv_sys_ps_wide_pid_option"p
fi
fi
rm -f ./ps.out.$$
echo "#! /bin/sh" > ./conf.test.this_is_a_very_long_executable_name_that_should_be_longer_than_any_other_name_including_full_path_than_will_reasonable_ever_happen_for_real_in_practise
echo "$PS $cw_cv_sys_ps_wide_pid_option \$\$" >> ./conf.test.this_is_a_very_long_executable_name_that_should_be_longer_than_any_other_name_including_full_path_than_will_reasonable_ever_happen_for_real_in_practise
chmod 700 ./conf.test.this_is_a_very_long_executable_name_that_should_be_longer_than_any_other_name_including_full_path_than_will_reasonable_ever_happen_for_real_in_practise
if ./conf.test.this_is_a_very_long_executable_name_that_should_be_longer_than_any_other_name_including_full_path_than_will_reasonable_ever_happen_for_real_in_practise | grep real_in_practise >/dev/null; then
  :
else
  if ./conf.test.this_is_a_very_long_executable_name_that_should_be_longer_than_any_other_name_including_full_path_than_will_reasonable_ever_happen_for_real_in_practise | grep that_should_be_longer >/dev/null; then
    AC_MSG_WARN([ps cuts off long path names, this will break executables with a long path or name that use libcwd!])
  else
    AC_MSG_ERROR([Cannot determine the correct ps arguments])
  fi
fi
rm -f ./conf.test.this_is_a_very_long_executable_name_that_should_be_longer_than_any_other_name_including_full_path_than_will_reasonable_ever_happen_for_real_in_practise
])
AC_DEFINE_UNQUOTED([PS_ARGUMENT], "$cw_cv_sys_ps_wide_pid_option", [This should be the argument to ps, causing it to print a wide output of a specified PID.])
])

dnl --------------------------------------------------------------------------------------------------------------17" monitors are a minimum
dnl Everything below isn't really a test but parts that are used in both, libcwd as well as in libcw.
dnl In order to avoid duplication it is put here as macro.

dnl CW_COMPILER_VERSIONS
dnl Test if the version of both, C and C++ compiler match.
AC_DEFUN([CW_COMPILER_VERSIONS],
[AC_MSG_CHECKING([whether the versions C and C++ compiler match])
CC_VERSION=`$CC -v 2>&1 | grep '^gcc version'`
CXX_VERSION=`$CXX -v 2>&1 | grep '^gcc version'`
if test x"$CC_VERSION" != x"$CXX_VERSION"; then
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([Versions of CC and CXX do not match.
                  $CC gives \"$CC_VERSION\" and $CXX  gives \"$CXX_VERSION\".
                  Please specify both, CC and CXX environment variables.])
else
  AC_MSG_RESULT([yes])
fi
])

dnl CW_ATEXITTEST
dnl Test if g++ was configured with --enable-__cxa_atexit,
dnl otherwise a call to dlopen() will destruct all global
dnl objects and might cause a segfault in libcwd, for which
dnl we don't want to get the blame.
AC_DEFUN([CW_ATEXITTEST],
[AC_CHECK_FUNCS([__cxa_atexit])
if test $ac_cv_func___cxa_atexit = "yes"; then
AC_CACHE_CHECK([if the compiler was configured for __cxa_atexit], cw_cv_cxa_atexit,
[cw_cv_cxa_atexit=yes	# By default, don't fail this test.
# We only need --enable-__cxa_atexit when the function exists on this OS.
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_RUN([static int y;
struct Foo { Foo() { y = 0; } ~Foo() { } } x;
extern "C" void __cxa_atexit() { y = 1; }
int main() { return y ? 0 : 1; }],
  cw_cv_cxa_atexit=yes,
  cw_cv_cxa_atexit=no,
  cw_cv_cxa_atexit=whatever)
AC_LANG_RESTORE])
if test "$cw_cv_cxa_atexit" = no; then
dnl Without --enable-__cxa_atexit a call to dlclose() for
dnl any shared library will cause ALL shared libraries to
dnl to be destructed, and thus all global objects to be
dnl destructed.
dnl See also http://gcc.gnu.org/bugzilla/show_bug.cgi?id=13227
AC_MSG_ERROR([

* This C++ compiler was not configured with --enable-__cxa_atexit!
* You need to fix this; reconfigure and recompile your compiler.
* See also http://gcc.gnu.org/gcc-3.2/c++-abi.html
])
fi
fi
])

dnl CW_OPG_FLAGS -- from cwautomacros, but stripped of libcwd detection stuff.
dnl
dnl Add --enable-optimize
dnl and --enable-profile
dnl options.
dnl
dnl This macro sets CXXFLAGS to include -g (or -ggdb on FreeBSD) when
dnl debugging is required, -O2 when optimization is required and
dnl appropriate warning flags.
dnl
dnl However, if CXXFLAGS already contains a -g* option then that is used
dnl instead of the default -g (-ggdb). If it contains a -O* option then
dnl that is used instead of -O2. Finally, if options are passed to
dnl the macro, then those are used instead of the default ones.
dnl
dnl Update CXXFLAGS and LDFLAGS accordingly.
dnl
dnl Further more, the following macros are set:
dnl
dnl CW_DEBUG_FLAGS	: Any -g* flags.
dnl CW_OPTIMISE_FLAGS	: Any -O* flags.
dnl CW_WARNING_FLAGS	: Any -W* flags.
dnl CW_STRIPPED_CXXFLAGS: Any other flags that were already in CXXFLAGS.
dnl
AC_DEFUN([CW_OPG_FLAGS], [dnl
dnl Containers for the respective options.
m4_pattern_allow(CW_DEBUG_FLAGS)
m4_pattern_allow(CW_OPTIMISE_FLAGS)
m4_pattern_allow(CW_WARNING_FLAGS)
m4_pattern_allow(CW_STRIPPED_CXXFLAGS)
m4_pattern_allow(CW_DEFAULT_DEBUG_FLAGS)

# Add args to configure
AC_ARG_ENABLE(optimize,      [  --enable-optimize       do code optimization @<:@auto@:>@], [cw_config_optimize=$enableval], [cw_config_optimize=])
AC_ARG_ENABLE(profile,       [  --enable-profile        add profiling code @<:@no@:>@], [cw_config_profile=$enableval], [cw_config_profile=])

# Strip possible -g and -O commandline options from CXXFLAGS.
CW_DEBUG_FLAGS=
CW_OPTIMISE_FLAGS=
CW_WARNING_FLAGS=
CW_STRIPPED_CXXFLAGS=
for arg in $CXXFLAGS; do
case "$arg" in # (
-g*)
        CW_DEBUG_FLAGS="$CW_DEBUG_FLAGS $arg"
        ;; # (
-O*)
        CW_OPTIMISE_FLAGS="$CW_OPTIMISE_FLAGS $arg"
        ;; # (
-W*)	CW_WARNING_FLAGS="$CW_WARNING_FLAGS $arg"
	;; # (
*)
        CW_STRIPPED_CXXFLAGS="$CW_STRIPPED_CXXFLAGS $arg"
        ;;
esac
done

# Set various defaults, depending on other options.

if test x"$cw_config_optimize" = x"no"; then
    CW_OPTIMISE_FLAGS=""        # Explicit --disable-optimize, strip optimization even from CXXFLAGS environment variable.
fi

if test x"$enable_maintainer_mode" = x"yes"; then
  if test -z "$cw_config_optimize"; then
    cw_config_optimize=no          # --enable-maintainer-mode, set default to --disable-optimize.
  fi
fi

if test -z "$cw_config_optimize"; then
  cw_config_optimize=no          # no --enable-optimize, set default to --disable-optimize.
fi

dnl Find out which debugging options we need
AC_CANONICAL_HOST
case "$host" in
  *freebsd*) CW_DEFAULT_DEBUG_FLAGS=-ggdb ;; dnl FreeBSD needs -ggdb to include sourcefile:linenumber info in its object files.
  *) CW_DEFAULT_DEBUG_FLAGS=-g ;;
esac

# Add debug flags.
test -n "$CW_DEBUG_FLAGS" || CW_DEBUG_FLAGS="$CW_DEFAULT_DEBUG_FLAGS"

# Handle cw_config_optimize; when not explicitly set to "no", use user provided
# optimization flags, or -O2 when nothing was provided.
if test x"$cw_config_optimize" != x"no"; then
  test -n "$CW_OPTIMISE_FLAGS" || CW_OPTIMISE_FLAGS="-O2"
fi

# Handle cw_config_profile.
if test x"$cw_config_profile" = x"yes"; then
  CW_STRIPPED_CXXFLAGS="$CW_STRIPPED_CXXFLAGS -pg"
  LDFLAGS="$LDFLAGS -pg"
fi

# Choose warning options to use.
# If not in maintainer mode, use the warning options that were in CXXFLAGS.
# Otherwise, use those plus any passed to the macro, or if neither are
# given a default string - and then filter out incompatible warnings.
if test x"$enable_maintainer_mode" = x"yes"; then
  if test -z "$1" -a -z "$CW_WARNING_FLAGS"; then
    CW_WARNING_FLAGS="-W -Wall -Woverloaded-virtual -Wundef -Wpointer-arith -Wwrite-strings -Werror -Winline"
  else
    CW_WARNING_FLAGS="$CW_WARNING_FLAGS $1"
  fi
  AC_EGREP_CPP(Winline-broken, [
#if __GNUC__ < 3
  Winline-broken
#endif
  ],
     dnl -Winline is broken.
     [CW_WARNING_FLAGS="$(echo "$CW_WARNING_FLAGS" | sed -e 's/ -Winline//g')"],
     dnl -Winline is not broken. Remove -Werror when optimizing though.
     [if test -n "$CW_OPTIMISE_FLAGS"; then
        CW_WARNING_FLAGS="$(echo "$CW_WARNING_FLAGS" | sed -e 's/ -Werror//g')"
      fi]
  )
fi

# Reassemble CXXFLAGS with debug and optimization flags.
[CXXFLAGS=`echo "$CW_DEBUG_FLAGS $CW_WARNING_FLAGS $CW_OPTIMISE_FLAGS $CW_STRIPPED_CXXFLAGS" | sed -e 's/^ *//' -e 's/  */ /g' -e 's/ *$//'`]

dnl Put CXXFLAGS into the Makefile.
AC_SUBST(CXXFLAGS)
dnl Allow fine tuning if necessary, by putting the substituting the parts too.
AC_SUBST(CW_DEBUG_FLAGS)
AC_SUBST(CW_WARNING_FLAGS)
AC_SUBST(CW_OPTIMISE_FLAGS)
AC_SUBST(CW_STRIPPED_CXXFLAGS)
])
