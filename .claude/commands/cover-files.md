---
description: 'Raise the unit-test coverage of the selected files to 100% - measure it with tools/coverage.py, write the missing tests (creating the test file and its CMake target when the file has none), and account for every line a test cannot reach. Use whenever the user asks to "cover", "increase/raise/improve coverage of", "get X to 100%", "add the missing tests for" or "write tests for" a file or files, says a file "has no tests", or pastes a coverage report and wants the gaps closed.'
---

Cover: $ARGUMENTS

With no argument, work on the file the IDE has open. Include the `.h`/`.cc` partner: inline
functions and templates in the header are part of the same job, and the report counts them
separately.

The goal is that every line of the selected files runs under a test **that would fail if the line
were wrong**. The number is how the gaps are found, not what is delivered: a test that calls a
function and asserts nothing reaches 100% and protects nothing. The user reads these tests later to
learn what the code does, so each one states a behaviour.

## 1. Measure

```bash
tools/coverage.py --branches <files>
```

It builds the tree with clang's source-based coverage in `build-coverage` (configured on first use;
about 2.5 minutes cold, a few seconds after that), runs every gtest executable, and prints for each
file the line, function and branch totals, the uncovered lines with their text, and the lines with a
branch side never taken. Code a death test reaches counts. Exit status 0 means every line of every
named file is covered and all tests pass.

Save the first report to the scratchpad; the final report quotes the before numbers.

Three results need no tests:

- **`no executable code`** - a header of declarations. Done.
- **Already 100% lines, no branch worth a test** (see step 3). Say so and stop.
- **Lines that cannot run** - see step 4 before writing a test that contorts itself to reach them.

## 2. Find or create the test

Look for `<name>_test.cc` beside the file, then grep the tests for the file's functions: the code
may be exercised from another suite (`ra/objects/target_test.cc` lives in `ra_game_test`). Add to
the existing test file when there is one.

When there is none, the report says either `no test reaches this file` (some test binary links it)
or `in no test binary` (none does). Create `<name>_test.cc` next to the source and register it in
that folder's `CMakeLists.txt`. Pick the lightest target that works, because a test that needs less
is faster to build and says more clearly what the code depends on:

| The file                                         | Target                                                                                                                                                                                                                    |
| ------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| is in `base/`, `engine/<lib>/` or `game/<lib>/`  | add the test source to the folder's existing `add_gtest` (`engine_gfx_test`, `game_core_test`); it links the library                                                                                                      |
| is in `ra/` or `td/` and stands alone            | its own `add_gtest` that compiles the source directly with the few libraries it needs (`ra_profile_test`: `SOURCES data/profile_test.cc data/profile.cc`)                                                                 |
| needs houses, objects, the map, heaps, the rules | a `ra_engine`/`td_engine` test built on a real `Game`: copy the `ra_carryover_test` block (the `startup.cc` source, `RA_NO_ENTRY_POINT`, `apply_dotenv_settings`, `apply_language_setting`) and derive from `GameFixture` |

`GameFixture` (`ra/startup/game_test_util.h`) gives a `Game` with house and vehicle types and an
empty map; a fixture builds the other types and heaps it needs in its constructor. Read
`ra/ai/reinforcements_test.cc` before writing one - it shows how to place buildings, teams and
aircraft without game data. Tiberian Dawn has no shared fixture yet; `td/ai/reinforcements_test.cc`
builds its own. Link `base` in any `add_gtest` that includes `base/` headers, or a failed include
makes clang-tidy report nonsense.

New test files follow CLAUDE.md like any new file: no EA header, a file comment, the game's
namespace with the tests in an anonymous namespace inside it.

## 3. Write the tests

Work through the report function by function, not line by line: read the function, list the
behaviours its branches distinguish, and write one test per behaviour, named for it
(`TEST_F(ReinforcementTest, InfantryLeavesTheBarracksAtTheOrigin)`). Covering a function this way
usually takes its uncovered lines together, and the test names end up as its specification.

- **Assert the outcome the lines produce**, not merely that they ran. For an early `return false`,
  assert the `false` _and_ that the state it guards was left alone.
