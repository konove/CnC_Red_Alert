---
description: 'Take a legacy file (and its .h/.cc partner) through the whole modernization sequence in one go - dead code, comments and the bugs they turn up, integer types, Google-style names, simplification, IDE inspections, and finally the file name - committing each stage. Use whenever the user asks to "modernize", "update", "clean up", "do the usual pass on" or "go through" a file or pair of files, or asks for several of /remove-dead-code, /update-comments, /migrate-types, /rename-google-style, /simplify and /fix-ide-issues on the same file.'
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
   whether a value is a count or a bit pattern, which is what the comment pass found out.
4. **Names** after the types, so a name can carry the unit the type now states (`distance_cells`).
5. **Simplify** what the earlier stages listed but kept out of their diffs - every one of them ends
   with "noticed but not changed", and the user always asks for those next.
6. **IDE inspections** as the polish on the code as it finally stands; any earlier and the next
   stage would change what the IDE flags.
7. **The file name last**, alone in its commit, so git's rename detection holds and every earlier
   commit reads under the path reviewers know.

## Before starting

- Scope is the named file(s) plus the `.h`/`.cc` partner; with no argument, the file the IDE has
  open. Note the twin in the other game (`src/ra` ↔ `src/td`): bug fixes go to it, nothing else
  does.
- `git status`: unrelated changes stay in the tree and out of every commit (stage by path).
- Ask the user once, now, to open **both** files (the `.h` too) in CLion. Stage 6 needs them; CLion
  only answers for open files, and a mid-run question stalls the run for nothing.
- Keep a ledger in the scratchpad (`modernize-<file>.md`) with three lists: **bugs**, **noticed**
  (unused or constant parameters, redundant checks, tables that could be `constexpr`, functions
  doing two jobs), and **for the user** (anything that changes simulation behaviour, names you were
  unsure of). Every stage appends to it; later stages read it. It is what makes the stages add up
  instead of each rediscovering the same things.

## The stages

Commit at the end of each stage that changed something, following `/commit` (it verifies what has
not been verified, splits RA and TD, and writes the message). A stage that finds nothing gets no
commit and one line in the final report. Do not stop between stages to ask; the user asked for the
whole run.

**1. Dead code** - `/remove-dead-code <files>` in sweep mode. Unused parameters it notices go on the
ledger's **noticed** list for stage 5; removing them changes callers across the tree and belongs
with the other interface clean-ups.

**2. Comments and bugs** - `/update-comments <files>`, asking for the fixes too ("update the
comments and fix the bugs you find"). The comment pass is committed with the fixes it is interleaved
with; a fix in different lines, and each twin fix, is its own commit. Fixes that change simulation
behaviour are not applied: they go to **for the user**.

**3. Types** - `/migrate-types <files>`. A narrowing that was hiding an overflow is a bug: ledger,
then fix it at the end of this stage.

**4. Names** - `/rename-google-style <files>`, **identifiers only**: stop before its file-rename
step, which is stage 7. Leave out of the table anything the ledger already marks as going in stage 5
(a derivable member, a static that belongs elsewhere); renaming what is about to be deleted is churn
in two commits. Its "noticed" and "unsure" lists go on the ledger.

**5. Simplify** - first work through the ledger's **noticed** list: dead or constant parameters
through `/remove-dead-code <claim>`, the rest by hand, each only if it keeps behaviour. Then run
`/simplify` on what this run has changed so far (`git diff <base>..HEAD -- <files>`, with `<base>`
the commit before stage 1) and apply what it finds on those files. Anything that would change
behaviour goes to **for the user**.

**6. IDE** - `/fix-ide-issues <files>`. If CLion still times out, say so in the report and skip
rather than guess.

**7. File name** - the file-rename step of `/rename-google-style` (its section 5, step 7, and the
naming rules under "The file name is part of the pass"). If the name already fits, record why and
make no commit.

**Bugs found in any stage** go on the ledger when found and are fixed at the end of that stage, as
their own commit, following the "Fixing the bugs the pass found" rules in `/update-comments`:
confirm against the callers, failing test first where the code is reachable from one, smallest fix,
twin fixed separately.

If a stage cannot be made to build or pass the strict checks, stop there: report what was committed,
what is left in the tree, and the error.

## Report

- One line per stage: the commit(s) it made (`git log --oneline <base>..HEAD`), or why there was
  none.
- Bugs fixed, each with its commit and whether the twin got it.
- **For the user**: behaviour-changing fixes not applied (what would play differently), names you
  were unsure of with the alternative, anything skipped (e.g. the IDE never answered).
- What is still uncommitted, if anything, and why.
