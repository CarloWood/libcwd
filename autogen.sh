#! /bin/bash

if test -f .git; then
  echo "Error: don't run $0 inside a submodule. Run it from the parent project's directory."
  exit 1
fi

if test "$(realpath $0)" != "$(realpath $(pwd)/autogen.sh)"; then
  echo "Error: run autogen.sh from the directory it resides in."
  exit 1
fi

if test -d .git; then
  # Take care of git submodule related stuff.
  MAINTAINER_HASH=dcc3e4640e3ff4769e3cee4a2ab8e5eb
  # If this was a clone without --recursive, fix that fact.
  if test ! -e cwm4/scripts/real_maintainer.sh; then
    git submodule update --init --recursive
  fi
  # If new git submodules were added by someone else, get them.
  if git submodule status --recursive | grep '^-' >/dev/null; then
    git submodule update --init --recursive
  fi
  # Update autogen.sh and cwm4 itself if we are the real maintainer.
  if test -f cwm4/scripts/real_maintainer.sh; then
    cwm4/scripts/real_maintainer.sh $MAINTAINER_HASH
    RET=$?
    # A return value of 2 means we need to continue below.
    # Otherwise, abort/stop returning that value.
    if test $RET -ne 2; then
      exit $RET
    fi
  fi
else
  # Clueless user check.
  if test ! -e cwm4/scripts/real_maintainer.sh; then
    echo "Houston, we have a problem: the cwm4 git submodule is missing from your source tree!?"
    echo "I'd suggest to clone the source code of this project from github:"
    echo "git clone --recursive https://github.com/CarloWood/libcwd.git"
    exit 1
  fi
fi

# Do some git sanity checks.
if test -d .git; then
  PUSH_RECURSESUBMODULES="$(git config push.recurseSubmodules)"
  if test -z "$PUSH_RECURSESUBMODULES"; then
    # Use this as default for now.
    git config push.recurseSubmodules check
    echo -e "\n*** WARNING: git config push.recurseSubmodules was not set!"
    echo "***      To prevent pushing a project that references unpushed submodules,"
    echo "***      this config was set to 'check'. Use instead the command"
    echo "***      > git config push.recurseSubmodules on-demand"
    echo "***      to automatically push submodules when pushing a reference to them."
    echo "***      See http://stackoverflow.com/a/10878273/1487069 and"
    echo "***      http://stackoverflow.com/a/34615803/1487069 for more info."
    echo
  fi
fi

if [ -e CMakeLists.txt ]; then
  # Set CMAKE_CONFIG to '$CMAKE_CONFIG' if not already set.
  : "${CMAKE_CONFIG:=\$CMAKE_CONFIG}"
  # Set BUILDDIR to '$BUILDDIR' if not already set.
  : "${BUILDDIR:=\$BUILDDIR}"

  echo -e "\nBuilding with cmake:\n"
  echo "To make a $CMAKE_CONFIG build, run:"
  [ -d "$BUILDDIR" ] || echo "mkdir -p \$BUILDDIR"
  echo -n "cmake -S \"\$REPOROOT\" -B \"\$BUILDDIR\" -DCMAKE_BUILD_TYPE=\"$CMAKE_CONFIG\""
  # Put quotes around options that contain spaces.
  for option in "${CMAKE_CONFIGURE_OPTIONS[@]}"; do
    if [[ $option == *" "* ]]; then
        printf ' "%s"' "$option"
    else
        printf ' %s' "$option"
    fi
  done
  echo
  echo "cmake --build \"\$BUILDDIR\" --config \"$CMAKE_CONFIG\" --parallel $(nproc)"
fi
