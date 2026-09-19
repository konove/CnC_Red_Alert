---
allowed-tools: Read, Grep, Glob, Edit, Write, Bash, mcp__ide__getDiagnostics
description: 'Fix what the IDE (CLion) flags in a file - its inspections ("parameter can be made const", "type can be replaced with auto", "can be moved into an anonymous namespace", redundant qualifiers, ...) - applying the project''s policy for each kind, skipping the ones the project has decided against, and verifying with the strict build. Use whenever the user asks to fix, clear or clean up "IDE issues/warnings/inspections/findings", "what CLion shows", "the yellow squiggles" or "the IDE diagnostics" in a file, or to make a file clean in the IDE.'
---

Fix the IDE issues in: $ARGUMENTS

With no argument, work on the file the IDE has open (the harness reports it as "The user opened the
file ... in the IDE"). Either way, include the `.h`/`.cc` partner of each file: a fix in one often
needs the other, and the header's inspections belong to the same job.

The IDE is CLion. Its inspections are not clang-tidy: they overlap with it, disagree with it in a
few places, and the project's profile (`.idea/inspectionProfiles/Project_Default.xml`) switches on
several that the strict build never checks. The user wants the file clean _in the IDE_, which is why
this is a separate job from making the strict build pass. The precedent is `b4f52b60` ("Clear the
compiler and IDE warnings in ra/conquer.cc"); read its message once if a category below leaves you
unsure.

## 1. Collect the findings

Call `mcp__ide__getDiagnostics` with `uri: file:///<absolute path>` for each file. Save the list to
the scratchpad: you will diff against it at the end.

- **"Timeout getting diagnostics"** means CLion has not analyzed the file, almost always because it
  is not open. Ask the user to open it (one sentence, name the file), and meanwhile do the rest.
  Don't guess the findings from reading the code: the point is to match what the IDE shows.
- **Ignore the diagnostics the harness pushes after an edit** ("new-diagnostics" with
  `'ra/defines.h' file not found`, `Unknown type name 'VocType'`, `C++20 extension`). They come from
  a clang instance that has no compile database; every other line in them follows from the missing
  include. They are not the IDE's inspections, and the strict build is the authority on real compile
  errors.
- Line numbers from `getDiagnostics` are 0-based; add one before reading the file.

Group the findings by message, then work category by category rather than line by line: one decision
per category keeps the result consistent across the file.

## 2. What to do with each kind

| Finding                                                             | Action                                                                                                                                                                                                                                                                                                                                                                                                                   |
| ------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Parameter 'x' can be made const                                     | Add top-level `const` to the by-value parameter **in the definition only**. In a separate declaration it means nothing to callers and Google style leaves it out, so the `.h` stays as it is. An inline definition in a header is a definition: const it there. For a pointer or reference parameter the finding is about the pointee; check that nothing writes through it or advances it (`*p++`) before believing it. |
| Type can be replaced with auto                                      | Apply. CLion only raises it when the type is already spelled on the right (a cast, a constructor call, an enumerator, a literal), so nothing is hidden. Keep the qualifiers: `const char* s = "x"` becomes `const auto* s = "x"`, `static VoxType v = VOX_NONE` becomes `static auto v = VOX_NONE`.                                                                                                                      |
| 'T' can be moved into an anonymous namespace                        | Wrap the `.cc`-local types (enums, structs, classes used only in this file) in one `namespace { ... }  // namespace` block near the top of the file. Functions and variables keep `static`: that is the codebase's form for them, and `misc-use-anonymous-namespace` stays excluded in `.clang-tidy`. A `static` table of a wrapped type may stay outside the block.                                                     |
| Redundant qualifier                                                 | Drop it (`AudCompression::SCOMP_NONE` where `using enum AudCompression` is in scope is `SCOMP_NONE`).                                                                                                                                                                                                                                                                                                                    |
| Redundant cast / else / braces / elaborated type specifier / static | Apply, unless it would change behaviour (a cast that picks an overload or narrows is not redundant; say so).                                                                                                                                                                                                                                                                                                             |
| Parameter names mismatch between declaration and definition         | Make them agree, picking the better name; comments that mention the parameter follow.                                                                                                                                                                                                                                                                                                                                    |
| Declaration and assignment can be joined / too-wide scope           | Apply: declare at first use, initialised.                                                                                                                                                                                                                                                                                                                                                                                |
| Parameter is never used                                             | Not a local fix: removing it changes every caller. Follow `.claude/commands/remove-dead-code.md` ("Dead parameters and functions") for it, or list it for the user if it is a virtual override or a callback whose signature is fixed.                                                                                                                                                                                   |
| Unused include directive                                            | Do not trust it. `misc-include-cleaner` in the strict build is the authority on includes; CLion misses direct uses that a header happens to supply transitively (`b4f52b60`: `<cstdint>`). Remove an include only if the strict build agrees.                                                                                                                                                                            |
| Includes order                                                      | `git clang-format` decides; do not hand-sort.                                                                                                                                                                                                                                                                                                                                                                            |
| Anything from a check `.clang-tidy` disables on purpose             | Skip, and name the `.clang-tidy` line in the report. CLion runs some clang-tidy checks regardless of the config (it reported `hicpp-signed-bitwise` in `b4f52b60`); silencing them in the source would undo a decision already recorded there.                                                                                                                                                                           |
| A finding you can show is wrong                                     | Leave the code, and say why in the report (e.g. a pointer "can be const" but is incremented). This is a good outcome, not a failure.                                                                                                                                                                                                                                                                                     |

A category not in the table: apply it when it is a pure style fix that the strict build and Google
style agree with; otherwise list it with what you would do and let the user decide. Never add
`// NOLINT` or a `// NOLINT`-style suppression to quiet the IDE.

These are cosmetic by design. If a fix would change what the code does (a cast that was selecting an
overload, a `const` that forces a copy, an `auto` that deduces a different type than was spelled),
it is not an IDE fix: leave it and report it. Bugs noticed while reading go in the report too, not
in this diff; the user usually wants them next, as a separate change.

## 3. Verify

1. `git clang-format -f -- <touched files>` (never `clang-format -i`, which reflows untouched legacy
   code).
2. `cmake --build build --parallel 22 && ctest --test-dir build`, or only the game's target when
   nothing shared changed.
3. `cmake --build build-strict --parallel 14`, in the foreground as its own command. Top-level
   `const` and `auto` are where `misc-const-correctness` and `modernize-*` disagree with the IDE, if
   they are going to.
4. Call `getDiagnostics` again on each file. CLion re-analyzes after the save, which can take a few
   seconds; if the list still shows fixed lines, wait briefly and ask again. What remains should be
   exactly the findings you skipped on purpose.

Do not commit unless asked. When the user says "commit", `/commit` applies; the subject follows the
precedent ("Clear the IDE warnings in ra/ww_audio.cc").

## 4. Report

- Before and after: how many findings the IDE showed for each file, and how many remain.
- What was fixed, by category with counts (not line by line).
- What remains and why: disabled in `.clang-tidy` (name the check), wrong (say why), needs a wider
  change such as an unused parameter (say what), or changes behaviour.
- Anything else noticed while reading - suspected bugs, dead code - as a short numbered list.
