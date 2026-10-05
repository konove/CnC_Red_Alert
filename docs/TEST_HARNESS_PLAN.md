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
| C. Missing game data in fixture | `Text_String()`, fonts, `TUTORIAL.INI`, animation types, TD's INI writers                                    | `GameFixture` builds only the house and unit type heaps        |
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

### 3. `ScriptedPlayer` (new `src/game/testing/`, shared by both games)

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
