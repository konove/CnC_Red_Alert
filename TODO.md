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
  - `ra/objects/warhead.cc:173-177` reads each missing entry as `Fixed::FromString(nullptr)`, i.e.
    0%, so that armor takes no damage from the warhead; keeping the previous modifier instead would
    change how such a modded rules file plays
  - The default `"100%%,..."` there is a leftover printf escape; it reads as 100% only because the
    percentage parse stops at the first `%`
  - Once the warhead (or `base::Tokenizer::Next()`, returning a `std::string_view`) handles the
    missing token, `Fixed::FromString(const char*)` can go: it exists only for that null
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
- Delete the commented-out code in `ra/wol/wol_logn.cc:439`, `ra/wol/wolapiob.cc:801` and
  `ra/core/jshell.cc:407`
  - `jshell.cc` still builds an `HsvColor` from a palette entry, which no longer compiles
  - The first two still name `Timer<SystemTickSource>` and `SystemMilliseconds()` unqualified, so it
    no longer matches the code around it
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
- Rename `ra/data/profile_buffer_test.cc` to `profile_test.cc`
  - It tests `ra/data/profile.h`; the misleading name keeps clang-format from treating
    `ra/data/profile.h` as its main include, and it shares a basename with
    `engine/file/profile_buffer_test.cc`
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
- Note that commit `6475411a` (the object pool collapse) describes two things wrongly
  - Its message calls the in-use list private (it is protected; `TFixedIHeapClass::Ptr()` and
    `Load()` use it) and says "TD's house loader" for what is `TeamClass::Serialize()`'s house check
    in `td/saveload/ioobj.cc`. It cannot be amended without rewriting history; record it in
    `docs/GAME_CONTAINERS_PLAN.md` if anyone needs the correction
