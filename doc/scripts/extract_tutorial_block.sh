#!/bin/sh
# Extract one named tutorial code block from a tutorial source file.
#
# Arguments are the tutorial input, the block name as it appears in START/END CODE comments,
# and the generated output path. The script decodes the HTML entities used in displayed code
# and strips highlighting spans so the generated source can be compiled directly.

set -eu

input=$1
block=$2
output=$3
tmp=${output}.tmp

mkdir -p "$(dirname "${output}")"

awk -v block="${block}" '
  BEGIN {
    start = "<!-- START CODE " block " -->";
    stop = "<!-- END CODE " block " -->";
  }
  $0 == start {
    in_block = 1;
    found = 1;
    next;
  }
  $0 == stop {
    in_block = 0;
    done = 1;
    next;
  }
  in_block {
    line = $0;
    gsub(/&lt;/, "<", line);
    gsub(/&gt;/, ">", line);
    gsub(/&quot;/, "\"", line);
    gsub(/&sect;/, "§", line);
    gsub(/&amp;/, "\\&", line);
    gsub(/<SPAN[^>]*>/, "", line);
    gsub(/<\/SPAN>/, "", line);
    print line;
  }
  END {
    if (!found || !done)
      exit 1;
  }
' "${input}" > "${tmp}"

mv "${tmp}" "${output}"
