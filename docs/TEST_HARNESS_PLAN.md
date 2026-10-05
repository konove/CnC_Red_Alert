# Test Harness Plan

Design for making the code that needs a screen, input, real time or game data testable. Approved
2026-10-05. The step-by-step implementation plan follows in a later commit.

## Goal

Every `TODO.md` item that says "needs the screen / input / real time / game data" or "check on a
real display" becomes either an ordinary gtest or one scripted command. No production behaviour
changes.

## The blocked items

| Group                           | TODO examples                                                                                                | What blocks it                                                 |
| ------------------------------- | ------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------- |
| A. Modal loops                  | `ra/ai/special_options.cc` dialogs, `ra/editor/trigger_type_dialog.cc`, `ra/screens/credits_roll.cc`         | Screen, input queue and real time inside a `while` loop        |
| B. "Check on a real display"    | TextPrintType split, keyboard and mouse refactors, fading tables, tactical redraw, page owners, VQA          | Nobody can press keys or look at pixels headlessly             |
| C. Missing game data in fixture | `Text_String()`, fonts, `TUTORIAL.INI`, a nimation types, TD's INI writers                                   | `GameFixture` builds only the house and unit type heaps        |
| D. Two-instance network         | Coordinate-type packets, the `rules.ini` ID in a lobby                                                       | Nothing exchanges packets                                      |
| E. Headless hang                | SCG03EA, SCG03EB, SCG07EA, SCG10EB, SCG13EA and all 24 SCM maps never reach frame 1 under the `dummy` driver | A bug, not a harness gap, but it limits any whole-game harness |

## What exists today

- `Game` (`ra/startup/game.h`, `td/startup/game.h`) is test-constructible and owns `Input`,
  `Display`, `Screen`, `AudioMixer` and the other subsystems, so modal code already resolves its
  `The*()` accessors in a test. `ra::GameFixture` (`ra/startup/game_test_util.h`) builds one.
- Modal loops hang in a test for two reasons: `Display::EndFrame()` does nothing before `Init()`, so
  nothing pumps input, and `SystemTicks()` reads `steady_clock` directly, so waits take real time.
- `DisplayClient::Pump()` runs once per event pass, and `KeyBuffer::PutKey()`/`PutClick()` inject
  input without SDL.
- Screenshots (`Screenshot_Key()`, PCX), `-NEWGAME`/`-LOADGAME`/`-QUITFRAME`, and the SDL
  `offscreen` driver, which renders real frames without a window.

## Decisions

- Two harnesses in one plan, the in-process one first: it runs in CI and counts in
  `tools/coverage.py`; the whole-game one needs the Steam data and stays a local tool.
- In-process seam: a frame hook and a fake clock. Rejected: real SDL under the `dummy` driver
  (process-global state, real-time waits, vsync) and rewriting every modal loop into a `Step()`
  state machine (dozens of behaviour-risking rewrites). Pure parts are split out only where
  `TODO.md` already names them (`DifficultyPrompt()`, the credits timetable).
- Screenshot goldens: per-shot hashes are committed; the images, which hold Westwood art, stay in a
  git-ignored directory.

## Components

### 1. Fake clock (`engine/platform/timer.h`)

- `SystemTicks()` and `SystemMilliseconds()` read through an installable override; with none
  installed they use `steady_clock` as today.
- `ManualClock`, a test type, installs itself for its scope and offers `Advance(ticks)`.

### 2. Headless `Display` (`engine/window/display.h`)

- `InitHeadless(width, height)` backs `Lock()` with an in-memory 8-bit buffer: no SDL window,
  renderer, vsync or redraw timer.
- `EndFrame()` in this mode calls `client_->Pump()`, then an optional frame observer. It never calls
  `SDL_PollEvent`.
- It keeps the last palette and frame, so a test or a screenshot can read what was presented.

### 3. `ScriptedPlayer` (`engine/window/scripted_player.h`, shared by both games)

- A list of steps keyed by present count: key press and release, mouse move and click, screenshot,
  and a callback for assertions in the middle of a loop.
- Installed as the frame observer. On each present it injects that frame's steps into
  `TheKeyboard()` and advances the `ManualClock` one tick.
- Runaway guard: when the script is exhausted and a frame budget passes, it throws `ScriptOverrun`,
  which the fixture reports as a test failure. A dialog that never closes fails in milliseconds
  instead of at a ctest timeout.

### 4. `UiFixture` (one per game, on top of `GameFixture`)

- A headless display with the `Screen` pages attached, a `ManualClock`, a `ScriptedPlayer`, and the
  audio left closed (`ServiceBackgroundTasks()` already skips a closed mixer).
- Small in-memory assets, added as tests need them: a text table, a synthetic font, identity
  palettes and remaps. This closes group C.

```cpp
TEST_F(UiFixture, EscClosesSpecialOptionsWithoutApplying) {
  player().At(2).Click(three_point_box).At(4).Key(KN_ESC);
  RunSpecialOptionsDialog();
  EXPECT_FALSE(TheSpecialOptions().three_point_turns);
}
```

### 5. Whole-game scripted runs

