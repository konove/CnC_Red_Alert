# TODO

## Bugs

- Make the Chronosphere's synthetic right-click actually end targeting mode
  - `TechnoClass::Take_Damage()` (`ra/objects/techno.cc`) hands `TheMap().AI()` a right-click event,
    but no map layer acts on one there: the tactical gadget takes real right-clicks before `AI()`
    runs, so it has never worked (see `docs/INPUT_EVENTS_PLAN.md`)
  - The call also runs every map layer once with the mouse at (0, 0); cancel the mode directly, as
    `DisplayClass::Mouse_Right_Press()` does, instead
- Let `HardwareCursor::SetShape()` retry a shape whose SDL cursor could not be created
  - It records `current_shape_` even when `SDL_CreateColorCursor()` fails, so the same shape is
    never tried again and SDL shows its default cursor
  - The hot spot is not checked against the shape size; SDL rejects one outside the cursor
- Decide what `WinX`/`WinY` (`engine/gfx/text_window.h`) should be now nothing writes them
  - The original `Change_Window()` set them to the active text window's origin; since it was stubbed
    (and deleted in `09717739`) they are always 0, so `ra/screens/menus.cc`, `td/screens/menus.cc`,
    `ra/objects/object.cc` and `td/objects/object.cc` place menus and text as if every window sat at
    the screen's top-left
  - Either delete them (no visible change) or set them from `WindowList` where the window changes
- Make Red Alert's cloak shimmer walk the other way on the right half of the window
  - `CC_Draw_Shape()` (`ra/assets/shape_draw.cc:163`) negates the predator offset when
    `x > WindowList[window][kWindowWidth] * 4`, TD's formula; TD stores window widths in 8-pixel
    units, RA in pixels (`engine/gfx/pixel_buffer.h:363`), so RA's test is "past four window widths"
    and never holds for a visible shape. RA never uses the down-and-left sample offsets
  - Fix: `x > width / 2` in pixels (one line); visual only, not simulation. Found in the
    `shape_blitter` `/modernize-file` run (`a5dd8079..b83db634`)
- Decide what a ghost or fading draw without its table should do
  - `engine::gfx::DrawShapeFrame()` (`engine/gfx/shape_blitter.cc`) silently draws nothing when the
    table a `ShapeFlags::kGhost` or `kFading` draw needs is missing or too short; RA's
    `CC_Draw_Shape()` substitutes `DisplayClass::SpecialGhost`/`FadingShade` first (the "E3 kludge",
    `ra/assets/shape_draw.cc:118-127`), TD's (`td/startup/conquer.cc`) does not
  - Options: give TD the same substitution (small; draws what now vanishes), and/or make a missing
    table a `CHECK` in the blitter instead of a no-op (turns a silent miss into a crash)
- Decide what a warhead with fewer than five `Verses` entries should get
  - `ra/objects/warhead.cc` reads each missing entry as `Fixed::FromString()` of an empty token,
    i.e. 0%, so that armor takes no damage from the warhead; keeping the previous modifier instead
    would change how such a modded rules file plays
  - The default `"100%%,..."` there is a leftover printf escape; it reads as 100% only because the
    percentage parse stops at the first `%`
- Decide how `Fixed::FromString()` (`base/fixed.cc`) should read "12.5%" and oversized numbers
  - "12.5%" reads as 12%, since the percentage takes only the leading digits; the original read it
    as the decimal 12.5, because it counted a `%` only right after the digits. A whole part past
    INT_MAX reads as 0, while smaller ones keep their low byte
  - Parsing once into a decimal (int64 digits over 10^k, at most eight fraction digits) and scaling
    to 8.8 once would read "12.5%" as 12.5% and wrap every oversized value alike. No shipped rules
    value changes; found by the `/simplify` stage of the `base/fixed` `/modernize-file` run
    (`4e995aa2..8fd12531`)
- Reject a keyframe shape frame whose LCW data ends early in `engine::gfx::BuildFrame()`
  (`engine/gfx/keyframe.cc:144`, `169`)
  - It ignores `LCW_Uncompress()`'s byte count, so a key frame that stops short leaves the rest of
    the frame 0, which draws as transparent; returning `{}` when fewer than width \* height bytes
    were written would drop such a frame instead. Only malformed data reaches it (the keyframe.h
    comment documents it). Found by the `ra/2keyfram.cc` `/modernize-file` run
    (`dfc0fa2b..193f2624`)
- Find why Tiberian Dawn's simulation depends on the build directory
  - The same source (2d8a614a) built in a scratch worktree logs a different
    `tdsdl -NOMOVIES -SEED1 -NEWGAMESCG01EA -QUITFRAME600` run than when built in `build/`: 22,384
    differing lines, starting at frame 2 in an anim's serialized fields. Every `build/` binary
    agrees with every other, and old-vs-old runs of one binary match, so it is not run-to-run noise
  - Smells of an uninitialized read whose garbage depends on the binary's layout or paths; until it
    is found, TD logic diffs only prove anything between binaries built in the same directory
    (`docs/GAME_CONTAINERS_PLAN.md`)
- Check whether joining a Tiberian Dawn internet game can index past the scenario list
  - The `Get_Connected()` branch of `td/net/netdlg.cc` (~2635) sets `scenario_index()` to the host's
    scenario file number rather than a list index, and ~1197 indexes `scenario_files()` with it.
    With files numbered from 1, the last map is one past the end: that used to read a null spare
    slot of the old vector and now aborts in `base::At`
  - First confirm the WChat/winsock path is reachable in this port
- Guard Red Alert's WWChat player color write in `ra/net/netdlg.cc` (~2880-2890)
  - It sets `playerlist.Colors.at(0)` without checking that the list has a row; the branch looks
    unreachable in this port, so verify that or add the guard
- Guard the map editor's popup handlers against an empty selection
  - `ra/editor/mapedit.cc` (~1205-1360) and the same handlers in `td/editor/mapedit.cc` read
    `current_object()[0]` unguarded; they run only while the popups are up, which needs a selection,
    but if the selection empties with the popups still showing the read now aborts instead of using
    a stale pointer

- Check whether Red Alert's team recruiting adds vehicles and ships once per loop pass
  - In `TeamClass::Recruit()` (`ra/ai/team.cc`), the `RTTI_UNIT` and `RTTI_VESSEL` branches run
    `if (best) { Add(best); added++; ... }` inside the `for` loop over the heap, so the best
    candidate so far is added, and `added` counted, on every later pass; the infantry and aircraft
    branches do it once after the loop. Compare with the original source and TD's `Recruit()`
    (`td/ai/team.cc`), which is structured differently, before moving the block
  - Found while converting `CCPtr` (`docs/CCPTR_REMOVAL_PLAN.md`, step 2)
- Check that Red Alert's scenario INI numbers team and trigger types the same way both ways
  - The writers emit a type's position in its heap's in-use list (`PositionOf()`), the readers
    resolve the number as a slot index (`SlotOrNull()`): `ra/ai/tevent.cc` (~651 write, ~674 read),
    `ra/ai/taction.cc` (~293, ~319), `ra/ai/teamtype.cc` (~2007, ~1841 and ~1937). They agree only
    while the type heaps have no holes, which holds after a fresh INI load; a type deleted in the
    scenario editor leaves one. The original game had the same pairing. Either write `SlotIndexOf()`
    or check that the editor saves its types in slot order
