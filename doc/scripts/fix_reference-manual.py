#!/usr/bin/env python3
"""Post-process doxygen output after renaming the group landing page.

Doxygen emits the page for the `reference-manual` @defgroup as
`group__reference-manual.html`. The build renames that file to the cleaner
`reference-manual.html`, but every other generated artifact still points at the
original doxygen name:

  * navtreedata.js      -- the 'Reference Manual' node carries it as its URL.
  * navtreeindexN.js    -- the per-page tree path is keyed on it.
  * *.html              -- every cross-reference (breadcrumbs, 'Related Pages',
                           the in-page link from index.html, ...) links to it.
  * search/*.js         -- the search index entries point at it.

Left untouched those references would all 404 after the rename, so this script
rewrites them in place to the new name.

Only the *exact* string `group__reference-manual.html` is rewritten. Two
look-alikes are deliberately left alone:

  * `group__reference-manual` (without `.html`) in navtreedata.js is the
    lazy-load pointer to the `group__reference-manual.js` children file, which
    the build does not rename, so the pointer must stay as it is.
  * `group__reference-manual-getting-started.html` and the other subgroups keep
    their doxygen names; they never match the exact string anyway (an extra
    `-suffix` sits before `.html`), but this is worth stating explicitly.

The script is invoked with the path to navtreedata.js, the same entry point
fix_navtree.py is run on next; every other file is discovered relative to it.
It runs *before* fix_navtree.py so that script sees the corrected URL while it
promotes 'Reference Manual' out of the 'Topics' tab.
"""

import sys
import os


# The doxygen-emitted filename for the `reference-manual` group page, and the
# clean name the build renames it to.
OLD_NAME = "group__reference-manual.html"
NEW_NAME = "reference-manual.html"

# Only text artifacts that can carry a link or a navtree URL are candidates.
# Binary assets and the copied theme directories simply never match OLD_NAME and
# are skipped by the occurrence check below.
SCANNED_EXTENSIONS = (".html", ".js")


def fix_reference_manual(filepath):
    """Rewrite every OLD_NAME reference to NEW_NAME in the doxygen output.

    filepath is the path to navtreedata.js. The directory holding it is scanned
    recursively for .html and .js files. Returns None; prints one line per file
    that was changed (or a 'nothing to do' line when no reference was found).
    """
    doc_dir = os.path.dirname(os.path.abspath(filepath))

    files_changed = 0
    refs_fixed = 0
    for root, dirs, files in os.walk(doc_dir):
        for name in files:
            if not name.endswith(SCANNED_EXTENSIONS):
                continue
            path = os.path.join(root, name)
            with open(path, 'r', encoding='utf-8') as f:
                content = f.read()
            count = content.count(OLD_NAME)
            if count == 0:
                continue
            content = content.replace(OLD_NAME, NEW_NAME)
            with open(path, 'w', encoding='utf-8') as f:
                f.write(content)
            # print("Fixed %d reference(s) to %s in %s"
            #       % (count, OLD_NAME, os.path.relpath(path, doc_dir)))
            files_changed += 1
            refs_fixed += count

    """
    if files_changed == 0:
        print("No references to %s found; nothing to do." % OLD_NAME)
    else:
        print("Rewrote %d reference(s) across %d file(s)."
              % (refs_fixed, files_changed))
    """

if __name__ == "__main__":
    fix_reference_manual(sys.argv[1])
