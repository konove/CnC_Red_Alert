# TODO

## Bugs

- Credit Red Alert's kills to the killer, or delete the tally (`b3411636`..`3423029b` replaced
  `CrewClass` with `TechnoClass::kills_` and kept this as it was)
  - `TechnoClass::Record_The_Kill()` (`ra/objects/techno.cc` ~4299) calls `Made_A_Kill()` on the
    object that died, not on `source`, as the original `Crew.Made_A_Kill()` did; Tiberian Dawn
    (`td/objects/techno.cc` ~2915) calls `source->Made_A_Kill()`. So no living RA object ever has a
    kill, and the `rawval + object->kills()` term of `Evaluate_Object()` (~1791) is always zero
  - Calling it on `source` makes the computer prefer targets that have scored kills, which changes
    the simulation (and multiplayer sync against older builds). The other option keeps today's play:
    delete `kills_`, `kills()` and `Made_A_Kill()` (virtual, no override, no other caller) from RA,
    which drops two bytes from each techno in the save game
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
  (`DecodeFrame()` in `engine/gfx/keyframe.cc`)
  - It pads what `LcwUncompress()` returns to the frame size, so a key frame that stops short leaves
    the rest of the frame 0, which draws as transparent; returning `{}` when fewer than width \*
    height bytes were decoded would drop such a frame instead. Only malformed data reaches it (the
    keyframe.h comment documents it). Found by the `ra/2keyfram.cc` `/modernize-file` run
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
    `ra/ai/trigger_action.cc` (~161, ~174), `ra/ai/teamtype.cc` (~2007, ~1841 and ~1937). They agree
    only while the type heaps have no holes, which holds after a fresh INI load; a type deleted in
    the scenario editor leaves one. The original game had the same pairing. Either write
    `SlotIndexOf()` or check that the editor saves its types in slot order
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
    as `Coordinate::FromPacked(c.packed() + n)`: `ai/reinforcements.cc` (`Docking_Coord() + 2`, read
    for its Y cell) and the `-MOBILETEST` fixture in `startup/init.cc` (`+ 0x200`, `+ 0x600`,
    `+ 0x800`). `+ Coordinate::FromLeptons(n, 0)` wraps within X instead
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
- Stop Tiberian Dawn's phone-book reader overwriting the line it is splitting
  - `td/net/mplayer.cc:652` does `base::SafeCopy(buf, tokenptr)` into the buffer being tokenized, so
    the fields after `DialMethod` always read as their defaults
- Give Tiberian Dawn's modem init strings a fallback when `CONQUER.INI` cannot be loaded
  - `Read_MultiPlayer_Settings()` (`td/net/mplayer.cc`) clears the init-string list and returns
    without the "ATZ" default the normal path adds; `td/net/nullmgr.cc:384` then calls `.at(0)` on
    an empty list if someone dials. The old reader had the same shape
  - `Build_Init_String_Listbox()` (`td/net/nulldlg.cc`) sorts the list in place while the stored
    indexes stay, so a saved `InitStringIndex` can name a different string afterwards
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
- Decide whether `HouseKeepsAircraft()` should count the owning house's aircraft
  - `ra/ai/reinforcements.cc:102` (was `Need_To_Take()`) decides whether a reinforcement Yak or MiG
    is kept or is a loaner by subtracting, from the house's airstrips, every kept Yak/MiG with
    `IsOwnedByPlayer`: the human player's, whichever house the new aircraft belongs to. A computer
    house with one airstrip therefore keeps every Yak/MiG it is sent
  - `other->House == aircraft->House` is a one-line change, but it changes which reinforcement
    aircraft leave the map, so it needs a sync check. Found in the reinforcements modernization
    (`ef09a028`..`47b4ae18`)
- Decide which way a reinforcement group should face when it enters the map
  - `DeliverReinforcements()` (`ra/ai/reinforcements.cc:314`) takes the facing from the edge
    (`SourceType * 2`), so a group from the north edge starts facing north, off the map; the
    original comment called it the "facing to enter map", which suggests the opposite was meant.
    Tiberian Dawn's `DeliverReinforcements()` (`td/ai/reinforcements.cc`) does the same
  - A house edge of `SOURCE_AIR` gives 8 there, which `AsFacing()` wraps to north;
    `Calculated_Cell()` treats it as north too, so it is harmless today
  - Turning the facing round changes the first frames of every edge reinforcement in both games
- Check a transport's room before infantry reinforcements are loaded into it
  - `FindHostNear()` (`ra/ai/reinforcements.cc:223`) accepts any vehicle next to the origin waypoint
    with `Max_Passengers() > 0`, and `SendGroupOutOf()` (`:74`) attaches the whole group to it
    without asking how much room is left, so an APC can be made to carry more than its limit for the
    moment before it unloads. Options: skip a full transport in `FindHostNear()`, or load only what
    fits and send the rest over the map edge; both change play
