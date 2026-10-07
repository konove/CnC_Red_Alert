---
description: Rename the identifiers a file declares to the Google C++ naming scheme and, while doing so, replace legacy names with ones that say what the code does - including the enums and their enumerators, which become `Enum::kName` with no `using enum` - and rename the file itself when its name no longer says what it holds, or move it to the folder its contents belong in. Use whenever the user asks to rename things in a file "according to the Google style guide / naming guidelines", to "use better names", to modernize names in a .cc/.h pair, to rename an enum or "drop using enum", or pastes the styleguide Naming link, even if they only mention one of the two goals.
---

Rename the identifiers declared in: $ARGUMENTS

Two jobs in one pass: put every name the file owns into the Google scheme, and make each name say
what the thing is or does. The second job is the one that gets skipped. The first time this was done
on `conquer.cc`, `Main_Loop` became `MainLoop` and the user had to come back with "since you are
renaming anyway, use better names" - after which it became `RunFrame`. A transliteration is only
right when the old name was already good.

**Every** identifier the file declares gets the second job, not only the ones whose case is wrong:
the types, the **enums and their enumerators**, the functions, the members, the constants, the
**parameters**, the locals, and the file name itself. Parameters and type names are the two that
keep getting left behind, and both cost more to come back for than to do now - a type name decides
the file name, and a parameter name is what a caller reads at every call site. Do the whole set in
this pass; a second pass over the same file is the failure mode this command exists to prevent.

## 1. Scope: what the file owns

Rename what the named files **declare**: functions, types, enums and their enumerators, constants,
file-scope and static variables, data members, parameters and locals. Every use elsewhere follows.
Names the files merely _use_ belong to some other file's pass.

| Leave alone                                             | Why                                                                                                                                                                                        |
| ------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Globals from `ra/externs.h`, names from other headers   | Not declared here; renaming them is a tree-wide job the user did not ask for.                                                                                                              |
| A virtual override whose base is declared elsewhere     | The whole hierarchy has to move together. If the base _is_ in scope, every override in the tree follows.                                                                                   |
| Macros                                                  | Google style keeps macros `ALL_CAPS`. A macro that is really a constant is better turned into a `constexpr kName`, but say so rather than doing it silently.                               |
| Text inside string literals                             | INI keys, file names and scenario codes look like identifiers and are not.                                                                                                                 |
| Tiberian Dawn's copy of an RA function, and the reverse | `src/ra` and `src/td` are separate targets that share many names. Rename within the game the file belongs to. Files under `src/engine/` are shared, so their callers are in all of `src/`. |

**Types declared in scope are renamed in this pass, not a later one.** A legacy `Class` / `Type`
suffix (`FileClass` -> `File`) says nothing, so it goes - but dropping the suffix is only half of
it. Ask the same "what is this?" question as for a function: `GraphicBufferClass` and
`GraphicViewPortClass` became `PixelBuffer` and `PixelView`, which say that one owns the pixels and
the other borrows a clipped rectangle of them, where `GraphicBuffer` and `GraphicView` would only
have dropped the noise. The rename is wide (a few hundred sites is normal) and scripted, so the cost
is in checking, not in typing:

- The new name must be free tree-wide (`grep -rnw '<new>' src/`), and must not be a word the tree
  already uses for something else (`Surface` was taken).
- A type used by both games lives in a shared directory; both targets change together and the commit
  cannot be split by game without breaking the build in between.
- **`.clang-tidy` names types in its check options.** `bugprone-throwing-static-initialization`'s
  `AllowedTypes` listed both of the types above, so renaming them silently un-suppressed two static
  objects in `radar.cc` and the strict build failed in a file the rename had barely touched. Grep
  `.clang-tidy`, `.iwyu_mappings` and `cmake/` for the old type name before believing the rename is
  done. Editing `.clang-tidy` re-analyzes the whole tree on the next strict build, which is slow but
  correct.
- Because the file is named after its type, the type name has to be settled **before** the file name
  (section 5, step 7). Renaming the file first and the type second means renaming the file twice.
- It is still its own commit, made in this pass, before the file rename.

## 2. The scheme

`CLAUDE.md` has the table; the points that come up in practice:

- Functions `PascalCase`, no underscores: `Set_Frame_Timer()` -> `StartFrameTimer()`.
- Accessors and mutators are named like the variable: `count()`, `set_count(int)`.
- Locals, parameters, struct fields `snake_case`; class data members `snake_case_`. A legacy
  `struct` with behaviour or an invariant is a class in waiting (`/restructure-class` section 2): do
  not settle its fields as plain `snake_case`; report it, or leave the fields for that pass.
