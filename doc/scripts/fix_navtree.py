#!/usr/bin/env python3
"""Post-process doxygen's navigation tree.

Doxygen places every @defgroup underneath the 'Topics' (modules) tab. This script
promotes selected top-level groups out of 'Topics' to direct children of the root
node (rendered as 'libcwd 2.0'), so they appear at the top level of the sidebar
instead of nested under Topics.

PROMOTE_GROUPS lists, in top-to-bottom display order, the group titles (the human
readable name from the @defgroup line, not its identifier) that must be hoisted
out of 'Topics'. For the libcwd site these are the two peer documentation entries
'Quick Reference' and 'Reference Manual', matching the order used on the front
page.

After every promoted group has been removed from 'Topics', two outcomes are
possible:

  * 'Topics' is left without any children -- the empty 'Topics' tab is dropped
    and the promoted groups take its slot (one slot is reused, the rest are
    inserted), so every later tab shifts down by (len(PROMOTE_GROUPS) - 1).
  * 'Topics' still has other children -- it is kept and the promoted groups are
    inserted just before it, shifting every later tab down by len(PROMOTE_GROUPS).

The navigation tree is split across several generated files:

  navtreedata.js   - the top levels of the tree (var NAVTREE = [...]). A node
                     whose third element is a *string* lazy-loads its children
                     from <that string>.js; 'Topics' is such a node.
  topics.js        - the children of 'Topics' (this is where the promoted groups
                     actually live).
  navtreeindex*.js - a map from every page URL to its tree path, used by
                     doxygen's navtree.js for panel synchronization.

So promoting a group touches all three: remove it from topics.js, place it among
(or in place of) the root's children in navtreedata.js, and rewrite the tree
paths in navtreeindex*.js so the current-page highlight stays correct.

The script is invoked with the path to navtreedata.js; the other files are
expected alongside it. It fails loudly (printing a message and returning without
rewriting anything) when the expected structure is not found, so a doxygen layout
change is noticed instead of silently producing a broken tree.
"""

import sys
import os
import json
import re


