#! /bin/sh

# Helps bootstrapping the application when checked out from CVS.
# Requires GNU autoconf, GNU automake and GNU which.
#
# Copyright (C) 2004, by
#
# Carlo Wood, Run on IRC <carlo@alinoe.com>
# RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
# Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
#

# Helps bootstrapping a project that was checked out from CVS.
# Requires GNU autoconf, GNU automake, GNU libtool and GNU which.

# Do sanity checks.
# Directory check.
if [ ! -f autogen.sh ]; then
  echo "Run ./autogen.sh from the directory it exists in."
  exit 1
fi

# Clueless user check.
if test ! -d CVS -a -f configure; then
  echo "You only need to run './autogen.sh' when you checked out this project using CVS."
  echo "Just run ./configure [--help]."
  exit 0
fi

AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOCONF=${AUTOCONF:-autoconf}

# libtool versions prior to 1.4.2 are broken.
# On OS other than Linux we require 1.4d or higher because libtool
# doesn't support shared C++ libraries in earlier versions.
os=`uname -s`
if test "$os" = "Linux"; then
  required_libtool_version="1.4.2";
else
  required_libtool_version="1.5";	# Version 1.4.2 will not work.  1.4d should work,
  					# see http://www.gnu.org/software/libtool/libtool.html
fi

($AUTOCONF --version) >/dev/null 2>/dev/null || (echo "You need GNU autoconf to install from CVS (ftp://ftp.gnu.org/gnu/autoconf/)"; exit 1) || exit 1
($AUTOMAKE --version) >/dev/null 2>/dev/null || (echo "You need GNU automake 1.6 or higher to install from CVS (ftp://ftp.gnu.org/gnu/automake/)"; exit 1) || exit 1
(libtool --version) >/dev/null 2>/dev/null || (echo "You need GNU libtool $required_libtool_version or higher to install from CVS (ftp://ftp.gnu.org/gnu/libtool/)"; exit 1) || exit 1

# Determine the version of libtool.
libtool_version=`libtool --version | head -n 1 | sed -e 's/[^12]*\([12]\.[0-9][^ ]*\).*/\1/'`
libtool_develversion=`libtool --version | head -n 1 | sed -e 's/.*[12]\.[0-9].*(\([^ ]*\).*/\1/'`

# Require required_libtool_version.
expr_libtool_version=`echo "$libtool_version" | sed -e 's%\.%.000%g' -e 's%^%000%' -e 's%0*\([0-9][0-9][0-9]\)%\1%g'`
expr_required_libtool_version=`echo "$required_libtool_version" | sed -e 's%\.%.000%g' -e 's%^%000%' -e 's%0*\([0-9][0-9][0-9]\)%\1%g'`
if expr "$expr_required_libtool_version" \> "$expr_libtool_version" >/dev/null; then
  libtool --version
  echo ""
  echo "Fatal error: libtool version $required_libtool_version or higher is required."
  exit 1
fi
libtool_files="config.guess config.sub ltmain.sh"

# Determine the version of automake.
automake_version=`$AUTOMAKE --version | head -n 1 | sed -e 's/[^12]*\([12]\.[0-9][^ ]*\).*/\1/'`

# Require automake 1.7 because 1.6 and below are broken.
expr_automake_version=`echo "$automake_version" | sed -e 's%\.%.000%g' -e 's%^%000%' -e 's%0*\([0-9][0-9][0-9]\)%\1%g'`
if expr "001.007" \> "$expr_automake_version" >/dev/null; then
  $AUTOMAKE --version | head -n 1
  echo "Fatal error: automake version 1.7 or higher is required."
  exit 1
fi

# Determine the version of autoconf.
autoconf_version=`$AUTOCONF --version | head -n 1 | sed -e 's/[^0-9]*\([0-9]\.[0-9]*\).*/\1/'`

# Require autoconf 2.57.
expr_autoconf_version=`echo "$autoconf_version" | sed -e 's%\.%.000%g' -e 's%^%000%' -e 's%0*\([0-9][0-9][0-9]\)%\1%g'`
if expr "002.057" \> "$expr_autoconf_version" >/dev/null; then
  $AUTOCONF --version | head -n 1
  echo "Fatal error: autoconf version 2.57 or higher is required."
  exit 1
fi

# Determine the installation paths of a few tools.
# This extracts from the automake script the two lines:
#   $prefix = "/usr";
#   $am_dir = "${prefix}/share/automake";
# OR (automake version 1.4):
#   $am_dir = "/usr/share/automake";
# OR (automake version 1.5):
#   my $libdir = "/usr/share/automake";
# OR (automake version 1.6):
#   my $libdir = "${prefix}/share/automake";
# OR (automake version 1.7):
#  my $libdir = '/usr/share/automake-1.7';
# OR (automake version 1.8):
#  my $perllibdir = $ENV{'perllibdir'} || '/usr/share/automake-1.8';
# and then puts "/usr/share/automake" into the variable automake_dir.
if test "`basename $AUTOMAKE`" = "automake"; then
  major_version=`echo "$automake_version" | sed -e 's/\([0-9]*\.[0-9]*\).*/\1/'`
  automake_path2="`which $AUTOMAKE-$major_version`"
  if test -x "$automake_path2"; then
    AUTOMAKE="$AUTOMAKE-$major_version"
  fi