- Constants, including `static constexpr` and `const` with static storage, `kPascalCase`.
- Enum types `PascalCase`, enumerators `kPascalCase`, always written `Enum::kName`. No
  `SCREAMING_CASE` enumerators and no `using enum` anywhere, header or `.cc`; see "Enums" in
  section 3.
- Legacy statics with a leading underscore (`_ftimer`, `_up`) lose it; they become ordinary
  `snake_case` names, which usually means they need a real name too (`pulse_timer`, `pulse_rising`).
- An abbreviation is a word: `PumpWolapiMessages`, `StartRpc`, not `PumpWOLAPIMessages`.
- A parameter is spelled the same in the declaration, the definition and the comment that mentions
  it. `readability-inconsistent-declaration-parameter-name` is an error in the strict build, so
  renaming a parameter in a `.cc` definition means editing the declaration in **whichever** header
  holds it, even one outside the pass's scope.

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

**Parameters are the names a caller reads, so they get the same care.** They are invisible in
`grep -rnw` counts and cost nothing to change, which is exactly why they get skipped. Read what the
body does with each one:

| Old                                             | New                                          | What made the old one wrong                                                                           |
| ----------------------------------------------- | -------------------------------------------- | ----------------------------------------------------------------------------------------------------- |
| `DrawLine(sx, sy, dx, dy)`                      | `DrawLine(x1, y1, x2, y2)`                   | They are two inclusive corners; `sx`/`dx` read as a source and a delta, which is neither.             |
| `Blit(x_pixel, y_pixel, dx_pixel, ...)`         | `Blit(src_x, src_y, dst_x, dst_y, ...)`      | The two ends of one copy were spelled in different styles, and `_pixel` is the type, not the meaning. |
| `Init(..., int32_t size)`                       | `Init(..., int32_t byte_count)`              | The body already distinguished it from a pixel count; the name did not.                               |
| `DrawScaledRotated(bmp, pt, ...)`               | `DrawScaledRotated(bitmap, center, ...)`     | `pt` is the point the bitmap's centre lands on - the name can say which point it is.                  |
| `Blit(..., bool trans)`                         | `Blit(..., bool transparent)`                | An abbreviation that is not a word.                                                                   |
| `Attach(GraphicBufferClass* graphic_buff, ...)` | `Attach(PixelBuffer* buffer, ...)`           | Repeated the type; the abbreviation bought nothing.                                                   |
| `Print(..., int fcolor, int bcolor)`            | `Print(..., int fore_color, int back_color)` | Same.                                                                                                 |

- Sibling parameters are spelled alike: if one end is `dst_x`, the other is `src_x`, never `x`.
- An unnamed or `/*commented*/` parameter is dead code, not a naming problem; list it for
  `/remove-dead-code`.

**Enums are renamed in full: the type, every enumerator, and every `using enum` goes.** The
`enum class` migration kept the 1996 spellings (`MULTI_AND`, `HOUSE_SPAIN`) and hoisted them back
into the surrounding scope with `using enum X;`. Google style has no room for either: an enumerator
is named like a constant, and the scope is what says which enum it belongs to.

| Old                                               | New                                    |                                                                                         |
| ------------------------------------------------- | -------------------------------------- | --------------------------------------------------------------------------------------- |
| `MultiStyle` / `MULTI_AND`                        | `MultiStyle::kAnd`                     | The prefix repeated the enum; the scope now carries it.                                 |
| `Persistence` / `SEMIPERSISTANT`                  | `Persistence::kSemiPersistent`         | Fix the spelling while there - unless a file format stores the text (check, see below). |
| `TEventType` / `TEVENT_NONE`                      | `TriggerEvent::kNone`                  | The type gets the "what is this?" question too, not just a case change.                 |
| `ThemeType` / `THEME_QUIET`, `THEME_PICK_ANOTHER` | `Theme::kQuiet`, `Theme::kPickAnother` | Negative sentinels are enumerators like the rest.                                       |

- **The type name** follows the rules for types in section 1: `PascalCase`, an abbreviation is a
  word (`RttiType`), and a `Type` suffix goes when the name reads as well without it and the bare
  name is free. Keep it when dropping it would collide with the class the enum classifies
  (`HousesType` next to `HouseClass`): pick the name by what one value _is_ (`HouseId`), or leave
  the suffix and say so.
