# Globals to Owned Subsystems Plan

## Findings

Red Alert declares about 240 mutable globals: 204 in `src/ra/externs.h`, 10 in `src/ra/globals.h`
and 26 in sixteen other headers (`const.h`, `internet.h`, `interpal.h`, `wolapiob.h`, `_wsproto.h`,
`nulldlg.h` and others). Most are defined in the 737-line `src/ra/globals.cc`. Tiberian Dawn has
about 300: 236 in `src/td/externs.h`, 22 in `src/td/globals.h` and 43 elsewhere. The 187
`extern const char* const` strings in `ra/wolstrng.h` are constants and not part of this. The
globals cause three problems:

- **No defined lifetime.** Objects are built in static-init order and destroyed after `main`
  returns, in whatever order the linker picked. Nothing says that the pages exist before
  `InitVideo()` sizes them, or that `Map` outlives the heaps it points into.
- **Tests fake symbols at link time.** `src/ra/intro_test.cc` defines its own `hidden_view`,
  `visible_view`, `CCPalette`, `BlackPalette`, `GamePalette` and `CurrentCD` so the code under test
  links. A test cannot hand the code a different instance, and it breaks when the code starts using
  one more global.
- **`inline` is not an option.** 5a4f0f81 had to keep the video pages `extern`, because an `inline`
  definition emits sdllib's constructors in every unit that includes `globals.h`.

Three more facts shape the plan:

- **A sixth of them need no subsystem.** In Red Alert 8 globals have no uses at all and 32 are used
  by a single `.cc` file: `LogLevel` and its three companions only by `nulldlg.cc`, `ScorePalette`
  and `ScoreObjs` only by `score.cc`, `MenuList` only by `menus.cc`. Tiberian Dawn has about 12
  and 32.
- **`main` does not bracket the game's lifetime.** `SDL_QUIT`, the 20 callers of `EmergencyExit()`
  and both memory-error handlers leave through `ShutDown()` and `exit()`, which never unwinds
  `main`. At the other end, `Parse_Command_Line` runs before the window exists and writes state that
  belongs to most of the subsystems below: `Special`, `Session`, `Whom`, `ScreenHeight`,
  `BreakoutAllowed` and the debug flags.
- **Static initialization hides order dependencies.** Every `Timer<FrameTickSource>` constructor
  reads `Frame` through `FrameTickSource::Tick()`, and `Scen` and `Map` hold such timers. That works
  today only because `Frame` is constant-initialized before any constructor runs. `CCPtr<T>::Heap`
  is bound to the address of a global heap the same way.

**Scope.** Namespace-scope variables only. Red Alert also has about 80 static data members
(`DisplayClass::Layer`, `PaletteClass::CurrentPalette`, `GScreenClass::ShadowPage`) and about 90
file-scope variables in `.cc` files. They stay, except where a phase names one.

**What this plan does not claim.** A subsystem that is reachable through one accessor is still
global state. The gains are an explicit construction and destruction order, state grouped by what it
is for, instances that tests can swap in, and fewer functions that fetch state they could be passed.

## Pattern

Every phase applies the same steps.

1. **File-local first.** A global that only one `.cc` uses moves into that file's anonymous
   namespace, or into the class the file implements. It needs no subsystem and no accessor.
2. **Subsystem class.** Add a class with Google-style members (`hidden_view_`, accessor
   `hidden_view()`) in a new `ra/<subsystem>.h/.cc`, with documentation and unit tests. Its
   constructor does no I/O and needs no window and no game data; an `Init` member does that work
   later. That lets a test build one, and lets `Game` exist before the command line is parsed.
3. **Owner.** Add a per-game `Game` object (`ra/game.h`, `td/game.h`). It holds the subsystems by
   value, declared in dependency order, so they are also destroyed in reverse dependency order.
   `main` creates it on the heap before anything else, and `ShutDown()` destroys it, so every exit
   path tears the subsystems down in the same order. Every caller of `ShutDown()` exits right after
   it, so nothing returns into a destroyed subsystem. `Game` does not live on the stack: it is about
   200 KB (`DoList` alone is 131 KB), and an Emscripten build has a 64 KB stack by default.
