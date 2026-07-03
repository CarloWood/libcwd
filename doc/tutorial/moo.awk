# moo.awk -- splice captured program output into the generated tutorial page.
#
# Input is tut5.in (markdown prose with <!-- START OUTPUT <command> --> markers
# inside fenced code blocks). For each marker the command (everything between
# "START OUTPUT " and " -->") is run with WORKING_DIRECTORY set to the build
# tutorial directory, so `examples5/<binary>` references resolve, and its
# combined stdout/stderr is HTML-escaped and spliced in place of the marker.
#
# The marker line itself is intentionally NOT echoed: in tut5.in it sits inside
# a markdown fenced block, and echoing it would make the literal `<!-- ... -->`
# comment visible to the reader of the generated tut5.md. Only the captured
# output is emitted, so the surrounding fence becomes a clean output block.

/^<!-- START OUTPUT .* -->$/ {
  sub(/^<!-- START OUTPUT /,"",$0);
  sub(/ -->$/,"",$0);
  # Run the command verbatim. Its output lands inside a markdown fenced block
  # in tut5.md, and doxygen HTML-escapes fenced content for display, so the
  # raw text must be spliced here without any pre-escaping (doing both would
  # double-escape into literal "&quot;" etc.).
  cmd=sprintf("%s 2>&1", $0);
  system(cmd);
  next;
}

{
  print;
}
