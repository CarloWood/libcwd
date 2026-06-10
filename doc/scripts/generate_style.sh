#!/bin/sh
# Generate one browser-specific stylesheet from a preprocessor template.
#
# Arguments are the C compiler, source styles directory, browser name, template file name,
# and generated output path. The browser name is upper-cased and passed as a preprocessor
# symbol so styles/defines.h can select browser-specific CSS fragments.

set -eu

cc=$1
source_dir=$2
browser=$3
template=$4
output=$5
macro=$(printf '%s' "${browser}" | tr '[:lower:]' '[:upper:]')
tmp=${output}.tmp

mkdir -p "$(dirname "${output}")"

cat "${source_dir}/${template}" | "${cc}" -I"${source_dir}" -D"${macro}" -E -c - |
  grep -E -v '^(#|$)' > "${tmp}"

mv "${tmp}" "${output}"