- Decide whether Red Alert's lepton distances should still wrap at 16 bits
  - Leptons became `int` in stage B1 of `docs/COORDINATE_TYPES_PLAN.md`, which kept every old
    `uint16_t` wrap explicitly. Three of them turn a large value into a small one:
    `Wide_Area_Damage()` (`ra/objects/combat.cc`) wraps a radius of 256 cells or more (an exploding
    unit's `MaxStrength` times `ExplosionSpread`), `Constrained_Look()` (`ra/map/display.cc`) a
    `GapRadius=` of 256 cells or more, and `Update_View_Dimensions()` a tactical view 6144 pixels
    wide; `Lepton_To_Pixel()` (`ra/core/packed.h`) also reads 128 cells or more as negative
  - None is reachable with the shipped rules and today's screen sizes; dropping a wrap changes
    behaviour, so do it in its own commit with a test
- Check the heap `Save()` results in Red Alert's save game
  - `ra/saveload/saveload.cc` (~281-340) ignores every `ObjectHeap<T>::Save()` result, which is
    `writer.ok()` since `90064ddd`; Tiberian Dawn's `saveload.cc` stops at the first false
- Decide whether Tiberian Dawn's `Scatter(true)` calls meant "forced"
  - The original passed `true` where `Scatter()` takes the threat coordinate, so `UnitClass`
    (`td/objects/unit.cc:1676`), `InfantryClass` (`infantry.cc:1096`) and the refinery's harvester
    (`building.cc:2538`) scatter away from the coordinate `FromPacked(1)`, near the map's top-left
    corner, unforced; stage B2 (TD) kept that as `Scatter(Coordinate::FromPacked(1))`
  - `Scatter(Coordinate{}, true)` would scatter in a random direction and change play, so it needs
    its own commit and an xcheck reference update
- Restore both games' `BuildingClass::Fire_Direction()` for buildings without a turret
  - The original returned `Direction(TarCom)`, the direction toward the target; `48924849` turned it
    into `AsDirection(TarCom)`, the low byte of the target word (the target's heap index), and stage
    B3 kept that as `game::AsDirection(TarCom.packed())` in `ra/objects/building.cc` and
    `td/objects/building.cc`
  - Fix: `return Direction(TarCom);`; it changes which way a defense without a turret fires, so it
    needs its own commit and an xcheck reference update per game
- Decide whether Tiberian Dawn's `Target::Make()` should mask its value to 12 bits
  - `Make()` (was `Build_Target()`) ORs the whole value over the kind (`td/core/target.h`), so a
    cell of -1 becomes the word 0xFFFF: kind 15, which names nothing, yet true as a bool. A team
    mission `TMISSION_MOVE` or `TMISSION_UNLOAD` whose waypoint is unset (`init.cc` fills the
    waypoints with -1) sends its units to that word (`td/ai/team.cc`, `TeamClass::AI()`), as does a
    `TMISSION_MOVECELL` argument outside 0..4095; the original did the same
  - Masking would turn -1 into cell 4095 and change play, so it needs its own commit and an xcheck
    reference update
- Decide which RTTI_CELL encoding Red Alert's cell targets should use
  - `Target::FromCellIndex()` stores the cell number, `FromCell()`/`FromCoordinate()` a 12-bit
    sub-cell spot (`ra/core/target.h`), and nothing in the word says which; `AsCell()` and
    `AsCoordinate()` read only the sub-cell form, `AsMovementCoordinate()` (no callers, like the
    original `As_Movement_Coord()`) only the cell index
  - A mismatch the original already had: the construction yard's ARCHIVE event sends
    `FromCellIndex(cell)` (`ra/objects/building.cc`, `Active_Click_With()`), and the MCV it
    undeploys into is sent to that word with `Assign_Destination()`, which reads it as a sub-cell
    spot, so it heads for the wrong cell
- Detach the right cells when Red Alert's `MapClass::Destroy_Bridge_At()` drops a bridge
  - The detach loop (`ra/map/map.cc`) moves `cell` down a row per row of the bridge but detaches
    `Target::FromCell(cell.Offset(tdata))`, where `tdata` counts every cell visited so far
    (`y * w + x`) rather than the column `x`; from the second row on it detaches targets on cells
    below and right of the bridge and leaves the bridge's own cells attached. The damage loop next
    to it uses `cell.Offset(x)` correctly
  - Fix: `cell.Offset(x)`; it changes which objects lose their target, so it needs its own commit
    and an xcheck reference update. Tiberian Dawn has no `Destroy_Bridge_At()`
- Send both games' PLACE event through its own constructor
  - `DisplayClass::Mouse_Left_Release()` (`ra/map/display.cc`, `td/map/display.cc`) passes
    `cell.index() + ZoneOffset`, an `int`, so the event is built by
    `EventClass(type, RTTIType, int id)`, which fills `Data.Specific.ID`; the PLACE handler reads
    `Data.Place.Cell`, the low half of that int on a little-endian machine. It works, but the high
    half (0 or 0xFFFF) also crosses the wire, and a big-endian peer would read 0
  - Fix: `EventClass(EventClass::PLACE, ..., cell.Offset(ZoneOffset))`; it changes two bytes of the
    packet (not the game), so do it together with any other event-format change
- Scan the right cells in Tiberian Dawn's `TechnoClass::Nearby_Location()`
  - The box scan (`td/objects/techno.cc`) adds `Cell::FromXY(x, -radius).index()` to the center
    cell, but TD's `FromXY()` (was `XY_Cell()`) does not mask its components: for a negative X the
    sign bits cover Y, so the offset is just X, and the scan of the box's left half visits cells on
    the center row instead of the top and bottom rows (and the column scan's `FromXY(-radius, y)`
    likewise visits `-radius` only). Red Alert's `Nearby_Location()` searches another way and is not
    affected
  - Fix: offset by `(dy * MAP_CELL_W) + dx` for each box cell (dx, dy); it changes where aircraft
    and units are sent, so it needs its own commit and an xcheck reference update
- Decide whether Tiberian Dawn's `Cell::y()` should mask to 6 bits
  - `y()` (was `Cell_Y()`, `td/core/cell.h`) is every bit above X of the sign-extended word, so a
    sentinel reads as a row of millions: `Cell::None().y()` is 67108863, and `None().Center()` is
    0xFF803F80 (cell 63, 255), where Red Alert reads 127 and (127, 127). Stage B4 (TD) found no path
    that hands a sentinel to `y()`, `Center()`, `Distance()`, `Direction()` or
    `MapClass::Cell_Distance()` in play, but nothing guards them
  - Masking changes results only for sentinels and off-map cells; do it in its own commit with a
    test, after checking every caller
- Decide whether Tiberian Dawn's `Cell::Containing()` should mask the X cell byte to 6 bits
  - `Containing()` (was `Coord_Cell()`, `td/core/cell.h`) shifts the Y cell byte over the whole
    8-bit X cell byte and ORs them, as the original assembly did, so an X cell of 64 or more (a
    coordinate off the right edge) sets Y bits: X cell 64 of row 0 reads as cell (0, 1). Red Alert's
    `Containing()` masks each byte to 7 bits instead. `cell_test.cc` pins TD's result
  - Only four places test a coordinate against `kHighCoordMask` before using it; the 224
    `Containing()` call sites otherwise trust it to be on the map. Masking changes the cell an
    off-map coordinate names, so do it in its own commit with a test and an xcheck run
- Decide whether Tiberian Dawn's carrying coordinate adds should become component adds
  - Four sites add an integer to the packed word, so X can carry into Y, and stage B2 (TD) kept them
    as `Coordinate::FromPacked(c.packed() + n)`: `ai/reinf.cc` (`Docking_Coord() + 2`, read for its
    Y cell) and the `-MOBILETEST` fixture in `startup/init.cc` (`+ 0x200`, `+ 0x600`, `+ 0x800`).
    `+ Coordinate::FromLeptons(n, 0)` wraps within X instead
  - The two differ only when X is within `n` leptons of 0x10000, which no on-map coordinate is, so
    the change is safe in play but alters the off-map results; do it in its own commit
- Compare the font too when Tiberian Dawn's message list matches messages
  - `td/ui/msglist.cc` (~232 duplicate detection, ~251 multi-segment matching) compares only
    `txtlabel->Style == style`; before `docs/TEXT_PRINT_SPLIT_PLAN.md` that value carried the font,
    so two messages with the same text, colour and flags in different fonts now match. No caller
    passes another font than the gradient one today; add `txtlabel->TextFont == font &&` to both
- Decide whether `HasGradientFontBits()` should mean only the gradient font
  - Nine widget and skin sites used to test `flags & TPF_6PT_GRAD`, which is 6 and so matched every
    font index with bit 1 or 2 set: in RA the 8-point, 3-point, LED, VCR, gradient, map, typewriter
    and score fonts, in TD the first six of those (indices 2-7). `HasGradientFontBits()` in
    `ra/ui/dialog.cc` and `td/ui/dialog.cc` keeps that set, so TD's 8-point buttons draw in the
    green gradient style. Narrowing it to `font == FontType::k6PointGradient` changes what those
    buttons and lists look like; decide on a real display, together with the display check under
    Testing
- Read Tiberian Dawn's `[Basic] CarryOverCap` under its own name
  - `td/data/ini.cc:437` and `:720` read and write the key `"carry_over_cap"`: b2135cdf ("Make the
    carry-over cap local to ini.cc") renamed the variable and the rename reached the string literal.
    Red Alert still uses `CarryOverCap`. A scenario that sets `CarryOverCap` is ignored today
  - One line each. It changes behaviour against any pre-fix binary, so re-take the headless baseline
    afterwards
- Stop Tiberian Dawn's phone-book reader overwriting the line it is splitting
  - `td/net/mplayer.cc:652` does `base::SafeCopy(buf, tokenptr)` into the buffer being tokenized, so
    the fields after `DialMethod` always read as their defaults
- Give Tiberian Dawn's modem init strings a fallback when `CONQUER.INI` cannot be loaded
  - `Read_MultiPlayer_Settings()` (`td/net/mplayer.cc`) clears the init-string list and returns
    without the "ATZ" default the normal path adds; `td/net/nullmgr.cc:384` then calls `.at(0)` on
    an empty list if someone dials. The old reader had the same shape
  - `Build_Init_String_Listbox()` (`td/net/nulldlg.cc`) sorts the list in place while the stored
    indexes stay, so a saved `InitStringIndex` can name a different string afterwards
- Decide what `Carryover::Recreate()` does with an object it cannot place
  - `ra/ai/carryover.cc:154` (marked with a TODO): when `Unlimbo()` fails because the old cell is
    off the new map or occupied, the object built by `base::NewPooled` stays in limbo and keeps its
    pool slot for the rest of the scenario. The original did the same. Options: `base::DeletePooled`
    it on failure (3 lines; changes pool occupancy, so later allocations and a multiplayer or
    recorded game can diverge), or leave it and replace the TODO with a comment saying why
  - Found in the carryover modernization (`9931845f`..`fe837a37`); Tiberian Dawn has no carryover
    objects
- Check whether a captured object leaves stale threat in the houses' regions
  - An object adds its `Risk()` to the other houses' `Region`s on `MARK_DOWN` and subtracts it on
    `MARK_UP` (`ra/objects/object.cc:1668,1682`, via `CellClass::Adjust_Threat()` in
    `ra/objects/cell.cc:1710`), and each call reads `Owner()` and the alliances afresh.
    `TechnoClass::Captured()` (`ra/objects/techno.cc:4047`) changes the owner without lifting the
    object, so the later `MARK_UP` skips the new owner and subtracts from the old one, which never
    had the amount added: the new owner keeps the threat for good and the old owner's region goes
    negative. A human house whose alliances change while objects are down drifts the same way. Not
    traced to the end: confirm with a campaign capture and a dump of `HouseClass::Regions`
  - Only the campaign tracks threat (`GAME_NORMAL`), where it steers `findpath.cc:1324` and
    `team.cc:567`. A fix (adjust the regions around the owner change in `Captured()`, about 10
    lines) changes computer path choices, so it is a simulation change to decide on
  - Tiberian Dawn has the same code (`td/objects/object.cc:1396,1409`). Found in the region
    modernization (`805bd6ba`..`1ad6409d`)

## Features

- Decide whether the sidebar strips should sort their buildables in both games
  - Today both list items in the order they became available. TD had a disabled `qsort` in
    `SidebarClass::StripClass::Add()` (`td/map/sidebar.cc`, deleted as dead code) that put buildings
    first, then walls by cost, then units and aircraft by RTTI, each group by the scenario it first
    appears in; RA never sorted
  - If wanted, write it fresh as a `std::ranges::stable_sort` with a projection in `Add()`, so items
    that tie keep their arrival order

## Refactoring

- Migrate legacy integer types to fixed-width types per Google C++ Style Guide
  - Priority: `long`/`unsigned long` first (different sizes on Win64 vs Linux x86_64)
  - Then: `short`/`unsigned short`
  - Convert unsigned number types to signed where appropriate
  - See `docs/TYPE_MIGRATION.md` for full conversion rules
  - Use `/migrate-types <file>` command for per-file migration
- Replace the manual `PixelView::Lock()`/`Unlock()` pairs with an RAII guard
  - About 30 sites left in `ra/`, `td/` and `engine/gfx/`, in five different shapes: unlock inside
    the `if`, unlock outside it, bare `Lock()` with the result dropped, `Lock()` folded into a
    larger `&&`, and two-view locks with hand-written unwind
  - A `[[nodiscard]] PixelLock` returned by `Lock()`, with `Unlock()` made private, would make the
    miswritten pattern stop compiling; guaranteed copy elision means it needs no move constructor
  - `ra/assets/winbits.h`'s `LockedWindow` is the same idea already hand-rolled for one caller, and
    would fold into it
  - 9 of the sites genuinely need the lock (`DrawShapeFrame` subspans `dest.pixels()` without
    locking, so its callers must hold one); the other 22 hold it to batch a render loop, since each
    outer unlock also runs `DropScaledFrame()` and arms `Present(false)`
  - `ra/map/radar.cc:1033` and `:1041` are early-return unwinds inside an outer lock - the shape a
    guard most obviously fixes
- Make `Display::DropScaledFrame()` private
  - Only `Display` itself calls it (`engine/window/display.cc`); the games use
    `PresentScaledFrame()`
- Remove the blank lines left where the `AllSurfaces.SurfacesRestored` blocks were deleted
  (`6be19256`)
  - 44 `while (process) {` loops in 20 files now open with a blank line (most in
    `td/net/nulldlg.cc`, `td/editor/mapeddlg.cc`, `td/net/netdlg.cc`), and `td/net/nulldlg.cc:166`,
    `:3514` and `:4831` have two in a row; `git clang-format` does not touch lines that were only
    deleted
- Finish moving the games' input handling onto `engine::window::InputEvent`
  - The map layers' `AI(InputEvent& event, ...)` still open with `KeyNumber& input = event.key;`,
    and the dialogs copy `input = input_event.key;`: transitional aliases that can read the event
  - `GScreenClass::Input(int& x, int& y)` returns the mouse position through out-parameters that
    none of its six callers (`ra/startup/conquer.cc`, `ra/net/queue.cc`, `td/startup/conquer.cc`,
    `td/net/queue.cc`) uses
- Delete dead code that still spells the old keyboard API
  - `td/ui/msgbox.cc`'s `#ifdef NEVER` block (C-style `KeyNumber` casts, `& 0xFF` masks)
  - Commented-out `kKeyReleaseBit` tests in `td/ui/edit.cc` and `ra/wol/woledit.cc`
- Delete commented-out code that still calls the mouse functions
  - `ra/ui/gadget.cc`, `ra/startup/init.cc`, `ra/wol/wol_logn.cc`, `td/startup/conquer.cc`,
    `td/debug/debug.cc`, `td/map/gscreen.cc`, `td/startup/init.cc`, `td/ai/scenario.cc`,
    `td/screens/score.cc`, and `// TheMouse()->GrabMouse();` in `ra/winstub.cc` and `td/winstub.cc`
- Move the mouse grab into `Display`
  - Make `mouse_grab_disabled` a `Display` option honoured by `SetMouseGrab()`, drop the static
    `HardwareCursor::GrabMouse()`/`ReleaseMouse()`, and have the startup and shutdown code
    (`ra/startup/startup.cc`, `td/startup/startup.cc`) and the focus handlers
    (`ra/startup/nondosstub.cc`, `td/startup/nondosstub.cc`, `td/winstub.cc`) call
    `TheDisplay().SetMouseGrab()` directly
- Consider skipping `HardwareCursor::UpdatePalette()` when colors 1-255 are unchanged
  - It recreates the OS cursor on every palette change while the cursor is visible; worth it only if
    a profile shows the cost
- Share the SDL helpers the engine repeats
  - `std::unique_ptr` deleters exist separately in `engine/window/hardware_cursor.h`,
    `src/tools/vqaplay.cc` and `engine/video/vqa/audio_output.h`, and `engine/window/display.cc`
    frees its `void*` handles by hand
  - The span over an `SDL_Palette`'s colors (`display.cc` twice, `hardware_cursor.cc`) and the
    `pitch * h` surface-bytes span (`Display::Lock()`, `hardware_cursor.cc`) repeat with their
    NOLINTs; the two-argument spans still print `-Wunsafe-buffer-usage-in-container` compiler
    warnings, since the NOLINT only silences clang-tidy
- Stop a redraw event running a nested event pump
  - `Display::PumpEvents()` ends the frame on the redraw event, and `Display::Present()` pumps
    events again, so a redraw event drains the rest of the queue in a nested loop
  - Either have the redraw timer set an atomic flag that `Display` checks itself (~15-20 lines;
    removes `redraw_event_` and `PostRedrawEvent()`), or stop `Present()` pumping (2 lines; first
    check that no loop that only presents relies on it to see `SDL_QUIT` or focus events)
- Move `engine/platform/file_system.h` into `engine::platform`
  - It is the last header in the folder outside the namespace
- Run `/fix-ide-issues` on `engine/platform/timer.h` and `timer.cc`
  - Stage 7 of the `/modernize-file` run (`0e463fee..eed3a0d2`) was skipped: CLion's
    `getDiagnostics` timed out on every file
- Settle on one spelling of "the countdown has time left"
  - `HasTimeLeft()` (35 sites), `!IsFinished()` (5: `td/objects/infantry.cc:1358`,
    `td/ai/team.cc:277`, `td/objects/drive.cc:894`, `td/objects/techno.cc:3553`,
    `engine/gfx/glow_pulse.h:40`) and `Remaining()` compared with 0 (5: `td/map/mouse.cc:235`,
    `td/map/scroll.cc:223` and `:230`, `td/net/nulldlg.cc:4432`, `td/ui/msgbox.cc:432`)
  - Either drop `HasTimeLeft()` for `!IsFinished()` or convert the other 10 sites to it
- Read `Arm.Remaining()` once in `ra/objects/foot.cc:622`
  - It calls `HasTimeLeft()` and then `Remaining()`, two `Tick()` reads for one answer
- Consider dropping the double check in `CurrentFrame()` (`ra/core/game_clock.h`,
  `td/core/game_clock.h`)
  - It tests `IsInstalled()` and then `Get()` checks the same pointer again on every frame-timer
    `Tick()`; a few ns, worth it only if `base::Installed` gains a get-or-null accessor
- Consider renaming `engine::platform::Timer<T>` to `Countdown<T>` (162 uses)
  - `Stopwatch` and `Timer` are both timers; the name kept by `docs/TIMER_UNIFICATION_PLAN.md` does
    not say which way it counts
  - `td/map/credits.h:75` has an `int Countdown` member that the new name would shadow
- Delete the commented-out code in `ra/wol/wol_logn.cc:439` and `ra/wol/wolapiob.cc:801`
  - Both still name `Timer<SystemTickSource>` and `SystemMilliseconds()` unqualified, so it no
    longer matches the code around it
- Consider stepping the source coordinates incrementally in `ScaleRotateShape()`
  (`ra/assets/shape_draw.cc`)
  - Each destination pixel recomputes `bx`/`by` with four multiplies; adding a constant per x (DDA)
    is cheaper, but the accumulated rounding can move pixels, so check it against a pixel diff of
    rotated and scaled shapes
- Add a shared `engine::gfx` point type or const image view only when a second user appears
  - `engine/gfx/bitmap.h` (`BitmapClass`, `TPoint2D`) was deleted for having one caller;
    `ra/screens/mapsel.cc` has a file-local `Point` and `VqaFrameView`
    (`engine/video/vqa/vqa_player.h`) is a width/height/pixels view that could become the shared one
- Share the options dialogs' brightness, contrast, color and tint math
  - `OptionsClass::Adjust_Palette()` in `ra/data/options.cc` and `td/data/options.cc` repeat the
    same ~25 lines, neutral-slider copy included; only the slider scale (RA's `Fixed * 256`, TD's
    `unsigned char`) and the skipped index (`kMouseColor` vs 255) differ. One `engine/gfx` function
    over an `HsvColor` and four 0..255 levels would do
- Define the 0..255 HSV scale once
  - `kMaxComponent` = 255 is a local in both `base::HsvToRgb8()` and `base::Rgb8ToHsv()`
    (`base/hsv.h:43`, `:100`) and again `PaletteColor::kMaxComponent`; `FadedToward()` still clamps
    with a literal 255 (`engine/gfx/palette_color.cc:33`). Hoist one `base::` constant into
    `base/hsv.h`
- Move the rest of `engine/gfx` into `engine::gfx`
  - Only `palette_color.*`, `hsv_color.*` (`babcd0a9`), `palette.*` (`b88cee17`), `keyframe.*`
    (`b98babe6`), `shape_blitter.*` (`b8ab89fe`) and `shape.*` (`a9332c6d`) are in it;
    `glow_pulse.h` and `glow_pulse_test.cc` write `engine::gfx::PaletteColor` and
    `engine::gfx::kBlackColor` and drop the qualifier when they move
- Run `/fix-ide-issues` on `engine/gfx/palette_color.h`, `palette_color.cc`, `palette_color_test.cc`
  and `hsv_color_test.cc`
  - Stage 7 of the `/modernize-file` run (`dc251088..f60d3070`) was skipped: CLion's
    `getDiagnostics` timed out on them (only `hsv_color.h` answered, with nothing to fix)
- Split `engine::gfx::ShapeEffects::predator_offset` into a start and a direction
  - One `int` carries both: its magnitude mod 8 picks the first sample offset and its sign picks the
    right-hand or the down-and-left table (`engine/gfx/shape_blitter.h`); both `CC_Draw_Shape()`s
    build it as `CurrentFrame()` negated on the right half of the window. A `predator_start` plus a
    `bool` (or the blitter choosing the direction from x and the view width) drops the sign trick.
    Pairs with the RA shimmer-direction bug above
- Consider making color 0 always transparent in `engine::gfx::DrawShapeFrame()`
  - Both callers pass `flags | ShapeFlags::kTransparent` (`ra/assets/shape_draw.cc:178`,
    `td/startup/conquer.cc:2401`) and no other code passes it, so the eight non-transparent entries
    of `kBlitters` (`engine/gfx/shape_blitter.cc`), including the whole-line copy, only run in
    `shape_blitter_test.cc`; dropping the flag halves the instantiations and deletes `kTransparent`
    (~6 files: `shape.h`, `shape_blitter.{h,cc}`, its test, the two callers)
- Move `engine::gfx::ShapeFlags` into `engine/gfx/shape_blitter.h`
  - The flags are `DrawShapeFrame()`'s parameter; `engine/gfx/shape.h` would then hold only the
    shape file format (`ShapeHeader`, `ExtractShapeFrame()`) and could become `shape_file.h`. 66
    files include `engine/gfx/shape.h`, most only for the flags, so it is an include rewrite
- Parse a keyframe shape's header once for callers that want its width and height
  - `KeyframeWidth()` and `KeyframeHeight()` (`engine/gfx/keyframe.h`) each re-read the header, and
    most callers ask for both: `ra/assets/shape_draw.cc:130-131`, `195-196`, `283-284`,
    `ra/data/udata.cc:1094-1095`, `ra/ui/shapebtn.cc:119-120`,
    `ra/ui/drop.cc:66-67`/`ra/ui/drop.h:196-197`, `td/startup/conquer.cc:2241-2242`, `2307-2308`,
    `2398-2399`, `td/data/udata.cc:1483-1484`, `td/ui/shapebtn.cc:109-110`. A size getter, or a
    small `KeyframeShape` view over the file with `frame_count()`, `width()`, `height()` and
    `BuildFrame()` (~25 call-site lines), would parse once. That would also settle the free
    functions' names, which the run was unsure of: the alternatives were
    `BuildFrameCount()`/`Width()`/`Height()` (nearer the legacy names) and `CountKeyframeFrames()`
    (parallel to `engine::gfx::CountShapeFrames()`)
- Share the "clip a destination and shift its source" pattern
  - `base/clip.h:47-50` says a caller carrying a source rectangle must use `OutCodeOf()` directly,
    but `DrawShapeFrame()` and `PixelView::RemapLocked()` show `ClipRect()` works whenever only the
    near corner moves the source (source shift = how far that corner moved); fix the comment
  - `PixelView::CopyFromBufferLocked()` (`engine/gfx/pixel_buffer.cc:270-331`) and the clips at
    `pixel_buffer.cc:282`, `370` and `541` still hand-write the outcode clip
- Run `/fix-ide-issues` on `engine/gfx/shape_blitter.h` and `shape_blitter_test.cc`
  - Stage 7 of the `/modernize-file` run (`a5dd8079..b83db634`) was skipped at the user's request
    (the files were not open in CLion); `shape_blitter.cc` was cleared later in `0af052ad`
- Run `/fix-ide-issues` on `engine/gfx/shape.cc`
  - Stage 7 of the `/modernize-file` run (`f4f3ea0b..a9332c6d`) was skipped for it: CLion's
    `getDiagnostics` timed out (`shape.h` answered, with nothing to fix)
- Move `Fixed` (`base/fixed.h`) into namespace `base`
  - The other `base/` headers are in `base::` except `clip.h`, `random.h` and the macros in
    `attributes.h`; `/modernize-file` namespaces only `src/engine` files, so the `base/fixed`
    `/modernize-file` run (`4e995aa2..8fd12531`) left `Fixed` global. 471 lines in 62 Red Alert
    files name it
- Run `/fix-ide-issues` on `base/fixed.h` and `base/fixed_test.cc`
  - Stage 7 of the `base/fixed` `/modernize-file` run (`4e995aa2..8fd12531`): `fixed.cc` answered
    (fixed in `7b451a36`); `fixed_test.cc` timed out every time, and `fixed.h` answered once with
    nothing to fix, then timed out
- Reject a zero cap in `Fixed::SaturateBelow()` (`base/fixed.h`)
  - A cap of 0 wraps to 255 255/256 (documented there); all nine callers pass 1. A `DCHECK_GT` or a
    no-argument `SaturateBelowOne()` would each do; `Saturate()` is likewise only called with 1
- Delete the commented-out `UNIT_BUILD_BIAS` defines at `ra/objects/techno.cc:699-702` (and the
  `// UNIT_BUILD_BIAS;` at `:714`)
- Use `base::At` for the last two `current_object().at(0)` calls
  - `td/editor/mapedsel.cc` (~349, receiver split across lines, so the scripted rewrite missed it)
    and `ra/editor/mapedsel.cc` (~372) compile against `std::vector::at`; both are guarded by an
    earlier empty-selection return, so this is consistency only
- Stop `LayerClass::Delete(const ObjectClass*)` accepting a literal 0
  - `layer.Delete(0)` would compile as `Delete(nullptr)` now the index overload is gone; a deleted
    `Delete(int)` overload in `ra/map/layer.h` and `td/map/layer.h` would reject it
- Move `LayerClass` to `game/`
  - `ra/map/layer.*` and `td/map/layer.*` are identical apart from headers, and since
    `docs/GAME_CONTAINERS_PLAN.md` the class holds a `std::vector<ObjectClass*>` instead of
    inheriting a game-specific vector; it needs `ObjectClass`'s ordering operators, so it would be a
    template over the object type or take a comparator

- Stop `AnimClass` writing into its type
  - The first time an animation plays, `ra/objects/anim.cc` (~500, ~659, ~672, ~753) fills
    `AnimTypeClass::Stages` and `LoopEnd` through `AnimClass::Class`, which is why that is the one
    type pointer left non-const (`docs/CCPTR_REMOVAL_PLAN.md`). Compute them when the type loads its
    image, or make them `mutable` as a lazy cache
- Replace `TeamTypeClass::Number` with a count of live teams
  - Red Alert keeps the number of live teams of a type on the type itself, bumped and dropped
    through `TeamClass::Class` in the `TeamClass` constructor and destructor (`ra/ai/team.cc` ~295,
    ~342); a count over `TheObjectHeaps().team()` would remove the write through the pointer
- Decide whether the two games should share their save-game pointer proxies
  - Red Alert saves heap pointers as slot indices through `game::HeapSlotPtr`
    (`game/core/heap_slot_ptr.h`); Tiberian Dawn has its own `TypePtr`, `HousePtr` and `TeamTypePtr`
    (`td/saveload/serialize.h`), which save type enums and resolve through `As_Reference`. Neither
    format can change without breaking saves, so sharing means one proxy per encoding in `game/`,
    alongside `docs/GAME_NAMESPACES_PLAN.md`
- Decide on the object heap's names (`game/core/object_heap.h`, `e58d9c9d..8b332893`)
  - `ObjectHeap`/`ObjectHeapBase` keep the tree's word "heap" (`TheObjectHeaps()`, `TypeHeaps`,
    `HeapSlotPtr`); `ObjectPool` was the alternative
  - `SlotIndexOf()` replaced `ID()`, but `OwnedVector`, `LayerClass` and `MapClass` still call the
    same idea `ID()`; rename them to match, or accept the difference
  - `ActiveAt(position)` reads the in-use list; `Active()` or `ObjectAt()` were the alternatives
- Make `ObjectHeap<T>::Save()` const
  - It is non-const only because each object's `Serialize()` is one template for reading and
    writing; a const writing path means const-correct `Serialize()` in the ~117 files that define
    one
- Consider keeping slot indices instead of pointers in `ObjectHeapBase`'s in-use list
  - The pointer list makes `Free()` and `PositionOf()` linear searches, and lets the list disagree
    with the in-use flags, which `CheckBookkeeping()` then has to detect. Indices (or an intrusive
    order list) would make that state unrepresentable, at the cost of arithmetic in `ActiveAt()`,
    which every per-frame object loop calls in both games; measure before changing it
- Delete Tiberian Dawn's `#ifdef NEVER` `As_Terrain()` in `td/objects/target.cc` (~349-355)
  - It would not compile: it takes the address of the `void*` a heap subscript used to return
- Fix the `DLOG` lines in `td/startup/init.cc` (~195-211) that name `Units.Set_Heap` and friends
  - Both the globals and `Set_Heap()` are gone; the heaps are `TheObjectHeaps().unit()` and
    `Reset()`
- Consider renaming `ra/core/heap_instances.cc` to `object_heap_instances.cc`
  - It holds the explicit `ObjectHeap<T>` instantiations; its name predates the class name
- Decide whether `ra::Coordinate` and `td::Coordinate` merge into `game::Coordinate`
  - Stage B2 of `docs/COORDINATE_TYPES_PLAN.md` deferred it. The two classes
    (`ra/core/coordinate.h`, `td/core/coordinate.h`) have the same packed layout and members; only
    `x()`/`y()` differ: TD reads each 16-bit word signed (-32768..32767), RA unsigned (0..65535).
    They disagree only for words of 0x8000 or more, which no on-map position has in either game, so
    only wrapped negative offsets and positions just off the top or left edge see the difference
  - TD's sign-sensitive readers: `Distance()`, the `Direction*()` helpers,
    `DisplayClass::Push_Onto_TacMap()`/`Select_These()`/`Coord_To_Pixel()`, `Coord_Move()` and the
    aircraft altitude offset in `AircraftClass::Target_Coord()`/`Fire_Coord()`. A merged class can
    read unsigned like RA and give TD a signed accessor (or `static_cast<int16_t>` at those sites);
    offsets that are only added or negated (`Smooth_Turn`, the track tables) do not care
  - The off-map mask (`kHighCoordMask`, 0x80008000 in RA, 0xC000C000 in TD) depends on the map size
    and stays per game
- Fix the commented-out code that `docs/COORDINATE_TYPES_PLAN.md` rewrote into forms that would not
  compile
  - `ra/net/event.cc` (~546 `Cell cell = 0;`, ~763 and ~778 call the removed `Invalidate()`; its
    equivalent is `= Target::NoneWithAllValueBits()`), `ra/objects/overlay.cc` (~401, a `Cell` loop
    counter), and the `#ifdef OBSOLETE` block in TD's `DisplayClass::In_View()` (`td/map/display.cc`
    ~3564, `cell -= TacticalCell;`). Convert them or delete them with `/remove-dead-code`
  - Restore comments clang-format reflowed during the plan: `ra/net/event.cc` (~639-641, a split
    `if (... ==` / `// RTTI_CELL) {`), `td/editor/mapedplc.cc` (~1159-1160, the `Flag.Composite |=`
    line) and `ra/core/const.h` (~130, a stray `*infantry *` block)
- Rename the lepton-word helpers in `ra/core/packed.h` to the Google scheme
  - `Lepton_Cell()`, `Lepton_Fraction()` and `Make_Lepton()` were new in Part A of
    `docs/COORDINATE_TYPES_PLAN.md` but took the legacy `Underscore_Pascal` names of their
    neighbours; `/rename-google-style` on `packed.h`. While there, decide whether `packed.h` should
    stay re-exported (`// IWYU pragma: export`) by `ra/core/defines.h`, `coord.h` and `inline.h` or
    be included directly by its callers
- Update the EA function index at the top of `ra/core/inline.h`
  - It still lists `Cell_Coord`, `Cell_To_Lepton`, `Coord_Add` and the other helpers that moved to
    `ra/core/packed.h` or became `Coordinate`/`Cell` members; `/update-comments`
- Give the unused `Target`/`Cell` accessors callers or delete them
  - Red Alert: `Target::NoneWithAllValueBits()` (no callers), `Target::AsMovementCoordinate()` (no
    callers) and `Target::cell_index()` (only for `AsMovementCoordinate()`), all in
    `ra/core/target.h`; `Cell::is_none()` in both games has no production callers, since every site
    compares against `Cell::None()`
  - Settle them together with the RTTI_CELL encoding entry under Bugs
- Make the coordinate-type code read the same in both games
  - TD's `Target` decoders repeat `Cell::FromIndex(value())` (`td/objects/target.cc` ~146, ~160)
    where RA has `cell_index()`; RA's radio handlers call `Target::FromPacked(param)` up to four
    times (`ra/objects/aircraft.cc` ~2814-2824, `ra/objects/foot.cc` ~1650-1658) where TD names it
    once (`where`)
  - `ra/core/defines.h` (~213) writes `kHighCoordMask` as `0x80008000` without TD's `U` suffix
  - TD's `Coordinate::XWord()`/`YWord()` (`td/core/coordinate.h`) share one comment;
    `td/objects/unit.cc` (~2118) keeps parentheses left from a nested `Coord_Add()`
  - The `Cell` class comment's example (`ra/core/cell.h` ~29-30) uses `MAP_CELL_W`, which the header
    neither declares nor includes
- Consider making `RadarClass::RadarCell` a `Cell` in both games
  - It is still `int`, so three sites per game wrap it in `Cell::FromIndex()`; check its saved width
    first
- Tidy up after `docs/TEXT_PRINT_SPLIT_PLAN.md`
  - `ra/ai/special.cc` (~306, `PWEditClass::Draw_Text`) finds the gradient font by comparing font
    data pointers; `TextFont == FontType::k6PointGradient` says the same, as TD's skin now does
  - Collapse the double blank line before the `HasGradientFontBits()` comment in `ra/ui/dialog.h`
    (~97) and `td/ui/dialog.h` (~76)
  - `TextButtonClass::Set_Style(font, style)` (`game/ui/textbtn.h`) has no callers; delete it
  - `kButtonFlags` and `kEButtonFlags` (`ra/core/defines.h`) hold the same value; keep one or say
    why there are two
  - `tools/split_text_print.py` still maps `TPF_LASTPOINT` to `FontType::k8Point`, although a print
    with no font used 8-point glyphs without the 8-point spacing; make it refuse such an argument if
    the tool is kept, or delete the tool
  - The plan's Outcome says TD's `& TPF_6PT_GRAD` set is "the first six" fonts; it is indices 2-7
    (8-point, 3-point, LED, VCR, gradient, map)
- Follow up the `ra/core/fading_table_builders` modernization (`71420e43`..`dec8840c`, formerly
  `ra/core/jshell`)
  - Make `PaletteClass::Closest_Color()` (`ra/assets/palette.cc:63`) a free function over
    `engine::gfx::Palette`: it reads only `at()` and `BuildClosestColorFadingTable()` is its one
    caller, which could then drop `PaletteClass` and `ra/assets/palette.h` (4 files, same bytes)
  - Move `TranslucentColor`, `BuildTranslucencyTableWith()` and `BuildTranslucencyTable()` next to
    `TranslucencyTable` in `engine/gfx/fading_table.h`: they use nothing from Red Alert (~5 files,
    same bytes); Tiberian Dawn reads its translucency tables from files, so this is for cohesion,
    not sharing
  - `BuildShadowFadingTable()` and `td::ConquerBuildFadingTable()` (`td/core/support.cc:21`) are one
    mechanism written twice with different rules: RA fades colors 1..240 to the closest of 240..254
    with `PaletteColor::FadedToward()`, TD fades 1..239 to 240..255 with `(diff * (frac / 2)) >> 7`.
    A shared engine builder needs the ranges as parameters and a test that each game's table bytes
    stay the same (~6 files); a plain merge changes the shadows on screen
  - `SpecialGhost` is assembled by hand in both games (`ra/map/display.cc:477`,
    `td/map/display.cc:484`): a zeroed lookup and one fade copied to `subspan(256)`. Either give the
    engine a helper for "every color through one fade", or make it a plain `FadingTable` after
    checking `ra/assets/shape_draw.cc:125`, which passes the whole table
  - `UShadowColsAir` (`ra/map/display.cc`) fills a `target_color` and `fade_ratio` its fade ignores;
    `BuildTranslucencyTableWith()` could take the source colors alone
  - `BuildTranslucencyTable()` and `BuildShadowTranslucencyTable()` differ only in the fade builder
    they call; kept apart for the names at their 6 call sites
  - Names to confirm: `BuildClosestColorFadingTable` (it could instead be folded into the engine's
    `BuildFadingTable()`, which changes `FadingBrighten`/`FadingWayDark` on screen),
    `BuildTranslucencyTableWith` (or an overload of `BuildTranslucencyTable`), and whether the file
    belongs in `ra/assets/` beside `palette.h` instead of `ra/core/`
  - Rename `td::ConquerBuildFadingTable()` and `td::Set_Window()` (`td/core/jshell.cc:84`,
    `td/core/support.cc:21`) when Tiberian Dawn's files get the same pass
  - `ra/startup/startup.cc:365-368` sets the `WINDOW_EDITOR` width and height that the scenario
    editor (`ra/editor/mapedplc.cc:392`, `mapedtm.cc:910`) overwrites before every read; delete the
    two stores
  - The column comments in `engine/gfx/text_window.h:13,15` ("X byte position", "width in bytes")
    are true for Tiberian Dawn only; Red Alert stores pixels
  - `ra/data/udata.cc:66`, `adata.cc`, `bbdata.cc` and `idata.cc` keep `engine/file/disk_file.h`
    under an `IWYU pragma: keep` for a "debug-only loose-file override below" that no longer exists;
    remove the include if the strict build agrees
  - Replace `PaletteClass::COLOR_COUNT` (`ra/assets/palette.h:50`) with
    `engine::gfx::Palette::kColorCount`, which it aliases

- Leftovers from the `game/core/cell_size.h` modernization (`c0677e15..d6422369`, formerly
  `display_constants.h`)
  - Decide whether cells need separate width and height constants. `kCellWidthPixels` /
    `kCellHeightPixels` are both 24 and `kCellWidthLeptons` / `kCellHeightLeptons` both 256, and the
    code already assumes a square cell (`kLeptonsPerPixel` is used on both axes). One `kCellPixels`
    and one `kCellLeptons` would rewrite the 102 uses of the two height constants; the pairs do say
    which axis a line measures. The alternative spelling `kCellPixelWidth` was also considered for
    the current names
  - `Cell::kLeptonsPerCell = 256` (`ra/core/cell.h:104`, `td/core/cell.h:118`, the latter
    `unsigned`) repeats `game::kCellWidthLeptons`. Define it from the shared constant, or leave the
    value classes free of the include; `cell.h` is included almost everywhere
  - `kLeptonsPerPixel` is 256/24 truncated to 10 (exact 10.67). `DriveClass` track stepping
    (`ra/objects/drive.cc:616-650`, `td/objects/drive.cc:621-654`), `FlyClass` (`fly.cc:77`, `79`)
    and RA's `combat.cc:138-140` rely on it, so it is shipped simulation behaviour; changing it
    desyncs. Recorded only so nobody "fixes" it
  - `ra/core/coord.cc:195-196` and `td/core/inline.h:23,34` convert leptons to pixels by hand,
    truncating, where `Lepton_To_Pixel()` / `Pixel_To_Lepton()` round. Check whether the difference
    is wanted before sharing the helper; a change moves what is drawn or refreshed by one pixel
  - Turn the macro constants `EDGE_ZONE` (`ra/map/display.cc:1603`, `td/map/display.cc:1636`) and
    `THREAT_THRESHOLD` (`ra/map/findpath.cc:127`, `td/map/findpath.cc:120`) into `constexpr`
    constants. Expect clang-tidy findings in the expressions that use them: macros hide their
    expansions from it, and this run uncovered `>> 1` on signed values and mergeable nested ifs that
    way
  - Delete the commented-out code that mentions the cell sizes: `ra/objects/combat.cc:202`,
    `ra/objects/techno.cc:1951-1953`, `td/map/display.cc:2593-2594`, `td/map/power.cc:183`,
    `td/objects/drive.cc:883`, `td/objects/techno.cc:1342` (`/remove-dead-code` on each file)
  - `td/map/findpath.cc` may have the nested ifs that were merged in Red Alert's path cost check
    (`c6404158`); it was not flagged, so compare the two by hand

- Leftovers from the `ra/ai/prebuilt_base` and `td/ai/prebuilt_base` modernizations (Red Alert
  `a40289da..694eabde`, Tiberian Dawn `1b228f44..b459d1bc`; formerly `ai/base`, class `BaseClass`)
  - Decide on the name `PrebuiltBase`. It follows the scenario files and the original comments
    ("prebuild base"); `AiBase` and `BaseBuildList` were the alternatives. A change renames the file
    again
  - A malformed `[Base]` entry used to stall the list: `FirstUnbuiltNode()` returned the
    `STRUCT_NONE` node forever and the nodes after it were never built. Both games now skip the
    entry (and one off the map), so such a (custom) scenario builds the rest of its base. No shipped
    scenario is known to have one; recorded in case a mission plays differently
  - Add `Cell::is_on_map()` to `ra/core/cell.h` and use it for the open-coded
    `index() < 0 || index() >= MAP_CELL_TOTAL`: `IsOnMap()` in `ra/ai/prebuilt_base.cc`,
    `ra/saveload/saveglobals.cc:164`, `ra/saveload/iomap.cc:126` (variants at `iomap.cc:381`,
    `ra/objects/crate.h:58`, `ra/map/findpath.cc:168-178`, `ra/map/display.cc:3777`,
    `ra/objects/terrain.cc:404`). About 4 lines plus the call sites
  - `PrebuiltBase::ReadIni()` rejects a node with `type == STRUCT_NONE` and `Serialize()` with
    `!magic_enum::enum_contains(type)`; one file-local `IsValidNode(const BaseNode&)` (about 5
    lines) would give a single definition of a node a building can fill
  - `PrebuiltBase::FindNode()` has one caller left, `ra/editor/mapedsel.cc:542`, which moves the
    node's cell. A `MoveNodeFor(building, cell)` would let it go and leave no way to mutate a node
    from outside
  - `HouseClass::AI_Building()` (`ra/objects/house.cc:5096`) calls `FirstUnbuiltNode()` every frame;
    once the base is complete that scans every node, each with a `dynamic_cast` per non-null
    overlapper in `FindBuilding()` (the run dropped the `What_Am_I()` pre-check as redundant).
    Estimated at microseconds, not profiled; measure before touching
  - Sharing the two `PrebuiltBase` classes in `src/game/` was looked at and not done. About 70 lines
    per game are identical (`FindBuilding()`, `HasNodeFor()`, `FindNode()`, `AddNode()`,
    `RemoveNodeFor()`, `FirstUnbuiltNode()`, the accessors); everything else differs: the node holds
    a `Cell` in Red Alert and a `Coordinate` in Tiberian Dawn (in the save and the INI too), the
    owner rule (`Player=` entry against "the house opposing `[BASIC]` Player"), and the save layout
    (TD has a `BASE` section tag and validates the house). `src/game` may not include either game,
    so the shared part would be a header-only `game::sim::PrebuiltBase<Traits>` (about 130 lines
    plus a test and a traits struct per game) with INI and `Serialize()` still per game: roughly as
    much glue as it removes. Both now read and write through `engine::file::IniFile`
    (`docs/INI_SHARING_PLAN.md`), which removes one difference; until someone revisits it keep the
    twins textually aligned
  - Converting Tiberian Dawn's `BaseNode::coord` to a `Cell` would align it with Red Alert: about 40
    lines in `td/ai/prebuilt_base.{h,cc}`, `td/saveload/saveglobals.cc` and five callers. The saved
    node shrinks from 4 to 2 bytes (`kSaveGameVersion` bump), and the INI must keep writing a packed
    coordinate. It does not by itself enable sharing: `td::Cell` and `ra::Cell` are different
    classes
  - Tiberian Dawn's `FillsNode()` (`td/ai/prebuilt_base.cc`) compares coordinates exactly, so a node
    with sub-cell bits is never built: buildings report whole-cell coordinates
    (`BuildingTypeClass::Coord_Fixup()`). The save self-test in `td/startup/init.cc:1582-1586` adds
    such nodes (`Cell::Center()`), and a `[Base]` entry could. `coord.Whole()` in `AddNode()` would
    fix both in one line, but changes which INI entries count as built; decide first
  - `PrebuiltBase::Serialize()` in `td/saveload/saveglobals.cc` accepts any saved coordinate, while
    `ReadIni()` rejects one off the map and `FindBuilding()` indexes the map with it; Red Alert
    checks in both. The check (`IsOnMap()` in `td/ai/prebuilt_base.cc`) is file-local, and `td/ai`
    may not include `td/saveload`, which is why `Serialize()` lives apart; make it a static member
    or a `Coordinate` method and call it on load
  - `HasNodeFor()`, `FindNode()` and `RemoveNodeFor()` are three spellings of one search in both
    games; one private helper returning the iterator would serve all three (about 10 lines each
    game)
  - Tiberian Dawn's `FindNode()` has one caller left, `td/editor/mapedsel.cc:570`, like Red Alert's
  - The commented-out code and the `obj` declaration noted below for Red Alert's editor have twins
    in `td/editor/mapedplc.cc` (`Build_Base_To()`); check `td/editor/mapeddlg.cc` and
    `td/objects/building.cc` too
  - Delete the commented-out code around the base's callers (`/remove-dead-code` on each file):
    `ra/objects/building.cc:1738-1739`, `ra/editor/mapedplc.cc:1008-1010`,
    `ra/editor/mapeddlg.cc:448-462`
  - `MapEditClass::Build_Base_To()` (`ra/editor/mapedplc.cc:1750`) declares `obj` at the top and
    first assigns it thirty lines down; declare it at its use
- Decide what `;` inside an INI value means (`engine/file/ini_file.cc`, `StripComment()`)
  - `IniFile::Load()` cuts every value at the first `;`. Red Alert always did; Tiberian Dawn's old
    text reader kept it, so a custom mission whose `[Briefing]` or `MISSION.INI` line contains `;`
    now shows truncated in game, and a handle, phone-book entry or init string with one is cut the
    next time settings are loaded. No shipped file of either game has `;` in a value
  - Treating `;` as a comment only at the start of a line or after a blank would fix it, but changes
    what Red Alert hashes for the multiplayer rules check if any of `RULES.INI`'s 177 commented
    values has no blank before the `;`; measure that first
- Finish the `IniFile` clean-up left by `docs/INI_SHARING_PLAN.md`
  - `GetHex()`, `PutHex()`, `SectionName()`, `SectionCount()` and `is_loaded()` have no caller
    outside tests; every `Put*()` and `Clear()` always returns true and could return `void`;
    `PutInt()`'s `format` is a magic 0/1/2. `PKey::Encode_Modulus()` and `Encode_Exponent()`
    (`engine/crypto/pk.cc`) lost their only caller with `PutPublicKey()`
  - `PutString(section, entry, "")` on a missing section creates an empty section, which `Save()`
    writes as a bare header and `Load()` drops, so `SaveWithDigest()` followed by `LoadVerified()`
    reports a mismatch for such a database. Harmless while callers accept a mismatch
  - `PutTextBlock()` and `PutBase64Block()` clear the section and then use `section` again; a
    `SectionName()` pointer passed as `section` dangles after the clear. No caller does that
  - Replacing or clearing an existing name scans its section; rewriting all 4,096 entries of a
    filled section in place is about 16.8 million name compares. Not measured as slow
  - A UTF-8 byte order mark makes the first section header unrecognized; not compared with the old
    readers
  - `/fix-ide-issues` on `ini_file.cc` (CLion timed out during the plan) and `/simplify` on it
- Modernize `ra/data/ini_types.{h,cc}` (`/modernize-file`)
  - The banners still say `CCINI.H`/`CCINI.CPP` and the comments at `ini_types.cc:231`, `:728` and
    `:1246` name `Get_Owners`/`Get_Buildings`
  - About 19 Red Alert headers include `engine/file/ini_file.h` (which pulls in `flat_hash_map`,
    `pk.h`, `sha.h` and three stream headers) for a `Read_INI(IniFile&)` parameter; forward-declare
    `engine::file::IniFile` as Tiberian Dawn's headers and `ra/ai/prebuilt_base.h` do
- Decide whether Red Alert should reject a scenario whose digest does not match
  - `IniFile::LoadVerified()` reports `kDigestMismatch`; `Read_Scenario_INI()` (`ra/ai/scenario.cc`)
    accepts it, as it has since the port. The original refused tampered official scenarios and
    exempted multiplayer maps 1-24

- Leftovers from the `base::Tokenizer` move to `std::string_view` (2026-10-03)
  - Delete the `const char*` overloads of `ParseInteger`, `ParseHex`, `ParseIntegerOr` and
    `ParseHexOr` (`base/strings/number_parse.h`) and of `Fixed::FromString` (`base/fixed.h`) once no
    caller can pass null. They existed for the null token `Tokenizer::Next()` used to return at the
    end of the text; it returns an empty view now
  - Check that no Red Alert type, team type or trigger type can have an empty name. The converted
    `From_Name()`/`As_Pointer()` lookups return none for `""` without scanning, where they used to
    compare it against every entry (Tiberian Dawn's callers were checked)
  - Return a `std::string_view` from `DropListClass::Current_Item()`. It returns null for an empty
    list, so `ra/ai/trigtype.cc` wraps it in a file-local `SelectedName()`
  - Three callees still need a NUL-terminated `const char*`, so their callers pass
    `std::string(token).c_str()`: `IconListClass::Add_Item_Detail()` (`ra/ui/iconlist`, virtual,
    stores the pointer), Tiberian Dawn's `NullModemClass::Send_Modem_Command()`, and the `name`
    parameter of the team and trigger `Fill_In()`, which feeds `Set_Name()`
- Replace `Carryover`'s kind-plus-union with a `std::variant`
  - `ra/ai/carryover.h:69` keeps `rtti_` and a union of `StructType`/`UnitType`/`InfantryType`/
    `VesselType`. A `std::variant<std::monostate, StructType, UnitType, InfantryType, VesselType>`
    with `std::visit` would drop `rtti_` and the three switches that each list all 28 `RTTIType`
    values for `-Wswitch-enum`: the constructor and `Recreate()` in `ra/ai/carryover.cc` and
    `Carryover::Serialize()` in `ra/saveload/saveglobals.cc:81`. `Serialize()` must keep writing the
    `RTTIType` and then the type so the save format holds; check with `tools/ra_saveload_smoke.sh`.
    About 150 lines; `docs/COORDINATE_TYPES_PLAN.md:90` lists the union as a candidate
  - `strength_` (`ra/ai/carryover.h:82`) is an `int` saved as 4 bytes while `ObjectClass::Strength`
    is `int16_t`, so `Recreate()` narrows it back with a cast. Making it `int16_t` changes the save
    format (a `kSaveGameVersion` bump); do it with the variant or not at all
  - The four loops that record the carryover objects in `ra/ai/scenario.cc:950-987` differ only in
    the heap they walk; one helper templated on the heap would replace them (about 30 lines less)
  - Left by the carryover modernization (`9931845f`..`fe837a37`)
- Tidy what the region fold left in `HouseClass` (both games)
  - `Region`/`RegionClass` were folded into `HouseClass::RegionThreat`, a `std::array<int64_t, ...>`
    read through `RegionThreatAt()`; the `region.h` headers are gone
  - `RegionThreat` is saved as 64 bits per region and read through `static_cast<int>`; making it an
    `int` changes the save format (a `kSaveGameVersion` bump)
  - `HouseClass::Adjust_Threat()` (`ra/objects/house.cc`, `td/objects/house.cc`) keeps its Westwood
    banner and the `static const` tables `_val`/`_div` (`_thr` in Tiberian Dawn), which could be
    `constexpr kNeighborOffsets`/`kNeighborDivisors`; it belongs to a pass over `house.cc`

## Testing

- Make `DisplayEventsTest` (`engine/window/display_test.cc`) check `SDL_Init` and pair it with
  `SDL_Quit`
  - A failed init now shows up as a confusing "no events seen" failure
- Unit-test the cursor shape decoding in `engine/window/hardware_cursor.cc`
  - `DecodeShapePixels()` (LCW plus zero-run decode) and `ScaleUpNearest()` are static and reachable
    only through SDL cursor creation; expose them in `engine::window::internal` and cover corrupt
    and truncated data, zero and overlong runs, and each scale
- Replace the tests' private `RecordingSink` copies with `VectorSink`
  (`engine/stream/vector_sink.h`)
  - `engine/stream/archive_test.cc`, `engine/crypto/pk_stream_test.cc`,
    `engine/crypto/blowfish_stream_test.cc`, and `engine/codec/stream_golden_test.cc`,
    `codec_corrupt_test.cc`, `codec_state_test.cc`, `lcw_comp_test.cc`, `stream_error_test.cc`
  - `engine/file/mix_archive_test.cc:95` defines its own class named `VectorSink`
- Check the INI sharing work by hand (`docs/INI_SHARING_PLAN.md`, Outcome)
  - Tiberian Dawn's scenario editor: load SCG01EA, save it under a new name, and compare the two
    files' sorted `section/key=value` lines. `Write_Scenario_Ini()` is reachable only from the
    editor, so no automated run exercises the load → writers → save path, and the unit, infantry,
    building, terrain, aircraft and house writers (which need live objects) have no test
  - Tiberian Dawn's options dialog: change a setting, quit, and check `CONQUER.INI` kept its unknown
    keys; the `DEMO` and Windows-only branches, `Check_From_WChat()` and `Read_Game_Options()` were
    converted by reading only
  - A two-player Red Alert lobby, to confirm both sides compute the same `rules.ini` ID
- Run `IniFileTest.RemovalAcceptsTheNameOfTheElementItRemoves` under ASan
  - It passes without the guard it documents in a normal build: the names are short enough to live
    in the string's inline buffer, which still reads back after the free. `-DENABLE_ASAN=ON` would
    catch it; no CI job builds with ASan
- Check the keyboard and mouse refactors on a real display (headless runs cannot press keys)
  - Keys: typing, Backspace, Enter and Esc in edit boxes and the message line; hall-of-fame name
    entry; battle hotkeys, including a Steam `REDALERT.INI` whose `[WinHotkeys]` holds Windows key
    codes; TD's debug hotkeys (Home, F7-F10, 0-9) and map-editor waypoint keys
  - Clicks: gadget and slider drags; map clicks, drag-select and right-click deselect; the
    multiplayer color pickers; RA's title logo click and Shift-click; TD's side choice and ending
  - Map editor: right-click menu, object and trigger placement, map-size drag handles, the team
    editor's held +/- buttons
  - Iron Curtain and Chronosphere targeting, alone and in a two-player game
  - The hardware cursor after the `SetShape()` rewrite: shapes, hot spots and transparency at 1x, 2x
    and 4x display scale, and its colors through fades
- Turn the `Fixed::FromString()` tests (`base/fixed_test.cc`) into one table of text and raw value
  - About 20 tests each spend five lines checking `whole()` and `fraction()`; one `{text, raw}`
    table checked with `EXPECT_EQ(Fixed::FromString(text), Fixed(raw, 256))` saves about 150 lines.
    Raised by `/simplify` in the `base/fixed` `/modernize-file` run (`4e995aa2..8fd12531`)
- Unit-test `LayerClass`'s ordering
  - `Sort()` is one bubble pass and `Sorted_Add()` inserts before the first object that sorts after
    the new one; both decide draw and logic order, and only the 600-frame headless logic logs cover
    them now. Test a partly sorted layer: one `Sort()` pass, then where `Sorted_Add()` lands

- Play a two-instance network game in each game to check `docs/COORDINATE_TYPES_PLAN.md`
  - The plan changed what `EventClass` packets carry (`Coordinate`, `Target`, `Cell` instead of
    integers) and never ran a real network game; `static_assert`s pin the packet sizes (RA 32, TD 24
    bytes) and `xcheck.sh` matched single-player logs, but nothing exchanged packets. Cover move,
    attack, sell, place (buildings and RA's Chronosphere/Iron Curtain targets) and repair
- Line up the coordinate-type tests between the games
  - RA's `target_test.cc` lives in `ra/objects/`, TD's in `td/core/`; TD's suites are `TdCellTest`
    and `TdCoordinateTest` but plain `TargetTest`; the ctest targets `ra_packed_test` and
    `td_coord_inline_test` now mostly hold the `Cell` and `Coordinate` tests, so their names mislead
  - `ra/core/cell_test.cc` repeats the four layout `static_assert`s that `ra/core/cell.h` already
    has
- Check Red Alert's scenario editor team-type and trigger-type lists on a real display
  - They hold plain pointers since `6f1cc3a2` (`ra/editor/mapedtm.cc`, `ra/editor/mapeddlg.cc`);
    check that both lists are sorted by name, the current entry is selected, and edit and delete
    work. The last open item of `docs/CCPTR_REMOVAL_PLAN.md`
- Find out why some Red Alert scenarios never reach frame 1 when started headless
  - `rasdl -NOMOVIES -NEWGAME<name> -QUITFRAME2` under `SDL_VIDEODRIVER=dummy` hangs (killed by a 60
    s timeout, no output) for SCG03EA, SCG03EB, SCG07EA, SCG10EB, SCG13EA and all 24 SCM multiplayer
    maps; the other 37 campaign scenarios run. The pre-plan binary (`2d6ef8ed`) does the same, so it
    predates `docs/CCPTR_REMOVAL_PLAN.md`
  - Until it is fixed, headless logic logs and save/load smoke tests cannot cover those scenarios
- Check the TextPrintType split on a real display in both games
  - `docs/TEXT_PRINT_SPLIT_PLAN.md` moved the font out of `TextPrintType` into a `FontType`
    parameter of every printer and text widget. The golden tests (`ra_text_style_test`,
    `td_text_style_test`) pin font, spacing and glyph palette per combination, but nobody has looked
    at the screen: RA main menu, options dialog, sidebar tooltips, map editor trigger and team
    dialogs, score screen; TD main menu, map selection, score screen. Text must look as on
    `1f60381e`

## Debugging

- Count the in-use flags in `ObjectHeapBase::CheckBookkeeping()` (`game/core/object_heap.cc`)
  - It checks that every listed pointer is a live slot, but not that every slot flagged in use is
    listed: a flag set without its list entry, or a slot listed twice next to an unlisted one, goes
    unreported by `-CHECKHEAPS`. Comparing `std::ranges::count(in_use_, true)` with `Count()` closes
    it
- Add a Dear ImGui debug overlay to replace the deleted monochrome debug screen
  - `engine/window` presents through `SDL_Renderer` (`engine/window/display.cc`), so the
    `imgui_impl_sdl2` + `imgui_impl_sdlrenderer2` backends drop in: feed `SDL_PollEvent` and draw
    before `SDL_RenderPresent`
  - A cheat-key window would show what the mono pages used to: selected object, house, logic/FPS and
    network queue state, read through accessors instead of a `Debug_Dump` hierarchy

## Other

- Delete the `[WinHotkeys]` section from the `REDALERT.INI` in the local CLion build dirs
  (`cmake-build-debug-ra/src/ra/`, `cmake-build-strict-ra-clang/src/ra/`)
  - An older build saved scancodes there; the hotkeys now load as Windows key codes, so those
    entries come back unbound until the section is removed and the game saves it again
- Build the Rust crate for Emscripten
  - `src/rust` is linked into every target since `docs/RUST_LCW_PLAN.md`, and no CI job builds
    Emscripten; Corrosion needs `Rust_CARGO_TARGET=wasm32-unknown-emscripten` and that target
    installed through rustup
- Decide when to delete the C++ LCW reference
  - `engine/codec/lcw_reference_test_util.cc` exists only for `LcwDifferentialTest` and `lcw_bench`;
    once the Rust kernels have shipped without a report, both can go
- Decide whether the strict build's `-Wnrvo` earns its keep
  - On the 2- and 4-byte trivially copyable `Coordinate`, `Target` and `Cell` it forced odd code:
    `return Cell::FromIndex(cell.index());` (`td/map/display.cc` ~2389) and restructured
    `Set_Cursor_Pos()` and `Calculated_Cell()` in both games, `BuildingClass::Target_Coord()` and
    `VesselClass::Fire_Coord()` in RA. Copy elision buys nothing for such types; either disable the
    warning or accept the shapes
- Note that commit `6475411a` (the object pool collapse) describes two things wrongly
  - Its message calls the in-use list private (it is protected; `TFixedIHeapClass::Ptr()` and
    `Load()` use it) and says "TD's house loader" for what is `TeamClass::Serialize()`'s house check
    in `td/saveload/ioobj.cc`. It cannot be amended without rewriting history; record it in
    `docs/GAME_CONTAINERS_PLAN.md` if anyone needs the correction
- Note the commit-history slips from `docs/COORDINATE_TYPES_PLAN.md`
  - `f0498c21` (Part A, step 1) fails the strict build on its own (four `display.h` includes it left
    unused; `22f80cbc` fixes them), and `f7d220ed` says "117 uses in 27 files" for what was 117
    lines in 29 files. Neither can be amended without rewriting history