4. **Accessor.** Add one shared helper, `base/installed.h`:

   ```cpp
   // T& Installed<T>::Get() CHECKs that an instance is installed.
   // Installed<T>::Scope installs one for its lifetime and restores the previous one.
   template <class T> class Installed;
   ```

   `Game` installs each subsystem as soon as it is built, and each header provides a thin accessor
   such as `Screen& TheScreen();`. A test writes `Installed<Screen>::Scope scope(fake);` instead of
   defining the global itself. `Get()` is inline, a pointer load and a `CHECK`, and is for the main
   thread only: the audio callback and the two SDL timer callbacks touch no game state. A hot loop
   takes the reference once, outside the loop.

5. **Parameters at the leaves.** A function that only reads, or draws into, one piece of state takes
   that state as a parameter (`GraphicViewPortClass&`, `const PaletteClass&`, `const RulesClass&`),
   and its callers pass `TheScreen().hidden_view()`. Deep legacy call chains such as `Map.Draw_It`
   or the dialog loops keep calling the accessor; threading a parameter through them costs more than
   it gains.
6. **Lifetime audit.** Before a global moves, read its constructor and destructor, and those of the
   static objects that use it. `Get()` fails its `CHECK` where the old code silently worked: in a
   static constructor that runs before `Game` exists, in a static destructor that runs after
   `ShutDown()`, and in a member constructor that reaches for the subsystem it is being built into.
7. **Migration.** Rename the uses across the tree, driven by the compiler. Skip strings and
   comments, and keep Red Alert and Tiberian Dawn apart. Run `git clang-format` only on the files
   you touched. For large sweeps, give fork agents disjoint sets of files. Make one commit per
   subsystem per game: Red Alert first, then Tiberian Dawn. A migration commit moves state and
   renames its uses; it changes no behavior.

## Phases

The phases run in order of payoff relative to risk. The use counts are `grep -rnw` hits in `src/ra`,
excluding the declaration and the definition. They include comments, so the counts for short names
(`Map`, `Session`) run high.

### 0. Groundwork

- **Delete the dead globals.** `ModeXBuff`, `Debug_Win`, `Debug_Lose`, `Debug_Remap`,
  `Debug_Find_Path`, `test2` and `test3` have no uses, and `HeapPointers` is declared but never
  defined. Three more are constants in disguise: `SpecialFlag` and `InDebugger` are read once and
  never written, and `GameVersion` is written once and never read.
- **Make the single-file globals file-local** (pattern step 1), one commit per file. To list them,
  count for each `extern` the files that name it, leaving out the header that declares it and
  `globals.cc`. This takes `LogLevel`, `LogLevelTime`, `LogLastTime`, `LogDump_Print`,
  `ScorePalette`, `ScoreObjs`, `MenuList`, `SpeakQueue`, `FrameTimer`, `InterpolationPalette`,
  `InterpolationPaletteChanged`, `PassedProximity`, `PreserveVQAScreen`, `Brokeout`, `NewConfig`,
  `AllDone`, `TheaterData`, `TheaterBuffer`, `ConquerMix`, `SlowKey` and five single-file `Debug_*`
  flags out of the later phases. Two stay for later: `local_rng`, whose one user is the header
  `inline.h`, and the startup options of phase 3.
- **Add `base/installed.h`** with its tests.
- **Add an empty `Game`**, created at the top of `main` and destroyed by `ShutDown()`. Tiberian Dawn
  first needs its exits in one place: 5dc041f8 routed the quit handler, the end of `main` and the
  memory-error exits through `ShutDown()`, but 15 `Prog_End(); exit(...)` pairs remain in `init.cc`,
  `mapedit.cc`, `debug.cc`, `jshell.cc`, `ini.cc`, `scenario.cc` and `saveload.cc`.

### 1. `Screen` (video): the pilot

| Global                           | Uses      | Target                                     |
| -------------------------------- | --------- | ------------------------------------------ |
| `visible_view` / `hidden_view`   | 242 / 154 | `Screen::visible_view()` / `hidden_view()` |
| `visible_page` / `hidden_page`   | 32 / 13   | `Screen::visible_page()` / `hidden_page()` |
| `SysMemPage`, `VQ640`, `IsVQ640` | 8, 4, 8   | members                                    |
| `ScreenWidth` / `ScreenHeight`   | 10 / 19   | `Screen::width()` / `height()`             |

The uses spread over 71 files; `score.cc` (46) and `init.cc` (39) hold the most. `InitVideo()`
becomes `Screen::Init()`. A default-constructed `Screen` has empty pages, and a test attaches its
views to memory buffers the way `intro_test.cc` does today.

