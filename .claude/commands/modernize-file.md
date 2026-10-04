---
description: 'Take a legacy file (and its .h/.cc partner) through the whole modernization sequence in one go - dead code, comments and the bugs they turn up, integer types, Google-style names, simplification, the engine namespace, unit-test coverage, IDE inspections, and the file name and folder - committing each stage, then recording every open item in TODO.md. Use whenever the user asks to "modernize", "update", "clean up", "do the usual pass on" or "go through" a file or pair of files, or asks for several of /remove-dead-code, /update-comments, /migrate-types, /rename-google-style, /simplify, /migrate-namespaces, /cover-files and /fix-ide-issues on the same file.'
---

Modernize: $ARGUMENTS

This runs the project's single-purpose commands on one file, in the order the user has settled on by
hand (`21f29657`..`16918991` for the WSA player, `34ec9bd5`..`a674b4e1` for the Red Alert audio).
Each stage is its own command with its own rules; this file only decides the order, what each stage
hands to the next, and where the commits fall. Invoke each one with the Skill tool and follow it as
written, except where a stage below says otherwise.

## Why this order

Each stage makes the next one cheaper or more accurate:

1. **Dead code first**, so nothing later documents, retypes or renames code that is about to go.
2. **Comments and bugs** next. The comment pass is the close reading: it is where the file is
   understood, and where the bugs surface. Everything after it leans on that understanding.
3. **Types** once the values are understood. Choosing `int32_t` over `uint32_t` needs to know
   whether a value is a count or a bit pattern, and choosing `std::string_view` over `std::string`,
   a span over a vector, or a returned value over a filled buffer, needs to know who owns the data -
   which is what the comment pass found out.
4. **Names** after the types, so a name can carry the unit the type now states (`distance_cells`).
5. **Simplify** what the earlier stages listed but kept out of their diffs - every one of them ends
   with "noticed but not changed", and the user always asks for those next.
6. **Namespace** once the names are final and the dead code is gone, so every caller is qualified
   once, with the name it keeps, and nothing about to be deleted gets a qualifier. Only for
   `src/engine` files; `src/ra` and `src/td` stay global.
7. **Coverage** once the interface is final, so each test is written once: against the names, types
   and namespace the code keeps, and for none of the code stages 1 and 5 deleted. It comes before
   the polish because tests are the last thing likely to change the code - they find bugs, and the
   lines no test can reach are often dead.
8. **IDE inspections** as the polish on the code as it finally stands; any earlier and the next
   stage would change what the IDE flags.
9. **The file's name and folder** after the code is final, alone in its commit, so git's rename
   detection holds and every earlier commit reads under the path reviewers know. Where a file
   belongs is decided by what it holds, and that is only settled once the dead code is gone and the
   stray functions have been moved out.
10. **Leftovers to `TODO.md`** at the very end, once the ledger has stopped growing and every entry
    can name the file by its final path.

## Before starting

- Scope is the named file(s) plus the `.h`/`.cc` partner; with no argument, the file the IDE has
  open. Note the twin in the other game (`src/ra` ↔ `src/td`): bug fixes go to it, nothing else
  does.
- `git status`: unrelated changes stay in the tree and out of every commit (stage by path).
- Ask the user once, now, to open **both** files (the `.h` too) in CLion. Stage 8 needs them; CLion
  only answers for open files, and a mid-run question stalls the run for nothing.
- Keep a ledger in the scratchpad (`modernize-<file>.md`) with three lists: **bugs**, **noticed**
  (unused or constant parameters, redundant checks, tables that could be `constexpr`, functions
  doing two jobs), and **for the user** (anything that changes simulation behaviour, names you were
  unsure of). Every stage appends to it; later stages read it. It is what makes the stages add up
  instead of each rediscovering the same things, and at the end it is the source for stage 10. Put
  everything on it, including what lies outside the files' scope and what a subagent reported: an
  item that is only in the conversation is lost when the run ends.

## The stages

Commit at the end of each stage that changed something, following `/commit` (it verifies what has
not been verified, splits RA and TD, and writes the message). A stage that finds nothing gets no
commit and one line in the final report. Do not stop between stages to ask; the user asked for the
whole run.

**The full strict build runs once, not once per stage.** A run edits the same header eight times,
and each edit re-analyzes everything that includes it - four minutes for an
`engine/gfx/pixel_buffer.h`-sized fan-out, half an hour over a run. So during the stages, verify
with `tools/strict_tu.py <touched files>` (the same clang-tidy pass and clang compile, on the
objects those files build, in seconds) and tell `/commit` the full pass is deferred. The plain
`cmake --build build --parallel 22` still runs every stage: it is what catches a rename that missed
a call site anywhere in the tree.

Then run `cmake --build build-strict --parallel 14` once, in the foreground, after stage 8 and
before stage 9's commit - the last point where the code is final. What it reports is fixed there and
then: amend the stage commit it belongs to if that commit is still the tip, otherwise give the fix
its own commit naming the stage it came from. Do not start stage 9 with findings outstanding, and do
not end a run without that pass having been clean.

**1. Dead code** - `/remove-dead-code <files>` in sweep mode. Unused parameters it notices go on the
ledger's **noticed** list for stage 5; removing them changes callers across the tree and belongs
with the other interface clean-ups.