- Bound Tiberian Dawn's reinforcement retries at the map edge
  - `EnterFromEdge()` (`td/ai/reinforcements.cc:121`) retries a member from a neighbouring edge cell
    whenever `Unlimbo()` fails, with no limit. Unlike Red Alert's it does not raise `scenario_init`,
    so `Unlimbo()` runs the can-enter check and a new cell normally cures the failure; a member that
    fails for another reason (the game not active, a failed mark-down) is walked from edge cell to
    edge cell until the walk leaves the map. With the new fixture every edge ended that way, so no
    endless case is known. Red Alert gives up after eight retries (`96a6794c`,
    `ReinforcementTest.GivesUpOnATeamThatNoEdgeCellTakes`); the same bound here is about five lines
    and cannot bind in play, where the cell a retry moves to has just passed the can-enter check
  - Both loops treat cell index 0 as "no cell" (`index() > 0`) where `Cell::None()` is -1
- Decide what Tiberian Dawn's `DeliverReinforcements()` reports when nothing reaches the map
  - It announces the reinforcements and returns true, which removes a volatile trigger, although no
    member is on the map in three cases: every member failed at the edge
    (`td/ai/reinforcements.cc:88`), no beach cell was free for the hovercraft (`:395`), and the
    house's edge is not a place to enter from (`:424`). Red Alert announces only when a member was
    placed
  - By air (`:377`) the result is that of the last aircraft alone: when it fails the team is deleted
    and the campaign returns false although earlier aircraft are on the map, and a multiplayer game
    returns true and announces whatever happened
  - One rule (count the placed members; none means delete the team and return false) is about 30
    lines, but a false result makes the trigger try again and changes what is announced, so it needs
    a sync check. Found in the modernization (`ef839f66`..`0ab14e0a`)
- Decide how Tiberian Dawn picks the way a mixed reinforcement group arrives
  - `arrives_by_hovercraft` (`td/ai/reinforcements.cc:234`) is assigned for each ground transport in
    turn, so a type that lists a hovercraft and then an APC drives in over the map edge with the
    hovercraft in the group; or-ing it would send it to the beach
  - The gunboat test looks at the first member only (`:248`), and a gunboat or hovercraft delivery
    places only the head of its list: `1336193b` deletes the rest, where placing them is the
    alternative
  - A group that flies in loads every member that is not a transport aircraft into the first
    transport (`:331`), an Orca included, and without a transport its ground members are deleted
  - No shipped scenario is known to use such a type; each change alters play for one that does
- Free the special reinforcement's team type in multiplayer when its team cannot be made
  - `DeliverSpecialReinforcement()` (`td/ai/reinforcements.cc:531`) deletes the one-off type only in
    the campaign, so a multiplayer game with a full team pool keeps it for good. `5a68e467` checks
    the pool slot to avoid deleting it twice; a clearer rule is to build the type with `IsTransient`
    off, deliver, then delete it when `TeamClass::Number` for its slot is 0 and switch `IsTransient`
    on otherwise. About ten lines; it changes pool contents in multiplayer, the same on every peer
- Look at the last column in `HasLandRouteFromSouth()`
  - `td/ai/reinforcements.cc:444` scans the south edge up to `MapCellWidth - 1`, so a map whose only
    way in is its last column gets a hovercraft; `Calculated_Cell(SOURCE_SOUTH)`
    (`td/map/display.cc:2183`) scans every column. Including it changes which special reinforcements
    come by hovercraft
- Balance `scenario_init` on the failure paths of `Read_Scenario()` and `Read_Scenario_INI()`
  - `ra/ai/scenario.cc:514` returns false after the `++` at `:463` without the `--` at `:516`, and
    `:2011` returns after the `++` at `:1898` (the `--` is at `:2319`). Both are masked by the
    resets in `conquer.cc:231` and `saveload.cc:1073`. `ra/objects/unit.cc:2704-2711` assigns 1 and
    then 0 instead of counting, which clobbers an outer level. The scoped guard under Refactoring
    fixes all three; Tiberian Dawn's copies were not checked
- Decide whether `TriggerActionType::kDestroyObject` should kill every object its trigger is
  attached to
  - `DestroyObjectsAttachedTo()` (`ra/ai/trigger_action.cc` ~100, TODO at ~95) walks a heap's in-use
    list by index and calls `Take_Damage()`. A unit, aircraft or building that deletes itself on the
    spot closes its gap in the list, and the loop steps over the object that moved into it, so of
    two neighbours attached to the same trigger the second survives. Stepping the index back, as
    `DestroyTriggerAction::Perform()` does (~568), fixes it but changes how missions play and
    desyncs against an unfixed peer. The original game had the same loop
- Decide what a trigger action reports when its team type is missing
  - Since `23f1b91c` the team actions skip a null team type instead of crashing:
    `ReinforcementsAction::Perform()` (`ra/ai/trigger_action.cc` ~527) returns false, so its trigger
    is never used up and retries on every spring, while `CreateTeamAction` and `DestroyTeamAction`
    report success. Reporting true for all three would retire the trigger; one line
