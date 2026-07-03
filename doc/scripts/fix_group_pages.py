#!/usr/bin/env python3
"""Post-process doxygen output after renaming group landing pages.

Doxygen emits the page for a top-level @defgroup as `group__<name>.html`. The
build renames a few of these to cleaner URLs (for example `group__reference-manual.html`
becomes `reference-manual.html`), but every other generated artifact still points
at the original doxygen name:

  * navtreedata.js      -- the node for the group carries it as its URL.
  * navtreeindexN.js    -- the per-page tree path is keyed on it.
  * *.html              -- every cross-reference (breadcrumbs, 'Related Pages',
                           the in-page link from index.html, ...) links to it.
  * search/*.js         -- the search index entries point at it.

Left untouched those references would all 404 after the rename, so this script
rewrites them in place to the new name.

RENAMES maps each original doxygen file name to its replacement. Only the *exact*
string `<old>` is rewritten. Two look-alikes are deliberately left alone:

  * `group__reference-manual` (without `.html`) in navtreedata.js is the
    lazy-load pointer to the `group__reference-manual.js` children file, which
    the build does not rename, so the pointer must stay as it is. (Quick
    Reference is a leaf group and has no such .js file.)
  * `group__reference-manual-getting-started.html` and the other subgroups keep
    their doxygen names; they never match the exact string anyway (an extra
    `-suffix` sits before `.html`), but this is worth stating explicitly.

The script is invoked with the path to navtreedata.js, the same entry point
fix_navtree.py is run on next; every other file is discovered relative to it.
It runs *before* fix_navtree.py so that script sees the corrected URLs while it
promotes groups out of the 'Topics' tab.
"""

import sys
import os


# Each entry is (doxygen_emitted_name, clean_name_the_build_uses). Only exact
# matches of the first string are rewritten; add a group here only after the
# build's `mv` step has started renaming its page.
RENAMES = [
    ("group__reference-manual.html", "reference-manual.html"),
    ("group__quick-reference.html", "quick-reference.html"),
]

# Only text artifacts that can carry a link or a navtree URL are candidates.
# Binary assets and the copied theme directories simply never match any RENAMES
# entry and are skipped by the occurrence check below.
SCANNED_EXTENSIONS = (".html", ".js")


def fix_group_pages(filepath):
    """Rewrite every RENAMES old name to its new name in the doxygen output.

    filepath is the path to navtreedata.js. The directory holding it is scanned
    recursively for .html and .js files. Returns None; prints one summary line.
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
            count = 0
            for old_name, new_name in RENAMES:
                occurrences = content.count(old_name)
                if occurrences:
                    content = content.replace(old_name, new_name)
                    count += occurrences
            if count == 0:
                continue
            with open(path, 'w', encoding='utf-8') as f:
                f.write(content)
            files_changed += 1
            refs_fixed += count

    #print("Rewrote %d reference(s) across %d file(s)." % (refs_fixed, files_changed))


if __name__ == "__main__":
    fix_group_pages(sys.argv[1])
