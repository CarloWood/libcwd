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
int main() { return 0; }
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

dnl CW_OUTPUT([FILE... [, EXTRA_CMDS [, INIT-CMDS]]])
dnl
dnl Like AC_OUTPUT, but preserve the timestamp of the output files
dnl when they are not changed.
dnl
AC_DEFUN(CW_OUTPUT,
[AC_OUTPUT_COMMANDS(
[for cw_outfile in $1; do
  if test -f $cw_outfile.$cw_pid; then
    if cmp -s $cw_outfile $cw_outfile.$cw_pid 2>/dev/null; then
      echo "$cw_outfile is unchanged"
      mv $cw_outfile.$cw_pid $cw_outfile
    fi
  fi
  rm -f $cw_outfile.$cw_pid
done], [cw_pid=$cw_pid])
cw_pid=$$
if test "$no_create" != yes; then
  for cw_outfile in $1; do
    if test -f $cw_outfile; then
      mv $cw_outfile $cw_outfile.$cw_pid
    fi
  done
fi]
dnl `automake' looks for AC_OUTPUT and thinks `$1.in' etc.
dnl is a literally required file unless we fool it a bit here:
[AC_OUTPUT]([$1], [$2], [$3]))

dnl CW_DEFINE_TYPE_INITIALIZATION
dnl
AC_DEFUN(CW_DEFINE_TYPE_INITIALIZATION,
CW_TYPEDEFS=
dnl We don't want automake to put this in Makefile.in
[AC_SUBST](CW_TYPEDEFS))

dnl CW_DEFINE_TYPE(NEWTYPE, OLDTYPE)
dnl
dnl Add `typedef OLDTYPE NEWTYPE' to the output variable CW_TYPEDEFS
dnl
AC_DEFUN(CW_DEFINE_TYPE,
[AC_REQUIRE([CW_DEFINE_TYPE_INITIALIZATION])
CW_TYPEDEFS="$CW_TYPEDEFS\\
typedef $2 $1;"
])

dnl CW_TYPE_EXTRACT_FROM(FUNCTION, INIT, ARGUMENTS, ARGUMENT)
dnl
dnl Extract the type of ARGUMENT argument of function FUNCTION with ARGUMENTS arguments.
dnl INIT are possibly needed #includes.  The result is put in `cw_result'.
dnl
AC_DEFUN(CW_TYPE_EXTRACT_FROM,
[cat > conftest.$ac_ext <<EOF
[$2]
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
  return 0;
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
  if test -z "$cw_result"; then
    AC_MSG_ERROR(Configure problem: Failed to determine type)
  fi
changequote([, ])dnl
else
  echo "configure: failed program was:" >&5
  cat conftest.$ac_ext >&5
  AC_MSG_ERROR(Configuration problem: Failed to compile a test program)
fi
CXXFLAGS="$save_CXXFLAGS"
rm -f conftest*
])