Most uses already pass a view to something else (`WWMouse->Erase_Mouse(&hidden_view, true)`,
`hidden_view.Blit(visible_view)`), so they only change spelling. The leaf candidates for a view
parameter are functions that name a view and never touch `LogicPage`; `PlayFirstLaunchIntro()` is
the first, and `intro_test.cc` then drops its two fake views. Its fake palettes go in phase 2 and
its `CurrentCD` in phase 8. `Set_Logic_Page(visible_view)` has 61 call sites and
`LogicPage == &visible_view` has 25, so `Screen` gets a small predicate,
`Screen::IsVisible(const GraphicViewPortClass*)`. `LogicPage` and `Set_Logic_Page` belong to sdllib
and stay unchanged in this phase.

Tiberian Dawn still has the old names. `SeenBuff`, `HidPage`, `VisiblePage` and `HiddenPage` go
straight to the accessors, with no rename in between.

### 2. `Palettes`

This phase covers `GamePalette` (86 uses), `CCPalette` (48), `BlackPalette` (33), `OriginalPalette`
(12), `WhitePalette` (7), `ColorRemaps` (175), `MetalScheme` (16), `GreyScheme` (15),
`InterpolatedPalettes`, `PalettesRead`, `PaletteCounter` and `SlowPalette`. `Palettes` is a member
of `Screen`, or its own subsystem if `Screen` grows too large. The static
`PaletteClass::CurrentPalette` stays where it is.

### 3. Startup options and debug state

`Parse_Command_Line` returns a plain `StartupOptions` struct instead of writing globals, building on
the command-line work in `docs/COMMAND_LINE_PLAN.md`. The struct covers everything the parser writes
today, not just the debug switches. `DebugNewGame`, `DebugLoadGame`, `DebugQuitAtFrame`,
`DebugSaveSlot` and `CustomSeed` exist only to carry a request from the command line, so they become
fields and the globals go. The house choice, the screen height, `BreakoutAllowed`, three `Special`
flags and five `Session` flags seed state that the game changes later; for these the struct holds
the requested value, `main` copies it into the owning subsystem, and the state itself moves in that
subsystem's phase. Nothing writes the struct after the parse.

The flags that change while the game runs become a separate `DebugState` subsystem: `Debug_Unshroud`
(22; the hotkeys, the map editor and `HouseClass` write it), `MapEditorActive` (63), `Debug_Flag`
(15), `Debug_Quiet` (15), `Debug_Print_Events` (13), `Debug_Playtest` and the rest of `Debug_*`.
`SerializeMultiplayer` saves `Debug_Unshroud`, so its position in the archive must not change.

### 4. `Assets`

This phase covers the eleven font spans, `SystemStrings`, `DebugStrings`, `LightningShapes`, the
`*Mix` archives, `TutorialTextData`, `TutorialTextOffsets`, `SpeechBuffer`, `SpeechRecord` and
`CDList`. `init.cc` loads them, and `Text_String` reads them through the accessor.

### 5. `Rules` and type data

State that is built once per process, adjusted by the rules files, and never saved: a loaded game
reads the rules again. `Rule` (470), `RuleINI` (29) and `AftermathINI` (20) become members of one
`Rules` subsystem, together with the tables the rules fill: `Ground` (110), `MissionControl` (52),
`CrateData`, `CrateShares` and `CrateAnims`. The loose tunables that `rules.cc` fills become
`RulesClass` members: `EngineerDamage`, `EngineerCaptureLevel`, `ChronoTankDuration`,
`Quake*Damage`, `QuakeDelay`, `MTankDistance`, `CarrierLaunchDelay`, `UnitBuildPenalty`,
`NewUnitsEnabled`, `SecretUnitsEnabled`, `AntsEnabled`, `bAftermathMultiplayer` and
`bAutoSonarPulse`.

The 12 type heaps (`BuildingTypes`, `UnitTypes`, ...) and `Weapons` and `Warheads` have the same
lifetime and move here too, not into `World`. They cost little: most code reaches a type through
`As_Reference()`, and no heap has more than 15 direct uses. The subsystem binds the matching
`CCPtr<T>::Heap` pointers when it is installed, in place of the static initializers in `globals.cc`.

### 6. `World` (scenario state)

