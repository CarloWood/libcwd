/^<!-- START OUTPUT .* -->$/ {
  print;
  sub(/^<!-- START OUTPUT /,"",$0);
  sub(/ -->$/,"",$0);
  cmd=sprintf("%s 2>&1 | sed -e 's/&/\\&amp;/g' -e 's/</\\&lt;/g' -e 's/>/\\&gt;/g' -e 's/\"/\\&quot;/g' -e 's/§/\\&sect;/g'", $0);
  system(cmd);
  next;
}

{
  print;
}