- Validate trigger actions when a scenario loads
  - `ReadTriggerActionRecord()` (`ra/ai/trigger_action.cc` ~1005) casts an unchecked number to
    `TriggerActionType`, and nothing checks that the action's waypoint or special weapon exists:
    `base::At(Waypoint, waypoint_)` aborts the game for the default -1 in `DropZoneFlareAction`,
    `RevealAroundAction` and `RevealZoneAction`, and `SuperWeapon.at()` does for `SPC_NONE` in
    `SpecialWeaponAction`. `MakeTriggerAction()` is the one place every action is made, so it could
    log and return a `NoAction` for a record that cannot be performed (a type number that is no
    action already becomes one)
  - `ReadIniEntry()` itself casts four unchecked numbers to `Persistence`, `HousesType` and
    `MultiStyle` (`trigger_type.cc` ~229), and `TEventClass::Read_INI()` (`ra/ai/tevent.cc` ~665)
    does the same for the event. A house of -1 then aborts in `HouseTypeClass::As_Reference()`. A
    shared `base::ParseEnumOr<E>(token, fallback)` over `magic_enum::enum_cast` would serve all
    three readers (about 20 lines plus call sites); it changes behaviour only for a malformed INI
- Range-check the waypoints typed into the trigger editor
  - `TriggerType::Edit()` (`ra/editor/trigger_type_dialog.cc`, the four `WaypointFromLetters()`
    calls) stores whatever the field holds: "ZZ" is 701, a digit is negative, and an empty field is
    -1. `ScenarioClass::Waypoint` has `kWaypointCount` (101) entries, so the game aborts in
    `base::At(TheScenario().Waypoint, waypoint_)` (`ra/ai/trigger_action.cc` ~615, ~638, ~664) when
    the trigger springs
  - Return `std::optional<int>` from `WaypointFromLetters()` (`ra/ai/waypoint_name.h`), rejecting
    empty and non-letter input, and keep the old value in `Edit()` when it is out of range; about 25
    lines plus tests. Editor behaviour only
- Detach a destroyed team type from the teams made from it
  - `TeamClass::Detach()` (`ra/ai/team.cc` ~1372) clears `Target`, `MissionTarget` and `Trigger` but
    never `TeamClass::Class` (`ra/ai/team.h` ~71). The editor deletes team types
    (`ra/editor/mapedtm.cc` ~166) and `HouseClass` does when a house is removed
    (`ra/objects/house.cc` ~3008); a live team of that type is left with a dangling pointer
  - Either `Destroy_All_Of()` before the delete or a clear in `TeamClass::Detach()`; about 5 lines.
    `95332fb6` did the same for the trigger events
- Make `TeamTypeClass::operator new` `noexcept`
  - `ra/ai/teamtype.h` ~148 returns nullptr from a full heap without `noexcept`, so `new` runs the
    constructor at address zero instead of testing the result; `fa858859` fixed the same crash in
    `TriggerType`. Check `TeamTypeClass::Read_INI()` for the null dereference that fix also needed
- Stop `TriggerType::Edit()` changing the house when it is cancelled
  - It turns `HOUSE_NONE` into `HOUSE_GOOD` before the dialog opens
    (`ra/editor/trigger_type_dialog.cc`, above the owner list), because the list cannot show "no
    house". Set it only on OK. Editor behaviour only
- Check the unchecked house and trigger lookups around the trigger code
  - `HouseClass::As_Pointer()` returns null for `HOUSE_NONE` and for a house the scenario lacks.
    Still dereferenced unchecked: `ra/ai/trigger.cc` ~206-212 and `ra/ai/scenario.cc` ~644 (the
    `TriggerActionType::kAllowWin` blockage), and in Tiberian Dawn `td/ai/trigger.cc` ~371-374,
    ~462-469, ~557, ~677-684, ~719, ~746-748, ~868-888, ~941, ~1113
  - `ra/ai/scenario.cc` ~583, ~586 and ~589 push the result of `Find_Or_Make()` into the trigger
    lists unchecked; it is null when the trigger heap is full
  - `MessageListClass::Add_Message()` (`ra/ui/msglist.cc` ~342) copies from a null `txt`

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
    stay re-exported (`// IWYU pragma: export`) by `ra/core/defines.h` and `coord.h` or be included
    directly by its callers