- **Each enumerator** is `k` + `PascalCase` of what is left after dropping the prefix that repeats
  the enum (`MISSION_`, `STRUCT_`, `TEVENT_`). What is left must still be an identifier and still
  say something (`kNone`, `kAttack`); where it would start with a digit, spell the number
  (`kThreeD`). Keep the game's vocabulary and the unit/building names as they are (`STRUCT_WEAP` ->
  `StructType::kWeap`, not `kWarFactory`): they match the INI names and the original source.
- **Every `using enum` for it is deleted** - the hoist after the definition and each one a `.cc`
  added - and every use is qualified: `Enum::kName` outside, `Class::Enum::kName` for a nested enum
  seen from outside its class, the namespace in front where the user is outside it. Do not replace
  the hoist with another shortcut (a `using` alias of the enumerators, a
  `constexpr auto kAnd = ...`); a long qualified name inside a `switch` is the intended result.
- **Check what turns the enumerator into text or reads it by spelling** before renaming:
  `git grep -n 'enum_name\|enum_cast\|enum_entries' -- src | grep -v DLOG` near the enum, and any
  macro that pastes the prefix (`git grep -n '<PREFIX>_##\|##.*<PREFIX>'`). Debug logs just print
  the new name. Anything that writes the name to an INI, a save or the wire, or parses it back,
  needs an explicit name table first (its own commit) so the stored text stays what it was. The
  `enum_range` specializations, `base::EnumArray` tables and `CNC_FLAG_ENUM` lines only name the
  type and follow the type rename.
- **Values and order do not change.** Saves, INI files and packets store the numbers.
- **Scope** is section 1's: an enum declared under `src/ra` is renamed in `src/ra`, one under
  `src/game` or `src/engine` across all of `src/`. The other game's own enum of the same name is its
  file's pass. A template in `src/game/` that names an enumerator through a game-supplied type
  (`Skin::...`) needs both games to spell it alike: rename both in the one commit, or report it.

Applying it is scripted, like a type rename, and the fan-out is whatever it is - `MissionType` has
thousands of uses and is still done in this pass (over a few hundred sites, split the files among
fork agents with the mapping as the recipe):

1. Build the mapping from the definition itself, one row per enumerator, and put it in the rename
   table. Check each new qualified name against the others (two old names must not collapse into
   one) and the new type name tree-wide.
2. Rewrite already-qualified uses first (`MultiStyle::MULTI_AND` -> `MultiStyle::kAnd`), then the
   bare ones (`MULTI_AND` -> `MultiStyle::kAnd`), on code and comment segments only, never inside
   strings. Inside the enum's own class or namespace the shorter qualification is fine; the script
   may emit the full one and leave shortening to `/fix-ide-issues`' redundant-qualifier rule.
3. Delete the `using enum` lines: `git grep -n 'using enum .*\b<Enum>\b' -- src` must come back
   empty.
4. The legacy prefixes are unique, so `git grep -nw` for each old enumerator must come back empty
   too, strings and the other game's copy aside.
