# Game namespaces plan

Decided 2026-09-30. Continues `docs/ENGINE_NAMESPACES_PLAN.md`, whose top-level namespaces were
`base`, `engine`, "and later `ra` and `td`". This plan puts the two games in `ra::` and `td::`,
splits their flat folders into subfolders, and moves the code both games duplicate into one shared
copy.

Measured on `main` at f46f7f39 with `diff -wB` per same-named file pair plus a line-similarity scan
(stripped lines, `difflib.SequenceMatcher`), and `nm` of the `ra_engine`/`td_engine` objects in
`build/`.

## Decisions

- **Shared game code lives in `src/game/`, namespace `game::`,** a layer between the engine and the
  games: `base -> engine -> game -> {ra, td}`. The engine stays free of C&C concepts (cells, houses,
  gadgets); code that needs none of them goes into the engine library it belongs to instead.
- **`game::` is flat**, one CMake library per folder (`game_core`, `game_sim`, `game_ui`), the way
  the games are flat. Callers qualify (`game::GadgetClass`) or use a using-declaration in a `.cc`.
- **`ra::` and `td::` are flat.** Subfolders (`ra/net/`, `ra/ui/`, ...) are for navigation and
  layering only; the style guide does not require namespaces to mirror folders, and the legacy
  classes are too intertwined for per-folder namespaces to pay off. Both games use the same folder
  names. No game folder or namespace is named `base`, `engine` or `game`.
- **Diverging copies unify on the better one,** usually RA's (it carries most of the bug fixes). Any
  difference that changes the simulation, a save file or a network packet is not unified: it becomes
  an explicit parameter (traits, template argument, constant passed in) so each game keeps its exact
  bytes and lockstep results.

## Findings

### Duplication

290 `.cc`/`.h` names exist in both `src/ra/` and `src/td/` (151 RA-only, 48 TD-only). About 90 pairs
are at least 80% identical. They fall into three clusters; the rest are game logic that only looks
alike (`techno`, `house`, `building`, `unit`: 40-55%) and stays per game.

**Network core (~5,000 lines, to `engine::net`).** None of it needs a game concept once the
transport is injected.

| Pair                            | Difference                                                           | Verdict                                            |
| ------------------------------- | -------------------------------------------------------------------- | -------------------------------------------------- |
| connmgr                         | comments                                                             | share                                              |
| combuf                          | RA adds ExtraBuffer/ExtraLen (`ra/combuf.h:78,91`)                   | share RA's (extra length defaults to 0)            |
| connect + `td/noseqcon`         | RA folds TD's `NonSequencedConnClass` into one class; ACK logic same | share RA's, delete noseqcon; merge both test files |
| ipxaddr                         | TD has `Set_Address(IPXHeaderType*)`, RA has `Serialize`             | share the union                                    |
| ipxconn, ipxmgr                 | transport: RA UDP via `packet_transport()`, TD TCP via `winsock()`   | share behind a `DatagramTransport` interface       |
| ipxgconn                        | wire format (below)                                                  | share with a header/ACK policy, or keep per game   |
| nullconn                        | TD's `Send` also writes TCP (`td/nullconn.cc:211-221`)               | share with an injected writer                      |
| nullmgr                         | protocol half (RA :214-1006) cosmetic; modem-dial half is game UI    | share the protocol half, keep the dial UI per game |
| wsproto/wspudp (RA), tcpip (TD) | different transports                                                 | both become `DatagramTransport` implementations    |

Game dependencies to cut: `TheSession().Type == GAME_INTERNET` in ipxmgr (an `Init(bool internet)`
parameter); RA ipxmgr's unknown-sender fix-up (`ra/ipxmgr.cc:969-1010`, decodes FRAMESYNC events and
player addresses: a callback the game provides); `TheKeyboard().Peek()` in the transports (message
pumping only); `westwood_port()` (a parameter). `network.h`, `session.h`, `event.*`, `netdlg`,
`nulldlg` and every packet payload stay per game: the dialogs switch over `NET_*` commands inside
their UI loops.

