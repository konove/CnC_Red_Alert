---
description: 'Take a legacy file (and its .h/.cc partner) through the whole modernization sequence in one go - dead code, comments and the bugs they turn up, integer types, Google-style names (enums and enumerators included), simplification, moving out what belongs in another file, private data members and the Google declaration order, the engine namespace, unit-test coverage, IDE inspections, and the file name and folder - committing each stage, then recording every open item in TODO.md. Use whenever the user asks to "modernize", "update", "clean up", "do the usual pass on" or "go through" a file or pair of files, or asks for several of /remove-dead-code, /update-comments, /migrate-types, /rename-google-style, /simplify, /restructure-class, /migrate-namespaces, /cover-files and /fix-ide-issues on the same file.'
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
   with "noticed but not changed", and the user always asks for those next. Then **move out** what
   the file holds but is not about. It goes after types and names so each piece arrives at its new
   home already modernized, and before the namespace, the tests and the file name, which should all
   be decided by what stays.
6. **Class shape** once the members have their names and the simplify stage has deleted the ones
   that were dead or derivable, so no accessor is written for a member about to go and no moved-out
   class is restructured in the wrong file. Data goes private first, then the declarations and the
   `.cc` definitions are put in order: the reorder is last because it must be a pure move, and
   before the namespace, the tests and the IDE pass because all three read better - and diff
   smaller - against the final layout and the final interface.
7. **Namespace** once the names are final and the dead code is gone, so every caller is qualified
   once, with the name it keeps, and nothing about to be deleted gets a qualifier. Only for
   `src/engine` files; `src/ra` and `src/td` stay global.
8. **Coverage** once the interface is final, so each test is written once: against the names, types,
   accessors and namespace the code keeps, and for none of the code stages 1 and 5 deleted. It comes
   before the polish because tests are the last thing likely to change the code - they find bugs,
   and the lines no test can reach are often dead.
9. **IDE inspections** as the polish on the code as it finally stands; any earlier and the next
   stage would change what the IDE flags.
10. **The file's name and folder** after the code is final, alone in its commit, so git's rename
    detection holds and every earlier commit reads under the path reviewers know. Where a file
    belongs is decided by what it holds, and that is only settled once the dead code is gone and the
    stray functions have been moved out.
11. **Leftovers to `TODO.md`** at the very end, once the ledger has stopped growing and every entry
    can name the file by its final path.

## Before starting

- Scope is the named file(s) plus the `.h`/`.cc` partner; with no argument, the file the IDE has
  open. Note the twin in the other game (`src/ra` ↔ `src/td`): bug fixes go to it, nothing else
  does.
- `git status`: unrelated changes stay in the tree and out of every commit (stage by path).
- Ask the user once, now, to open **both** files (the `.h` too) in CLion. Stage 9 needs them; CLion
  only answers for open files, and a mid-run question stalls the run for nothing.
- Keep a ledger in the scratchpad (`modernize-<file>.md`) with four lists: **bugs**, **noticed**
  (unused or constant parameters, redundant checks, tables that could be `constexpr`, functions
  doing two jobs), **misplaced** (an enum, class, constant or function the file holds but is not
  about, with where it looks like it belongs), and **for the user** (anything that changes
  simulation behaviour, names you were unsure of). Every stage appends to it; later stages read it.
  It is what makes the stages add up instead of each rediscovering the same things, and at the end
  it is the source for stage 11. Put everything on it, including what lies outside the files' scope
  and what a subagent reported: an item that is only in the conversation is lost when the run ends.

## The stages

Commit at the end of each stage that changed something, following `/commit` (it verifies what has
not been verified, splits RA and TD, and writes the message). A stage that finds nothing gets no
commit and one line in the final report. Do not stop between stages to ask; the user asked for the
whole run.

**The full strict build runs once, not once per stage.** A run edits the same header nine times, and
each edit re-analyzes everything that includes it - four minutes for an
`engine/gfx/pixel_buffer.h`-sized fan-out, half an hour over a run. So during the stages, verify
with `tools/strict_tu.py <touched files>` (the same clang-tidy pass and clang compile, on the
objects those files build, in seconds) and tell `/commit` the full pass is deferred. The plain
`cmake --build build --parallel 22` still runs every stage: it is what catches a rename that missed
a call site anywhere in the tree.

