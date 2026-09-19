---
description: Rename the identifiers a file declares to the Google C++ naming scheme and, while doing so, replace legacy names with ones that say what the code does. Use whenever the user asks to rename things in a file "according to the Google style guide / naming guidelines", to "use better names", to modernize names in a .cc/.h pair, or pastes the styleguide Naming link, even if they only mention one of the two goals.
---

Rename the identifiers declared in: $ARGUMENTS

Two jobs in one pass: put every name the file owns into the Google scheme, and make each name say
what the thing is or does. The second job is the one that gets skipped. The first time this was done
on `conquer.cc`, `Main_Loop` became `MainLoop` and the user had to come back with "since you are
renaming anyway, use better names" - after which it became `RunFrame`. A transliteration is only
right when the old name was already good.

## 1. Scope: what the file owns

Rename what the named files **declare**: functions, types, constants, file-scope and static
variables, data members, parameters and locals. Every use elsewhere follows. Names the files merely
_use_ belong to some other file's pass.

| Leave alone                                             | Why                                                                                                                                                                                                                    |
| ------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Globals from `ra/externs.h`, names from other headers   | Not declared here; renaming them is a tree-wide job the user did not ask for.                                                                                                                                          |
| A virtual override whose base is declared elsewhere     | The whole hierarchy has to move together. If the base _is_ in scope, every override in the tree follows.                                                                                                               |
| Enumerators                                             | They were deliberately kept through the `enum class` migration, are used tree-wide through `using enum`, and `magic_enum` turns them into text. Ask before touching them.                                              |
| Macros                                                  | Google style keeps macros `ALL_CAPS`. A macro that is really a constant is better turned into a `constexpr kName`, but say so rather than doing it silently.                                                           |
| Text inside string literals                             | INI keys, file names and scenario codes look like identifiers and are not.                                                                                                                                             |
| Tiberian Dawn's copy of an RA function, and the reverse | `src/ra` and `src/td` are separate targets that share many names. Rename within the game the file belongs to. Files under `sdllib`, `tech`, `port`, `base`, `winvq` are shared, so their callers are in all of `src/`. |
| File names                                              | Only when asked.                                                                                                                                                                                                       |

Dropping a legacy `Class` / `Type` suffix from a type (`FileClass` -> `File`) is welcome, but it is
a wide change with real collision risk. Do it when the type is declared in scope, check the new name
is free, and keep it apart from the rest (its own commit later).

## 2. The scheme

`CLAUDE.md` has the table; the points that come up in practice:

- Functions `PascalCase`, no underscores: `Set_Frame_Timer()` -> `StartFrameTimer()`.
- Accessors and mutators are named like the variable: `count()`, `set_count(int)`.
- Locals, parameters, struct fields `snake_case`; class data members `snake_case_`.
- Constants, including `static constexpr` and `const` with static storage, `kPascalCase`.
- Legacy statics with a leading underscore (`_ftimer`, `_up`) lose it; they become ordinary
  `snake_case` names, which usually means they need a real name too (`pulse_timer`, `pulse_rising`).
- An abbreviation is a word: `PumpWolapiMessages`, `StartRpc`, not `PumpWOLAPIMessages`.
- A parameter is spelled the same in the declaration, the definition and the comment that mentions
  it.

## 3. Better names

Read the body before naming anything. The name describes what the code does **now**, which in this
codebase is often not what the 1996 name says.

**Functions are verb phrases that state the effect.** Ask "what is true after this returns?"

| Old                           | New                        | What made the old one wrong                                                        |
| ----------------------------- | -------------------------- | ---------------------------------------------------------------------------------- |
| `Call_Back()`                 | `ServiceRealTime()`        | Named after how it was invoked, not what it does (pump sound, network, timers).    |
| `Main_Game()` / `Main_Loop()` | `RunGame()` / `RunFrame()` | Two "mains"; the pair now says which one is the whole game and which is one frame. |
| `Sync_Delay()`                | `WaitForNextFrame()`       | Says what the caller gets, not the mechanism.                                      |
| `Color_Cycle()`               | `CyclePalette()`           | Verb first; it is the palette that cycles.                                         |
| `Run_Special_Dialog()`        | `RunPendingDialog()`       | "Special" carries no information; "pending" is the condition it checks.            |
| `Debug_Quit_Frame()`          | `LogFrameAndQuitIfDue()`   | It did two things and the name hid one.                                            |
| `Process_Input()`             | `ProcessInput()`           | Already right. Keep it.                                                            |

- Predicates read as questions: `IsFinished()`, `HasCargo()`, `CanEnter()`, `ShouldRedraw()`.
- Siblings are parallel. If one is `BeginScenario()`, the others are `RunScenario()` and
  `EndScenario()`, not `ScenarioLoop()` and `FinishUp()`.
- A name that needs "And" is telling you the function does two jobs. Name it honestly, and mention
  it in the report; do not split it as part of a rename.

**Variables are nouns for what they hold, with the unit when it is not obvious.**

