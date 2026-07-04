#!/bin/sh
# Mirror the freshly built libcwd documentation site into the CarloWood.github.io
# checkout and commit the delta under libcwd/ only.
#
# Usage: syncdoc.sh <doc_build_dir>
#
# <doc_build_dir> is ${CMAKE_CURRENT_BINARY_DIR} of doc/, i.e. the directory
# doxygen wrote its HTML output into (this is where index.html lives, because
# doxygen.config.in sets OUTPUT_DIRECTORY=. and HTML_OUTPUT=.). It is passed in
# by the syncdoc custom target.
#
# The destination is $OUTPUT_DIRECTORY/libcwd, where $OUTPUT_DIRECTORY is a
# checkout of https://github.com/CarloWood/CarloWood.github.io.git (the published
# website). Only the libcwd/ subdirectory is touched: files no longer present
# (or now excluded) in the source are removed from the destination, new or
# changed files are staged, and a single commit records the delta. Nothing
# outside libcwd/ is staged or committed, so unrelated changes already waiting in
# the website repository are left alone.
#
# Requires rsync. $OUTPUT_DIRECTORY must be set in the environment and point at
# a checkout of the website repository; the script never writes anywhere else.

set -eu

# Needed so rsync's `--delete-excluded` can purge anything previously published
# that the exclude list below now drops (e.g. external/ after it was reclassified
# as a doxygen input rather than site content).
command -v rsync >/dev/null 2>&1 || { echo "syncdoc: rsync is required but was not found." >&2; exit 1; }

src="${1:-}"
if [ -z "$src" ] || [ ! -d "$src" ]; then
  echo "syncdoc: usage: $0 <doc_build_dir>" >&2
  echo "syncdoc: <doc_build_dir> must exist and contain the built index.html." >&2
  exit 1
fi

: "${OUTPUT_DIRECTORY:?OUTPUT_DIRECTORY is not set (point it at a checkout of CarloWood.github.io)}"
if [ ! -d "$OUTPUT_DIRECTORY" ]; then
  echo "syncdoc: OUTPUT_DIRECTORY '$OUTPUT_DIRECTORY' does not exist." >&2
  exit 1
fi

# Create libcwd/ inside the website checkout so `git -C` can run from it even on
# the first sync. It is a plain subdirectory of the website repository.
dest="$OUTPUT_DIRECTORY/libcwd"
mkdir -p "$dest"

# Sanity-check that the destination really lives inside the website git checkout
# (the .git directory is at the repository root, not under libcwd/).
if ! git -C "$dest" rev-parse --git-dir >/dev/null 2>&1; then
  echo "syncdoc: '$dest' is not inside a git repository." >&2
  echo "syncdoc: OUTPUT_DIRECTORY should be a checkout of CarloWood.github.io." >&2
  exit 1
fi

# Mirror the build directory verbatim, then prune anything that is not actual
# website content. The excludes fall into three groups:
#   * doxygen inputs: Introduction.md (configured), external/, doxygen-examples/,
#     tutorial/ and doxygen.config. These are read by doxygen and embedded into
#     the generated HTML; they are never linked from a page, so publishing them
#     serves no purpose (and tutorial/ even holds compiled example binaries).
#   * CMake/build machinery: CMakeFiles/, CMakeCache.txt, Makefile,
#     cmake_install.cmake, the ninja files and *.stamp.
#   * build helper scripts: only scripts/*.py (fix_navtree.py, fix_group_pages.py)
#     are dropped; scripts/libcwd-doxygen-theme.js stays because the pages load it.
# --delete makes the destination track source removals; --delete-excluded also
# purges anything matching the excludes that a previous run may have published
# before the file was reclassified, so the result is exactly "source minus
# excludes".
rsync -a --delete --delete-excluded \
  --exclude='CMakeFiles/' \
  --exclude='CMakeCache.txt' \
  --exclude='Makefile' \
  --exclude='cmake_install.cmake' \
  --exclude='build.ninja' \
  --exclude='.ninja_deps' \
  --exclude='.ninja_log' \
  --exclude='*.stamp' \
  --exclude='doxygen.config' \
  --exclude='Introduction.md' \
  --exclude='external/' \
  --exclude='doxygen-examples/' \
  --exclude='tutorial/' \
  --exclude='scripts/*.py' \
  --exclude='scripts/syncdoc.sh' \
  "$src/" "$dest/"

# Stage everything under libcwd/ only. Running git from $dest makes the pathspec
# "." refer to libcwd/, so nothing elsewhere in the website repository is staged.
git -C "$dest" add -A .

# Commit only if something under libcwd/ actually changed. diff --cached --quiet
# exits zero when there is nothing staged, so the negation means "we have a
# delta". The trailing "-- ." restricts the commit to libcwd/ even if the wider
# repository has unrelated staged changes waiting.
if ! git -C "$dest" diff --cached --quiet -- .; then
  git -C "$dest" commit -m "Update libcwd documentation" -- .
else
  echo "syncdoc: nothing to commit (libcwd documentation is up to date)."
fi