- Finish what dissolving `ra/core/inline.h` left open (`2691ac0a`..`bb4cf4eb`: its helpers went to
  `ra/core/coord.h`, `ra/core/random_pick.h` and `ra/data/text.h`)
  - `TextString()` (`ra/data/text.h`) returns `nullptr` for a text number the table lacks, because
    `Extract_String()` returns an empty view and the function hands out its `.data()`; callers pass
    the result to `SafeCopy`, `"%s"` and `std::string_view` unchecked. TD's `Text_String()`
    (`td/data/text.h:42`) does the same. Returning `std::string_view` fixes both and is the type the
    data wants, but 231 call sites in 39 RA files (189 in TD) feed printers that read to the NUL;
    `UiSkin::Text_String()` in both games and `game/ui/list.h`, `textbtn.h` move with it. A one-line
    stopgap is to return `""` for an empty view
  - `ra/core/random_pick.h` includes `ra/ai/scenario.h` and `ra/map/world.h` for the one expression
    `TheScenario().sync_rng_`, and 29 files include it. A `RandomClass& SyncRandom()` defined in a
    new `random_pick.cc` would leave the header with `base/random.h` alone
  - `LocalRandom()` is a function-local static rather than part of `Game`, so it is never reseeded
    between games or tests; decide whether it belongs in `World` beside `sync_rng_` (not saved)
  - `PointDistance()` in `ra/objects/techno.cc` (~232) repeats the "longer axis plus half the
    shorter" body of `Distance(Coordinate, Coordinate)` in `ra/core/coord.cc:127`; share one
    `constexpr` helper over two `int` deltas
  - Move `kAdjacentCellOffsets` and `kAdjacentCoordOffsets` from `ra/core/const.h` (~154-175) to
    `ra/core/coord.h`: `AdjacentCell()` there and `CellClass::Adjacent_Offset()`
    (`ra/objects/cell.cc:1689`) are their only users, and `coord.h` includes `const.h` just for them
  - `Rescale()` in `ra/core/coord.h` (~99-112) has no test; `/cover-files src/ra/core/coord.h`
  - `game::Desired_Facing256()` returns 0x3F for due east, one step short of `DIR_E` (0x40);
    `ra/core/coord_test.cc` pins it. It is shipped simulation behaviour shared by both games, so
    recorded only so nobody "fixes" it without deciding to
  - `td/core/inline.h` is the same mix (pixel/lepton conversions, `Direction()`, `Adjacent_Cell()`,
    two `Distance()` overloads) and still carries EA banners; `/modernize-file` on it, sending the
    geometry to `td/core/coord.h` as RA did. TD's `Random_Pick` is still a macro over
    `Random_Picky()` with unused `__FILE__`/`__LINE__` parameters (`td/core/rand.h:12-18`)
  - `ra/objects/building.cc:5262-5264` keeps a commented-out `Force_Track(... Adjacent_Cell(...))`
    call; `/remove-dead-code src/ra/objects/building.cc`
  - CLion returned no inspections for `ra/core/coord.h`, `ra/core/random_pick.h` and the three new
    tests (it timed out once); run `/fix-ide-issues` on them with the files open
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
    `Carryover::Serialize()` in `ra/ai/carryover.cc`, `ra/saveload/iomap.cc:126` (variants at
    `iomap.cc:381`, `ra/objects/crate.h:58`, `ra/map/findpath.cc:168-178`, `ra/map/display.cc:3777`,
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
    list, so `ra/editor/trigger_type_dialog.cc` wraps it in a file-local `SelectedName()`
  - Three callees still need a NUL-terminated `const char*`, so their callers pass
    `std::string(token).c_str()`: `IconListClass::Add_Item_Detail()` (`ra/ui/iconlist`, virtual,
    stores the pointer), Tiberian Dawn's `NullModemClass::Send_Modem_Command()`, and the `name`
    parameter of the team and trigger `Fill_In()`, which feeds `Set_Name()`
- Tidy what the region fold left in `HouseClass` (both games)
  - `Region`/`RegionClass` were folded into `HouseClass::RegionThreat`, a `std::array<int64_t, ...>`
    read through `RegionThreatAt()`; the `region.h` headers are gone
  - `RegionThreat` is saved as 64 bits per region and read through `static_cast<int>`; making it an
    `int` changes the save format (a `kSaveGameVersion` bump)
  - `HouseClass::Adjust_Threat()` (`ra/objects/house.cc`, `td/objects/house.cc`) keeps its Westwood
    banner and the `static const` tables `_val`/`_div` (`_thr` in Tiberian Dawn), which could be
    `constexpr kNeighborOffsets`/`kNeighborDivisors`; it belongs to a pass over `house.cc`
- Follow-ups from the `ra/ai/reinforcements.cc` modernization (`ef09a028`..`47b4ae18`)
  - Give `World` a scoped `scenario_init` guard. The counter is raised and lowered by hand at about
    40 sites in 20 files per game (`building.cc` alone has 37 references), nine of them with a
    duplicated `--` before an early return (`building.cc:2428/2431`, `2452/2455`, `2468/2471`,
    `2613/2616`, `2650/2653`, `2760/2763`, `bullet.cc:204/208`, `mapedplc.cc:1789/1792`,
    `crate.cc:155/159`), and `2abc87b6` fixed one that had none. A `World::ScopedScenarioInit`, plus
    a "suspend to 0" variant for `crate.cc:131`, `carryover.cc:54` and `scenario.cc:2286`, makes the
    imbalance impossible; `reinforcements.cc` has five pairs to start with
  - Give `Exit_Object()` an `enum class ExitResult` (refund 0, retry later 1, done 2). It is a
    `virtual int` in `object.h:243`, `techno.h:452`, `building.h:319` and `aircraft.h:169`, with
    about ten return sites, documented only in the banner above `building.cc:2397`; callers switch
    on the literals (`building.cc:5981`), compare with 2 (`reinforcements.cc:77`) or test for
    non-zero (`aircraft.cc:1255`, `house.cc:2815`, which must stay "not refunded"). Both games
  - `CreateGroup()` could return a `std::vector<FootClass*>` (or a struct of transports and riders)
    instead of a list through `ObjectClass::Next`, so that "delete what was not placed" is one loop
    and cannot miss a member (`81867a0a` fixed two that were missed). About 60 lines in four
    functions; keep the reverse creation order, which decides the placement cells
  - `DeliverReinforcements()` gives a team type with no missions an attack-waypoint mission and
    leaves it on the type (`:279`), which is why it takes a non-const `TeamTypeClass*` where
    Tiberian Dawn's takes const. `house.cc:2521` sets its own, so only trigger teams rely on it.
    Applying the default where team types are loaded (`teamtype.cc` ~1912) would let the parameter
    be const but gives every mission-less type the mission: a behaviour change to decide on
  - `LaunchAirStrike()`'s result is ignored by both callers (`house.cc:2556`, `:2569`); it could
    return void, at the cost of the assertion in `ra_reinforcements_test`. Its `DCHECK`s range-check
    two `enum class` values through `static_cast<unsigned>` (`:381`, `:384`)
  - The unload-mission scan in `CreateGroup()` (`:124`) and the three-way ant comparison (`:322`,
    repeated in `unit.cc:1141`) want a `TeamTypeClass::HasMission()` and a `UnitClass` ant predicate
  - `house.cc:2543` holds a commented-out `LaunchAirStrike()` call for the paratroopers
  - CLion's "can be replaced with structured bindings" on `entry` (`:140`) was left: binding
    `TeamMemberClass` by position would break silently if its members were reordered
  - Names to confirm: `DeliverReinforcements()` (or `BringInReinforcements()`), `riders` for the
    non-transport list in `CreateGroup()` (or `passengers`), `FindHostNear()`
- Follow-ups from the `td/ai/reinforcements.cc` modernization (`ef839f66`..`0ab14e0a`)
  - Six of the hand-written `scenario_init` pairs are here (`:157`, `:173`, `:271`, `:284`, `:555`,
    `:570`); they go with the scoped guard above. The two in `FlyIn()` must stay separate:
    `Find_Docking_Bay()` runs between them with the counter lowered
  - One owner for the group still in limbo. `DeliverReinforcements()` has five `DeletePooled(team)`
    and four `DeleteGroup()` sites, one per way of failing; a small `LimboGroup` (push, `PopHead()`,
    a destructor that deletes the rest) passed to `EnterFromEdge()` and `FlyIn()` makes "delete what
    was not placed" impossible to miss, as `1336193b` found four that were. About 40 lines; members
    must still be deleted before the team
  - `Calculated_Cell()` (`td/map/display.cc:2118`) reports "no cell" as cell 0, which callers decode
    as `index() > 0` and `index() == 0`, and `FlyIn()` raises `scenario_init` only to switch off
    `Is_Generally_Clear()` inside it. Returning `std::optional<Cell>` and taking an "ignore
    occupancy" parameter is about 25 lines and six call sites (`unit.cc:1308` among them); the
    `Random_Pick()` calls must stay as they are
  - `HasLandRouteFromSouth()` (`:441`) repeats the scan in `Calculated_Cell(SOURCE_SOUTH)` without
    the random start, and the edge-to-facing factor (`:94`) and `LaunchAirStrike()`'s north fallback
    (`:548`) are edge knowledge too: a `DisplayClass::HasEdgeEntry(SourceType)` and an
    `EdgeFacing(SourceType)` would hold them, about 30 lines
  - `LaunchAirStrike()`'s one caller (`house.cc:2574`) always passes `MISSION_HUNT` and no
    destination and ignores the result; the parameters were kept to match Red Alert's twin
  - `IsUnitType()` (`:66`) could replace the open-coded `What_Am_I()` plus `dynamic_cast` tests in
    `td/objects/house.cc:582-612`
  - `PopHead()` and `FindEdgeCellNextTo()` are copies of Red Alert's; sharing them means templates
    over each game's `FootClass` in `src/game`
  - CLion flags the fixture's `Game game_;` in `reinforcements_test.cc:83` (no message; the member
    is only there for its lifetime), as it would Red Alert's
  - Names to confirm: `DeliverSpecialReinforcement()` (or `DeliverUnplannedReinforcement()`),
    `EnterFromEdge()` and `FlyIn()`, `arrives_by_hovercraft`, `riders`