This is the largest phase, and the saved games depend on it. It covers the 17 object heaps
(`Infantry` 205, `Buildings` 193, `Units` 145, ..., including `TeamTypes` and `TriggerTypes`, which
belong to the scenario and are saved), `CurrentObject` (143), `PlayerPtr` (348), `Scen` (559), `Map`
(1056), `Logic`, `Base`, `ChronalVortex`, `Score`, the trigger lists (`LogicTriggers`,
`MapTriggers`, `HouseTriggers`, `MapTriggerID`, `LogicTriggerID`), the team and formation speeds
(`TeamMaxSpeed`, `TeamSpeed`, `FormMove`, `FormSpeed`, `FormMaxSpeed`), `IsTanyaDead`, `SaveTanya`,
`TimeQuake`, `PendingTimeQuake`, `TimeQuakeCenter`, `Carryover`, `ScenarioInit` (142),
`ScenarioCRC`, `BuildLevel`, `Whom`, `CurrentCell`, `LastTheater`, and the scenario loader's
`staging_buffer` and `NewINIFormat`.

- **`Frame` (223) goes first, into its own `GameClock`** declared ahead of `World` in `Game`. The
  frame timers inside `Scen` and `Map` read it while `World` is still being constructed, so it
  cannot be a member of `World`.
- **Heaps.** `World` binds `CCPtr<T>::Heap` for its 17 heaps when it is installed. Every game class
  allocates from its heap in `operator new`, so the heaps stay behind the accessor; `As_Pointer` and
  the per-type loops account for thousands of uses. Per-heap accessors can follow as a sub-phase.
- **Saving.** The save file is a list of tagged sections (`FRAM`, `SCEN`, `MAP_`, `HOUS` ... `VESL`,
  `LOGC`, `TRGV`, `LAYR`, `SCOR`, `BASE`, `CARY`, `MISC` and, for a network save, `MPLY`), and
  `Put_All` services the background tasks between them. `Put_All` and its reading counterpart stay
  the drivers and take a `World&`; `SerializeMisc`, `SerializeTriggerLists` and `SerializeCarryover`
  become `World` members. `SerializeMultiplayer` stays a free function, because `MPLY` mixes state
  from four phases: `Session`, `BuildLevel`, `Debug_Unshroud`, `Seed`, `Whom`, `Special` and
  `Options`. The section order and the field order inside each section do not change.

### 7. Session and network

This phase covers `Session` (2141), `NullModem` (123), `Ipx` (134), `OutList` (107), `DoList` (95),
`pWolapi` (94), `PacketTransport` (44), `ModemRegistry` (32), `ModemRXString` (15),
`CountDownTimer`, `TickCount`, `NewMaxAheadFrame1/2`, `PacketLater`, `Seed`, `FastKey`, `local_rng`,
the `PlanetWestwood*` values, `GameStatisticsPacketSent`, `ConnectionLost` and
`bReconnectDialogCancelled`. The simulation must stay deterministic, so this phase only moves
ownership and changes no update order. `Session` is the largest rename in the plan; split it among
fork agents by file.

### 8. Input, UI and the rest

This phase covers `Keyboard` (176), `WWMouse`, `MouseInstalled`, `Options` (611), `Theme`, `Audio`
(already a class, so only its owner changes; it stays ahead of `Theme`, which plays through it),
`Special` (293), `SpecialDialog`, `AnimControl`, `AllowVoice`, `ScoresPresent`, `GameInFocus`,
`GameActive`, `InMovie`, `bNoMovies`, `BreakoutAllowed`, `PlayerWins`, `PlayerLoses`,
`PlayerRestarts`, `RequiredCD`, `CurrentCD`, `VerNum`, `SoundOn`, `IsTheaterShape` (36), `EngMisStr`
(24), `UnknownKey`, `RedrawOptionsMenu`, `cancel_current_msgbox`, `disable_current_msgbox`,
`bTabKeyPressedHack`, `LParam` and the `NameOverride` / `NameIDOverride` pair.

`WindowList` (94) is a link seam: `sdllib/ww_win.h` declares it, and each game, as well as
`sdllib/keyframe_test.cc`, defines it. sdllib takes over the storage, and each game fills in its
rows at startup.

### 9. Cleanup

This phase deletes what remains of `ra/externs.h` and `ra/globals.cc`, along with any global that
lost its last use along the way. The function declarations at the end of `externs.h` move to the
headers of the files that define them.

**Tiberian Dawn** follows each phase with the same class names under `td/`. A class moves to `tech/`
only where the two games' layouts already match.

## Constraints

- **Save format.** A variable that moves into a class keeps its section and its position in it
  (`Put_All`, `SerializeMisc` and `SerializeMultiplayer` in `src/ra/saveload.cc`).