# Group titles (the second argument of their @defgroup line) to hoist from the
# 'Topics' tab to top-level sidebar entries, in the order they must appear.
PROMOTE_GROUPS = ["Quick Reference", "Reference Manual"]


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
    """Promote PROMOTE_GROUPS from the 'Topics' tab to root-level nodes.

    filepath is the path to navtreedata.js. Returns None; prints a one-line
    status message per promoted group (or an explanatory message when the
    expected structure is missing).
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

    # The promoted groups are children of 'Topics'. Those children are either
    # inline (a list) or, when doxygen stored a string, lazy-loaded from
    # <ref>.js -- in which case 'Topics' points at topics.js, where the groups
    # really live. Normalize to a single mutable list we can search and edit:
    # `topics_children` (the original child list) and `topics_owner` describing
    # how to persist the leftovers.
    topics_js_path = None
    topics_js_content = None       # staged rewrite for topics.js, when kept
    topics_js_match = None         # the regex match for the topics array

    if isinstance(topics_ref, list):
        topics_children = topics_ref
    elif isinstance(topics_ref, str):
        topics_js_path = os.path.join(doc_dir, topics_ref + ".js")
        if not os.path.exists(topics_js_path):
            print("'Topics' references", topics_ref + ".js, which does not exist.")
            return
        with open(topics_js_path, 'r', encoding='utf-8') as f:
            topics_js_content = f.read()
        topics_js_match, topics_children = _load_array_var(topics_js_content, topics_ref)
        if topics_js_match is None:
            print("Could not parse", topics_ref, "array in", topics_js_path)
            return
    else:
        print("'Topics' node has an unexpected children field; nothing to do.")
        return

    # Locate each promoted group inside topics_children, recording the original
    # child index (needed for navtreeindex path remapping) and the node itself.
    # All indices are gathered before anything is removed so they refer to the
    # original layout. A missing group is reported but does not abort the others.
    promoted = []                  # list of (original_child_index, node), in
                                   # PROMOTE_GROUPS (display) order
    found_indices = set()
    for title in PROMOTE_GROUPS:
        idx = None
        for j, group in enumerate(topics_children):
            if j in found_indices:
                continue
            if len(group) > 0 and group[0] == title:
                idx = j
                break
        if idx is None:
            print("Could not find '%s' inside 'Topics'; skipping it." % title)
            continue
        found_indices.add(idx)
        promoted.append((idx, topics_children[idx]))

    if not promoted:
        print("None of the promoted groups were found inside 'Topics'.")
        return

    n = len(promoted)
    promoted_nodes = [node for (_idx, node) in promoted]
    promoted_orig_indices = {idx for (idx, _node) in promoted}
    # Each promoted group lands at root level: the k-th one (0-based) goes to
    # root_children[topics_index + k].
    promoted_new_root = {idx: topics_index + k
                         for (k, (idx, _node)) in enumerate(promoted)}

    remaining_children = [child for j, child in enumerate(topics_children)
                          if j not in found_indices]
    remaining_count = len(remaining_children)

    # If Topics is now empty, drop it and let the promoted groups take its slot
    # (so later tabs keep valid indices). Otherwise keep Topics and insert the
    # promoted groups just before it.
    remove_topics = (remaining_count == 0)
    topics_js_delete = False
    if remove_topics:
        # Replace the single 'Topics' slot with the promoted nodes.
        root_children[topics_index:topics_index + 1] = promoted_nodes
        if topics_js_path is not None:
            topics_js_delete = True           # topics.js is now an orphan
            topics_js_content = None
    else:
        # Insert the promoted nodes before 'Topics', which shifts down by n.
        root_children[topics_index:topics_index] = promoted_nodes

    # Stage the file writes (applied only once everything succeeded).
    pending_writes = [(filepath, _splice(
        navtree_content, navtree_match,
        json.dumps(navtree, separators=(',', ': '))))]
    # Only rewrite topics.js when Topics is kept *and* its children were lazy
    # loaded (inline children were already edited in place through topics_ref).
    if topics_js_content is not None and topics_js_match is not None:
        topics_js_content = _splice(
            topics_js_content, topics_js_match,
            json.dumps(remaining_children, separators=(',', ': ')))
        pending_writes.append((topics_js_path, topics_js_content))

    # --- Rewrite the per-page tree paths in navtreeindex{0,1}.js so that    ---
    # --- panel synchronization keeps highlighting the current page.          ---
    #
    # A page that lived under the k-th promoted group had path
    # [topics_index, orig_idx, ...] and now has [topics_index + k, ...].
    # The 'Topics' node itself ([topics_index]) is dropped when Topics is
    # removed, otherwise it shifts to [topics_index + n]. Any tab strictly after
    # 'Topics' shifts by (n - 1) when Topics is removed (one slot freed, n taken)
    # or by n when Topics is kept. Remaining children of a kept Topics keep their
    # subtree but move under the shifted Topics node and have their own child
    # indices compacted by the promoted groups that sat before them.
    later_shift = n if not remove_topics else n - 1

    def remap_path(path):
        if not path:
            return path
        if path[0] < topics_index:
            return path                          # a tab before 'Topics'
        if path[0] > topics_index:
            return [path[0] + later_shift] + path[1:]   # a tab after 'Topics'
        # path[0] == topics_index: Topics itself or one of its subtrees.
        if len(path) == 1:
            if remove_topics:
                return None                      # the 'Topics' node is gone
            return [topics_index + n]            # Topics kept, shifted down
        c = path[1]
        if c in promoted_new_root:
            return [promoted_new_root[c]] + path[2:]   # a promoted group's subtree
        if remove_topics:
            # No remaining children exist once Topics is dropped.
            return path
        # A leftover child of the kept Topics node: compact its child index.
        shifted_c = c - sum(1 for ci in promoted_orig_indices if ci < c)
        return [topics_index + n, shifted_c] + path[2:]

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

    #print("Promoted %d group(s) from 'Topics' to top-level sidebar entries." % n)


if __name__ == "__main__":
    fix_navtree(sys.argv[1])