- **Check that the test can fail.** For anything not obvious, break the line (flip the condition,
  drop the statement), watch the test fail, and restore it. A test that passes either way is
  measuring nothing; this is the check `96a6794c` describes ("undoing 2abc87b6 by hand fails the
  last of these"). Run the broken build's test from `build/`, not `build-coverage`: an instrumented
  binary run by hand drops a `default.profraw` in the working directory.
- **Branches.** Line coverage is the gate; the `--branches` list is the to-do list for real
  conditions. Give each side of an `if`, each operand of `&&`/`||` that changes the result, and both
  the zero-trip and the many-trip case of a loop a test. The failure side of `CHECK`/`DCHECK` and
  the branches inside logging macros are not worth one - `DCHECK` is compiled out of this build
  anyway. A `CHECK` that guards a public contract is worth an `EXPECT_DEATH`.
- **Inputs come from the code, not from guesses.** Build the files, palettes and INI text a test
  needs in memory (`MemoryStream`, `IniFile` from text); tests run with no game data and must not
  read the Steam install.
- **A test that hangs or crashes has found a bug.** So has one whose correct expectation fails. Stop
  writing tests around it: report it, and fix it as its own change when the user agrees (the
  precedent is `96a6794c`, where a test for an unplaceable team hung on an unbounded retry loop).
  Never bend an assertion to match behaviour you believe is wrong; if the behaviour must stay, name
  the test for what it does and say why in a comment.
- **Private state.** Test through the public interface. If a line is only reachable by poking at
  internals, that is evidence for step 4, not a reason for a `friend` or a `#define private public`.

Re-measure as you go, narrowed to the suite you are editing so the loop stays in seconds:

```bash
tools/coverage.py --tests 'ra_reinforcements' --branches <files>
```

## 4. Lines no test can reach

Some lines stay uncovered for a reason. Decide which reason, because each has a different fix, and
none of them is a test written only to make the number move:

| Why the line is unreachable                                                                   | What to do                                                                                                                                                      |
| --------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| No caller can produce the state (a `default:` over a full enum, a null check after a `CHECK`) | It is dead code. Remove it under `.claude/commands/remove-dead-code.md`, as a separate change, and say so in the report.                                        |
| It needs the display, the audio device, a socket, real game data or the main loop             | Leave it uncovered and list it. If a small seam would make it testable (pass the stream in, split the pure part out), propose the seam; don't refactor unasked. |
| It is a platform branch that does not compile here (`_WIN32`)                                 | Not in the report at all; nothing to do.                                                                                                                        |
| It is reachable, but only through a long set-up the fixture cannot build yet                  | Extend the fixture when that is modest (`afd00292` added `Init_Cells()`); otherwise list it with what the fixture lacks.                                        |

Do not add coverage-exclusion comments or pragmas: the project has none, and an excluded line reads
as covered to the next person. A listed exception in the report is the honest record.

## 5. Verify

1. `tools/coverage.py --branches <files>` with no `--tests`, so the final numbers count every suite
   and every test in the tree still passes under instrumentation.
2. `git clang-format -f -- <touched files>`.
3. `cmake --build build --parallel 22 && ctest --test-dir build` - the coverage build is clang with
   `NDEBUG`; this one is the everyday GCC build.
4. `tools/strict_tu.py <test files and any source touched>`. Test code is held to the same
   clang-tidy checks as the rest; GoogleTest's macros need the `NOLINTNEXTLINE` forms the existing
   tests use (see `game/core/owned_vector_test.cc` for `EXPECT_DEATH`).

Do not commit unless asked (`/modernize-file` asks: it commits each stage). When the user says
"commit", `/commit` applies; tests go in one commit with a subject that names what is now covered
("Cover the rest of Red Alert's reinforcement deliveries"), and each bug fix or dead-code removal in
its own.

## 6. Report

- Per file, before and after: lines, functions, branches.
- The tests added, as a list of the behaviours they pin down, and any new test target.
- Every line still uncovered, grouped by the reason from step 4. If that list is empty, say the
  files are at 100%.
- Bugs and dead code the tests turned up, numbered, with what you did about each (fixed, removed, or
  left for the user to decide).