5. One commit per enum (a file's small enums may share one), separate from the identifier rename:
   "Rename `<Enum>`'s enumerators to the Google style".

**Keep the game's own vocabulary.** Techno, house, theater, cell, coord, lepton, facing, mission,
shape, mix, remap are the project's terms of art, and replacing them with generic words makes the
code harder to search and to match against the original source. For any concept, look at what the
neighbouring, already-modernized code calls it and use the same word; consistency beats a slightly
better word used once.

**Do not rename for the sake of it.** If the old name is accurate and only the case is wrong, change
the case and move on.

**The file name is part of the pass.** Decide it last, once the identifiers have their new names,
because the file should be named after what it holds now:

- A file built around one class is named after that class in `snake_case`: `AudioMixer` lives in
  `audio_mixer.h`, `WsaAnimation` in `wsa_animation.h`, `MixArchive` in `mix_archive.h`.
- A file of free functions and tables is named after its subject, in the game's vocabulary, and a
  `.cc`/`.h` pair shares one name.
- Westwood-era names usually fail this: 8.3 truncations (`blwstraw` -> `blowfish_source`, `b64pipe`
  -> `base64_sink`), library prefixes that meant "from the Westwood library" (`ww_`), and names of
  what a file used to hold.
- Keep the name when it already fits, and keep the original's name when it is still accurate: the
  twin in the other game (`td/assets/audio.cc` for `ra/ww_audio.cc`) and the original source
  (`AUDIO.CPP`) are worth matching, because they are what a reader compares against.
- The new name must be free in its directory and should not echo a file in a shared directory that
  holds something else: `ra/audio_mixer.h` next to `engine/audio/audio_mixer.h` would mislead.
- Tests follow their file: `wsa_test.cc` became `wsa_animation_test.cc`.

**So is the folder.** The files were sorted into folders by their old names, in bulk, so a file can
sit where its name pointed rather than where its contents belong. Decide the folder together with
the name, from what the file holds now, against the folder descriptions in `CLAUDE.md`'s
Architecture section:

- Judge by what the code does and what it includes, not by the name or by who calls it. A file whose
  functions build a dialog, draw gadgets and run an input loop is `screens/` or `ui/` wherever it
  sits; a table of type data is `data/`; a file every folder includes for a small helper is `core/`.
- **Between a game's own subfolders** (`ra/ai/` -> `ra/ui/`): move it. Those folders are navigation
  only - same namespace, same library, sources globbed - so the move costs include paths, the guard
  and the test's `CMakeLists.txt` line.
- **Out of the game or across engine libraries** (`ra/` -> `game/sim/`, `engine/gfx/` ->
  `engine/window/`): do not move it in this pass. That changes the namespace, the library and what
  the file may include (`tools/check_layers.py`), and usually needs the twin merged or a dependency
  cut first. Report it as a proposal with what stands in the way.
- A file that holds two things belonging in two folders is a split, not a move: report it.
- The twin in the other game moves to the same folder when it holds the same thing - both games keep
  the same layout - as its own commit, keeping its name. If the twin holds something else, leave it
  and say so.
- Leave the file where it is when the fit is arguable. A move touches every includer; it has to buy
  a reader finding the file where they look first. Put the doubtful ones in the report with the
  alternative.
- The name must be free in the new folder, and name and folder are read together: a move can make
  part of the name redundant or a rename unnecessary.

## 4. Build the rename table first

Before editing, write the table to the scratchpad: every name the files declare, its new name, and
for anything beyond a case change a few words on why. Then, for each row:

- **Blast radius:** `grep -rnw '<old>' <scope>` (scope from section 1). Note a second _definition_
  of the same name on an unrelated class - `Set_Name` exists on files, types, triggers and audio
  classes - because those rows need the compiler-driven method below.
- **Collision:** `grep -rnw '<new>' <scope>`. Look for an existing function, macro or member with
  that name, a local that would now shadow a function or member (`-Wshadow` is an error in the
  strict build), and an accessor that would collide with its own member.
- **Collision with the accessors this same pass creates.** Renaming `Get_Width()` to `width()` and
  then a parameter to `width` is right, but inside a member body an unqualified `width()` call now
  resolves to the parameter: `'width' cannot be used as a function`. It is a compile error, never
  silent, and the fix is to use the member (`width_`) or to name the parameter for its role
  (`dst_width`). The reverse also bites: renaming a parameter to something short like `x` collides
  with the `for (int x = ...)` loop in its own body, which `-Wshadow` rejects - give the loop a name
  that says what it walks (`column`).

Do not stop to get the table approved; the user asked for the rename. Proceed, and put the table in
the final report, where a name they dislike is a one-line fix.

## 5. Apply it

Work from the narrowest scope outwards.

1. **Locals and parameters:** edit inside the one function. A plain English word (`size`, `first`,
   `changed`) must never be replaced file-wide - it will hit comments ("the first frame"), other
   functions' locals and members of unrelated types. A **one- or two-letter** name is worse still:
   `\bh\b` matches the `h` of `"absl/base/attributes.h"` and of `surface->h`, so a scripted `h` ->
   `height` rewrites the include paths to `attributes.height`. Either restrict the pattern to the
   function's text, or require the declaration context (`int h` -> `int height`) and fix the uses by
   hand.
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
   the build (TD's `rawfile.cc` / `cdfile.cc`, the root `vq/` tree). After the build is clean,
   `grep -rnw '<old>'` over the scope must come back empty, or show only the other game's own
   function.
6. **Everything that mentions the old name in prose:** the header's file comment, tests,
   `docs/*.md`, `CLAUDE.md` (its Key Files and examples name real functions), and the TODO items
   that name it (`mcp__clion__todo_list` with `compact: true` and `text: <old name>`; fix them with
   `mcp__clion__todo_update`, or several in one `mcp__clion__todo_batch`).
7. **The file rename and move, last** - after the type rename of section 1, since the file is named
   after its type and doing it the other way round means renaming the file twice. A new name and a
   new folder are one `git mv` to the final path, not two. `git mv` the `.h`, the `.cc` and the
   `_test.cc` together, then:
   - Rewrite the path-qualified includes over the whole scope of section 1 (`#include "ra/ai/old.h"`
     -> `"ra/ui/new.h"`); for a shared directory that is all of `src/`. The shell is zsh, which does
     not word-split `$files`, so pipe the list:
     `git grep -lz '"ra/ai/old.h"' -- src | xargs -0 sed -i 's#"ra/ai/old.h"#"ra/ui/new.h"#'`.
   - Re-sort the include blocks the path change left out of order: a new folder moves the line
     further than a new basename does, and `git clang-format` only sorts within the lines it is
     given. Run it with `-f` on every file whose include changed.
   - Rename the include guard to match the path (`CNC_RED_ALERT_RA_UI_NEW_H_`); `engine_layers_test`
     fails on a guard that does not.
   - After a move, run `ctest --test-dir build -R engine_layers_test`: it checks the guard and that
     the file sits in a folder `tools/engine_layout.py` knows.
   - `git grep` the old basename, with and without extension, and fix what names the file:
     `CMakeLists.txt` files (sources are globbed, but tests, `optimize_in_debug()`, per-platform
     lists and glob exclusions name files), `cmake/`, `.iwyu_mappings`, `CLAUDE.md`, living `docs/`,
     `tools/`, `.claude/commands/` and the TODO items in `.todos/items.json` (their text through
     `mcp__clion__todo_update` or one `mcp__clion__todo_batch`; an item anchored in the file follows
     the move by itself only if the IDE sees it, so re-attach whatever `mcp__clion__todo_list` with
     `lost: true` returns, with `mcp__clion__todo_update` and the new `path`). Leave historical
     records alone - a baseline or a finished plan's log (`docs/MEMBER_INIT_BASELINE.tsv`) describes
     the tree as it was.
   - Grep the old folder-qualified path too (`ai/old`), not only the basename: a move with no rename
     leaves the basename everywhere and only the path stale.
   - Do not rename the other game's twin in the same pass; it gets its own. A twin that moves with
     the file (see "So is the folder") is a separate `git mv` and a separate commit.

This pass changes names only. Unused parameters, a parameter every caller passes the same value for,
dead code and outright bugs will turn up while reading closely; the user nearly always wants them
dealt with next, so list them in the report, but keep them out of this diff so it stays reviewable
as a pure rename. The one exception is a name whose meaning is gone: a `GBC_VIDEOMEM` flag that
nothing reads cannot be given a better name, because there is nothing left for it to mean. Delete
it, in its own commit, and say so.

## 6. Verify

- `git clang-format -f -- <every touched file>`: longer names re-wrap lines. Never
  `clang-format -i`, which reflows the untouched legacy code in the same files.
- `cmake --build build --parallel 22 && ctest --test-dir build`.
- The strict checks on the files you touched: `tools/strict_tu.py <touched files>` when a
  multi-stage run has deferred the full pass, otherwise `cmake --build build-strict --parallel 14`,
  in the foreground as its own command. This is where a new shadowing warning or a missed clang-only
  call site shows up. A file rename makes CMake re-run its globs, which re-populates `_deps` in that
  directory: if the build then reports hundreds of `absl/...` or `gtest/...` "file not found"
  errors, build once more before believing any of them.
- `git grep -n '<old basename>'` comes back empty apart from historical records, and `git status`
  shows the renamed files as `R`, not as a delete and an add.
- Skim `git diff --stat` for files that should not be there, and grep the diff for changes inside
  quotes: `git diff -U0 | grep '^[-+]' | grep '"'`.

Commit only when asked, through `/commit`: one commit per game, a type rename on its own, each enum
on its own, the file rename and move on its own after the identifier rename (a commit that only
moves the file and fixes paths keeps git's rename detection, so `git log --follow` still finds the
history; see `0073bcc3`), and a message that names the renames worth knowing about the way
`615ce9a1` does.

## 7. Report

- The rename table, non-mechanical rows first, each with its reason. The type renames, the enum and
  enumerator renames, the parameter renames and the file rename are rows too, or a line each saying
  why they were left.
- The folder: where the file moved and why, or why it stays; and any move not made - across
  libraries, a split, an arguable fit - with what it would take.
- Names deliberately left alone, and why (section 1, or "already accurate").
- Names you were unsure of, with the alternative you considered.
- What you noticed but did not change: unused or constant parameters, functions doing two jobs, dead
  code, suspected bugs.
