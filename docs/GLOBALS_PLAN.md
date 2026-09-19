# Globals to Owned Subsystems Plan

## Findings

Red Alert declares about 210 globals: 200 in `src/ra/externs.h`, 10 in `src/ra/globals.h` and a few
more in `session.h`, `display.h` and other headers. Most are defined in the 737-line
`src/ra/globals.cc`. Tiberian Dawn has about 240 in `src/td/externs.h` and 22 in `src/td/globals.h`.
This causes three problems:

- **No defined lifetime.** Objects are built in static-init order and destroyed after `main`
  returns, in whatever order the linker picked. Nothing says that the pages exist before
  `InitVideo()` sizes them, or that `Map` outlives the heaps it points into.
- **Tests fake symbols at link time.** `src/ra/intro_test.cc` defines its own `hidden_view`,
  `visible_view`, `CCPalette`, `BlackPalette`, `GamePalette` and `CurrentCD` so the code under test
  links. A test cannot hand the code a different instance, and it breaks when the code starts using
  one more global.
- **`inline` is not an option.** 5a4f0f81 had to keep the video pages `extern`, because an `inline`
  definition emits sdllib's constructors in every unit that includes `globals.h`.

**What this plan does not claim.** A subsystem that is reachable through one accessor is still
global state. The gains are an explicit construction and destruction order, state grouped by what it
is for, instances that tests can swap in, and fewer functions that fetch state they could be passed.

## Pattern

Every phase applies the same five steps.

1. **Subsystem class.** Add a class with Google-style members (`hidden_view_`, accessor
   `hidden_view()`) in a new `ra/<subsystem>.h/.cc`, with documentation and unit tests.
2. **Owner.** Add a per-game `Game` object (`ra/game.h`, `td/game.h`) that `main` constructs. It
   holds the subsystems by value, declared in dependency order, so they are also destroyed in
   reverse dependency order.
3. **Accessor.** Add one shared helper, `base/installed.h`:

   ```cpp
   // T& Installed<T>::Get() CHECKs that an instance is installed.
   // Installed<T>::Scope installs one for its lifetime and restores the previous one.
   template <class T> class Installed;
   ```

   `Game` installs its subsystems, and each header provides a thin accessor such as
   `Screen& TheScreen();`. A test writes `Installed<Screen>::Scope scope(fake);` instead of defining
   the global itself.

4. **Parameters at the leaves.** A function that only reads, or draws into, one piece of state takes
   that state as a parameter (`GraphicViewPortClass&`, `const PaletteClass&`, `const RulesClass&`),
   and its callers pass `TheScreen().hidden_view()`. Deep legacy call chains such as `Map.Draw_It`
   or the dialog loops keep calling the accessor; threading a parameter through them costs more than
   it gains.
5. **Migration.** Rename the uses across the tree, driven by the compiler. Skip strings and
   comments, and keep Red Alert and Tiberian Dawn apart. Run `git clang-format` only on the files
   you touched. For large sweeps, give fork agents disjoint sets of files. Make one commit per
   subsystem per game: Red Alert first, then Tiberian Dawn.

## Phases

The phases run in order of payoff relative to risk. The use counts are `grep -rnw` hits in `src/ra`,
excluding the declaration and the definition. They include comments, so the counts for short names
(`Map`, `Session`) run high.

### 1. `Screen` (video): the pilot

| Global                           | Uses      | Target                                     |
| -------------------------------- | --------- | ------------------------------------------ |
| `visible_view` / `hidden_view`   | 242 / 154 | `Screen::visible_view()` / `hidden_view()` |
| `visible_page` / `hidden_page`   | 32 / 13   | `Screen::visible_page()` / `hidden_page()` |
| `SysMemPage`, `VQ640`, `IsVQ640` | 8, 4, 8   | members                                    |
| `ScreenWidth` / `ScreenHeight`   | 10 / 19   | `Screen::width()` / `height()`             |
| `ModeXBuff`                      | 0         | delete (dead)                              |

These leaf helpers take a view parameter: `Format_Window_String`, `Interpolate_2X_Scale`,
`WWMouse->Erase_Mouse` / `Draw_Mouse`, and the gadget draw helpers. `Set_Logic_Page(visible_view)`
has 59 call sites and `LogicPage == &visible_view` has 24, so `Screen` gets a small predicate,
`Screen::IsVisible(const GraphicViewPortClass*)`. `LogicPage` and `Set_Logic_Page` belong to sdllib
and stay unchanged in this phase. The phase also retires the fakes in `intro_test.cc`.

### 2. `Palettes`

This phase covers `GamePalette` (86 uses), `CCPalette` (48), `BlackPalette` (33), `OriginalPalette`
(12), `ScorePalette` (10), `WhitePalette` (7), `ColorRemaps` (175), `MetalScheme` (16), `GreyScheme`
(15), `InterpolatedPalettes`, `PalettesRead`, `PaletteCounter` and `SlowPalette`. `Palettes` is a
member of `Screen`, or its own subsystem if `Screen` grows too large.

### 3. `DebugOptions`