- A `-SCRIPT<file>` switch in both games loads the same step list from text and runs it on the real
  `Display` under `SDL_VIDEODRIVER=offscreen`, with the fake clock ticking once per present, so runs
  are fast and repeatable:

  ```
  at 12 key ESC
  at 30 click 320 200
  at 31 shot main_menu
  ```

- `tools/ui_check.py <script>` runs the binary, hashes each shot and compares it with the committed
  manifest `src/<game>/testdata/ui/<script>.hashes`. On a mismatch it writes the PNGs to the
  git-ignored `build/ui_check/` for review; `--accept` updates the manifest.
- It needs the Steam data, so it is a local tool like `tools/ra_saveload_smoke.sh`, not a ctest.

## Phases

| #   | Delivers                                                                                             | TODO items it closes or unblocks                                                           |
| --- | ---------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------ |
| 1   | Fake clock, headless `Display`, `ScriptedPlayer`, RA `UiFixture`; first test is `special_options.cc` | The special options dialogs                                                                |
| 2   | In-memory text table, font and palettes in the fixture                                               | `trigger_type.cc` `Description()` and `Draw_It()`, `trigger_action.cc` `TextAction`        |
| 3   | Tests for the remaining RA modal screens: credits roll, trigger type dialog, editor lists            | Credits roll, `trigger_type_dialog.cc`, the editor-list check of `CCPTR_REMOVAL_PLAN.md`   |
| 4   | TD `UiFixture`; TD editor INI save round trip and options dialog                                     | The two TD hand checks of `INI_SHARING_PLAN.md`                                            |
| 5   | `-SCRIPT`, `tools/ui_check.py`, first scripts: menus, options, score, map editor dialogs             | The TextPrintType, keyboard and mouse, fading table, tactical redraw and page owner checks |
| 6   | The headless frame-1 hang fixed, then a two-process scripted network game                            | The coordinate-type packet check and the `rules.ini` ID lobby check                        |

## Progress

- **Phase 1: done 2026-10-05.** `engine::platform::ManualClock` (`engine/platform/manual_clock.h`),
  `Display::SetHeadlessVideoMode()` and `DisplayObserver` (`engine/window/display_observer.h`),
  `ScriptedPlayer`, `Screen::InitHeadless()` and `ra::UiFixture` (`ra/startup/ui_test_util.h`).
  `RunSpecialOptionsDialog()` is covered by seven scripted tests in `ra/ai/special_options_test.cc`.
  What differs from the design above:
  - `ScriptedPlayer` needs nothing above the engine, so it lives in `engine/window/`, not in a new
    `src/game/testing/`.
  - The frame hook is an observer on `Display` with two calls, `OnFrameEnd()` and `OnPump()`. The
    second lets the player cut short a loop that polls the keyboard without ever presenting, which
    would otherwise hang instead of failing.
  - A headless `Display` does not keep scaled frames or backdrops; presenting one only ends the
    frame. Phase 5 adds that when screenshots need it.
  - The synthetic font moved up from phase 2: without a font a text button is 8x2 pixels and cannot
    be clicked. `MakeBlockFont()` (`engine/gfx/font_test_util.h`) makes one, and `UiFixture`
    installs it for every `FontType`.
  - `PromptForDifficulty()` is not covered yet: it copies `TextString(TXT_DIFFICULTY)`, which is
    null without a string table. Phase 2's text table opens it.

- **Phase 2: done 2026-10-05.** `MakeStringTable()` (`engine/file/string_table_test_util.h`),
  `Assets::set_system_strings()` and `set_tutorial_text()`, and `GameFixture::SetText()`, which
  installs a text table on first use. `UiFixture` installs it up front, so every string is empty
  rather than null. Newly covered: `PromptForDifficulty()` (seven tests, including the slider's
  ends), the object names in `TriggerType::Description()` and `TextAction` with a tutorial message.
  `special_options.cc` went from 20 to 137 of 141 lines.
  - Palettes and colour remaps were not needed by anything tested so far; they are added with the
    first screen that reads them.

- **Phase 3, credits roll: done 2026-10-05.** `ra/screens/credits_roll.cc` went from 0 to 217 of 224
  lines through 19 tests in `ra/screens/credits_roll_test.cc`, with no change to the file itself;
  the whole roll takes about 10 ms. What it needed from the harness:
  - `SleepMs()` moves a `ManualClock` instead of sleeping. The credits wait for each frame's
    deadline in a loop that sleeps and polls the keyboard without presenting, which is the case the
    Risks section predicted. The clock now counts 1/3000 s so that ticks and milliseconds mix
    without drift.
  - `GameFixture::WriteGameFile()` and `GameFilePath()` put loose files on the search path
    (`credits.txt`, `EGOPAL.PAL`, slides written with `Write_PCX_File()`).
  - `UiFixture` gives every colour scheme one palette index of its own, `SchemeColor()`, so text can
    be found on the page.