Then run `cmake --build build-strict --parallel 14` once, in the foreground, after stage 9 and
before stage 10's commit - the last point where the code is final. What it reports is fixed there
and then: amend the stage commit it belongs to if that commit is still the tip, otherwise give the
fix its own commit naming the stage it came from. Do not start stage 10 with findings outstanding,
and do not end a run without that pass having been clean.

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
step, which is stage 10. Identifiers means all of them - the types the files declare (including
dropping a `Class` suffix and picking a better name than the one under it), the enums with every
enumerator (its "Enums" rules: `Enum::kName`, each `using enum` deleted, one commit per enum), the
parameters, and the locals, not just the functions and members. Settling the type name here is what
lets stage 10 name the file once instead of twice. Leave out of the table anything the ledger
already marks as being deleted in stage 5 (a derivable member, a parameter nobody passes); renaming
what is about to be deleted is churn in two commits. What is on the **misplaced** list is renamed
like the rest: it survives, and should arrive at its new home with its final name. Decide here, with
`/restructure-class` section 2's test, whether each `struct` the files declare is really a class (it
has behaviour or an invariant: `TriggerAction`), and treat its fields as class data members from
this stage on - not as struct fields to be given their final `snake_case` names now. A class data
member that code outside the class reads or writes gets its row in the table but is **not applied
here**: put the row on the ledger's **noticed** list for stage 6, which renames it and puts it
behind its accessor in one edit of each caller (`obj.Quantity` -> `obj.quantity()`, never by way of
`obj.quantity_`). Members only the class itself touches are renamed here as usual. Its "noticed" and
"unsure" lists go on the ledger.

**5. Simplify** - first work through the ledger's **noticed** list: dead or constant parameters
through `/remove-dead-code <claim>`, the rest by hand, each only if it keeps behaviour. Then run
`/simplify` on what this run has changed so far (`git diff <base>..HEAD -- <files>`, with `<base>`
the commit before stage 1) and apply what it finds on those files. Anything that would change
behaviour goes to **for the user**. Last, work through the **misplaced** list under "Moving out what
does not belong" below, one commit per move.

**6. Class shape** - `/restructure-class <files>`, both of its jobs, on every class and struct the
files still declare after stage 5's moves. A struct that stage 4 judged to be a class gets the
`class` keyword first, in its own commit; one that is passive data stays a struct and is only
checked against the declaration order. Per class: the data members go private (applying the member
rows stage 4 deferred), then the header's declarations and the `.cc`'s definitions are put in the
Google declaration order, as the two commits that command describes. The plain `build` must pass
before each - the accessor fan-out is tree-wide. Verb methods it added, `mutable_` accessors, and
writers that break an invariant go on the ledger (**noticed**, or **for the user** when the fix
would change behaviour). A file that declares no class and whose `.cc` is already in its header's
order gets one line in the report and no commit.

**7. Namespace** - `/migrate-namespaces <files>`, for a file under `src/engine` that is not in its
library's namespace yet; otherwise one line in the report and no commit. It runs in its single-file
mode: the siblings it lists as now qualifying names they will drop later go on the ledger's
**noticed** list and into the report. Its fan-out is tree-wide (every caller in `src/ra` and
`src/td`), so the plain `build` must pass before the commit; its strict checks go through
`tools/strict_tu.py` on every file it touched, like the other stages. Hooks it keeps global and any
it would rather move go to **for the user**.

**8. Coverage** - `/cover-files <files>`, to every line a test can reach. The tests are one commit
("Cover ..."); a new test target's `CMakeLists.txt` entry goes with them. What the tests turn up
follows this run's rules rather than waiting for the user: a bug is fixed at the end of the stage
under the rule below (the test that found it is its failing test), and a line no caller can reach is
removed through `/remove-dead-code`, each as its own commit. The lines left uncovered, with the
reason for each, and the branch sides not worth a test go on the ledger's **noticed** list. A file
that needs the display, the main loop or game data throughout may end the stage with no tests: say
what seam would make it testable on the **for the user** list, and make no commit.

**9. IDE** - `/fix-ide-issues <files>`, the test file included. If CLion still times out, say so in
the report and skip rather than guess.

**10. File name and folder** - the file-rename step of `/rename-google-style` (its section 5, step
7, and the rules under "The file name is part of the pass" and "So is the folder"). A rename and a
move are one `git mv` and one commit; the `_test.cc` and its test target go with the pair. A twin
that moves with the file is its own commit. A move that step does not make - out of the game, across
engine libraries, or a split - goes to **for the user** with what stands in the way, and so to
stage 11. If the name and the folder already fit, record why and make no commit.