**2. Comments and bugs** - `/update-comments <files>`, asking for the fixes too ("update the
comments and fix the bugs you find"). The comment pass is committed with the fixes it is interleaved
with; a fix in different lines, and each twin fix, is its own commit. Fixes that change simulation
behaviour are not applied: they go to **for the user**.

**3. Types** - `/migrate-types <files>`, all four families: its output-parameter section is where
the files' functions stop filling a caller's buffer and return the result instead. A narrowing that
was hiding an overflow is a bug: ledger, then fix it at the end of this stage. Output parameters it
keeps because of a virtual chain or fan-out go on the **for the user** list.

**4. Names** - `/rename-google-style <files>`, **identifiers only**: stop before its file-rename
step, which is stage 9. Identifiers means all of them - the types the files declare (including
dropping a `Class` suffix and picking a better name than the one under it), the parameters, and the
locals, not just the functions and members. Settling the type name here is what lets stage 9 name
the file once instead of twice. Leave out of the table anything the ledger already marks as going in
stage 5 (a derivable member, a static that belongs elsewhere); renaming what is about to be deleted
is churn in two commits. Its "noticed" and "unsure" lists go on the ledger.

**5. Simplify** - first work through the ledger's **noticed** list: dead or constant parameters
through `/remove-dead-code <claim>`, the rest by hand, each only if it keeps behaviour. Then run
`/simplify` on what this run has changed so far (`git diff <base>..HEAD -- <files>`, with `<base>`
the commit before stage 1) and apply what it finds on those files. Anything that would change
behaviour goes to **for the user**.

**6. Namespace** - `/migrate-namespaces <files>`, for a file under `src/engine` that is not in its
library's namespace yet; otherwise one line in the report and no commit. It runs in its single-file
mode: the siblings it lists as now qualifying names they will drop later go on the ledger's
**noticed** list and into the report. Its fan-out is tree-wide (every caller in `src/ra` and
`src/td`), so the plain `build` must pass before the commit; its strict checks go through
`tools/strict_tu.py` on every file it touched, like the other stages. Hooks it keeps global and any
it would rather move go to **for the user**.

**7. Coverage** - `/cover-files <files>`, to every line a test can reach. The tests are one commit
("Cover ..."); a new test target's `CMakeLists.txt` entry goes with them. What the tests turn up
follows this run's rules rather than waiting for the user: a bug is fixed at the end of the stage
under the rule below (the test that found it is its failing test), and a line no caller can reach is
removed through `/remove-dead-code`, each as its own commit. The lines left uncovered, with the
reason for each, and the branch sides not worth a test go on the ledger's **noticed** list. A file
that needs the display, the main loop or game data throughout may end the stage with no tests: say
what seam would make it testable on the **for the user** list, and make no commit.

**8. IDE** - `/fix-ide-issues <files>`, the test file included. If CLion still times out, say so in
the report and skip rather than guess.

**9. File name and folder** - the file-rename step of `/rename-google-style` (its section 5, step 7,
and the rules under "The file name is part of the pass" and "So is the folder"). A rename and a move
are one `git mv` and one commit; the `_test.cc` and its test target go with the pair. A twin that
moves with the file is its own commit. A move that step does not make - out of the game, across
engine libraries, or a split - goes to **for the user** with what stands in the way, and so to
stage 10. If the name and the folder already fit, record why and make no commit.

**10. Leftovers to `TODO.md`** - every open item from the ledger, however small, following
`/add-todo` for wording and section: bugs not fixed (behaviour-changing, out of scope, twin not
done), **noticed** items not acted on, **for the user** decisions, design ideas `/simplify` raised
but did not apply, lines stage 7 left uncovered and what would make them testable, stages skipped (a
CLion timeout becomes "Run `/fix-ide-issues` on ..."), and things spotted outside the files' scope.
Give each entry what it takes to act without re-reading the run: the files and lines, what is wrong
or possible, the options with their size, and the run's commit range. Before adding, grep `TODO.md`
for an entry that already covers it (an existing item may only need the new file name or an extra
detail); delete entries this run finished (a "move X into its namespace" item the namespace stage
closed). Run Prettier on `TODO.md` (the pre-commit hook rejects it otherwise) and commit it alone,
as `c8a9d6c4` did: "Add the <file> modernization's leftovers to TODO.md". A run whose ledger ends
empty says so in the report and makes no commit.

**Bugs found in any stage** go on the ledger when found and are fixed at the end of that stage, as
their own commit, following the "Fixing the bugs the pass found" rules in `/update-comments`:
confirm against the callers, failing test first where the code is reachable from one, smallest fix,
twin fixed separately.

If a stage cannot be made to build or pass the strict checks, stop there: report what was committed,
what is left in the tree, and the error. That includes the full strict pass before stage 9: a run
that ends with it unclean is a failed run, however many stages committed.

## Report

- One line per stage: the commit(s) it made (`git log --oneline <base>..HEAD`), or why there was
  none, and what the full strict pass before stage 9 turned up.
- Bugs fixed, each with its commit and whether the twin got it.
- Coverage of each file before and after stage 7, and the lines still uncovered.
- **For the user**: behaviour-changing fixes not applied (what would play differently), names you
  were unsure of with the alternative, anything skipped (e.g. the IDE never answered).
- The `TODO.md` commit from stage 10, with its entries in one line each.
- What is still uncommitted, if anything, and why.