**Widgets (~4,500 lines, to `game_ui`).** No widget touches Map, Scenario, Session, the pages or
`Sound_Effect`; they draw into the `PixelView&` they are given.

- Identical modulo cosmetics: `input.*` (wraps `engine::window`), `link.*` (take TD's const
  `Head_Of`), `control.*` (peer notified before the flags in TD, `td/control.cc:109`, after in RA,
  `ra/control.cc:122`: pick one).
- Differ only in colour, box style and font: `gadget` (RA is a superset: `ColorScheme`,
  `Set_Position`, destructor clearing `StuckOn`/`Focused`/`LastList`), `toggle` (RA's hit-test fix,
  `ra/toggle.cc:139-183`), `gauge`/`slider` (TD's `Thumb_Pixels` 8 looks like a bug against the
  4-pixel thumb it draws, `td/gauge.h:60`), `textbtn`, `shapebtn`, `txtlabel`, `checkbox`, `edit`,
  `list` (RA adds clamp fixes, `ra/list.cc:132,538`, and `TListClass`), `colrlist`, `cheklist` (RA
  keeps state in a vector, TD in the item's first character), `direction_dial`/`dial8`.
- The structural difference is colour: RA passes `RemapControlType*`, TD an `int` palette index and
  branches on `TPF_6PT_GRAD` as a bitmask, so 8-point, 3-point and LED fonts also take TD's "green"
  branch (`td/textbtn.cc:281,317`, `td/list.cc:668`, `td/colrlist.cc:247-288`). Box styles differ
  too (RA 6, `ra/defines.h:2159`; TD 12, `td/defines.h:1657`).
- Seam: a `ColorScheme` struct plus a `UiSkin` interface each game implements: `DrawBox`, `Font`,
  `Print`, `Text(id)`, `DrawShape`, the slider arrow shapes, an unhandled-key hook (the screenshot
  key in `GadgetClass::Input`, `ra/gadget.cc:439-467`) and a per-frame hook (msgbox).
- Per game: the `DisplayClass` chain (`gscreen`, `map`, `display`, `radar`, `power`, `sidebar`,
  `tab`, `help`, `scroll`, `mouse`), `msglist`, the option/load/save dialogs, `credits`, and the
  game-side widget subclasses (`EListClass`, `IconList`, `DropList`, `BigCheck`, ...).

**Containers and simulation utilities (~3,500 lines, to `game_core`/`game_sim`).**

| Item                                                                                                                           | Verdict                                                                                                                                         |
| ------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| `VectorClass`/`DynamicVectorClass` (~1,000)                                                                                    | share RA's (TD's `Delete(int)` lacks the negative check, `td/vector_impl.h:549`); `vector.cc` instantiation lists stay per game                 |
| `ra/owned_vector.h`                                                                                                            | share                                                                                                                                           |
| `FixedHeapClass` trio (~900)                                                                                                   | share RA's superset plus TD's `Is_Allocated` and empty-buffer guard; save bytes identical                                                       |
| `QueueClass<T,size>` (~250)                                                                                                    | share the template; the `Queue_*` prototypes stay per game (`ra/queue.h:296-303`)                                                               |
| `game_clock.h`, `path_overlap.h` (+test), `interpal`, `utracker`, `crew.h`, `display_constants.h`                              | identical: move                                                                                                                                 |
| `flasher`                                                                                                                      | identical bytes; keep TD's load check (`td/flasher.h:65-74`)                                                                                    |
| direction: DirType, FacingType, Facing16/32 tables, `face.cc`, `facing.*` (~600)                                               | share without `DIR_MAX` (below)                                                                                                                 |
| identical enums in `defines.h` (CloakType, FireErrorType, MPHType, FacingType, ArmorType, ...)                                 | share once the trailing `*_COUNT` convention is settled: TD's extra enumerator changes `magic_enum::enum_count` and so `base::EnumArray`'s size |
| `door`                                                                                                                         | `DoorClass<Stage>`; `StageClass` stays per game                                                                                                 |
| `fly`, `fuse`                                                                                                                  | share over a `MapTraits` (off-map mask, cell bits); needs golden tests                                                                          |
| `Cardinal_To_Fixed`/`Fixed_To_Cardinal`, `ra/count_up.h`                                                                       | vocabulary: `base/`                                                                                                                             |
| palette fade loop                                                                                                              | `engine::gfx` (RA's version) with a mouse-palette hook                                                                                          |
| target, combat, radio, mission, abstract, findpath, serialize, world, game, game_state, StageClass, cargo, layer, object_heaps | per game                                                                                                                                        |

### Differences that must stay per game

- `DIR_MAX` is 254 in TD (`td/defines.h:2016`) and 255 in RA (`ra/face.h:61`), and is the bound of
  `Random_Pick(DIR_N, DIR_MAX)` on the synced RNG (`td/coord.cc:256`, `td/aircraft.cc:1082`, ...).
- Map: TD is 64 cells (6-bit fields, off-map mask `0xC000C000`), RA 128 (7-bit, `0x80008000`). TD's
  `Coord_X` is signed, RA's unsigned; RA's `Cell_To_Lepton` wraps at 256 (`ra/inline.h:275`).
- `TARGET`: RA `int32` with an 8-bit RTTI and 24-bit index (`ra/defines.h:312`), TD `uint16` with 4
  and 12 bits (`td/defines.h:1562`). Every saved object reference differs.
- Saved layouts of `StageClass`, `RadioClass`, `AbstractClass`, `MissionClass::Status`.
- Wire: TD's `GlobalHeaderType` embeds an `IPXAddressClass` (`td/ipxgconn.h:76-80`), RA's carries it
  in the queue's extra buffer; RA ACKs global packets immediately with resend detection, TD in
  `Service_Receive_Queue` without; magic 0x1235 vs 0x1234 (`ra/ipxgconn.h:127`); product ID 0xaa00
  vs 0xaa01; queue and packet sizes (`ra/network.h:154-167` vs `td/network.h:231-244`);
  `CONNECT_MAX` 7 vs 6; nullmgr `DEFAULT_TIMEOUT` 1500 vs 2000; `EventClass` bitfields
  (`Frame:26/ID:5` vs `Frame:27/ID:4`).
- RA saves `IPXAddressClass` (`ra/saveglobals.cc:206-210`): the shared class keeps its 4+6 bytes.

### Dead code

- RA sources excluded from the build (`src/ra/CMakeLists.txt:26-40`): adpcm, bmp8, cominit, dtable,
  dpmi, ipx95, itable, number, rand, stub, turret, w95trace, winstub; headers only they include
  (bmp8, cominit, dpmi, turret, w95trace, function); `class.cc` (license text only); `rng.h` (no
  includer); `ipx.cc` (only `IPX_SPX_Installed`, never called); `wspipx.*` (never constructed).
- TD: `winstub.cc` (excluded; it is also the only ra/td cross-include, `td/winstub.cc:553` ->
  `ra/filepcx.h`), `function.h`, `ipx95.*` and `ipxstub.cc` (stubs returning false: TD LAN play is
  unavailable today), every `#ifdef NOT_FOR_WIN95` (about 1,000 of `td/ipx.cc`'s 1,163 lines) and
  `VIRTUAL_SUBNET_SERVER` block (~15, ipxaddr/ipxconn/ipxmgr).
- Both: `led.h`, `message.h`, `savedlg.h` (no includers), `CONN_DEBUG` blocks in nullmgr, RA's
  unused `kCommandAndConquer` 0xaa01.
- Legacy non-source files in `src/ra/`: `*.mak`, `makefile`, `*.bat`, `*.pjt`, `*.ide`, `*.lnt`,
  `*.cfg`, `*.mac`, `profile.def`, `cwstub.c`, the `*.txt` notes and `eng/`. Keep `cc_icon.rc/.rh`,
  the icons and `testdata/` (used by `tools/ra_saveload_smoke.sh`).
- Stale entries: `externs.h` and `memcheck.h` in the CMake header remove-lists, `alloc`/`surface` in
  `src/ra/CPPLINT.cfg`, `jshell` in `.clang-tidy`'s `HeaderFilterRegex` (which also misses
  `src/base/`).

### Structure

- Both games glob `*.cc` recursively (`src/ra/CMakeLists.txt:22`), so new subfolders build without
  CMake changes (`ra/wolapi/` already does). The 25 RA and 14 TD `add_gtest` targets list game
  sources by relative path and must follow every move.
- Header guards (`CNC_RED_ALERT_RA_<PATH>_H_`) all match their paths today, but nothing enforces it:
  CPPLINT filters `build/header_guard` and clang-tidy disables `llvm-header-guard`.
- `tools/check_layers.py` checks only `engine/` and `base/`; nothing checks game includes.
- Game code already uses three named namespaces (`config`, `dib`, `td_save_detail`), no
  `using namespace`, and reopens `engine::gfx` once (`ra/graphics_loader.h:31`).
- 108 classes and 90 free functions are defined in both games (`AbstractClass`, `ListClass`,
  `IPXManagerClass`, `Init_Game`, `Main_Menu`, ...). Harmless today (two executables); each
  extraction removes the TD copy.
- The engine still knows the games: the `TD` macro changes `engine/gfx/pixel_buffer.h:365` and
  `engine/gfx/tile.h:34,43` (an ODR hazard for any shared object), and `Read_PCX_File` is declared
  in `engine/gfx/pcx_file.h:79` but defined per game and never called by the engine.
- `defines.h` is the hub: included directly by 160 RA `.cc` + 94 `.h` (TD 128 + 77), and it pulls in
  `jshell.h`, `palette.h` and `compat.h`. Strict layering inside a game is not possible until it is
  split, so in-game layering is recorded, then enforced only where it already holds.

### Proposed game folders

Same names in both games; counts are `.cc`/`.h` stems in RA.

| Folder     | Holds                                                                                                                           |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------- |
| `objects`  | abstract, aircraft, anim, building, bullet, cargo, cell, techno, unit, infantry, vessel, ... (~38)                              |
| `data`     | the `*data.cc` tables, rules, ini, ccini, profile, text ids (~18)                                                               |
| `map`      | the display chain (map, display, radar, power, sidebar, tab, help, mouse, scroll, gscreen), layer, logic, findpath, world (~16) |
| `ai`       | base (BaseClass), team, teamtype, trigger, trigtype, tevent, taction, reinf, scenario, special (~13)                            |
| `ui`       | game-side widgets and dialog helpers that stay per game (~25, shrinking with phase 5d)                                          |
| `screens`  | option/load/save/sound/visual dialogs, mapsel, score, intro, movie, menus, hotkeys (~19)                                        |
| `net`      | event, queue, session, network, mplayer, netdlg, nulldlg, the transports until 5c (~24)                                         |
| `wol`      | RA only: Westwood Online, absorbing `wolapi/` (~19)                                                                             |
| `editor`   | maped\* (5)                                                                                                                     |
| `saveload` | saveload, saveglobals, serialize, ioobj, iomap, record_playback (~7)                                                            |
| `assets`   | assets, installation, audio, theme, palette(s), screen, shape_draw, dib, writepcx (~16)                                         |
| `startup`  | startup, init, conquer, game, game_state, version, stubs (~9)                                                                   |
| `core`     | defines, const, inline, config, jshell, coord, face, heaps, vectors (~25, shrinking with 5a/5b)                                 |
| `debug`    | debug, debug_state (2-3)                                                                                                        |

## Plan

0. **Commit this plan** and add a "Current Work" memory line pointing at it.
1. **Cleanup.** Delete the dead code and files listed under Findings; drop the stale CMake, CPPLINT
   and `.clang-tidy` entries (header filter becomes `src/(base|engine|game|ra|td)/`). Make the
   engine game-agnostic: replace the `TD` macro in `pixel_buffer.h`/`tile.h` with a parameter the
   game passes, and move the `Read_PCX_File` declaration into the games.
2. **Tooling.** Teach `tools/engine_layout.py` and `tools/check_layers.py` the layer order
   `base -> engine -> game -> {ra, td}`: `game/` includes no game, ra and td never include each
   other, `game/` libraries follow their own `DEPENDS_ON` table (`core` <- `sim`, `core` <- `ui`).
   Add a check that every header guard matches its path, since every later step renames guards.
3. **Game folders.** One scripted move per game into the folders above: `git mv`, includes, header
   guards, `add_gtest` source lists, CPPLINT configs, `tools/*smoke*.sh` paths. Record each folder's
   actual includes; enforce the subset of folder rules that already holds (for example, that nothing
   outside `editor/` includes it).
4. **Namespaces.** Wrap all game code in `namespace ra {`/`namespace td {` (a header's engine
   forward declarations stay outside), fold `config`/`dib`/`td_save_detail` into it as nested
   namespaces, keep `main` and the SDL entry point global, qualify the tests (or `using` in the test
   `.cc`). Verify with `nm -C`: no game symbol outside `ra::`/`td::` except `main`.
5. **Extraction waves.** Each wave is independently committable and deletes the TD copy (or both
   copies) in the same commit that adds the shared one. Order by payoff and risk:
   - **5a `game_core`:** Vector/DynamicVector/OwnedVector, the FixedHeap trio, `QueueClass`,
     `game_clock`, `path_overlap`, `interpal`, `display_constants`; `Cardinal_To_Fixed` and
     `count_up.h` to `base/`.
   - **5b `game_sim`:** utracker, crew, flasher, the direction module (each game keeps its own
     random-direction bound in place of `DIR_MAX`), the identical `defines.h` enums (settle the
     `*_COUNT` convention first), `DoorClass<Stage>`. Palette fade to `engine::gfx`.
   - **5c `engine::net`:** `DatagramTransport` with the RA UDP and TD TCP implementations; combuf,
     merged connect, connmgr, `IPXAddressClass`, ipxconn, ipxmgr, nullconn, the nullmgr protocol
     half. The global channel (ipxgconn) takes a policy for the header layout and ACK style so both
     wire formats stay byte-exact. Add a loopback transport and a unit test that runs two managers
     against each other.
   - **5d `game_ui`:** first migrate TD from `int fore` to `ColorScheme` and define `UiSkin` in both
     games (TD gets a "green" and a "plain" scheme); then extract in dependency order: `LinkClass`,
     `Input`, `ColorScheme`/`BoxStyle`/`UiSkin` -> `GadgetClass` -> `ControlClass`,
     `TextLabelClass`, `StaticButtonClass` -> `ToggleClass`, `GaugeClass`, `EditClass`,
     `DirectionDial` -> `TextButtonClass`, `ShapeButtonClass`, `CheckBoxClass` -> `SliderClass` ->
     `ListClass` -> `ColorListClass`, `CheckListClass`, `TListClass`; optionally the message box.
   - **5e (optional) map traits:** `MapTraits` (cell bits, off-map mask, signed `Coord_X`) under
     `fly`, `fuse` and the `inline.h`/`coord.cc` helpers, only after golden tests pin both games'
     current results for on- and off-map coordinates.

## Verification

Per step: `cmake --build build --parallel 22 && ctest --test-dir build --output-on-failure`
(includes `engine_layers_test` and `cpplint_test`), then the strict build
(`cmake --build build-strict --parallel 14`, foreground) before committing.

- Steps 3-5: `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` with every fixture
  (`--team`, `--building`, ...): a loaded run must match a continuous one, and save files written
  before the step must still load after it.
- Step 4: `nm -C` of `ra_engine`/`td_engine` objects shows no global game symbols except `main`.
- 5c: the loopback test, plus a manual two-instance multiplayer game per game (no networked harness
  exists). TD keeps 0x1234 and its header layout; a packet capture before and after must match.
- 5d: the user's real-display check of the main menu, options, load/save and multiplayer dialogs in
  both games, in particular TD's green and plain looks and RA's colour schemes.

## Progress

- [x] 0. Plan committed
- [x] 1. Cleanup and game-agnostic engine
- [x] 2. Layer and header-guard tooling
- [ ] 3. Game folders
- [ ] 4. `ra::`/`td::` namespaces
- [ ] 5a. `game_core`
- [ ] 5b. `game_sim`
- [ ] 5c. `engine::net` core
- [ ] 5d. `game_ui`
- [ ] 5e. Map traits (optional)