- **Multiplayer determinism.** Ownership changes only. Nothing changes the order of state updates.
- **`misc-include-cleaner`.** Each `.cc` includes the subsystem header it uses. That also lets units
  drop `externs.h`.
- **Out of scope** unless requested (CLAUDE.md): refactoring the class hierarchy, and converting to
  smart pointers wholesale. Static data members and file-scope variables are out of scope too (see
  Findings).

## Verification (per phase)

- `build/` (`--parallel 22`) and the strict directory (`--parallel 14`) build with no new findings.
- `ctest` passes, including new tests for each subsystem and for `base/installed.h`.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` pass with their fixtures. They
  compare a loaded run against a continuous one, which checks that no state was lost and that the
  simulation still runs the same.
- `tools/ra_saveload_smoke.sh --load-fixture` passes. Only this shows that the save format did not
  change: it loads a save written by an older binary, whereas a round trip passes even when the
  writer and the reader change together. Tiberian Dawn has no such fixture; add one, written by the
  binary from before its phase 6, along with the `--load-fixture` mode.
- An ASan build (`-DENABLE_ASAN=ON`) starts, reaches the menu and exits cleanly three ways: through
  the menu's exit button, by closing the window (`SDL_QUIT`), and with
  `-NEWGAME<scenario> -QUITFRAME<n>`. This is the check on the destruction order.
- After phases 1 and 2, run Red Alert with the game data from the menu into a mission and through a
  movie, and check that it still draws correctly. This needs a real display: under
  `SDL_VIDEODRIVER=dummy` the palettes never load.

## Progress

- 2026-09-19: plan written.
- 2026-09-19: reviewed against the tree. Added phase 0, the lifetime rules (`Game` on the heap and
  destroyed by `ShutDown()`, constructors without I/O, the lifetime audit, `GameClock`), the split
  of startup options from debug state and of type heaps from object heaps, homes for the globals no
  phase named (`Ground`, `MissionControl`, `pWolapi`, `WindowList` and others), and the
  `--load-fixture` and ASan checks. Corrected: `MenuList` is live (`Do_Menu` has four callers),
  there are 31 heaps and not 33, and `session.h` and `display.h` declare no globals.
- 2026-09-19: phase 0 done for both games (90c5aebf..dfb957e3). Many of the single-file globals
  turned out to be write-only or never set once they were looked at in their one file, so they were
  deleted rather than moved: in Red Alert `TheaterBuffer` (a 1.1 MB allocation nothing read),
  `SlowKey`, `ConquerMix`, `NewConfig` with `Read_Private_Config_Struct()`, `PassedProximity`,
  `PreserveVQAScreen`, `Debug_Threat`, `Debug_Trap_Check_Heap` with `Session.TrapCheckHeap`, the
  whole uncalled `Smart_Print`/`Log_*_Time` cluster with its seven globals, and
  `InterpolationPaletteChanged` with the table builder it never triggered; in Tiberian Dawn
  `MPlayerWinner`, `TutorFlags` and `TrapCell`. The moved ones took Google-style names. TD's
  `TrapObject` stays (file-local in `conquer.cc`): it is the sync-bug trap's output, meant for a
  debugger. `base/installed.h` and an empty `Game` per game exist; `main` creates it and
  `ShutDown()` destroys it, and TD's 14 live `Prog_End(); exit()` pairs now call `ShutDown()`. Left
  for later: RA's `PaletteInterpolationTable` is now write-only (phase 2, with
  `InterpolatedPalettes` and `PaletteCounter`); TD's `NewConfig` is write-only too; `ipx95.cc` is
  Windows-only and not built; the early `return`s in both `main`s (bad command line, low disk or
  RAM, no video mode) skip `ShutDown()`, which only matters once `Game` owns something, so phase 1
  must route them through it. The ASan exit check was not run: `Game` owns nothing yet.
- 2026-09-19: phase 1 done for both games (2e6903c7..67d27480), except the check on a real display.
  `Screen` (`ra/screen.h`, `td/screen.h`) replaces the page and view globals and is a member of
  `Game`; `InitVideo()` and TD's inline mode code became `Screen::Init()`. Deviations from the
  table: `ScreenWidth` was never written and `ScreenHeight` meant both the requested mode and, after
  init, the 400-line game area, so the game area is the constants `Screen::kWidth`/`kHeight` and the
  requested mode is `mode_height()`. `IsVQ640` stayed with `Screen` as the table says, though it is
  movie state. The lifetime audit found three things: sdllib's `LogicPage` and `WindowBuffer` would
  dangle once `Game` destroyed the pages (the destructors now reset them),
  `GraphicBufferClass::Un_Init()` was an empty stub so a destroyed visible page leaked its SDL
  texture and surfaces (it now frees them), and TD's static sidebar buttons called
  `Get_Resolution_Factor()`, which read the visible view during static initialization (it now tests
  `Screen::kWidth`). Both `main`s now route their early returns through `ShutDown()`. ASan:
  `-QUITFRAME` (the same `RunGame()` return as the menu's exit button) and `SDL_QUIT` (sent as
  SIGINT) exit cleanly in both games. LeakSanitizer still reports leaks that predate this work and
  belong to later phases: RA's type-class `DimensionData`/`RadarIcon` vectors, which `Prog_End()`
  clears without freeing (phase 5), and TD's `HouseClass` trackers (phase 6). Still to do: run Red
  Alert on a real display into a mission and through a movie.
- 2026-09-19: phase 2 done for both games (0494113d..c5396ae3). `Palettes` (`ra/palettes.h`,
  `td/palettes.h`) is its own subsystem rather than part of `Screen`: palettes are data, not video
  pages, and `intro_test.cc` now installs one instead of defining `BlackPalette`, `CCPalette` and
  `GamePalette`. `CCPalette` and TD's `Palette` are `title_palette()`. Tiberian Dawn has no remap
  tables among its globals, and its palettes are raw 768-byte vectors, so its class differs. Before
  the move, both games' movie interpolation tables (`InterpolatedPalettes`, `PalettesRead`,
  `PaletteCounter`, `PaletteInterpolationTable`, TD's `InterpolationPalette`) were deleted: nothing
  has read the table since the SDL scaler replaced the 2x interpolation, and TD's map screen wrote
  it out as junk `.PAL` files. `PaletteClass`'s fill constructor and `at()` are inline now, so
  `Palettes` needs no `palette.cc`. Found on the way: TD's `Uninit_Game()` freed the MIX archives
  and speech buffer while the audio thread could still be mixing from them (a smoke run crashed in
  `AudioMixer::RefillConverter()`); it now closes the audio first. TD's `Prog_End()` no longer
  clears the title palette, which made `ShutDown()` fail without a `Game`. ASan: `-QUITFRAME` and
  `SDL_QUIT` exit cleanly in both games. One of 22 ASan runs of Red Alert's `-QUITFRAME` stalled
  after `Set_Video_Mode` and hit the timeout; it did not recur in 20 more runs of either build, and
  the first ASan run in phase 1 also hung, so there may be an intermittent startup hang. Still to
  do, as for phase 1: run Red Alert on a real display into a mission and through a movie.
- 2026-09-20: phase 3 done for both games (de4cd0f3..4759aa96). `DebugState` (`ra/debug_state.h`,
  `td/debug_state.h`) holds the switches that change while the game runs -- 11 in Red Alert, 13 in
  Tiberian Dawn -- and `StartupOptions` (`ra/startup_options.h`, `td/startup_options.h`) holds what
  the command line asks for. `Parse_Command_Line` returns `std::optional<StartupOptions>` and writes
  nothing; `main` sets the defaults the parser used to (`Whom`, `Special.Init()`, and Tiberian
  Dawn's `Scenario`, `ScenPlayer` and `ScenDir`), installs the options in `Game`, and hands each
  value to its owner. The switches took names that say what they do: `Debug_Flag` is
  `developer_mode()`, `Debug_Cheat` is `build_anything()`, `Debug_Icon` is `show_cell_info()`,
  `MapEditorActive` and TD's `Debug_Map` are `map_editor_active()`. Deviations from the plan: the
  struct also carries the seven `-*TEST` fixtures, `-WCHAT`, `-MMX` and `-HANSOLO`, which no phase
  had named; `Debug_Unshroud` is written to the recording as well as the multiplayer save, and a
  local `bool` stands in for it in both, so neither archive changed; and `DebugNewGame` and
  `DebugLoadGame` were cleared on use, which a const struct cannot do, so the startup path notes it
  in a file-local `startup_game_started`. Found on the way: the command line lost to the config file
  for everything both could set, because `Parse_Command_Line` ran first and the config read
  overwrote it, so `-480` (both games) and `-O` (Tiberian Dawn) never had any effect; the parse now
  wins. Tiberian Dawn's `Read_Setup_Options` had its own copy of the `-DESTNET` address parser and
  now calls `ParseDestNet`. Its archive round-trip test installs a `DebugState` of its own, because
  `ObjectClass::Select()` reads the switches. Red Alert's commented-out `KN_F4` case in `DebugKeys`
  went with the rename; the live Ctrl-F4 case does the same thing. ASan: `-QUITFRAME` exits cleanly
  in both games, with only the leaks phase 1 recorded (RA's type-class vectors, phase 5; TD's
  `HouseClass` trackers, phase 6), and one Red Alert run hung after `Set_Video_Mode` again. Two
  things are pre-existing, checked against 5e412117 and unchanged by this phase: a mid-game `SIGINT`
  does not end either game, and Tiberian Dawn's `--globals`, `--map`, `--building` and `--mobile`
  smoke fixtures fail their corrupt-save rejection checks. Still to do, as for phases 1 and 2: run
  Red Alert on a real display into a mission and through a movie.
- 2026-09-20: phase 4 done for both games (693c2395..ae16f529). `Assets` (`ra/assets.h`,
  `td/assets.h`) holds the fonts, the string tables, the speech buffers and the handles for the
  archives a disc change re-registers, plus, in Red Alert, the tutorial messages and the
  chronosphere's lightning shapes. Both games' fonts became one `base::EnumArray` keyed by a
  `FontType` enum, so `Init_Fonts()` is a loop over a table of file names and `Fancy_Text_Print()`
  names the font it wants; Tiberian Dawn's table also says whether an archive serves the file,
  because six of its fonts are loose files. Those six used to be read into function-local statics
  that were never freed. Two shapes changed: Red Alert's tutorial messages were a 225-entry uint16
  offset table into one `std::vector<char>` and are an array of `std::string` now, and
  `SpeechBuffer[2]` and `SpeechRecord[2]` were parallel arrays and became one array of `SpeechSlot`.
  The speech buffers are sized by the `Assets` constructor rather than by `Init_Heaps` /
  `Init_Game`; everything else still loads where it did, through `LoadFonts()`, `LoadStrings()` and
  `LoadTutorialText()`. `CDList` was deleted rather than moved (693c2395): `GetCDClass` enumerated
  the machine's CD drives on DOS and Windows and nothing has filled it in since the port, so
  `Get_Number_Of_Drives()` always returned 0, both games scanned an empty drive list in
  `Force_CD_Available()`, and Tiberian Dawn's bootstrap always took the "no CD drives" error exit.
  Red Alert keeps `SearchPaths::SetCdDrive(-1)` as a literal, because `Scan()` only substitutes the
  `"?:\"` placeholder for a non-zero drive. Tiberian Dawn's list tests now install an `Assets`:
  measuring a list item reads the font it is printed in. Verification: both build dirs clean, 681
  tests pass, Red Alert's round trip and `--load-fixture` pass, Tiberian Dawn's fixtures pass their
  comparison (the four corrupt-save rejection checks phase 3 recorded still fail, unchanged), and
  ASan `-QUITFRAME` exits cleanly in both games with only the leaks phases 1 and 3 recorded (RA's
  type-class vectors, phase 5; TD's `HouseClass` trackers, phase 6). Still to do, as for phases 1-3:
  run Red Alert on a real display into a mission and through a movie.
- 2026-09-20: phase 5 done for both games (4a22639a..bde649e6). Red Alert's rules moved in three
  steps. First the twelve loose tunables that only `RulesClass`'s constructor set became members of
  it (`NewUnitsEnabled`, `MTankDistance`, the time quake values, `ChronoTankDuration`,
  `EngineerDamage`, `EngineerCaptureLevel`, `CarrierLaunchDelay`, `UnitBuildPenalty`); their
  definitions in `globals.cc` carried defaults that never took effect, because `Rule` is defined
  later in the same file and its constructor overwrote them all. Then `RulesClass` itself became the
  subsystem -- it also holds `RuleINI`, `AftermathINI`, `Ground`, `MissionControl` and the three
  crate tables -- so `Rule.X` reads `TheRules().X`, and `const.cc`, which held only those tables, is
  gone. Finally the twelve type heaps and the weapon and warhead heaps became `TypeHeaps`
  (`ra/type_heaps.h`), which binds the `CCPtr<T>::Heap` pointers in its constructor; `CCPtr` gained
  `BindHeap()` because `Heap` is private. Deviations: `AntsEnabled`, `bAftermathMultiplayer` and
  `bAutoSonarPulse` stay for phases 6 and 7 -- nothing in `rules.cc` writes them -- and the type
  heaps are their own class rather than part of `RulesClass`, because they need the type headers and
  the rules do not. The lifetime audit found one thing: the static `Scen` seeded its `ShroudTimer`
  from `Rule.ShroudRate` during static initialization, which worked only because `globals.cc`
  happens to define `Rule` first; it starts at zero now, which is what `Clear_Scenario()` sets at
  the start of every scenario anyway. Red Alert's build gained an `ra_engine` object library, like
  Tiberian Dawn's `td_engine`, so `ra_game_test` can link the engine and build a real `Game`;
  `startup.cc` keeps `main()` behind `RA_NO_ENTRY_POINT`. Tiberian Dawn's phase 5 is one line: it
  has no `RulesClass`, no rules files and no type heaps, and its only phase 5 global, `Ground`, is
  never written, so it became `const`. Still open: moving the release of the type objects'
  `DimensionData` and `RadarIcon` from `Prog_End()` into `~TypeHeaps` did not clear the 26 KB
  LeakSanitizer reports. The loop runs with the right counts, and
  `__lsan_do_recoverable_leak_check()` reports nothing leaked at that point, but the vectors'
  capacity is unchanged after the loop assigns an empty vector over them, and the final total is the
  same whether the loop runs or not. A TODO in `type_heaps.cc` records it.
- 2026-09-20: phase 6 done for both games in four steps (460aec01..78aeda09), with the exceptions
  below. `GameClock` (`ra/game_clock.h`, `td/game_clock.h`) holds the frame counter and is the first
  member of `Game`, so it exists before anything that holds a frame-based timer; reads go through
  `CurrentFrame()`, which reports zero when no clock is installed, because timers inside statically
  constructed objects read it while they are being built and the global was zero then too. Then the
  object heaps became `ObjectHeaps` (`ra/object_heaps.h`, `td/object_heaps.h`), binding the
  `CCPtr<T>::Heap` pointers in Red Alert as `TypeHeaps` does; Tiberian Dawn has none to bind,
  because it stores object references as raw pointers. Finally `World` (`ra/world.h`, `td/world.h`)
  took the scenario state: the map, the scenario record, the player's house, the logic layer, the
  base, the score, the selection, the triggers and the loose flags. Red Alert has a `ScenarioClass`
  to hold most of the scenario fields; Tiberian Dawn keeps them loose, as the original did, so its
  `World` holds them directly. Most of `World` hands out references rather than getter and setter
  pairs: this state is read and written all over the simulation. The save format did not change in
  any of the four steps. The lifetime audit found one thing in each game, the same thing:
  `MapEditClass`'s constructor zeroed the home waypoint and the editor's current cell, which it
  could do while those were globals but cannot while `World` is building its own members, so
  `World`'s constructor does it. What made the sweeps slow was name collisions: `MouseClass`,
  `EventClass` and the infantry animation struct all have a `Frame` member, `MapClass` has a
  `Logic()` method, and `ThemeClass`, `SessionClass`, `TEventClass`, `CarryoverClass`,
  `BuildingTypeClass` and `MapEditClass` have members called `Score`, `BuildLevel`, `Infantry`,
  `Aircraft`, `Anims`, `Scenario` and `ScenVar`. The rename script skips comments, strings and
  member access, but an unqualified use inside the owning class looks exactly like the global; each
  one was found by the compiler and put back by hand. Left for later: `Whom` and `BuildLevel` in
  both games, because `EventClass` and `SessionClass` have members of those names and they belong
  with phase 7's session state anyway; Tiberian Dawn's `ScenVar`, because `MapEditClass` has one;
  Red Alert's `AntsEnabled` and `bAutoSonarPulse`, noted in phase 5; and `staging_buffer`, which is
  scratch space rather than state. Red Alert's `externs.h` is down from 204 declarations to 39.
  Verification: both build dirs clean, 681 tests pass, all save/load smoke fixtures pass in both
  games including Red Alert's `--load-fixture`, and ASan `-QUITFRAME` exits cleanly with no errors
  in either game -- only the leak the phase 5 TODO already records, which grew from 26 KB to 29 KB
  in Red Alert because more type objects fill their `DimensionData` cache before the exit.