- **Phase 3, editor dialogs: done 2026-10-05.** `ra/editor/trigger_type_dialog.cc` went from 0 to
  1,184 of 1,239 lines (`trigger_type_dialog_test.cc`: a round trip of every event and action type
  through the dialog, the two style buttons, typing a name, OK, Cancel). The trigger and team lists
  of `MapEditClass` are covered by `map_editor_lists_test.cc`, which closes the last manual check of
  `docs/CCPTR_REMOVAL_PLAN.md`.
  - `ScriptedPlayer::Type()` types text, and `Key()` gives Return, Backspace, Tab, Esc and Space
    their character. Without SDL's keyboard there is no layout for `KeyBuffer::ToAscii()` to look a
    key up in, so the character has to travel with the event, as SDL's text input delivers it.
  - Not done: clicking a drop list open. The drop buttons take their size from `EBTN-UP.SHP`, which
    the fixture does not have; a synthetic shape file would open them.

- **Phase 4: done 2026-10-05.** Tiberian Dawn has the same two fixtures, `td::GameFixture` and
  `td::UiFixture` (`td/startup/game_test_util.h`, `ui_test_util.h`), with `Screen::InitHeadless()`
  and `Assets::set_system_strings()`. The loose-file helper both games' fixtures use is
  `engine::file::TestGameFiles` (`engine/file/game_files_test_util.h`).
  - `td/screens/options_dialogs_test.cc`: saving the options keeps the sections and keys of
    `CONQUER.INI` the game does not write, and the visual controls dialog is played from the
    keyboard.
  - `td/data/scenario_objects_ini_test.cc`: a vehicle, a soldier, a building, an aircraft and the
    houses are read from INI text and written back. It found that an aircraft's mission is always
    saved as Move (`TODO.md`).
  - Not done: a whole shipped scenario through the editor's Save. That needs the game data and
    belongs to the scripted runs of phase 5.

- **Phase 5: done 2026-10-05.** Both games take `-SCRIPT<file>` (and `-EDITOR`, the scenario editor
  without its pass phrase, in a build that has one). `tools/ui_check.py` runs the scripts in
  `src/<game>/testdata/ui/` under the offscreen driver and compares each screenshot's SHA-256 with
  the `.hashes` file beside the script. Eight scripts, 37 screenshots, about ten seconds for all;
  three runs in a row gave the same hashes.
  - The script format is `engine/window/player_script.h`; `engine::window::ScriptedRun` holds the
    clock and the player for a game.
  - What a whole game needed that one dialog did not: frames ended for loops that only poll the
    keyboard (`set_idle_pumps_per_frame()`); held mouse buttons (`IsLeftButtonDown()` read SDL's
    live state, so a drag selected nothing); no redraw timer and no frame pacing in the display; no
    sound, which plays in real time; and silent movies paced by the game's clock (`MovieClock`).
  - Two bugs found on the way. `AudioMixer::Play()` on a mixer that was never opened left the sound
    "playing" for good, so Tiberian Dawn's side choice, which waits for a voice to end, hung on a
    machine without a sound device; it now returns -1. And the map editor's pop-up menus draw their
    text at the screen's corner, away from their box, which `TODO.md` already had as the
    `WinX`/`WinY` item; the screenshot shows it.
  - Not scripted: the score screens and TD's map selection (they need a mission won), and anything
    with Shift, Ctrl or Alt held.
  - A screenshot holds the game's page, not the movie frames or backdrops the display stretches
    around it.
- **After phase 5: the seams are interfaces, done 2026-10-05.** The harness first reached into
  production code through checks: is a manual clock installed, is the display headless, are scripted
  mouse buttons installed. Each is now an interface with the real thing as one implementation and
  the test's as another, and the production functions have no branch:
  - `engine::platform::Clock` (`clock.h`): the machine's clock and `ManualClock`. `SystemTicks()`,
    `SystemMilliseconds()` and `SleepMs()` call `CurrentClock()`.
  - `engine::window::Display` (`display.h`) is abstract. `WindowDisplay` is the program's window;
    `ScriptedWindowDisplay` overrides how it schedules redraws and paces presents, which is all that
    differs in a scripted run; `HeadlessDisplay` is the surface alone. `Game` takes a `DisplayKind`:
    `main()` asks for the scripted window when `-SCRIPT` is given, the fixtures for the headless
    one. `Screen::InitHeadless()` is gone: `Screen::Init()` works on any of them.
  - `engine::window::MouseButtons` (`hardware_cursor.h`): SDL's live state, or the buttons a
    `ScriptedPlayer` holds.
  - Where the Components section above says "installable override" and "`InitHeadless()`", read
    these.

## Out of scope

- Audio output assertions, beyond the accessor for `ThemeClass::Pending` that `TODO.md` already asks
  for.
- The hardware cursor's look at 1x, 2x and 4x: SDL draws it, not the page, so it stays a manual
  check.
- Rewriting modal loops into state machines, beyond the pure parts `TODO.md` already names.

## Risks

- Phase 5 hashes match exactly, so every intended pixel change means re-accepting shots.
- Phase 6 depends on a bug of unknown size and may become its own plan.
- A modal loop may wait on `SystemTicks()` alone, without calling `ServiceRealTime()`. Such a loop
  needs the clock to advance on read; add that only if phase 3 finds one.