**11. Leftovers to `TODO.md`** - every open item from the ledger, however small, following
`/add-todo` for wording and section: bugs not fixed (behaviour-changing, out of scope, twin not
done), **noticed** items not acted on, **misplaced** items not moved, **for the user** decisions,
design ideas `/simplify` raised but did not apply, lines stage 8 left uncovered and what would make
them testable, stages skipped (a CLion timeout becomes "Run `/fix-ide-issues` on ..."), and things
spotted outside the files' scope. Give each entry what it takes to act without re-reading the run:
the files and lines, what is wrong or possible, the options with their size, and the run's commit
range. Before adding, grep `TODO.md` for an entry that already covers it (an existing item may only
need the new file name or an extra detail); delete entries this run finished (a "move X into its
namespace" item the namespace stage closed). Run Prettier on `TODO.md` (the pre-commit hook rejects
it otherwise) and commit it alone, as `c8a9d6c4` did: "Add the <file> modernization's leftovers to
TODO.md". A run whose ledger ends empty says so in the report and makes no commit.

**Bugs found in any stage** go on the ledger when found and are fixed at the end of that stage, as
their own commit, following the "Fixing the bugs the pass found" rules in `/update-comments`:
confirm against the callers, failing test first where the code is reachable from one, smallest fix,
twin fixed separately.

If a stage cannot be made to build or pass the strict checks, stop there: report what was committed,
what is left in the tree, and the error. That includes the full strict pass before stage 10: a run
that ends with it unclean is a failed run, however many stages committed.

## Moving out what does not belong

Legacy files collect things by accident: an enum declared in the first header that needed it, a
helper class beside its only early user, a table in `defines.h` because everything included it.
Stage 2's close reading is where these are spotted and put on the **misplaced** list; stage 5 moves
them.

**What is misplaced.** A declaration is in the wrong file when a reader looking for it would not
open this file, or when other files include this header only to get it. Check before deciding:
`git grep -nw '<name>' -- src` shows who uses it. Typical cases:

- An enum, struct or constant that describes another subsystem, or that many files use and this one
  merely happened to declare.
- A second class in the header that is not a part of the first.
- A free function or table whose callers are all elsewhere.
- Something in the header that only this `.cc` uses.
- The reverse: a declaration elsewhere that is plainly this file's (its own enum living in
  `defines.h`). It is pulled in under the same rules.

**Where it goes**, the nearest fit first:

1. Into the `.cc`'s anonymous namespace, when only this `.cc` uses it. The cheapest move and the
   most common.
2. Into the class, as a nested type or a private member, when only that class uses it.
3. Into the existing file whose subject it is.
4. Into a new `.h`/`.cc` pair named after it (`/rename-google-style`'s file-name and folder rules;
   new-file rules in `CLAUDE.md`), when it is a subject of its own and has no home. A header holding
   two unrelated classes is split this way.

**What is done here and what is reported.** A move inside the game's own folders, or inside one
engine library, is made. A move that crosses a library or a namespace (`ra/` -> `game/sim/`,
`engine/gfx/` -> `engine/window/`), that would need a dependency cut for `tools/check_layers.py` to
pass, or that changes what a save game or a network packet contains goes to **for the user** with
what stands in the way. So does a doubtful fit: say where it might go and leave it. If the file is a
catch-all (`defines.h`, `function.h`), move out what has an obvious existing home and list the rest;
dissolving it is a plan of its own, not a stage.

**How.**

- Move the declaration, its definition and its comment verbatim. The commit is a pure move, so
  `git diff --color-moved` shows nothing but the move and the include changes; any edit to the moved
  code is a separate commit, before or after.
- Its tests move to the destination's test file in the same commit.
- Fix the includes over every user: each one includes the new header, and drops the old one if it
  used nothing else from it. `misc-include-cleaner` finds both in `.cc` files; headers are checked
  by hand. An include that disappears from a widely used header is part of the gain - say in the
  commit message when a move shrinks a fan-out or breaks a cycle.
- `git clang-format -f` on the touched files, the plain `build` (the fan-out is tree-wide),
  `tools/strict_tu.py` on the touched files, and `engine_layers_test`.
- One commit per move, "Move `<what>` from `<file>` to `<file>`", through `/commit`.
- The twin's copy stays where it is; note it for stage 11 so the other game's run picks it up.

After this stage the later ones see only what stayed: stage 6 restructures it, stage 8 covers it,
and stage 10 names and places the file by it.

## Report

- One line per stage: the commit(s) it made (`git log --oneline <base>..HEAD`), or why there was
  none, and what the full strict pass before stage 10 turned up.
- Bugs fixed, each with its commit and whether the twin got it.
- What moved out or in, each with its commit, and the misplaced items left where they are with the
  reason.
- The accessors stage 6 added per class, and any member left public or protected with the reason.
- Coverage of each file before and after stage 8, and the lines still uncovered.
- **For the user**: behaviour-changing fixes not applied (what would play differently), names you
  were unsure of with the alternative, anything skipped (e.g. the IDE never answered).
- The `TODO.md` commit from stage 11, with its entries in one line each.
- What is still uncommitted, if anything, and why.
