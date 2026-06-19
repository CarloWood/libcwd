#!/usr/bin/env python3
"""Post-process doxygen's navigation tree.

doxygen places every @defgroup underneath the 'Topics' (modules) tab. This
script promotes the 'Reference Manual' group out of 'Topics' to a direct child
of the root node (rendered as 'libcwd 2.0'), so it appears at the top level of
the sidebar instead of nested under Topics.

When, after the move, 'Topics' is left without any children, the empty 'Topics'
tab is dropped entirely and 'Reference Manual' takes its slot. (If 'Topics'
still had other children it is kept, and 'Reference Manual' is inserted just
before it instead.)

The navigation tree is split across several generated files:

  navtreedata.js   - the top levels of the tree (var NAVTREE = [...]). A node
                     whose third element is a *string* lazy-loads its children
                     from <that string>.js; 'Topics' is such a node.
  topics.js        - the children of 'Topics' (this is where 'Reference Manual'
                     actually lives).
  navtreeindex*.js - a map from every page URL to its tree path, used by
                     doxygen's navtree.js for panel synchronization.

So promoting 'Reference Manual' touches all three: remove it from topics.js,
place it among (or in place of) the root's children in navtreedata.js, and
rewrite the tree paths in navtreeindex*.js so the current-page highlight stays
correct.

The script is invoked with the path to navtreedata.js; the other files are
expected alongside it.
"""

import sys
import os
import json
import re


def _load_array_var(content, var_name):
    """Find `var <var_name> = [ ... ];` in content, returning (match, parsed_list).

    The match is non-greedy (DOTALL) so it stops at the first '];', which is
    always the array's own closing bracket: no '];' occurs inside doxygen's
    nested arrays, whose inner closings are written as '],' or '] ]'. This
    matters because navtreedata.js also contains other arrays (NAVTREEINDEX)
    after NAVTREE. Returns (None, None) when the variable is not found.
    """
    pattern = r'var\s+' + re.escape(var_name) + r'\s*=\s*(\[.*?\]);'
    match = re.search(pattern, content, re.DOTALL)
    if match is None:
        return None, None
    return match, json.loads(match.group(1))


def _splice(content, match, replacement):
    """Replace the text of match.group(1) inside content, leaving the rest intact.

    Used to rewrite only one JS array within a file while preserving the
    surrounding license header, other variables, and appended overrides.
    """
    return content[:match.start(1)] + replacement + content[match.end(1):]