- Finish what the `ra/ai/special_options` modernization left (`2d3c8713..820577e9`)
  - Decide what to do with `SpecialOptions::three_point_turns` (`ra/ai/special_options.h` ~65):
    nothing in Red Alert reads it, so its check box in `RunSpecialOptionsDialog()`
    (`special_options.cc` ~83) does nothing. Either drop the check box (one file, changes the
    developer dialog) or wire the flag into the unit turning code as TD does (a simulation change)
  - `ore_grows` and `ore_spreads` are written at about 14 sites in `ra/net/netdlg.cc`,
    `ra/net/nulldlg.cc` and `ra/wol/wol_gsup.cc` beside the rules' `IsTGrowth`/`IsTSpread`, and only
    the rules' copies are read. Turning the three unread flags into reserved bits and deleting the
    writers keeps the layout but changes bits 5-7 of the saved byte and of the game-options packet,
    so it needs a multiplayer check
  - The flags stay `unsigned x : 1` (8 bits, `sizeof` 4): `EventClass`'s union and the packets in
    `ra/net/session.h` (~339, ~412) send the struct raw, so `bool x : 1` would be a wire change
  - Replace `SpecialOptions::Reset()` with an initializer at the owner:
    `SpecialOptions special_{.ore_grows = true, .ore_spreads = true};` in `ra/startup/game.h` and no
    call in `ra/startup/startup.cc` (~302). Four files; test-built `Game`s would then start with the
    ore bits set, and `-Wmissing-designated-field-initializers` may object. Default member
    initializers are not an option: `EventClass`'s `Data{}` would set bits 6-7 in every event
  - Split the header: the flag struct is included by about 20 files, the two dialogs by three, and
    none of it is AI. The struct could be a header under `ra/core/` or `ra/data/`, the options
    dialog and `PromptForDifficulty()` (campaign start, no relation to the flags) files under
    `ra/screens/`; about 25 include sites
  - `RunSpecialOptionsDialog()` applies the boxes on Escape as well as on OK (TD's
    `Special_Dialog()` does the same); decide whether Escape should cancel
  - Its `redraw` flag is one-shot like the one `PromptForDifficulty()` lost, but hoisting the draw
    would move it in front of the first `RunFrame()`
  - Give TD's twin the same pass: `td/ai/special.cc` still has `SpecialClass`, `TheSpecial()`, the
    label-keyed `_options` table with its `Setting` mirror, raw `Button` pointers and the
    always-true `if (Button)`
  - Names to confirm: `SpecialOptions` (or `SpecialFlags`), `shroud_regrows` (or `shadow_grows`, as
    in the rules' `IsMPShadowGrow`), `PromptForDifficulty()` (or `AskDifficulty()`)
  - `special_options_test.cc` (~40): CLion warns that the RAII `Installed<>::Scope` local is unused;
    a false positive, the same form as in `ra/saveload/saveglobals_test.cc`
- Return the measurements from `Format_Window_String()` (`ra/ui/dialog.h` ~30, 16 callers)
  - It fills `int& width, int& height` and returns the line count; `PromptForDifficulty()` passes
    two locals it never reads. A small struct `{lines, width, height}` removes them
- Consider a shared modal dialog loop and `ToggleClass::Set(bool)`
  - Every dialog hand-rolls the same `redraw`/`process` loop around `Input()`; nothing in `ra/ui/`,
    `game/ui/` or `ra/screens/` offers one
  - `ToggleClass` (`game/ui/toggle.h`) has only `Turn_On()`/`Turn_Off()`, so callers write the same
    if/else (`ra/screens/sounddlg.cc` ~249-262)
- Finish what the `ra/ai/trigger_type` modernization left (`7e448b91..57fdf1fc`)
  - Rebuild `TriggerType::Edit()` (`ra/editor/trigger_type_dialog.cc`, 1,550 lines) around one
    struct per slot: an event slot (list, house, team, number, building, infantry, aircraft and unit
    gadgets) used twice and an action slot used twice, each with `Show()` and `Store()`. Every block
    is written two or four times today; roughly 400 lines after. Needs a by-hand check in the editor
  - The same file carries dead gadgets: no event needs a waypoint or a time (`Event_Needs()`,
    `ra/ai/tevent.cc` ~720, never returns `NEED_WAYPOINT` or `NEED_TIME`), so the two event waypoint
    fields (`event1_waypoint_edit`, `event2_waypoint_edit`, `kEvent1Waypoint`, `kEvent2Waypoint`),
    their `NEED_WAYPOINT` cases in the redraw pass and on OK, and the `NEED_TIME` labels on the
    event switches cannot run. `fd64bc8a` removed the matching case from `Description()`
  - One list-entry drawing helper in `ra/ui/`. `TriggerType::Draw_It()` (`ra/ai/trigger_type.cc`
    ~276), `ActionChoice::Draw_It()` (`trigger_type_dialog.cc` ~100), `EventChoiceClass::Draw_It()`
    (`ra/ai/tevent.cc` ~176), `TeamTypeClass::Draw_It()` (`ra/ai/teamtype.cc` ~150) and
    `TriggerClass::Draw_It()` (`ra/ai/trigger.cc` ~120) are the same body. The last has no caller:
    no `TListClass<TriggerClass*>` exists, so it and `TriggerClass::Description()` can go
  - Give `TEventClass` a `Detach(Target)` (`ra/ai/tevent.h`) so `TriggerType::Detach()` is four
    symmetric calls instead of reaching into `event1.Team`; about 15 lines, no behaviour change
  - Use `WaypointLetters()`/`WaypointFromLetters()` (`ra/ai/waypoint_name.h`) where the letter
    arithmetic is still written out: `ra/ai/teamtype.cc` ~771, ~1157, ~1221, ~1288, ~1355, ~1463,
    ~1607, ~1678 and `ra/objects/cell.cc` ~1079; about 60 lines removed. `WaypointLetters(-1)` gives
    "@", as the open-coded version did; an empty string would read better
  - One case-insensitive less-than in `base/strings/`. The `lexicographical_compare` over
    `absl::ascii_tolower` is written in `trigger_type_dialog.cc` ~90, `ra/ai/tevent.h` ~220,
    `ra/editor/mapedtm.cc` ~290, `ra/editor/mapeddlg.cc` ~2725 and `ra/wol/rawolapi.cc` ~517
  - A find-by-name on `game::ObjectHeap` (`game/core/object_heap.h`): `TriggerType::FromName()`,
    `TeamTypeClass::From_Name()` and `TeamTypeClass::As_Pointer()` (`ra/ai/teamtype.cc` ~248, ~488)
    are the same loop, and the last two are the same function under two names
  - The `is_active` writes in `TriggerType::operator new`/`operator delete` (`trigger_type.cc`
    ~63-76) look redundant: the member initializer sets the flag, `Allocate()` hands out zeroed
    slots, and the only reader is `Serialize()` on live objects. Pin that with a save/load test,
    then drop them and the comment in `trigger_type.h` ~65
  - `TriggerType::ToIniEntry()` formats with `std::format`, as `ra/ai/tevent.cc` ~643 and
    `ra/ai/trigger_action.cc` ~160 do; CLAUDE.md prefers Abseil. Check `HousesType`'s underlying
    type before `absl::StrCat`, which prints a `char` as a character
  - `TeamTypeClass::Detach()` and `TeamClass::Detach()` ignore their `all` parameter, as
    `TriggerType::Detach()` did (`b3914ac1`); `ra/objects/tracker.cc` ~93-96 is the caller
  - `TeamTypeClass::Description()` and `Member_Description()` (`ra/ai/teamtype.cc` ~1547, ~1594)
    still return a pointer into a static buffer, and `teamtype.h` ~204 still has the
    `operator const char*` that `TriggerType` lost in `022e3617`
  - `TriggerType::WriteIni()` could take the trigger types in a range-for once the heap offers one
  - Names to confirm: the data members have no trailing underscore though `TriggerType` is a class
    (they are public, like `TriggerAction`'s); `MultiStyle` (or `SecondaryMode`); `DestroyAll()`
    (its siblings in `ra/ai/scenario.cc` ~761 are still `Init()`); `event_style` in `Edit()`.
    `Draw_It()` and `Description()` keep their names for `ra/ui/tlist.h` and `drop.h`
  - Run `/fix-ide-issues` on `ra/editor/trigger_type_dialog.cc`, `ra/ai/trigger_type_test.cc` and
    `ra/ai/waypoint_name.{h,cc}` once CLion has reloaded the CMake project: it returned no findings
    for the files this run created, which means not analyzed rather than clean
    (`ActionChoice::Draw_It()` has the by-value parameters it flagged in `trigger_type.cc`). Its two
    "can be `if constexpr`" findings in `trigger_type.cc` are skipped on purpose
- Finish what the `ra/ai/trigger_action` modernization left (`7eeaf347..cd29efc1`, class shape
  `de41dba2..0036d5a1`)
  - Follow-ups of `docs/TRIGGER_ACTION_CLASSES_PLAN.md` (one class per action, done 2026-10-04):
    - `TEventClass` has the shape `TriggerAction` had (one class, a type enum, a shared `Data`
      union) and wants the same plan of its own
    - `ra/ai/trigger_action.cc` grew to about 1,150 lines for 29 classes, where the plan
      expected 700. `BeginProductionAction`, `AllHuntAction`, `FireSaleAction` and
      `AutocreateAction` differ in one line each, as do the classes around a waypoint; decide
      between a `HouseAction` base like `TeamAction`, or splitting the file by payload
    - Check the trigger dialog by hand: it opens an existing trigger with the right widgets filled,
      switching the action type switches the value widget, and OK then reopening shows what was set
    - `TriggerTypeTest.TheShippedTriggersAreReadBackAsWritten` only runs with `RA_SCENARIO_DIR` set
      to a directory of extracted scenario INIs (`mixdump <game-dir> SCG01EA.INI <out>`); it passed
      on the 42 campaign scenarios of the Steam install
    - `TeamTypeClass::operator new` (`ra/ai/teamtype.cc` ~324) is not `noexcept`, so
      `base::NewPooled<TeamTypeClass>()` on a full heap constructs at null and crashes instead of
      returning nullptr as `TriggerType`'s does
  - `tevent.h`'s `TEventClass` (~145) is declared `struct` but reads and writes its INI form and
    draws itself: it becomes a class with private data when `ra/ai/tevent` gets its pass
  - Run `/fix-ide-issues` on `ra/ai/trigger_action_test.cc`: CLion answered for the header and the
    `.cc` (no findings) but timed out on the test after `0036d5a1`
  - Run the same pass on `ra/ai/tevent`: `TEventClass` -> `TriggerEvent`, and `EventChoiceClass`
    with its `EventChoices` table belongs in `ra/editor/trigger_type_dialog.cc` beside
    `ActionChoice` (~75), its only user. The two classes are member-for-member copies; one
    `Choice<E, NameFn>` template would serve both, and `EventChoices` can become a local filled from
    the enum as `action_choices` did (`ee567c17`)
  - `Name_From_Action()`'s successor `ActionName()` and `ActionChoice::Description()` return
    `const char*` because `Conquer_Clip_Text_Print()` (`ra/ui/dialog.h` ~33) takes one; they become
    `std::string_view` when the printer does
  - `TEventClass` and `TeamTypeClass` still have `Build_INI_Entry(std::string&)`;
    `ToIniEntry(const TriggerActionRecord&)` and `TriggerType::ToIniEntry()` return the string
  - `kTicksPerTimerUnit` (`trigger_action.cc` ~84) is also spelled `kTicksPerMinute / 10` in
    `ra/ai/tevent.cc` ~219 and `ra/ai/team.cc` ~688; it belongs beside `kTicksPerMinute` in
    `ra/core/defines.h` ~2207
  - `Assets::tutorial_text()` (`ra/assets/assets.cc` ~74) turns an empty string into nullptr; a
    `std::string_view` where empty means none is cleaner (one caller)
  - Names to confirm: `ActionNeed()` (or `ActionDataNeed()`), `TriggerAction::type()` (or `kind`),
    `TriggerActionType::kDropZoneFlare` (or `kDz`)
  - `ActionChoice::Draw_It()` ignores its index parameter; the signature is `TListClass`'s
    (`ra/ui/tlist.h` ~330), shared by seven item types

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
- Cover the two dialogs in `ra/ai/special_options.cc` (20 of 141 lines covered)
  - `RunSpecialOptionsDialog()` and `PromptForDifficulty()` need the screen, the mouse,
    `ServiceRealTime()`/`RunFrame()` and input. Splitting out the pure parts would cover the logic:
    the prompt trim as `DifficultyPrompt(text, is_expansion_mission)` and the slider's stops and
    level scaling; the loops themselves need a fixture that can queue key presses
- Cover the rest of `ra/ai/trigger_action.cc` (605 of 639 lines covered)
  - `TextAction` with a text (~694) needs `TUTORIAL.INI`; a setter for the tutorial text on
    `Assets`, or loading it from an `IniFile` passed in, would do
  - `PlaySoundAction`, `PlayMusicAction` and `PlaySpeechAction` (~798-840) need the audio device; an
    accessor for `ThemeClass::Pending` would let the music action be asserted
  - `PlayMovieAction` (~773) needs the display and a movie; the player's special weapon button
    (~892) needs the sidebar's game data; `DropZoneFlareAction` (~613) needs the animation types
- Cover the rest of `ra/ai/trigger_type.cc` (79.7% of lines after `7476793a`)
  - `Description()`'s unit, aircraft, building and infantry names (~126-145) call `Text_String()`,
    which needs the text table, and `GameFixture` builds only the house and unit type heaps. A
    fixture that installs a small in-memory text table would reach them
  - `Draw_It()` (~276-303) needs the fonts and the colour remaps; it becomes one test of the shared
    list-entry helper when the five copies are merged
  - The placement `operator new` in `trigger_type.h` ~113 is only used by `ObjectHeap::Load()`; a
    save and load of the trigger type heap through an `ArchiveWriter` would cover it and
    `TriggerType::Serialize()`
  - `ra/editor/trigger_type_dialog.cc` (1,227 lines, 0%) is a modal loop over the display and the
    input queue. The per-slot structs above would leave `Store()` testable without either
  - `TheDescriptionIsEmptyWithoutTheEditor` only runs in a release-version build; the local `.env`
    builds the internal version, where it is skipped

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
- Decide whether the strict build's `-Wnrvo` earns its keep
  - On the 2- and 4-byte trivially copyable `Coordinate`, `Target` and `Cell` it forced odd code:
    `return Cell::FromIndex(cell.index());` (`td/map/display.cc` ~2389) and restructured
    `Set_Cursor_Pos()` and `Calculated_Cell()` in both games, `BuildingClass::Target_Coord()` and
    `VesselClass::Fire_Coord()` in RA. Copy elision buys nothing for such types; either disable the
    warning or accept the shapes