| Old         | New               |                                                             |
| ----------- | ----------------- | ----------------------------------------------------------- |
| `size`      | `frame_bytes`     | Size of what, in what?                                      |
| `val`       | `pulse_level`     |                                                             |
| `changed`   | `palette_changed` | A flag says what it is a flag _of_.                         |
| `_up`       | `pulse_rising`    | Booleans read as a statement that is true or false.         |
| `sequence`  | `captured_count`  | It counted; it was not a sequence.                          |
| `first`     | `wrapped_color`   | Named for its position in the code rather than its meaning. |
| `temp_page` | `frame_page`      | `temp`, `tmp`, `my`, `the`, `data`, `info` say nothing.     |

- Put the unit in the name where the codebase has several: ticks / frames / ms, bytes / pixels /
  cells / leptons. Never put the type in (`lpszName`, `pBuffer`, `nCount`).
- Length follows scope. `i` is fine for a five-line loop; a member or a file-scope static needs a
  full name. If an index survives past its loop, name what it indexes (`house_index`).
- Do not repeat the context: `Cargo::cargo_count_` is `count_`.
- No abbreviations a newcomer would have to ask about (`cnt`, `idx`, `buf`, `ptr`, `num` are out;
  `id`, `max`, `min`, `src`/`dst` in blit code are fine).

**Keep the game's own vocabulary.** Techno, house, theater, cell, coord, lepton, facing, mission,
shape, mix, remap are the project's terms of art, and replacing them with generic words makes the
code harder to search and to match against the original source. For any concept, look at what the
neighbouring, already-modernized code calls it and use the same word; consistency beats a slightly
better word used once.

**Do not rename for the sake of it.** If the old name is accurate and only the case is wrong, change
the case and move on.

## 4. Build the rename table first

Before editing, write the table to the scratchpad: every name the files declare, its new name, and
for anything beyond a case change a few words on why. Then, for each row:

- **Blast radius:** `grep -rnw '<old>' <scope>` (scope from section 1). Note a second _definition_
  of the same name on an unrelated class - `Set_Name` exists on files, types, triggers and audio
  classes - because those rows need the compiler-driven method below.
- **Collision:** `grep -rnw '<new>' <scope>`. Look for an existing function, macro or member with
  that name, a local that would now shadow a function or member (`-Wshadow` is an error in the
  strict build), and an accessor that would collide with its own member.

Do not stop to get the table approved; the user asked for the rename. Proceed, and put the table in
the final report, where a name they dislike is a one-line fix.

## 5. Apply it

Work from the narrowest scope outwards.

1. **Locals and parameters:** edit inside the one function. A plain English word (`size`, `first`,
   `changed`) must never be replaced file-wide - it will hit comments ("the first frame"), other
   functions' locals and members of unrelated types.
2. **File-local functions and statics:** replace within the file.
3. **Names visible to other files, unique spelling** (`Main_Loop`): a scripted replace over the
   scope is safe, including in comments, which should keep referring to the right function. Split
   each file into code / comment / string segments first and never substitute inside strings.
4. **Names shared with unrelated declarations:** rename the declaration and definition only, then
   let the compiler find the callers:
   `cmake --build build --parallel 22 -- -k 0 2>&1 | grep 'error:'`. Each
   `no member named 'Set_Name'; did you mean 'SetName'` gives an exact `file:line:col`; patch those
   sites and rebuild until clean.
5. **Code the compiler never sees:** `#if 0` blocks, `#ifdef WIN32` branches and files excluded from
   the build (`winvq/vqaview`, TD's `rawfile.cc` / `cdfile.cc`). After the build is clean,
   `grep -rnw '<old>'` over the scope must come back empty, or show only the other game's own
   function.
6. **Everything that mentions the old name in prose:** the header's file comment, tests,
   `docs/*.md`, `CLAUDE.md` (its Key Files and examples name real functions), `TODO.md`.

This pass changes names only. Unused parameters, a parameter every caller passes the same value for,
dead code and outright bugs will turn up while reading closely; the user nearly always wants them
dealt with next, so list them in the report, but keep them out of this diff so it stays reviewable
as a pure rename.

## 6. Verify

- `git clang-format -f -- <every touched file>`: longer names re-wrap lines. Never
  `clang-format -i`, which reflows the untouched legacy code in the same files.
- `cmake --build build --parallel 22 && ctest --test-dir build`.
- `cmake --build build-strict --parallel 14`, in the foreground as its own command. This is where a
  new shadowing warning or a missed clang-only call site shows up.
- Skim `git diff --stat` for files that should not be there, and grep the diff for changes inside
  quotes: `git diff -U0 | grep '^[-+]' | grep '"'`.

Commit only when asked, through `/commit`: one commit per game, a type rename on its own, and a
message that names the renames worth knowing about the way `615ce9a1` does.

## 7. Report

- The rename table, non-mechanical rows first, each with its reason.
- Names deliberately left alone, and why (section 1, or "already accurate").
- Names you were unsure of, with the alternative you considered.
- What you noticed but did not change: unused or constant parameters, functions doing two jobs, dead
  code, suspected bugs.