fi
automake_path=`which $AUTOMAKE`
automake_prefix=`egrep '(^my |^)\\$prefix = ' $automake_path | sed -e 's/.*"\(.*\)".*/\1/'`
automake_tmp=`egrep '(^my \\$libdir|^\\$am_dir|my \\$perllibdir) = ' $automake_path | tail -n 1 | sed -e 's/.*"\(\${prefix}\)\(.*\)".*/$automake_prefix\2/' | sed -e 's/.*['"'"'"]\([^'"'"'"]*\)['"'"'"].*/\1/'`
eval "automake_dir=$automake_tmp"
if test -f $automake_dir/depcomp; then
  # Do not include depcomp in this list: we wrote our own!
  automake_files="install-sh missing mkinstalldirs"
  # automake 1.6 also use $automake_dir/compile, but we don't need that because we only support gcc.
else
  automake_files="install-sh missing mkinstalldirs"
fi
if ! test -f $automake_dir/missing; then
  echo "Failed to determine the automake directory.  Sorry."
  exit 1
fi

# Assume that this test is not necessary when we failed find a automake_prefix.
if test -n "$automake_prefix"; then
  # Check where aclocal reads its m4 scripts from and check if that is where libtool stored its scripts.
  share_aclocal=`$ACLOCAL --print-ac-dir`
  share_aclocal_inode=`ls -id "$share_aclocal" | sed -e 's/[^0-9]*\([0-9]*\).*/\1/'`
  share_libtool=`which libtool | sed -e 's%/bin/libtool%/share/aclocal%'`
  share_libtool_inode=`ls -id "$share_libtool" | sed -e 's/[^0-9]*\([0-9]*\).*/\1/'`
  if test "$share_aclocal_inode" != "$share_libtool_inode"; then
    libtool_prefix=`which libtool | sed -e 's%/bin/libtool%%'`
    echo "Fatal error: automake is not installed with the same prefix as libtool."
    echo "This causes aclocal to fail reading the libtool macros or it might read the wrong ones."
    echo "To fix this you can do one of two things:"
    echo "1. Remove the installation of libtool in \"$libtool_prefix\" and reinstall it with prefix \"$automake_prefix\"."
    echo "2. Install automake with prefix \"$libtool_prefix\"."
    echo "You can install more than one version of libtool, but you will need to re-install automake for each of them."
    echo "Then you can choose which version to use by adjusting your PATH."
    exit 1
  fi
fi

# Check if bootstrap was run before and if the installed files are the same version.
if test -f ltmain.sh; then
  installed_libtool=`grep '^VERSION=' ltmain.sh | sed -e 's/.*\([12]\.[^ ]*\).*/\1/'`
  installed_timestamp=`grep '^TIMESTAMP=' ltmain.sh | sed -e 's/.*(\([0-9]*\.[^ ]*\).*/\1/'`
  if test "$installed_libtool" != "$libtool_version" -o X"$installed_timestamp" != X"$libtool_develversion"; then
    echo "Re-installing new libtool files ($installed_libtool ($installed_timestamp) -> $libtool_version ($libtool_develversion))"
    rm -f config.guess config.sub ltmain.sh ltconfig
  fi
fi

# Generate automake/autoconf/libtool files:

# This is needed when someone just upgraded automake and this cache is still generated by an old version.
rm -rf autom4te.cache config.cache

echo "*** Generating automake/autoconf/libtool files:"
echo "    aclocal.m4 ..."
$ACLOCAL
echo "    config.h.in ..."
$AUTOHEADER 2>&1 | egrep -v '(config.h.in.* is (updated|created)|config.h.in.* is unchanged|warning: AC_TRY_RUN|^WARNING:|^$)'
echo timestamp > stamp-h.in 2> /dev/null
echo "    configure ..."
$AUTOCONF 2>&1 | egrep -v 'warning: AC_TRY_RUN|warning: do not use m4_(patsubst|regexp):'
# The --add-missing --copy of automake is broken, so we do it ourselfs.
# We need to copy install-sh anyway before running libtoolize.
for i in $automake_files; do	# install-sh, missing, mkinstalldirs[, depcomp]
  test -f ./$i && cmp -s ./$i $automake_dir/$i || if true; then
    echo "    $i ..."
    cp $automake_dir/$i ./$i;
  fi
done
echo "    config.guess, config.sub, ltmain.sh ..."
libtoolize --automake --copy	# config.guess, config.sub, ltmain.sh
echo "    Makefile.in ..."
$AUTOMAKE			# Makefile.in

# Sanity check
for i in aclocal.m4 config.h.in configure $automake_files $libtool_files; do
  if test ! -f $i; then
    echo "Warning: missing \"$i\" in `pwd`"
  fi
done

echo "*** done"
