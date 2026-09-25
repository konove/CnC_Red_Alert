# Plan: adopt cpplint

Written 2026-09-25 against cpplint 2.0.2 (the `cpplint/cpplint` fork on PyPI, which the style
guide's [cpplint section](https://google.github.io/styleguide/cppguide.html#cpplint) points to).

## Context

The style guide recommends `cpplint.py` for catching style errors. The tree already enforces most of
the guide through clang-format (layout) and clang-tidy (`google-*`, `readability-*`,
`misc-include-cleaner`, the cast and integer checks), so the question is what cpplint adds on top.

A default run over the 1,096 tracked `.h`/`.cc` files under `src/`, measured 2026-09-25:

| Category                     | Findings | Verdict                                                                                                                                     |
| ---------------------------- | -------: | ------------------------------------------------------------------------------------------------------------------------------------------- |
| `whitespace/*`               |   15,394 | Off for good. clang-format is the formatter of record; the rest are tabs and `//comment` spacing inside the original EA comment blocks.     |
| `build/header_guard`         |    1,044 | Off for good. The project guard is `CNC_RED_ALERT_<PATH>_H_`; cpplint can only derive `<PATH>_H_` (`root=src`) or one that includes `SRC_`. |
| `legal/copyright`            |      400 | Off for good. CLAUDE.md: new files carry no copyright header.                                                                               |
| `readability/nolint`         |      246 | Off for good. The `NOLINT` comments name clang-tidy checks, which cpplint does not know.                                                    |
| `build/c++11`, `build/c++17` |       53 | Off for good. Google's internal ban list (`<filesystem>`, `<thread>`, `<chrono>`); the tree uses them on purpose.                           |
| `build/include_what_you_use` |       48 | Off for good. `misc-include-cleaner` owns includes; cpplint matches bare names, so a parameter called `string` asks for `<string>`.         |
| `readability/check`          |       37 | Off for good. `DCHECK(a > b)` is deliberate: the static analyzer cannot see through `DCHECK_GT`.                                            |
| `readability/todo`           |        3 | Off for good. It wants `TODO(username)`; CLAUDE.md writes `// TODO: bug 12345 - ...`.                                                       |
| everything else              |     ~500 | Real. Fix, then enable (step 3).                                                                                                            |

The real findings are what cpplint is for here. Most sit in code the compiler never sees on Linux -
`#ifdef OLDWAY`, `WIN32`, WOLAPI and the other dead branches - where clang-tidy cannot reach: `long`
and `short`, C-style casts, `sprintf`, `strtok`. The rest are in built code and break rules no other
tool checks: `#include "rand.h"` without its directory (`td/anim.cc:72`, against CLAUDE.md's include
rule), a system header after a project header, a `virtual` on a `final` method, a `;` after `}`.

Files the build never compiles at all (RA's `alloc.cc`, `surface.cc`, `turret.cc`, ... and both
games' Windows stubs) are excluded rather than cleaned up; they are dead-code candidates.

## Design

**Configuration** lives in `CPPLINT.cfg` at the repository root (`set noparent`, `root=src`,
`linelength=80`, the filter list with a comment per entry), which cpplint and IDE plugins read by
themselves. The filter has two blocks: the permanent one above, and a temporary one listing each
real category with its count. `src/ra/CPPLINT.cfg` and `src/td/CPPLINT.cfg` hold the `exclude_files`
for the unbuilt files, since cpplint matches that option only against files in the config's own
directory.

**Enforcement** is a ctest, `cpplint_test`, next to `engine_layers_test`: `tools/run_cpplint.py`
walks `src/` and runs cpplint in parallel (a single process takes ~45 s; the test takes a few). CI's
build job runs `ctest` already; the Linux runner installs a pinned cpplint with `pipx`, and CMake
skips the test where cpplint is missing, so no machine needs it to build. CMake's own
`CMAKE_CXX_CPPLINT` was rejected: it only lints the `.cc` files a target compiles, never the
headers, and its findings do not fail the build.

**Skills, not a new one.** The rules are mechanical, so the test enforces them. `/commit` gains a
row to run cpplint on the touched files; CLAUDE.md's Tools table and dependency list gain cpplint.

## Steps

1. `CPPLINT.cfg` files with both filter blocks, so the tree passes from the start.
2. `tools/run_cpplint.py`, the `cpplint_test` ctest, the CI install, CLAUDE.md and `/commit`.
3. Ratchet: fix one temporary category at a time and delete its filter line, one commit per category
   (RA and TD split as usual). Cheap and real first: `build/include_subdir` (23),
   `build/include_order` (16), `runtime/explicit` (22), `readability/inheritance` (10),
   `build/include` (5), then `readability/braces` (182, mostly `;` after `}` and one-sided `else`
   braces), then the dead-branch `runtime/int`, `readability/casting` and `runtime/printf`. A
   finding in a dead branch is often better fixed by deleting the branch (`/remove-dead-code`) than
   by modernizing it.

## Progress

- [x] Step 1 (2026-09-25)
- [x] Step 2 (2026-09-25): `cpplint_test` passes in 4.2 s locally.
- [ ] Step 3
  - `build/include` and `build/include_subdir` (2026-09-25). The first was hidden behind the
    second's filter: cpplint filters match by prefix. Its 5 findings were an `init.cc` duplicate and
    two dead files per game, `crew.cc` (only its comment header) and `findpath.h` (a free
    `Optimize_Moves()` nothing defines; the real one is a `FootClass` member), now deleted. The 23
    directory-less includes gained `ra/` or `td/`, 4 of them duplicates that went instead, and
    `wspipx.cc`'s Windows branch includes the SDK's `<wsnwlink.h>` as a system header.
  - `build/include_order` (2026-09-25): 16 findings in 7 files, plus the 7 other tests that spelled
    `<gtest/gtest.h>`, which cpplint reads as a C system header; all 107 other includes of it are
    `"gtest/gtest.h"`. `ra/netdlg.cc` had a second include block below its constants.
  - `runtime/explicit` moved to the permanent block (2026-09-25). Of its 22 findings, 20 are
    deliberate implicit conversions already marked `NOLINTNEXTLINE(*-explicit-constructor)` for
    clang-tidy's `misc-explicit-constructor`, which enforces the rule on compiled code; cpplint
    cannot read that comment. The other two sit in dead branches (`#ifdef NEVER` in `td/target.h`,
    `#ifdef JAPANESE` in `td/msgbox.h`) and are `/remove-dead-code` material.
  - `readability/inheritance` (2026-09-25): 10 methods declared `virtual ... final` in the class
    that introduces them (`LinkClass::Head_Of_List()`, `ListClass::Remove_Scroll_Bar()`,
    `WinsockInterfaceClass::Close_Socket()`, ...), which nothing can override. Both keywords went.
