# CW_LIB_LIBCWD(OPTIONNAME, WANTED, THREADED,
#               [ACTION_IF_FOUND], [ACTION-IF-NOT-FOUND])
# -------------------------------------------
#
# OPTIONNAME is the name used in AC_ARG_ENABLE that requests
# libcwd support.
#
# WANTED can be [yes], [no] or [auto]/[] and should be the result
# of use --enable-OPTIONNAME, --disable-OPTIONNAME or neither.
# For example:
# AC_ARG_ENABLE(debugging, [  --enable-debugging      enable debugging code.])
# Where OPTIONNAME is [debugging] and WANTED is [$enable_debugging].
#
# THREADED can be [yes] or [no] when the application is threaded
# or non-threaded respectively.
#
# This macro tests for the usability of libcwd and sets the the macro
# `cw_used_libcwd' to "yes" when it is detected, "no" otherwise.
#
# The default ACTION_IF_FOUND is, if WANTED is unequal "no",
# to update CXXFLAGS and LIBS.
#
# The default ACTION-IF-NOT-FOUND is to print an error message;
# ACTION-IF-NOT-FOUND is only executed when WANTED is "yes" and no
# libcwd was found.

AC_DEFUN([CW_LIB_LIBCWD],
[if test x"$$2" = x"no"; then
  cw_used_libcwd=no
else
  cw_libname=cwd
  test "$3" = "yes" && cw_libname=cwd_r
  AC_CACHE_CHECK([if libcwd is available], cw_cv_lib_libcwd,
[  # Check if we have libcwd
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  cw_save_LIBS="$LIBS"
  LIBS="`pkg-config --libs lib$cw_libname` $LIBS"
  AC_LINK_IFELSE([AC_LANG_CALL([], [__libcwd_version])], [cw_cv_lib_libcwd=yes], [cw_cv_lib_libcwd=no])
  LIBS="$cw_save_LIBS"
  AC_LANG_RESTORE])
  cw_use_libcwd="$2"
  test -n "$cw_use_libcwd" || cw_use_libcwd=auto
  test "$cw_use_libcwd" = "auto" && cw_use_libcwd=$cw_cv_lib_libcwd
  if test "$cw_use_libcwd" = "yes"; then
    if test "$cw_cv_lib_libcwd" = "no"; then
      m4_default([$5], [dnl
      AC_MSG_ERROR([
  --enable-$1: You need to have libcwd installed to enable this.
  Or perhaps you need to add its location to PKG_CONFIG_PATH and LD_LIBRARY_PATH, for example:
  PKG_CONFIG_PATH=/opt/install/lib/pkgconfig LD_LIBRARY_PATH=/opt/install/lib ./configure])])
    else
      cw_used_libcwd=yes
      m4_default([$4], [dnl
      CXXFLAGS="`pkg-config --cflags lib$cw_libname` $CXXFLAGS"
      LIBS="$LIBS `pkg-config --libs lib$cw_libname`"])
    fi
  else
    cw_used_libcwd=no
  fi
fi
])