def fix_navtree(filepath):
    """Promote 'Reference Manual' from the 'Topics' tab to a root-level node.

    filepath is the path to navtreedata.js. Returns None; prints a one-line
    status message describing what was (or was not) done.
    """
    doc_dir = os.path.dirname(os.path.abspath(filepath))

    # --- Parse NAVTREE out of navtreedata.js. ---
    with open(filepath, 'r', encoding='utf-8') as f:
        navtree_content = f.read()
    navtree_match, navtree = _load_array_var(navtree_content, "NAVTREE")
    if navtree_match is None:
        print("Could not find NAVTREE array in", filepath)
        return

    # The root node NAVTREE[0] is rendered as "libcwd 2.0" (its label is later
    # overridden by the appended NAVTREE[0][0] = "libcwd 2.0"). Its direct
    # children -- the tabs shown at the top of the nav tree -- live in
    # NAVTREE[0][2].
    if not (navtree and isinstance(navtree[0], list) and len(navtree[0]) > 2
            and isinstance(navtree[0][2], list)):
        print("NAVTREE root node has no children; nothing to do.")
        return
    root_children = navtree[0][2]

    # Find the 'Topics' tab among the root's children.
    topics_index = None
    for i, child in enumerate(root_children):
        if len(child) > 0 and child[0] == "Topics":
            topics_index = i
            break
    if topics_index is None:
        print("Could not find 'Topics' among the root node's children.")
        return
    topics_ref = root_children[topics_index][2]

    # 'Reference Manual' is a child of 'Topics'. Those children are either inline
    # (a list) or, when doxygen stored a string, lazy-loaded from <ref>.js -- in
    # which case 'Topics' points at topics.js, where 'Reference Manual' really
    # lives. We record rm_child_index (RM's position inside Topics) and
    # remaining_count (how many children Topics has left), both needed below.
    ref_manual_node = None
    rm_child_index = None
    topics_js_path = None
    topics_js_content = None      # staged new content for topics.js, when kept
    remaining_count = None

    if isinstance(topics_ref, list):
        for j, group in enumerate(topics_ref):
            if len(group) > 0 and group[0] == "Reference Manual":
                ref_manual_node = topics_ref.pop(j)
                rm_child_index = j
                break
        remaining_count = len(topics_ref)
    elif isinstance(topics_ref, str):
        topics_js_path = os.path.join(doc_dir, topics_ref + ".js")
        if os.path.exists(topics_js_path):
            with open(topics_js_path, 'r', encoding='utf-8') as f:
                topics_js_content = f.read()
            topics_js_match, topics_children = _load_array_var(topics_js_content, topics_ref)
            if topics_js_match is not None:
                for j, group in enumerate(topics_children):
                    if len(group) > 0 and group[0] == "Reference Manual":
                        ref_manual_node = topics_children.pop(j)
                        rm_child_index = j
                        break
                remaining_count = len(topics_children)
                topics_js_content = _splice(
                    topics_js_content, topics_js_match,
                    json.dumps(topics_children, separators=(',', ': ')))
            else:
                print("Could not parse", topics_ref, "array in", topics_js_path)
                topics_js_content = None
        else:
            print("'Topics' references", topics_ref + ".js, which does not exist.")

    if ref_manual_node is None:
        print("Could not find 'Reference Manual' inside 'Topics'.")
        return

    # If Topics is now empty, drop it and let Reference Manual take its slot so
    # every later tab keeps its index (and its navtreeindex paths stay valid).
    # Otherwise keep Topics and insert Reference Manual just before it.
    remove_topics = (remaining_count == 0)
    topics_js_delete = False
    if remove_topics:
        root_children[topics_index] = ref_manual_node
        if topics_js_path is not None:
            topics_js_delete = True           # topics.js is now an orphan
            topics_js_content = None
    else:
        root_children.insert(topics_index, ref_manual_node)

    # Stage the file writes (applied only once everything succeeded).
    pending_writes = [(filepath, _splice(
        navtree_content, navtree_match,
        json.dumps(navtree, separators=(',', ': '))))]
    if topics_js_content is not None:         # only when Topics is kept + lazy-loaded
        pending_writes.append((topics_js_path, topics_js_content))

    # --- Rewrite the per-page tree paths in navtreeindex{0,1}.js so that    ---
    # --- panel synchronization keeps highlighting the current page.          ---
    #
    # Reference Manual was Topics's child rm_child_index, so every page in its
    # subtree had a path [topics_index, rm_child_index, ...]; it now sits at the
    # root level at index topics_index, so those become [topics_index, ...].
    # When Topics is removed, the 'topics.html' entry (path [topics_index]) is
    # dropped and every other path is unchanged. When Topics is kept, Topics and
    # every later tab shift down by one instead.
    def remap_path(path):
        if not path:
            return path
        if len(path) >= 2 and path[0] == topics_index and path[1] == rm_child_index:
            return [topics_index] + path[2:]     # Reference Manual subtree
        if remove_topics:
            if len(path) == 1 and path[0] == topics_index:
                return None                      # the 'Topics' node is gone
            return path                          # later tabs keep their indices
        if path[0] == topics_index:              # Topics itself or a sibling
            return [topics_index + 1] + path[1:]
        if path[0] > topics_index:               # a later tab
            return [path[0] + 1] + path[1:]
        return path

    # Match a whole `"url":[path],\n` entry so we can both rewrite the path and
    # drop an entry entirely (when remap_path returns None).
    entry_re = re.compile(r'"([^"]+)"\s*:\s*(\[[\d,]*\])(,?)[ \t]*\n?')

    def rewrite_entry(match):
        ints = json.loads(match.group(2))
        new_path = remap_path(ints)
        if new_path is None:
            return ''                            # remove the entry
        return '"%s":[%s]%s\n' % (
            match.group(1), ','.join(str(x) for x in new_path), match.group(3))

    for name in ("navtreeindex0.js", "navtreeindex1.js"):
        idx_path = os.path.join(doc_dir, name)
        if not os.path.exists(idx_path):
            continue
        with open(idx_path, 'r', encoding='utf-8') as f:
            idx_content = f.read()
        idx_content = entry_re.sub(rewrite_entry, idx_content)
        pending_writes.append((idx_path, idx_content))

    for path, content in pending_writes:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)
    if topics_js_delete and os.path.exists(topics_js_path):
        os.remove(topics_js_path)

    if remove_topics:
        print("Successfully moved 'Reference Manual' to root level "
              "and removed the empty 'Topics' tab.")
    else:
        print("Successfully moved 'Reference Manual' to root level.")


if __name__ == "__main__":
    fix_navtree(sys.argv[1])