This phase covers the `Debug_*` flags (`Debug_Unshroud` 22, `Debug_Quiet` 15, `Debug_Flag` 15,
`Debug_Print_Events` 13 and the rest), `DebugNewGame`, `DebugLoadGame`, `DebugQuitAtFrame`,
`DebugSaveSlot`, `MapEditorActive` (63), `LogLevel*`, `LogLastTime` and `LogDump_Print`.
`Parse_Command_Line` returns the struct instead of writing globals, building on the command-line
work in `docs/COMMAND_LINE_PLAN.md`. `SerializeMultiplayer` saves `Debug_Unshroud`, so its position
in the archive must not change.

The following have no uses outside their definition and are deleted: `Debug_Win`, `Debug_Lose`,
`Debug_Remap` and `Debug_Find_Path`.

### 4. `Assets`

This phase covers the eleven font spans, `SystemStrings`, `DebugStrings`, `LightningShapes`, the
`*Mix` archives, `TheaterData`, `TheaterBuffer`, `TutorialTextData`, `TutorialTextOffsets`,
`SpeechBuffer`, `SpeechRecord` and `CDList`. `init.cc` loads them, and `Text_String` reads them
through the accessor.

### 5. Rules

`RuleINI` (29), `AftermathINI` (20) and `Rule` (470) stay together. The loose tunables that
`rules.cc` fills become `RulesClass` members: `EngineerDamage`, `EngineerCaptureLevel`,
`ChronoTankDuration`, `Quake*Damage`, `QuakeDelay`, `MTankDistance`, `CarrierLaunchDelay`,
`UnitBuildPenalty`, `NewUnitsEnabled`, `SecretUnitsEnabled`, `AntsEnabled`, `bAftermathMultiplayer`
and `bAutoSonarPulse`.

### 6. `World` (scenario state)

This is the largest phase, and the saved games depend on it. It covers the 33 `TFixedIHeapClass`
heaps (`Infantry` 204, `Buildings` 193, `Units` 145, ...), `CurrentObject` (143), `PlayerPtr` (348),
`Scen` (559), `Map` (1054), `Logic`, `Base`, `ChronalVortex`, the trigger lists (`LogicTriggers`,
`MapTriggers`, `HouseTriggers`, `MapTriggerID`, `LogicTriggerID`), the team and formation speeds
(`TeamMaxSpeed`, `TeamSpeed`, `FormMove`, `FormSpeed`, `FormMaxSpeed`), `IsTanyaDead`, `SaveTanya`,
`TimeQuake`, `PendingTimeQuake`, `TimeQuakeCenter`, `Carryover`, `ScenarioInit` (142),
`ScenarioCRC`, `BuildLevel`, `Whom`, `CurrentCell` and `LastTheater`.

`SerializeMisc` and the heap save code move into `World::Serialize`, with the archive order
unchanged. The heaps stay behind the accessor, because `As_Pointer` and the per-type loops account
for thousands of uses. Per-heap accessors can follow as a sub-phase. `HeapPointers` is declared but
never defined or used, so it is deleted.

### 7. Session and network

This phase covers `Session` (2141), `NullModem` (123), `Ipx` (134), `OutList` (107), `DoList` (95),
`FrameTimer`, `CountDownTimer`, `TickCount`, `NewMaxAheadFrame1/2`, `PacketLater`, `Seed`,
`CustomSeed`, `FastKey`, `SlowKey` and `local_rng`. The simulation must stay deterministic, so this
phase only moves ownership and changes no update order.

### 8. Input, UI and the rest

This phase covers `Keyboard` (176), `WWMouse`, `MouseInstalled`, `Options` (608), `Theme`, `Audio`
(already a class, so only its owner changes), `Score`, `Special` (292), `SpecialDialog`,
`AnimControl`, `SpeakQueue`, `AllowVoice`, `GameInFocus`, `GameActive`, `InMovie`, `AllDone`,
`BreakoutAllowed`, `Brokeout`, `PlayerWins`, `PlayerLoses`, `PlayerRestarts`, `RequiredCD`,
`CurrentCD`, `VerNum`, `Frame`, `SoundOn` and the `NameOverride` / `NameIDOverride` pair.

### 9. Cleanup

This phase deletes the code that no longer has uses: `test2` and `test3`, which have no uses at all,
`MenuList` and the other dead globals found along the way, then what remains of `ra/externs.h` and
`ra/globals.cc`.

**Tiberian Dawn** follows each phase with the same class names under `td/`. A class moves to `tech/`
only where the two games' layouts already match.

## Constraints

- **Save format.** A variable that moves into a class keeps its position in the archive
  (`src/ra/saveload.cc`, `SerializeMisc` and `SerializeMultiplayer`).
- **Multiplayer determinism.** Ownership changes only. Nothing changes the order of state updates.
- **`misc-include-cleaner`.** Each `.cc` includes the subsystem header it uses. That also lets units
  drop `externs.h`.
- **Out of scope** unless requested (CLAUDE.md): refactoring the class hierarchy, and converting to
  smart pointers wholesale.

## Verification (per phase)

- `build/` (`--parallel 22`) and the strict directory (`--parallel 14`) build with no new findings.
- `ctest` passes, including new tests for each subsystem and for `base/installed.h`.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` pass with their fixtures, which
  shows that the save format did not change.
- After phase 1, run Red Alert with the game data from the menu into a mission and through a movie,
  and check that it still draws correctly.

## Progress

- 2026-09-19: plan written. No phase has started.
