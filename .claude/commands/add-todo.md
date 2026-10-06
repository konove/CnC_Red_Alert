---
allowed-tools: Read, Grep, Glob, ToolSearch, mcp__clion__todo_list, mcp__clion__todo_get, mcp__clion__todo_create, mcp__clion__todo_update
description: Add a new TODO item to the project's Not My TODO list (the CLion plugin, stored in .todos/items.json), rephrased for clarity, tagged by kind and anchored to the code it is about
---

Add a TODO item based on the user's input: $ARGUMENTS

The project's TODO list is kept by CLion's Not My TODO plugin in `.todos/items.json` and is read and
written through the `mcp__clion__todo_*` tools (load them with ToolSearch; pass
`projectPath: /home/konsto/projects/CnC_Red_Alert`). There is no `TODO.md` any more.

Steps:

1. Look for an item that already covers it: `todo_list` with the kind's tag (below), or grep
   `.todos/items.json` for the function or file name. If one does, `todo_update` its details (the
   new file name, the extra case) instead of adding a second item.
2. Write the item:
   - **title**: one imperative line that says what to do ("Fix X", "Decide whether Y"), without
     backticks and without file paths.
   - **details**: Markdown, with identifiers and paths in backticks. Give what it takes to act
     without re-reading the conversation: the files, what is wrong or possible, the options with
     their size, and the commit range of the run that found it. One point is a paragraph; several
     are a `- ` list.
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
   - **`needs-decision`** as one more tag when the item waits for the user: a "Decide whether ..."
     or "Consider ..." item, a choice between options that play or read differently, a check that
     needs eyes on a real display or a second player. The plugin lists every other open item under
     "Agent can fix" and hands those to agents in bulk (Fix All Shown), so leave the tag off only
     when the details say exactly what to change. Say in the details what has to be decided and what
     each answer costs.
   - **priority**: `p2` for `bugs`, `p3` for the rest, unless the user says otherwise; `p1` only
     when the user asks for it.
   - **anchor** (`path`, `startLine`, `endLine`): when the item is about one place in the code, the
     definition or the lines it means, as a path from the project root (`src/ra/map/layer.cc`). Read
     the file for the line; do not take a line number from memory or from an old note. Leave the
     anchor out for an item that spans several files or is a check by hand - a wrong anchor is worse
     than none.
3. `todo_create` it and report the new id (`T-<n>`) with its title.

Do not fix an item tagged `needs-decision` unless the user picks it and gives the answer. If fixing
any other item turns out to need a decision that is the user's, do not guess and do not change code:
add the tag (`todo_update` replaces the whole tag list, so pass the old tags too), write what has to
be decided in the details, and leave the status alone.

An item that is finished is not deleted: `todo_update` it to `status: fixed` so the user can review
it, or to `wont_fix` with the reason in the details.
