#!/usr/bin/env python3
"""Post-process doxygen's navigation tree.

doxygen places every @defgroup underneath the 'Topics' (modules) tab. This
script promotes the 'Reference Manual' group out of 'Topics' to a direct child
of the root node (rendered as 'libcwd 2.0'), so it appears at the top level of
the sidebar instead of nested under Topics.

The navigation tree is split across several generated files:

  navtreedata.js   - the top levels of the tree (var NAVTREE = [...]). A node
                     whose third element is a *string* lazy-loads its children
                     from <that string>.js; 'Topics' is such a node.
  topics.js        - the children of 'Topics' (this is where 'Reference Manual'
                     actually lives).
  navtreeindex*.js - a map from every page URL to its tree path, used by
                     doxygen's navtree.js for panel synchronization.

Moving 'Reference Manual' therefore touches all three: remove it from topics.js,
insert it among the root's children in navtreedata.js, and rewrite the tree
paths in navtreeindex*.js so the current-page highlight stays correct.

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

    Used to rewrite only one JS array/object within a file while preserving the
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
    # lives. Either way we record rm_child_index, the child position RM occupied
    # inside Topics (needed to remap the navtreeindex paths below).
    ref_manual_node = None
    rm_child_index = None
    topics_js_path = None
    topics_js_content = None
    pending_writes = []  # (path, new_content) pairs, applied only on success

    if isinstance(topics_ref, list):
        for j, group in enumerate(topics_ref):
            if len(group) > 0 and group[0] == "Reference Manual":
                ref_manual_node = topics_ref.pop(j)
                rm_child_index = j
                break
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

    # Re-parent Reference Manual as a direct child of the root node, placed
    # right before 'Topics'. 'Topics' and every tab after it shift down by one.
    root_children.insert(topics_index, ref_manual_node)

    # Stage the file writes. navtreedata.js keeps everything except the NAVTREE
    # array; topics.js keeps everything except the removed child.
    pending_writes.append((filepath, _splice(
        navtree_content, navtree_match,
        json.dumps(navtree, separators=(',', ': ')))))
    if topics_js_path is not None and topics_js_content is not None:
        pending_writes.append((topics_js_path, topics_js_content))

    # --- Rewrite the per-page tree paths in navtreeindex{0,1}.js so that    ---
    # --- panel synchronization keeps highlighting the current page.          ---
    #
    # Before the move, Reference Manual was Topics's child rm_child_index, so
    # every page in its subtree had a path [topics_index, rm_child_index, ...].
    # It now sits at the root level at index topics_index, so those paths become
    # [topics_index, ...] (one level shallower). 'Topics' itself and every later
    # tab shifted down by one, so any path at or past topics_index that is not
    # part of the Reference Manual subtree is incremented (and siblings of RM
    # within the old Topics node are renumbered accordingly).
    def remap_path(path):
        if not path or path[0] < topics_index:
            return path
        if path[0] > topics_index:
            return [path[0] + 1] + path[1:]
        # path[0] == topics_index: Topics itself or one of its (former) children.
        if len(path) == 1:
            return [topics_index + 1]                       # Topics itself
        if path[1] == rm_child_index:
            return [topics_index] + path[2:]                # Reference Manual subtree
        if path[1] > rm_child_index:                        # a later Topics sibling
            return [topics_index + 1, path[1] - 1] + path[2:]
        return [topics_index + 1] + path[1:]                # an earlier Topics sibling

    def rewrite_path(match):
        ints = [int(x) for x in match.group(1).split(',') if x != '']
        return '[' + ','.join(str(x) for x in remap_path(ints)) + ']'

    for name in ("navtreeindex0.js", "navtreeindex1.js"):
        idx_path = os.path.join(doc_dir, name)
        if not os.path.exists(idx_path):
            continue
        with open(idx_path, 'r', encoding='utf-8') as f:
            idx_content = f.read()
        idx_content = re.sub(r'\[([\d,]*)\]', rewrite_path, idx_content)
        pending_writes.append((idx_path, idx_content))

    # Apply all writes only once everything succeeded.
    for path, content in pending_writes:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)

    print("Successfully moved 'Reference Manual' to root level.")


if __name__ == "__main__":
    fix_navtree(sys.argv[1])
