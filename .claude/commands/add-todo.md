---
allowed-tools: Read, Grep, Glob, ToolSearch, mcp__clion__todo_list, mcp__clion__todo_get, mcp__clion__todo_create, mcp__clion__todo_update, mcp__clion__todo_batch, mcp__clion__todo_comment, mcp__clion__todo_decided
description: Add a new TODO item to the project's Not My TODO list (the CLion plugin, stored in .todos/items.json), rephrased for clarity, tagged by kind and anchored to the code it is about
---

Add a TODO item based on the user's input: $ARGUMENTS

The project's TODO list is kept by CLion's Not My TODO plugin in `.todos/items.json` and is read and
written only through the IDE's MCP tools. Load them in one call,
`ToolSearch("select:mcp__clion__todo_list,mcp__clion__todo_get,mcp__clion__todo_create,mcp__clion__todo_update,mcp__clion__todo_batch,mcp__clion__todo_comment,mcp__clion__todo_decided")`,
and pass `projectPath: /home/konsto/projects/CnC_Red_Alert` to each. Use the `mcp__clion__` set (the
`mcp__idea__` one belongs to another IDE). Do not edit, script or grep the JSON: the tools search
it, keep the anchors and save several changes in one write. There is no `TODO.md` any more.

Steps:

1. Look for an item that already covers it: `todo_list` with `compact: true` (one line per item; a
   plain list returns every item's details) and `text` set to the function or file name, or `path`
   set to the file or folder, or `tags` set to the kind. `todo_get` the likely ones. If one covers
   it, add the new case with `todo_comment`, or `todo_update` its title or details when they are now
   wrong, instead of adding a second item.
2. Write the item:
   - **title**: one imperative line that says what to do ("Fix X", "Decide whether Y"), without
     backticks and without file paths.
   - **details**: Markdown, with identifiers and paths in backticks. Give what it takes to act
     without re-reading the conversation: the files, what is wrong or possible, the options with
     their size. One point is a paragraph; several are a `- ` list.
   - **source**: the commit or commit range of the run that found it (`30c6f6ae..e9ac6245`), here
     and not in the details.
   - **tags**: the kind first, then the area (`ra`, `td`, `engine`, `base`, `game`, `tools`,
     `cmake`), then a topic if one helps find it (`coverage`, `cleanup`, `ui-check`, `net-check`,
     `manual-check`, `editor`, `strict-build`, `local-env`, ...). Reuse an existing tag before
     inventing one. The kinds:
     - `bugs`: issues, crashes, incorrect behavior, and "decide whether this is a bug"
     - `features`: new functionality, enhancements
     - `refactoring`: code cleanup, modernization, technical debt
     - `build`: CMake, dependencies, CI
     - `documentation`: comments, README, guides
     - `testing`: unit tests, test infrastructure, checks to make by hand
     - `debugging`: debug tooling and diagnostics
     - `other`: anything that doesn't fit above
   - **toDecide** when the item waits for the user: a "Decide whether ..." or "Consider ..." item, a
     choice between options that play or read differently, a check that needs eyes on a real display
     or a second player. Write there what has to be decided and what each answer costs; passing it
     adds the `needs-decision` tag, so do not add the tag by hand. The plugin lists every other open
     item under "Agent can fix" and hands those to agents in bulk (Fix All Shown), so leave
     `toDecide` out only when the details say exactly what to change.
   - **priority**: `p2` for `bugs`, `p3` for the rest, unless the user says otherwise; `p1` only
     when the user asks for it.
   - **effort**: `s` (under an hour), `m` (a few hours) or `l` (a day or more), whenever you can
     tell.
   - **anchor**: when the item is about one place in the code, `path` and `startLine` (`endLine` for
     a range) of the definition or the lines it means, as a path from the project root
     (`src/ra/map/layer.cc`). Read the file for the line; do not take a line number from memory or
     from an old note. `path` alone attaches to the whole file; `places`
     (`"src/a.cc:12, src/b.cc:40-52"`) attaches to several definite places. Leave the anchor out for
     an item about the tree as a whole or a check by hand - a wrong anchor is worse than none.
   - **links**: `parent` for an item that is one part of another (one level only), `blockedBy` for
     one that cannot start until others are closed, `duplicateOf` for one that repeats another.
3. One item: `todo_create`. Two or more: one `todo_batch` with all of them (it is all or nothing,
   and the error names the entry that is wrong). Items that belong together get a parent: create the
   parent with `todo_create` first, then batch the parts with its id as `parent`. Report each new id
   (`T-<n>`) with its title.

Changing items:

- `todo_update` changes only the fields passed, but `tags` and `details` are replaced whole: pass
  the old tags along with a new one, and add a finding or a progress note with `todo_comment`
  instead of rewriting the details.
- A `todo_batch` entry with an `id` takes the fields of `todo_update` plus `comment`, but not
  `path`, `startLine`, `endLine` or `places`: re-anchor with `todo_update`.
- Do not fix an item tagged `needs-decision` unless the user picks it. Then ask what it waits for
  with AskUserQuestion, record the answer with `todo_decided` (question, the options offered, the
  answer in the user's words) and only then do the work.
- If fixing any other item turns out to need a decision that is the user's, do not guess and do not
  change code: `todo_update` it with `toDecide` and leave the status alone.
- A finished item is not deleted. Commit the fix first, then close the item in one `todo_update`:
  `status: fixed`, `resolution` (a sentence or two on what was done), `fixedIn` (the commit) and the
  new anchor if the code moved. Not `fixed` before the commit and `fixedIn` after it, and no "Not
  committed" in the resolution. `wont_fix` takes the reason in `resolution`; a duplicate is
  `wont_fix` with `duplicateOf`.
- Put everything one item needs in one call: status, tags, resolution and anchor together. A second
  `todo_update` on the same item right after the first is a call wasted, and each one returns the
  whole item.
- Never read `.todos/items.json` to see what a call did: the call's result is the item. Before
  committing the file, `git diff --stat` is enough.
- After moving or rewriting anchored code, `todo_list` with `lost: true` and re-attach what it
  returns with `todo_update` (`path`, `startLine`).
