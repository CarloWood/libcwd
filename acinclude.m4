dnl CW_CHECK_STATICLIB(LIBRARY, FUNCTION [, ACTION-IF-FOUND
dnl		 [, ACTION-IF-NOT-FOUND  [, OTHER-LIBRARIES]]])
dnl Like AC_CHECK_LIB but looking for static libraries, however
dnl unlike AC_CHECK_LIB this macro adds OTHER-LIBRARIES to LIBS
dnl when successful.  LIBRARY must be of the form libxxx.a.
AC_DEFUN(CW_CHECK_STATICLIB,
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

dnl CW_BUG_REDEFINES_INITIALIZATION
dnl
AC_DEFUN(CW_BUG_REDEFINES_INITIALIZATION,
CW_REDEFINES_FIX=
dnl We don't want automake to put this in Makefile.in
[AC_SUBST](CW_REDEFINES_FIX))

dnl CW_BUG_REDEFINES([HEADERFILE])
dnl
dnl Check whether the HEADERFILE causes macros to be redefined
dnl
AC_DEFUN(CW_BUG_REDEFINES,
[AC_REQUIRE([CW_BUG_REDEFINES_INITIALIZATION])
changequote(, )dnl
cw_bug_var=`echo $1 | sed -e 'y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/' -e 's/ //g' -e 's/[^a-z0-9]/_/g'`
changequote([, ])dnl
AC_MSG_CHECKING([if $1 redefines macros])
AC_CACHE_VAL(cw_cv_bug_redefines_$cw_bug_var,
[cat > conftest.$ac_ext <<EOF
#include <sys/types.h>
#include <sys/time.h>
#include <$1>
#ifdef __cplusplus
#include <cstdlib>
#endif
int main() { exit(0); }
EOF
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="`echo $CXXFLAGS | sed -e 's/-Werror//g'`"
if { (eval echo configure: \"$ac_compile\") 1>&5; (eval $ac_compile) 2>&1 | tee conftest.out >&5; }; then
changequote(, )dnl
  cw_result="`grep 'warning.*redefined' conftest.out | sed -e 's/[^A-Z_]*redefined.*//' -e 's/.*warning.* [^A-Z_]*//'`"
  eval "cw_cv_bug_redefines_$cw_bug_var=\"\$cw_result\""
  cw_result="`grep 'previous.*defin' conftest.out | sed -e 's/:.*//' -e 's%.*include/%%g' | sort | uniq`"
changequote([, ])dnl
  eval "unset cw_cv_bug_redefines_${cw_bug_var}_prev"
  AC_CACHE_VAL(cw_cv_bug_redefines_${cw_bug_var}_prev, [eval "cw_cv_bug_redefines_${cw_bug_var}_prev=\"$cw_result\""])
else
  echo "configure: failed program was:" >&5
  cat conftest.$ac_ext >&5
  eval "cw_cv_bug_redefines_$cw_bug_var="
  eval "cw_cv_bug_redefines_${cw_bug_var}_prev="
fi
CXXFLAGS="$save_CXXFLAGS"
rm -f conftest*])
eval "cw_redefined_macros=\"\$cw_cv_bug_redefines_$cw_bug_var\""
eval "cw_redefined_from=\"\$cw_cv_bug_redefines_${cw_bug_var}_prev\""
if test x"$cw_redefined_macros" = x; then
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT($cw_redefined_macros from $cw_redefined_from)
fi
for i in $cw_redefined_from; do
CW_REDEFINES_FIX="$CW_REDEFINES_FIX\\
#include <$i>"
done
for i in $cw_redefined_macros; do
CW_REDEFINES_FIX="$CW_REDEFINES_FIX\\
#undef $i"
done])

dnl CW_DEFINE_TYPE_INITIALIZATION
dnl
AC_DEFUN(CW_DEFINE_TYPE_INITIALIZATION,
CW_TYPEDEFS=
dnl We don't want automake to put this in Makefile.in
[AC_SUBST](CW_TYPEDEFS))

dnl CW_DEFINE_TYPE(NEWTYPE, OLDTYPE)
dnl
dnl Add `typedef OLDTYPE NEWTYPE;' to the output variable CW_TYPEDEFS
dnl
AC_DEFUN(CW_DEFINE_TYPE,
[AC_REQUIRE([CW_DEFINE_TYPE_INITIALIZATION])
CW_TYPEDEFS="typedef $2 $1; $CW_TYPEDEFS"
])

dnl CW_TYPE_EXTRACT_FROM(FUNCTION, INIT, ARGUMENTS, ARGUMENT)
dnl
dnl Extract the type of ARGUMENT argument of function FUNCTION with ARGUMENTS arguments.
dnl INIT are possibly needed #includes.  The result is put in `cw_result'.
dnl
AC_DEFUN(CW_TYPE_EXTRACT_FROM,
[cat > conftest.$ac_ext <<EOF
[$2]
#ifdef __cplusplus
#include <cstdlib>
#endif
template<typename ARG>
  void detect_type(ARG)
  {
    return 1;
  }
EOF
echo $ac_n "template<typename ARG0[,] $ac_c" >> conftest.$ac_ext
i=1
while test "$i" != "$3"; do
echo $ac_n "typename ARG$i[,] $ac_c" >> conftest.$ac_ext
i=`echo $i | sed -e 'y/012345678/123456789/'`
done
echo "typename ARG$3>" >> conftest.$ac_ext
echo $ac_n "void foo(ARG0(*f)($ac_c" >> conftest.$ac_ext
i=1
while test "$i" != "$3"; do
echo $ac_n "ARG$i[,] $ac_c" >> conftest.$ac_ext
i=`echo $i | sed -e 'y/012345678/123456789/'`
done
echo "ARG$3)) { ARG$4 arg;" >> conftest.$ac_ext
cat >> conftest.$ac_ext <<EOF
  detect_type(arg);
}
int main(void)
{
  foo($1);
  exit(0);
}
EOF
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="`echo $CXXFLAGS | sed -e 's/-Werror//g'`"
if { (eval echo configure: \"$ac_compile\") 1>&5; (eval $ac_compile) 2>&1 | tee conftest.out >&5; }; then
changequote(, )dnl
  cw_result="`grep 'detect_type<.*>' conftest.out | sed -e 's/.*detect_type<//g' -e 's/>[^>]*//' | head -n 1`"
  if test -z "$cw_result"; then
    cw_result="`cat conftest.out`"
    dnl We need this comment to work around a bug in autoconf or m4: '['
    cw_result="`echo $cw_result | sed -e 's/.*detect_type.*with ARG = //g' -e 's/].*//'`"
  fi
changequote([, ])dnl
  if test -z "$cw_result"; then
    AC_MSG_ERROR([Configure problem: Failed to determine type])
  fi
else
  echo "configure: failed program was:" >&5
  cat conftest.$ac_ext >&5
  AC_MSG_ERROR([Configuration problem: Failed to compile a test program])
fi
CXXFLAGS="$save_CXXFLAGS"
rm -f conftest*
])

dnl CW_TRY_RUN
dnl
dnl Like `AC_TRY_RUN' but also works when the language is C++.
dnl
dnl CW_TRY_RUN(PROGRAM, [ACTION-IF-TRUE [, ACTION-IF-FALSE [, ACTION-IF-CROSS-COMPILING]]])
AC_DEFUN(CW_TRY_RUN,
[if test "$cross_compiling" = yes; then
  ifelse([$4], ,
    [errprint(__file__:__line__: warning: [CW_TRY_RUN] called without default to allow cross compiling
)dnl
  AC_MSG_ERROR(can not run test program while cross compiling)],
  [$4])
else
  CW_TRY_RUN_NATIVE([$1], [$2], [$3])
fi
])

dnl CW_TRY_RUN_NATIVE
dnl
dnl Like `AC_TRY_RUN_NATIVE' but also works when the language is C++.
dnl Like CW_TRY_RUN but assumes a native-environment (non-cross) compiler.
dnl CW_TRY_RUN_NATIVE(PROGRAM, [ACTION-IF-TRUE [, ACTION-IF-FALSE]])
AC_DEFUN(CW_TRY_RUN_NATIVE,
[cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
#ifdef __cplusplus
#include <cstdlib>
#endif
[$1]
EOF
if AC_TRY_EVAL(ac_link) && test -s conftest${ac_exeext} && (./conftest; exit) 2>/dev/null
then
dnl Don't remove the temporary files here, so they can be examined.
  ifelse([$2], , :, [$2])
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
ifelse([$3], , , [  rm -fr conftest*
  $3
])dnl
fi
rm -fr conftest*])

dnl CW_MALLOC_OVERHEAD
dnl
dnl Defines CW_MALLOC_OVERHEAD_C to be the number of bytes extra
dnl allocated for a call to malloc.
dnl
AC_DEFUN(CW_MALLOC_OVERHEAD,
[AC_CACHE_CHECK(malloc overhead in bytes, cw_cv_system_mallocoverhead,
[CW_TRY_RUN([#include <cstddef>
#include <cstdlib>

bool bulk_alloc(size_t malloc_overhead_attempt, size_t size)
{
  int const number = 100;
  long int distance = 9999;
  char* ptr[number];
  ptr[0] = (char*)malloc(size - malloc_overhead_attempt);
  for (int i = 1; i < number; ++i)
  {
    ptr[i] = (char*)malloc(size - malloc_overhead_attempt);
    if (ptr[i] > ptr[i - 1] && (ptr[i] - ptr[i - 1]) < distance)
      distance = ptr[i] - ptr[i - 1];
  }
  for (int i = 0; i < number; ++i)
    free(ptr[i]);
  return (distance == (long int)size);
}

int main(int argc, char* argv[])
{
  if (argc == 1)
    exit(0);	// This wasn't the real test yet
  for (size_t s = 0; s <= 64; s += 2)
    if (bulk_alloc(s, 2048))
      exit(s);
  exit(8);	// Guess a default
}],
./conftest run
cw_cv_system_mallocoverhead=$?,
[AC_MSG_ERROR(Failed to compile a test program!?)],
cw_cv_system_mallocoverhead=4 dnl Guess a default for cross compiling
)])
eval "CW_MALLOC_OVERHEAD_C=$cw_cv_system_mallocoverhead"
AC_SUBST(CW_MALLOC_OVERHEAD_C)
])

dnl CW_NEED_WORD_ALIGNMENT
dnl
dnl Defines LIBCWD_NEED_WORD_ALIGNMENT when the host needs
dnl respectively size_t alignment or not.
AC_DEFUN(CW_NEED_WORD_ALIGNMENT,
[AC_CACHE_CHECK(if machine needs word alignment, cw_cv_system_needwordalignment,
[CW_TRY_RUN([#include <cstddef>
#include <cstdlib>

int main(void)
{
  size_t* p = reinterpret_cast<size_t*>((char*)malloc(5) + 1);
  *p = 0x12345678;
#ifdef __alpha__	// Works, but still should use alignment.
  exit(-1);
#else
  exit ((((unsigned long)p & 1UL) && *p == 0x12345678) ? 0 : -1);
#endif
}],
cw_cv_system_needwordalignment=no,
cw_cv_system_needwordalignment=yes,
cw_cv_system_needwordalignment="why not")])
if test "$cw_cv_system_needwordalignment" != no; then
  AC_DEFINE_UNQUOTED([LIBCWD_NEED_WORD_ALIGNMENT], 1)
fi
])

dnl CW_TYPE_GETGROUPS
dnl
dnl Like AC_TYPE_GETGROUPS but with bug fix for C++ and adding a
dnl typedef getgroups_t instead of defining the macro GETGROUPS_T.
AC_DEFUN(CW_TYPE_GETGROUPS,
[AC_REQUIRE([AC_TYPE_UID_T])dnl
AC_CACHE_CHECK(type of array argument to getgroups, ac_cv_type_getgroups,
[CW_TRY_RUN(
changequote(<<, >>)dnl
<<
/* Thanks to Mike Rendell for this test.  */
#include <sys/types.h>
#ifdef __cplusplus
extern "C" int getgroups(size_t, gid_t*);
#endif
#define NGID 256
#undef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
main()
{
  gid_t gidset[NGID];
  int i, n;
  union { gid_t gval; long lval; }  val;

  val.lval = -1;
  for (i = 0; i < NGID; i++)
    gidset[i] = val.gval;
  n = getgroups (sizeof (gidset) / MAX (sizeof (int), sizeof (gid_t)) - 1, gidset);
  /* Exit non-zero if getgroups seems to require an array of ints.  This
     happens when gid_t is short but getgroups modifies an array of ints.  */
  exit ((n > 0 && gidset[n] != val.gval) ? 1 : 0);
}
>>,
changequote([, ])dnl
  [CW_TYPE_EXTRACT_FROM(getgroups, [#include <unistd.h>], 2, 2)
  eval "cw_result2=\"$cw_result\""
  ac_cv_type_getgroups=`echo "$cw_result2" | sed -e 's/ *\*$//'`],
  ac_cv_type_getgroups=int,
  ac_cv_type_getgroups=cross)
if test "$ac_cv_type_getgroups" = cross; then
  dnl When we can't run the test program (we are cross compiling), presume
  dnl that <unistd.h> has either an accurate prototype for getgroups or none.
  dnl Old systems without prototypes probably use int.
  AC_EGREP_HEADER([getgroups.*int.*gid_t], unistd.h,
                  ac_cv_type_getgroups=gid_t, ac_cv_type_getgroups=int)
fi])
CW_DEFINE_TYPE(getgroups_t, [$ac_cv_type_getgroups])
])

dnl CW_PROG_CXX
dnl
dnl Like AC_PROG_CXX, except that it demands that GNU g++-2.95.1
dnl or higher is available.
AC_DEFUN(CW_PROG_CXX,
[AC_BEFORE([$0], [CW_PROG_CXXCPP])
AC_REQUIRE([AC_PROG_CXX])
AC_CACHE_CHECK(whether we are using GNU C++ version 2.95.1 or later, ac_cv_prog_gxx_version,
[dnl The semicolon is to pacify NeXT's syntax-checking cpp.
cat > conftest.C <<EOF
#ifdef __GNUG__
  gnu;
#if __GNUG__ > 2 || (__GNUG__ == 2 && __GNUC_MINOR__ >= 95)
  yes;
#endif
#endif
EOF
if AC_TRY_COMMAND(${CXX-g++} -E conftest.C) | egrep yes >/dev/null 2>&1; then
  ac_cv_prog_gxx_version=yes
else
  if AC_TRY_COMMAND(${CXX-g++} -E conftest.C) | egrep gnu >/dev/null 2>&1; then
    ac_cv_prog_gxx_version="old version"
  else
    ac_cv_prog_gxx_version=no
  fi
fi])
if test "$ac_cv_prog_gxx_version" = yes; then
  ac_cv_prog_gxx=yes
  GXX=yes
else
  if test "$ac_cv_prog_gxx_version" = "old version"; then
    AC_MSG_ERROR([Installation problem: GNU C++ version 2.95.1 or higher is required])
  else
    AC_MSG_ERROR([Installation problem: Cannot find GNU C++ compiler])
  fi
fi
])

dnl CW_PROG_CXXCPP
dnl
dnl Like AC_PROG_CXXCPP but with bug work around that allows user to override CXXCPP.
AC_DEFUN(CW_PROG_CXXCPP,
[AC_BEFORE([$0], [AC_PROG_CXXCPP])
AC_REQUIRE([CW_PROG_CXX])
dnl This triggers the bug:
if test -n "$CXXCPP"; then
dnl Work around:
  unset ac_cv_prog_CXXCPP
  eval "save_CXXCPP=\"$CXXCPP\""
fi
AC_PROG_CXXCPP
dnl This is the result of the bug:
if test x"$CXXCPP" = x; then
  eval "CXXCPP=\"$save_CXXCPP\""; # Allow user to override
  eval "ac_cv_prog_CXXCPP=\"$save_CXXCPP\""; # This is needed because the buggy AC_PROG_CXXCPP is called from many other places as well.
fi])

dnl CW_PROG_CXX_FINGER_PRINTS
dnl
dnl Extract finger prints of C++ compiler and preprocessor and C compiler which is used for linking.
AC_DEFUN(CW_PROG_CXX_FINGER_PRINTS,
[AC_REQUIRE([AC_PROG_CXX])
AC_REQUIRE([CW_PROG_CXXCPP])
AC_REQUIRE([AC_PROG_CC])
cw_prog_cxx_finger_print="`$CXX -v 2>&1 | grep version | head -n 1`"
cw_prog_cxxcpp_finger_print="`echo | $CXXCPP -v 2>&1 | grep version | head -n 1`"
cw_prog_cc_finger_print="`$CC -v 2>&1 | grep version | head -n 1`"
])

dnl CW_SYS_BUILTIN_RETURN_ADDRESS_OFFSET
dnl
dnl Determines the size of a call, in other words: what needs to be substracted
dnl from __builtin_return_address(0) to get the start of the assembly instruction
dnl that did the call.
AC_DEFUN(CW_SYS_BUILTIN_RETURN_ADDRESS_OFFSET,
[AC_CACHE_CHECK(needed offset to __builtin_return_address(), cw_cv_sys_builtin_return_address_offset,
[save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS=""
CW_TRY_RUN(
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
  exit(size > 4 ? 1 : 0);
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

dnl CW_SYS_RECURSIVE_BUILTIN_RETURN_ADDRESS
dnl
dnl Determines if __builtin_return_address(1) is supported by compiler.
AC_DEFUN(CW_SYS_RECURSIVE_BUILTIN_RETURN_ADDRESS,
[AC_CACHE_CHECK([whether __builtin_return_address(1) works], cw_cv_sys_recursive_builtin_return_address,
[AC_LANG_SAVE
AC_LANG_C
save_CFLAGS="$CFLAGS"
CFLAGS=""
AC_TRY_RUN(
[void f(void) { exit(__builtin_return_address(1) ? 0 : 1); }
int main(void) { f(); }],
cw_cv_sys_recursive_builtin_return_address=yes,
cw_cv_sys_recursive_builtin_return_address=no,
cw_cv_sys_recursive_builtin_return_address=unknown)
CFLAGS="$save_CFLAGS"
AC_LANG_RESTORE])
if test "$cw_cv_sys_recursive_builtin_return_address" = "no"; then
CW_CONFIG_RECURSIVE_BUILTIN_RETURN_ADDRESS=undef
else
CW_CONFIG_RECURSIVE_BUILTIN_RETURN_ADDRESS=define
fi
AC_SUBST(CW_CONFIG_RECURSIVE_BUILTIN_RETURN_ADDRESS)
])

dnl CW_SYS_FRAME_ADDRESS_OFFSET
dnl
dnl Defines CW_FRAME_ADDRESS_OFFSET_C to be the number of void*
dnl between a frame pointer and the next frame pointer.
dnl
AC_DEFUN(CW_SYS_FRAME_ADDRESS_OFFSET,
[AC_REQUIRE([CW_SYS_RECURSIVE_BUILTIN_RETURN_ADDRESS])
CW_CONFIG_FRAME_ADDRESS_OFFSET=undef
if test "$cw_cv_sys_recursive_builtin_return_address" != "no"; then
AC_CACHE_CHECK(frame pointer offset in frame structure, cw_sys_frame_address_offset,
[save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS=""
CW_TRY_RUN([
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
    exit(0);	// This wasn't the real rest yet
  calculate_offset();
  exit(255);	// Failure
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

dnl CW_BUG_G_CONFIG_H
dnl Check if /usr/include/_G_config.h forgets to define a few macros
AC_DEFUN(CW_BUG_G_CONFIG_H,
[AC_LANG_SAVE
AC_LANG_C
AC_CHECK_FUNCS(labs)
AC_LANG_RESTORE
AC_CACHE_CHECK([whether _G_config.h forgets to define macros], cw_cv_sys_G_config_h_macros,
[AC_EGREP_CPP(_G_CLOG_CONFLICT,
[#ifndef HAVE__G_CONFIG_H
#include <_G_config.h>
#endif
#ifndef _G_CLOG_CONFLICT
_G_CLOG_CONFLICT
#endif
], cw_cv_sys_G_config_h_macros=_G_CLOG_CONFLICT, cw_cv_sys_G_config_h_macros=no)
AC_EGREP_CPP(_G_HAS_LABS,
[#ifdef HAVE__G_CONFIG_H
#include <_G_config.h>
#endif
#ifndef _G_HAS_LABS
_G_HAS_LABS
#endif
], [if test "$cw_cv_sys_G_config_h_macros" = "no"; then
  cw_cv_sys_G_config_h_macros=_G_HAS_LABS
else
  cw_cv_sys_G_config_h_macros="$cw_cv_sys_G_config_h_macros _G_HAS_LABS"
fi])])
if test "$cw_cv_sys_G_config_h_macros" != no; then
  CW_CONFIG_G_CONFIG_H_MACROS=define
else
  CW_CONFIG_G_CONFIG_H_MACROS=undef
fi
AC_SUBST(CW_CONFIG_G_CONFIG_H_MACROS)
if test "$ac_cv_func_labs" = yes; then
  CW_HAVE_LABS=1
else
  CW_HAVE_LABS=0
fi
AC_SUBST(CW_HAVE_LABS)
])

dnl CW_RPM
dnl Generate directories and files needed for an rpm target.
dnl For this to work, the Makefile.am needs to contain the string @RPM_TARGET@
AC_DEFUN(CW_RPM,
[if test "$USE_MAINTAINER_MODE" = yes; then
AC_CHECK_PROGS(rpm)
if test "$ac_cv_prog_rpm" = yes; then

fi
fi])

dnl CW_PIPE_EXTRAOPTS
dnl If the compiler understands -pipe, add it to EXTRAOPTS if not already.
AC_DEFUN(CW_PIPE_EXTRAOPTS,
[AC_MSG_CHECKING([if the compiler understands -pipe])
AC_CACHE_VAL(cw_cv_pipe_flag,
[save_CXXFLAGS="$CXXFLAGS"
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
CXXFLAGS="-pipe"
AC_TRY_COMPILE(,,cw_cv_pipe_flag=yes,cw_cv_pipe_flag=no)
AC_LANG_RESTORE
CXXFLAGS="$save_CXXFLAGS"])
if test "$cw_cv_pipe_flag" = yes ; then
  AC_MSG_RESULT(yes)
  x=`echo "$EXTRAOPTS" | grep 'pipe' 2>/dev/null`
  if test "$x" = "" ; then
    EXTRAOPTS="$EXTRAOPTS -pipe"
  fi
else
  AC_MSG_RESULT(no)
fi
])

dnl CW_SYS_PS_WIDE_PID_OPTION
dnl Determines the options needed for `ps' to print the full command of a specified PID
AC_DEFUN(CW_SYS_PS_WIDE_PID_OPTION,
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
AC_DEFINE_UNQUOTED([PS_ARGUMENT], "$cw_cv_sys_ps_wide_pid_option")
])

dnl --------------------------------------------------------------------------------------------------------------17" monitors are a minimum
dnl Everything below isn't really a test but parts that are used in both, libcwd as well as in libcw.
dnl In order to avoid duplication it is put here as macro.

dnl CW_SETUP_RPM_DIRS
dnl Set up rpm directory when on linux and in maintainer-mode
AC_DEFUN(CW_SETUP_RPM_DIRS,
[if test "$USE_MAINTAINER_MODE" = yes; then
  LSMFILE="$PACKAGE.lsm"
  AC_SUBST(LSMFILE)
  SPECFILE="$PACKAGE.spec"
  AC_SUBST(SPECFILE)
  if expr "$host" : ".*linux.*" >/dev/null; then
    top_builddir="`pwd`"
    test -d rpm || mkdir rpm
    cd rpm
    test -d BUILD || mkdir BUILD
    test -d SOURCES || mkdir SOURCES
    test -d SRPMS || mkdir SRPMS
    test -d RPMS || mkdir RPMS
    cd RPMS
    TARGET=i386
    AC_SUBST(TARGET)
    test -d $TARGET || mkdir $TARGET
    cd ..
    echo "%_require_vendor 1" > macros
    echo "%_require_distribution 1" >> macros
    echo "%_distribution http://sourceforge.net/project/showfiles.php?group_id=47536" >> macros
    echo "%vendor Carlo Wood" >> macros
    echo "%_topdir "$top_builddir"/rpm" >> macros
    echo "%_pgp_path "$PGPPATH >> macros
    echo "%_signature pgp5" >> macros
    echo "%_pgp_name carlo@alinoe.com" >> macros
    echo "macrofiles: /usr/lib/rpm/macros:"$top_builddir"/rpm/macros" > rpmrc
    echo "buildarchtranslate: i686: i386" >> rpmrc
    cd ..
  fi
fi
])

dnl CW_CLEAN_CACHE
AC_DEFUN(CW_CLEAN_CACHE,
[AC_MSG_CHECKING([if we can use cached results for the tests])
CW_PROG_CXX_FINGER_PRINTS
if test "$cw_cv_sys_CXX_finger_print" != "$cw_prog_cxx_finger_print" -o \
        "$cw_cv_sys_CXXCPP_finger_print" != "$cw_prog_cxxcpp_finger_print" -o \
	"$cw_cv_sys_CC_finger_print" != "$cw_prog_cc_finger_print" -o \
        "$cw_cv_sys_CPPFLAGS" != "$CPPFLAGS" -o \
        "$cw_cv_sys_CXXFLAGS" != "$CXXFLAGS" -o \
        "$cw_cv_sys_LDFLAGS" != "$LDFLAGS" -o \
        "$cw_cv_sys_LIBS" != "$LIBS"; then
changequote(<<, >>)dnl
for i in `set | grep -v '^ac_cv_prog_[Ccg][CXx]' | grep '^[a-z]*_cv_' | sed -e 's/=.*$//'`; do
  unset $i
done
changequote([, ])dnl
AC_MSG_RESULT([no])
else
AC_MSG_RESULT([yes])
fi
dnl Store important environment variables in the cache file
cw_cv_sys_CXX_finger_print="$cw_prog_cxx_finger_print"
cw_cv_sys_CXXCPP_finger_print="$cw_prog_cxxcpp_finger_print"
cw_cv_sys_CC_finger_print="$cw_prog_cc_finger_print"
cw_cv_sys_CPPFLAGS="$CPPFLAGS"
cw_cv_sys_CXXFLAGS="$CXXFLAGS"
cw_cv_sys_LDFLAGS="$LDFLAGS"
cw_cv_sys_LIBS="$LIBS"
])

dnl CW_DO_OPTIONS
dnl Chose reasonable default values for WARNOPTS, DEBUGOPTS and EXTRAOPTS
AC_DEFUN(CW_DO_OPTIONS, [dnl
dnl Choose warning options to use
if test "$USE_MAINTAINER_MODE" = yes; then
AC_EGREP_CPP(Winline-broken,
[#if __GNUC__ < 3
Winline-broken
#endif
],
WARNOPTS="-Wall -Woverloaded-virtual -Wundef -Wpointer-arith -Wwrite-strings -Werror",
WARNOPTS="-Wall -Woverloaded-virtual -Wundef -Wpointer-arith -Wwrite-strings -Werror -Winline")
else
WARNOPTS=
fi
AC_SUBST(WARNOPTS)

dnl Stop automake from adding the `-I. -I. -I.' nonsense
AC_SUBST(DEFS)

dnl Find out which debugging options we need
AC_CANONICAL_HOST
case "$host" in
  *freebsd*) DEBUGOPTS=-ggdb ;; dnl FreeBSD needs -ggdb to include sourcefile:linenumber info in its object files.
  *) DEBUGOPTS=-g ;;
esac
AC_SUBST(DEBUGOPTS)

dnl Other options
dnl -fno-exceptions is really only needed when using a compiler that was configured
dnl with --enable-slsj-exceptions, in order to avoid calls to calloc() from
dnl __pthread_setspecific when being 'internal'.
if test "$USE_MAINTAINER_MODE" = yes; then
EXTRAOPTS="-fno-exceptions"
else
EXTRAOPTS="-O -fno-exceptions"
fi
AC_SUBST(EXTRAOPTS)

dnl Test options
TESTOPTS=""
AC_SUBST(TESTOPTS)
])

dnl CW_ENVIRONMENT
dnl Load environment from cache when invoked as config.status --recheck.
dnl Always let CXX and CXXCPP override cached values
AC_DEFUN(CW_ENVIRONMENT,
[if test x"$no_create" = xyes -a x"$no_recursion" = xyes; then
  eval "CPPFLAGS=\"$cw_cv_sys_CPPFLAGS\""
  eval "CXXFLAGS=\"$cw_cv_sys_CXXFLAGS\""
  eval "LDFLAGS=\"$cw_cv_sys_LDFLAGS\""
  eval "LIBS=\"$cw_cv_sys_LIBS\""
fi
if test x"$CXX" != "x" -o x"$CXXCPP" != "x"; then
  unset ac_cv_prog_CXX
  unset ac_cv_prog_CXXCPP
  unset ac_cv_prog_cxx_cross
  unset ac_cv_prog_cxx_works
  unset ac_cv_prog_gxx
  unset ac_cv_prog_gxx_version
fi
if test x"$CC" != "x" -o x"$CPP" != "x"; then
  unset ac_cv_prog_CC
  unset ac_cv_prog_CPP
  unset ac_cv_prog_cc_cross
  unset ac_cv_prog_g
  unset ac_cv_prog_cc_works
  unset ac_cv_prog_gcc
fi
])

dnl CW_COMPILER_VERSIONS
dnl Test if the version of both, C and C++ compiler match.
AC_DEFUN(CW_COMPILER_VERSIONS,
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

dnl CW_RPATH_OPTION
dnl Figure out the commandline option to gcc needed to pass
dnl a runtime path to the linker.
AC_DEFUN(CW_RPATH_OPTION,
[AC_CACHE_CHECK([how to pass a runtime path to the linker], cw_cv_rpath_option,
[save_CXXFLAGS="$CXXFLAGS"
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
CXXFLAGS="$save_CXXFLAGS -Wl,--rpath,/tmp"
AC_TRY_LINK(,,[cw_cv_rpath_option="-Wl,--rpath,"],[cw_cv_rpath_option="-Wl,-R"])
AC_LANG_RESTORE
CXXFLAGS="$save_CXXFLAGS"])
RPATH_OPTION="$cw_cv_rpath_option"
AC_SUBST(RPATH_OPTION)
])