dnl CW_TYPE_SOCKLEN_T
dnl
dnl If `socklen_t' is not defined in sys/socket.h,
dnl define it to be the type of the fifth argument
dnl of `setsockopt'.
dnl
AC_DEFUN(CW_TYPE_SOCKLEN_T,
[AC_CACHE_CHECK(type socklen_t, cw_cv_type_socklen_t,
[AC_EGREP_HEADER(socklen_t, sys/socket.h, cw_cv_type_socklen_t=exists,
[CW_TYPE_EXTRACT_FROM(setsockopt, [#include <sys/types.h>
#include <sys/socket.h>], 5, 5)
eval "cw_cv_type_socklen_t=\"$cw_result\""])])
if test "$cw_cv_type_socklen_t" != exists; then
  CW_DEFINE_TYPE(socklen_t, [$cw_cv_type_socklen_t])
fi
])

dnl CW_TYPE_OPTVAL_T
dnl
dnl If `optval_t' is not defined in sys/socket.h,
dnl define it to be the type of the fourth argument
dnl of `getsockopt'.
dnl
AC_DEFUN(CW_TYPE_OPTVAL_T,
[AC_CACHE_CHECK(type optval_t, cw_cv_type_optval_t,
[AC_EGREP_HEADER(optval_t, sys/socket.h, cw_cv_type_optval_t=exists,
[CW_TYPE_EXTRACT_FROM(getsockopt, [#include <sys/types.h>
#include <sys/socket.h>], 5, 4)
eval "cw_cv_type_optval_t=\"$cw_result\""])])
if test "$cw_cv_type_optval_t" != exists; then
  CW_DEFINE_TYPE(optval_t, [$cw_cv_type_optval_t])
fi
])

dnl CW_TYPE_SIGHANDLER_PARAM_T
dnl
dnl If `sighandler_param_t' is not defined in signal.h,
dnl define it to be the type of the the first argument of `SIG_IGN'.
dnl
AC_DEFUN(CW_TYPE_SIGHANDLER_PARAM_T,
[AC_CACHE_CHECK(type sighandler_param_t, cw_cv_type_sighandler_param_t,
[AC_EGREP_HEADER(sighandler_param_t, signal.h, cw_cv_type_sighandler_param_t=exists,
[CW_TYPE_EXTRACT_FROM(SIG_IGN, [#include <signal.h>], 1, 1)
eval "cw_cv_type_sighandler_param_t=\"$cw_result\""])])
if test "$cw_cv_type_sighandler_param_t" != exists; then
  CW_DEFINE_TYPE(sighandler_param_t, [$cw_cv_type_sighandler_param_t])
fi
])

dnl CW_MALLOC_OVERHEAD
dnl
dnl Defines CW_MALLOC_OVERHEAD_C to be the number of bytes extra
dnl allocated for a call to malloc.
dnl
AC_DEFUN(CW_MALLOC_OVERHEAD,
[AC_CACHE_CHECK(malloc overhead in bytes, cw_cv_system_mallocoverhead,
[AC_TRY_RUN([#include <cstdlib>
#include <cstddef>

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
    exit(0);	// This wasn't the real rest yet
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
dnl Defines CW_CONFIG_NEED_WORD_ALIGNMENT to `define' or `undef'
dnl when the host needs respectively size_t alignment or not.
AC_DEFUN(CW_NEED_WORD_ALIGNMENT,
[AC_CACHE_CHECK(if machine needs word alignment, cw_cv_system_needwordalignment,
[AC_TRY_RUN([#include <cstddef>
#include <cstdlib>

int main(void)
{
  size_t* p = reinterpret_cast<size_t*>((char*)malloc(5) + 1);
  *p = 0x12345678;
  exit ((((unsigned long)p & 1UL) && *p == 0x12345678) ? 0 : -1);
}],
cw_cv_system_needwordalignment=no,
cw_cv_system_needwordalignment=yes,
cw_cv_system_needwordalignment="why not")])
if test "$cw_cv_system_needwordalignment" = no; then
  CW_CONFIG_NEED_WORD_ALIGNMENT=undef
else
  CW_CONFIG_NEED_WORD_ALIGNMENT=define
fi
AC_SUBST(CW_CONFIG_NEED_WORD_ALIGNMENT)
])

dnl CW_NBLOCK
dnl
dnl Defines CW_CONFIG_NBLOCK to be `POSIX', `BSD' or `SYSV'
dnl depending on whether socket non-blocking stuff is
dnl posix, bsd or sysv style respectively.
AC_DEFUN(CW_NBLOCK,
[save_LIBS="$LIBS"
AC_CHECK_LIB(c, socket, [true],
[AC_CHECK_LIB(socket, socket, LIBS="-lsocket $LIBS")])
AC_CACHE_CHECK(non-blocking socket flavour, cw_cv_system_nblock,
[AC_REQUIRE([AC_TYPE_SIGNAL])
AC_REQUIRE([CW_TYPE_SIGHANDLER_PARAM_T])
CW_TYPE_EXTRACT_FROM(recvfrom, [#include <sys/types.h>
#include <sys/socket.h>], 6, 6)
cw_recvfrom_param_six_t="$cw_result"
AC_TRY_RUN([#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <signal.h>
#include <unistd.h>
$ac_cv_type_signal alarmed($cw_cv_type_sighandler_param_t) { exit(1); }
int main(int argc, char* argv[])
{
  if (argc == 1)
    exit(0);
  char b[12];
  struct sockaddr x;
  size_t l = sizeof(x);
  int f = socket(AF_INET, SOCK_DGRAM, 0);
  if (f >= 0 && !(fcntl(f, F_SETFL, (*argv[1] == 'P') ? O_NONBLOCK : O_NDELAY)))
  {
    signal(SIGALRM, alarmed);
    alarm(2);
    recvfrom(f, b, 12, 0, &x, ($cw_recvfrom_param_six_t)&l);
    alarm(0);
    exit(0);
  }
  exit(1);
}],
[./conftest POSIX
if test "$?" = "0"; then
  cw_cv_system_nblock=POSIX
else
  ./conftest BSD
  if test "$?" = "0"; then
    cw_cv_system_nblock=BSD
  else
    cw_cv_system_nblock=SYSV
  fi
fi],
[AC_MSG_ERROR(Failed to compile a test program!?)],
[cw_cv_system_nblock=crosscompiled_set_to_POSIX_BSD_or_SYSV
AC_CACHE_SAVE
AC_MSG_WARN(Cannot set cw_cv_system_nblock for unknown platform (you are cross-compiling).)
AC_MSG_ERROR(Please edit config.cache and rerun ./configure to correct this!)])])
if test "$cw_cv_system_nblock" = crosscompiled_set_to_POSIX_BSD_or_SYSV; then
  AC_MSG_ERROR(Please edit config.cache and correct the value of cw_cv_system_nblock, then rerun ./configure)
fi
CW_CONFIG_NBLOCK=$cw_cv_system_nblock
AC_SUBST(CW_CONFIG_NBLOCK)
LIBS="$save_LIBS"])

dnl CW_TYPE_GETGROUPS
dnl
dnl Like AC_TYPE_GETGROUPS but with bug fix for C++ and adding a
dnl typedef getgroups_t instead of defining the macro GETGROUPS_T.
AC_DEFUN(CW_TYPE_GETGROUPS,
[AC_REQUIRE([AC_TYPE_UID_T])dnl
AC_CACHE_CHECK(type of array argument to getgroups, ac_cv_type_getgroups,
[AC_TRY_RUN(
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
  n = getgroups (sizeof (gidset) / MAX (sizeof (int), sizeof (gid_t)) - 1,
                 gidset);
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
[AC_BEFORE([$0], [AC_PROG_CXXCPP])dnl
AC_CHECK_PROGS(CXX, g++ c++)
AC_PROG_CXX_WORKS
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
[dnl This triggers the bug:
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
dnl Extract finger prints of C++ compiler and preprocessor
AC_DEFUN(CW_PROG_CXX_FINGER_PRINTS,
[AC_REQUIRE([CW_PROG_CXX])
AC_REQUIRE([CW_PROG_CXXCPP])
cw_prog_cxx_finger_print="`$CXX -v 2>&1 | grep version | head -n 1`"
cw_prog_cxxcpp_finger_print="`echo | $CXXCPP -v 2>&1 | grep version | head -n 1`"
])

dnl CW_SYS_BUILTIN_RETURN_ADDRESS_OFFSET
dnl
dnl Determines if an offset of -1 works or not.
dnl Assumed is that the test program compiles and works,
dnl if anything fails then an offset of -1 is assumed.
AC_DEFUN(CW_SYS_BUILTIN_RETURN_ADDRESS_OFFSET,
[AC_CACHE_CHECK(needed offset to __builtin_return_address(), cw_cv_sys_builtin_return_address_offset,
[AC_TRY_RUN(
changequote(<<, >>)dnl
<<#include <cstdlib>
#include <bfd.h>
void* addr2;
void test2(void)
{
  addr2 = __builtin_return_address(0);
}

unsigned int const realline = __LINE__ + 3;
void test(void)
{
  test2();
}

int main(int argc, char* argv[])
{
  bfd_init();
  bfd* abfd = bfd_openr(argv[0], NULL);
  bfd_check_format (abfd, bfd_archive);
  char** matching;
  bfd_check_format_matches (abfd, bfd_object, &matching);
  long storage_needed = bfd_get_symtab_upper_bound (abfd);
  asymbol** symbol_table = (asymbol**) malloc(storage_needed);
  long number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
  asymbol** se = &symbol_table[number_of_symbols - 1];
  for (asymbol** s = symbol_table; s <= se; ++s)
    if (!strcmp((*s)->name, "main"))
    {
      asection* sect = bfd_get_section(*s);
      char const* file;
      char const* func;
      unsigned int line;
      test();
      bfd_find_nearest_line(abfd, sect, symbol_table, (unsigned int)((char*)addr2 - sect->vma) - 1, &file, &func, &line);
      exit((line == realline) ? 1 : 0);
    }
  exit(1);
}
>>,
changequote([, ])dnl
cw_cv_sys_builtin_return_address_offset=0,
cw_cv_sys_builtin_return_address_offset=-1,
cw_cv_sys_builtin_return_address_offset=-1)])
eval "CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET=\"$cw_cv_sys_builtin_return_address_offset\""
AC_SUBST(CW_CONFIG_BUILTIN_RETURN_ADDRESS_OFFSET)
])

dnl CW_BUG_G_CONFIG_H
dnl Check if /usr/include/_G_config.h forgets to define a few macros
AC_DEFUN(CW_BUG_G_CONFIG_H,
[AC_CHECK_FUNCS(labs)
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
