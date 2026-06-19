#!/bin/sh

set -eu

# Transform the generated Doxygen layout template into the layout consumed by the documentation build.
#
# The first optional argument is the input template path and defaults to doxygen.layout.xml.in.
# The second optional argument is the output layout path and defaults to doxygen.layout.xml.
# The output is overwritten after replacing Doxygen's empty main-page title marker with MAINPAGE.
input=${1:-doxygen.layout.xml.in}
output=${2:-doxygen.layout.xml}

# Remove $output - it is regenerated from scratch.
echo -n > "$output"

# Create a temporary file for the replacement snippet.
temp_snippet=$(mktemp)

# Write the replacement snippet to the temporary file.
cat > "$temp_snippet" << 'EOF'
    <tab type="usergroup" url="@ref reference-manual"
         visible="yes" title="Reference Manual" intro="">
      <tab type="user" url="@ref reference-manual-getting-started"
           visible="yes" title="Configuration, Installation And Getting Started" intro=""/>
      <tab type="user" url="@ref reference-manual-writing-debug-output"
           visible="yes" title="Writing Debug Output" intro=""/>
      <tab type="user" url="@ref reference-manual-symbols"
           visible="yes" title="Symbols Access And Interpretation" intro=""/>
      <tab type="user" url="@ref reference-manual-runtime"
           visible="yes" title="Miscellaneous Runtime" intro=""/>
    </tab>
EOF

# Process the input file.
found=false
while IFS= read -r line || [ -n "$line" ]; do
  # Check if the line contains the search string.
  if [[ "$line" == *'<tab type="pages" '* ]] && [ "$found" = false ]; then
    # Found the line, replace it with the snippet.
    cat "$temp_snippet" >> "$output"
    found=true
  else
    # Write the line to output.
    echo "$line" >> "$output"
  fi
done < "$input"

# Clean up temporary file.
rm -f "$temp_snippet"

# If the pattern wasn't found, output a warning.
if [ "$found" = false ]; then
  echo "Warning: Pattern '<tab type=\"pages\" ' was not found in the input file" >&2
fi
