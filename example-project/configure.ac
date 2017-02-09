# Package name and version.
AC_INIT([project], [0.0.1], [project-bugs@yourmail.org])

# Ensure that the directory specified with --srcdir was correct
# by demanding that it contains `program.cc'.
AC_CONFIG_SRCDIR([program.cc])

# Put resulting configuration defines in this header file.
AM_CONFIG_HEADER(config.h)

# Automake options.
AM_INIT_AUTOMAKE

# Demand at least version 2.57 of autoconf.
AC_PREREQ(2.57)

# Include --enable-maintainer-mode option.
AM_MAINTAINER_MODE

# Manual configuration options of project.
AC_ARG_ENABLE(debugging, [  --enable-debugging      enable debugging code.])

# Check for compiler and preprocessor
AC_PROG_CXX
AC_PROG_CXXCPP

# Checks for other programs.
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Set language requirements for the tests below.
AC_LANG_CPLUSPLUS
AC_REQUIRE_CPP

# Configuration defines

# Check if we have libcwd, $enable_debugging can be "yes", "no" or empty (auto).
test -n "$enable_debugging" || enable_debugging=no	# Set the default to an explicit "no".
# This sets 'cw_use_libcwd' to "yes" if libcwd is actually used; CXXFLAGS and LDFLAGS are updated.
CW_LIB_LIBCWD([debugging], [$enable_debugging], [no])

# Generate output files.
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

