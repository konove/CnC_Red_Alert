# TODO

## Bugs

- Make the Chronosphere's synthetic right-click actually end targeting mode
  - `TechnoClass::Take_Damage()` (`ra/techno.cc`) hands `TheMap().AI()` a right-click event, but no
    map layer acts on one there: the tactical gadget takes real right-clicks before `AI()` runs, so
    it has never worked (see `docs/INPUT_EVENTS_PLAN.md`)
  - The call also runs every map layer once with the mouse at (0, 0); cancel the mode directly, as
    `DisplayClass::Mouse_Right_Press()` does, instead
- Let `HardwareCursor::SetShape()` retry a shape whose SDL cursor could not be created
  - It records `current_shape_` even when `SDL_CreateColorCursor()` fails, so the same shape is
    never tried again and SDL shows its default cursor
  - The hot spot is not checked against the shape size; SDL rejects one outside the cursor
- Decide what `WinX`/`WinY` (`engine/gfx/text_window.h`) should be now nothing writes them
  - The original `Change_Window()` set them to the active text window's origin; since it was stubbed
    (and deleted in `09717739`) they are always 0, so `ra/menus.cc`, `td/menus.cc`, `ra/object.cc`
    and `td/object.cc` place menus and text as if every window sat at the screen's top-left
  - Either delete them (no visible change) or set them from `WindowList` where the window changes
- Pass Red Alert's stored brightness and contrast to `Adjust_Palette()` during grey fades
  - `ScenarioClass::Do_Fade_AI()` (`ra/scenario.cc`) passes `Get_Brightness()` and `Get_Contrast()`,
    the slider positions, but `OptionsClass` stores and applies `1/4 + slider / 2`; off centre, the
    palette's brightness and contrast jump for the length of a Chronosphere/grey fade (slider at 1:
    value times 2 instead of 1.5)
  - Found while unifying the HSV conversions (`docs/PALETTE_HSV_PLAN.md`)
- Decide whether `PaletteColor::red()`/`green()`/`blue()` should read full intensity as 255
  - `ToEightBits()` (`engine/gfx/palette_color.h`, TODO there) is `gun * 4 + gun / 64`, the original
    `(gun << 2) | (gun >> 6)`; the second term is always 0 for a 6-bit gun, so 63 reads as 252.
    `gun >> 4` would copy the top bits down (63 -> 255), as was probably meant
  - Changes `ToHsv()` values (the options palette), `PaletteClass::Closest_Color()` matches and the
    palette `ra/wolapiob.cc` exports
- Make Red Alert's cloak shimmer walk the other way on the right half of the window
  - `CC_Draw_Shape()` (`ra/shape_draw.cc:179`) negates the predator offset when
    `x > WindowList[window][kWindowWidth] * 4`, TD's formula; TD stores window widths in 8-pixel
    units, RA in pixels (`engine/gfx/pixel_buffer.h:361`), so RA's test is "past four window widths"
    and never holds for a visible shape. RA never uses the down-and-left sample offsets
  - Fix: `x > width / 2` in pixels (one line); visual only, not simulation. Found in the
    `shape_blitter` `/modernize-file` run (`a5dd8079..b83db634`)
- Decide what a ghost or fading draw without its table should do
  - `engine::gfx::DrawShapeFrame()` (`engine/gfx/shape_blitter.cc`) silently draws nothing when the
    table a `ShapeFlags::kGhost` or `kFading` draw needs is missing or too short; RA's
    `CC_Draw_Shape()` substitutes `DisplayClass::SpecialGhost`/`FadingShade` first (the "E3 kludge",
    `ra/shape_draw.cc:127-133`), TD's (`td/conquer.cc`) does not
  - Options: give TD the same substitution (small; draws what now vanishes), and/or make a missing
    table a `CHECK` in the blitter instead of a no-op (turns a silent miss into a crash)

## Refactoring

- Migrate legacy integer types to fixed-width types per Google C++ Style Guide
  - Priority: `long`/`unsigned long` first (different sizes on Win64 vs Linux x86_64)
  - Then: `short`/`unsigned short`
  - Convert unsigned number types to signed where appropriate
  - See `docs/TYPE_MIGRATION.md` for full conversion rules
  - Use `/migrate-types <file>` command for per-file migration
- Replace the manual `PixelView::Lock()`/`Unlock()` pairs with an RAII guard
  - 31 sites left in `ra/`, `td/`, `engine/gfx/` and `engine/window/`, in five different shapes:
    unlock inside the `if`, unlock outside it, bare `Lock()` with the result dropped, `Lock()`
    folded into a larger `&&`, and two-view locks with hand-written unwind
  - A `[[nodiscard]] PixelLock` returned by `Lock()`, with `Unlock()` made private, would make the
    miswritten pattern stop compiling; guaranteed copy elision means it needs no move constructor
  - `ra/winbits.h`'s `LockedWindow` is the same idea already hand-rolled for one caller, and would
    fold into it
  - 9 of the sites genuinely need the lock (`DrawShapeFrame` subspans `dest.pixels()` without
    locking, so its callers must hold one); the other 22 hold it to batch a render loop, since each
    outer unlock also runs `DropScaledFrame()` and arms `Present(false)`
  - `ra/radar.cc:1036` and `:1044` are early-return unwinds inside an outer lock - the shape a guard
    most obviously fixes
- Make `Display::DropScaledFrame()` and `Display::UpdatePalette()` private
  - Only `Display` itself and the tests call them; the games use `PresentScaledFrame()`
- Remove the blank lines left where the `AllSurfaces.SurfacesRestored` blocks were deleted
  - About 44 `while (process) {` loops now open with a blank line, some sites have two in a row
    (`td/nulldlg.cc`, `td/queue.cc`, `ra/goptions.cc`); `git clang-format` does not touch lines that
    were only deleted
- Finish moving the games' input handling onto `engine::window::InputEvent`
  - The map layers' `AI(InputEvent& event, ...)` still open with `KeyNumber& input = event.key;`,
    and the dialogs copy `input = input_event.key;`: transitional aliases that can read the event
  - `GScreenClass::Input(int& x, int& y)` returns the mouse position through out-parameters that
    none of its six callers (`ra/conquer.cc`, `ra/queue.cc`, `td/conquer.cc`, `td/queue.cc`) uses
- Delete dead code that still spells the old keyboard API
  - `td/msgbox.cc`'s `#ifdef NEVER` block (C-style `KeyNumber` casts, `& 0xFF` masks)
  - Commented-out `kKeyReleaseBit` tests in `td/edit.cc` and `ra/woledit.cc`
- Delete commented-out code that still calls the mouse functions
  - `ra/gadget.cc`, `ra/init.cc`, `ra/wol_chat.cc`, `ra/wol_logn.cc`, `td/conquer.cc`,
    `td/debug.cc`, `td/gscreen.cc`, `td/init.cc`, `td/scenario.cc`, `td/score.cc`, and
    `// TheMouse()->GrabMouse();` in `ra/winstub.cc` and `td/winstub.cc`
- Move the mouse grab into `Display`
  - Make `mouse_grab_disabled` a `Display` option honoured by `SetMouseGrab()`, drop the static
    `HardwareCursor::GrabMouse()`/`ReleaseMouse()`, and have the startup and shutdown code
    (`ra/startup.cc`, `td/startup.cc`) and the focus handlers (`ra/nondosstub.cc`,
    `td/nondosstub.cc`, `td/winstub.cc`) call `TheDisplay().SetMouseGrab()` directly
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
  - `HasTimeLeft()` (35 sites), `!IsFinished()` (4: `td/infantry.cc:1357`, `td/team.cc:277`,
    `td/drive.cc:894`, `engine/gfx/glow_pulse.h:40`) and `Remaining()` compared with 0 (5:
    `td/mouse.cc:232`, `td/scroll.cc:222` and `:229`, `td/nulldlg.cc:4427`, `td/msgbox.cc:431`)
  - Either drop `HasTimeLeft()` for `!IsFinished()` or convert the other 9 sites to it
- Read `Arm.Remaining()` once in `ra/foot.cc:622`
  - It calls `HasTimeLeft()` and then `Remaining()`, two `Tick()` reads for one answer
- Consider dropping the double check in `CurrentFrame()` (`ra/game_clock.h`, `td/game_clock.h`)
  - It tests `IsInstalled()` and then `Get()` checks the same pointer again on every frame-timer
    `Tick()`; a few ns, worth it only if `base::Installed` gains a get-or-null accessor
- Consider renaming `engine::platform::Timer<T>` to `Countdown<T>` (149 uses)
  - `Stopwatch` and `Timer` are both timers; the name kept by `docs/TIMER_UNIFICATION_PLAN.md` does
    not say which way it counts
- Delete the commented-out code in `ra/wol_logn.cc:439`, `ra/wolapiob.cc:797` and `ra/jshell.cc:420`
  - `jshell.cc` still builds an `HsvColor` from a palette entry, which no longer compiles
  - The first two still name `Timer<SystemTickSource>` and `SystemMilliseconds()` unqualified, so it
    no longer matches the code around it
- Consider stepping the source coordinates incrementally in `ScaleRotateShape()`
  (`ra/shape_draw.cc`)
  - Each destination pixel recomputes `bx`/`by` with four multiplies; adding a constant per x (DDA)
    is cheaper, but the accumulated rounding can move pixels, so check it against a pixel diff of
    rotated and scaled shapes
- Add a shared `engine::gfx` point type or const image view only when a second user appears
  - `engine/gfx/bitmap.h` (`BitmapClass`, `TPoint2D`) was deleted for having one caller;
    `ra/mapsel.cc` has a file-local `Point` and `VqaFrameView` (`engine/video/vqa/vqa_player.h`) is
    a width/height/pixels view that could become the shared one
- Share the options dialogs' brightness, contrast, color and tint math
  - `OptionsClass::Adjust_Palette()` in `ra/options.cc` and `td/options.cc` repeat the same ~25
    lines; only the slider scale differs (RA's `Fixed * 256`, TD's `unsigned char`). One
    `engine/gfx` function over an `HsvColor` and four 0..255 levels would do, best after the
    palette-drift bug above settles on one HSV conversion
- Define the 0..255 HSV scale once
  - `kMaxComponent` = 255 is a local in `base::HsvToRgb8()` (`base/hsv.h`), a file constant in
    `engine/gfx/hsv_color.cc` and `PaletteColor::kMaxComponent`, and `engine/gfx/palette_color.cc`
    still spells it `255` (the fade ratio and the HSV scale); hoist one `base::` constant into
    `base/hsv.h`
- Move the rest of `engine/gfx` into `engine::gfx`
  - Only `palette_color.*`, `hsv_color.*` (`babcd0a9`), `shape_blitter.*` (`b8ab89fe`) and `shape.*`
    (`a9332c6d`) are in it; `glow_pulse.h` and `glow_pulse_test.cc` write
    `engine::gfx::PaletteColor` and `engine::gfx::kBlackColor` and drop the qualifier when they move
- Run `/fix-ide-issues` on `engine/gfx/palette_color.h`, `palette_color.cc`, `palette_color_test.cc`
  and `hsv_color_test.cc`
  - Stage 7 of the `/modernize-file` run (`dc251088..f60d3070`) was skipped: CLion's
    `getDiagnostics` timed out on them (only `hsv_color.h` answered, with nothing to fix)
- Consider small clean-ups in `engine/gfx/palette_color.cc` and `hsv_color.cc`
  - `PaletteColor::FadedToward()` does three float `std::lerp()`s per entry; an integer form is
    cheaper but rounds a falling gun toward its start instead of down, so it changes fades
  - `Rgb6ToHsv()` computes all three shortfalls where each branch uses one (two wasted divides, TD's
    options dialog only), and could take a `const Rgb6&` to mirror `HsvToRgb6()`
  - Names the run was unsure of: `PaletteColor` (alternative `VgaColor`), `FadedToward()`
    (`Blend()`), and `red()` returning the 8-bit value while `red_` holds the 6-bit gun (`red8()`)
- Split `engine::gfx::ShapeEffects::predator_offset` into a start and a direction
  - One `int` carries both: its magnitude mod 8 picks the first sample offset and its sign picks the
    right-hand or the down-and-left table (`engine/gfx/shape_blitter.h`); both `CC_Draw_Shape()`s
    build it as `CurrentFrame()` negated on the right half of the window. A `predator_start` plus a
    `bool` (or the blitter choosing the direction from x and the view width) drops the sign trick.
    Pairs with the RA shimmer-direction bug above
- Consider making color 0 always transparent in `engine::gfx::DrawShapeFrame()`
  - Both callers pass `flags | ShapeFlags::kTransparent` (`ra/shape_draw.cc:191`,
    `td/conquer.cc:2407`) and no other code passes it, so the eight non-transparent entries of
    `kBlitters` (`engine/gfx/shape_blitter.cc`), including the whole-line copy, only run in
    `shape_blitter_test.cc`; dropping the flag halves the instantiations and deletes `kTransparent`
    (~6 files: `shape.h`, `shape_blitter.{h,cc}`, its test, the two callers)
- Move `engine::gfx::ShapeFlags` into `engine/gfx/shape_blitter.h`
  - The flags are `DrawShapeFrame()`'s parameter; `engine/gfx/shape.h` would then hold only the
    shape file format (`ShapeHeader`, `ExtractShapeFrame()`) and could become `shape_file.h`. 66
    files include `engine/gfx/shape.h`, most only for the flags, so it is an include rewrite
- Give the keyframe staging buffers a trivial destructor
  - The two-argument `Build_Frame()` keeps `static std::vector<uint8_t> staging(kShapeBufferSize)`
    (`td/keyframe.cc:150`, from `1818e8af`; `ra/2keyfram.cc:145`, from `c66b4d19`); the strict build
    reports `-Wexit-time-destructors` for the TD one (RA's was not reported, likely cached). A
    `static std::array<uint8_t, kShapeBufferSize>` (40,000 bytes) has no destructor to run; fix both
    games the same way
- Share the "clip a destination and shift its source" pattern
  - `base/clip.h:47-50` says a caller carrying a source rectangle must use `OutCodeOf()` directly,
    but `DrawShapeFrame()` and `PixelView::RemapLocked()` show `ClipRect()` works whenever only the
    near corner moves the source (source shift = how far that corner moved); fix the comment
  - `PixelView::CopyFromBufferLocked()` (`engine/gfx/pixel_buffer.cc:272-313`) and the clips at
    `pixel_buffer.cc:282`, `370` and `541` still hand-write the outcode clip
- Run `/fix-ide-issues` on `engine/gfx/shape_blitter.h`, `shape_blitter.cc` and
  `shape_blitter_test.cc`
  - Stage 7 of the `/modernize-file` run (`a5dd8079..b83db634`) was skipped at the user's request
    (the files were not open in CLion)
- Run `/fix-ide-issues` on `engine/gfx/shape.cc`
  - Stage 7 of the `/modernize-file` run (`f4f3ea0b..a9332c6d`) was skipped for it: CLion's
    `getDiagnostics` timed out (`shape.h` answered, with nothing to fix)
- Decide what `PaletteClass::Adjust()` and `Partial_Adjust()` (`ra/palette.cc:88-103`) should do
  - Both are empty stubs; the `Adjust()` callers (`ra/movie.cc`, `ra/goptions.cc`) only nudge the
    black palette and back, but `ra/egos.cc:325,345` fades the credits with `Partial_Adjust()`,
    which prints its name and does nothing

## Testing

- Move `keyframe_test` from `engine/gfx/` to `src/ra/`
  - It compiles `ra/2keyfram.cc` and includes `ra/keyframe.h`, so it tests game code; it is the only
    test that relies on `tools/check_layers.py` exempting tests from the no-games rule
- Make `DisplayEventsTest` (`engine/window/display_test.cc`) check `SDL_Init` and pair it with
  `SDL_Quit`
  - A failed init now shows up as a confusing "no events seen" failure
- Unit-test the cursor shape decoding in `engine/window/hardware_cursor.cc`
  - `DecodeShapePixels()` (LCW plus zero-run decode) and `ScaleUpNearest()` are static and reachable
    only through SDL cursor creation; expose them in `engine::window::internal` and cover corrupt
    and truncated data, zero and overlong runs, and each scale
- Replace the tests' private `RecordingSink` copies with `VectorSink`
  (`engine/stream/vector_sink.h`)
  - `engine/stream/archive_test.cc`, `ra/heap_test.cc`, `engine/crypto/pk_stream_test.cc`,
    `engine/crypto/blowfish_stream_test.cc`, and `engine/codec/stream_golden_test.cc`,
    `codec_corrupt_test.cc`, `codec_state_test.cc`
  - `engine/file/mix_archive_test.cc:95` defines its own class named `VectorSink`
- Rename `ra/profile_buffer_test.cc` to `profile_test.cc`
  - It tests `ra/profile.h`; the misleading name keeps clang-format from treating `ra/profile.h` as
    its main include, and it shares a basename with `engine/file/profile_buffer_test.cc`
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

## Debugging

- Add a Dear ImGui debug overlay to replace the deleted monochrome debug screen
  - `engine/window` presents through `SDL_Renderer` (`engine/window/display.cc`), so the
    `imgui_impl_sdl2` + `imgui_impl_sdlrenderer2` backends drop in: feed `SDL_PollEvent` and draw
    before `SDL_RenderPresent`
  - A cheat-key window would show what the mono pages used to: selected object, house, logic/FPS and
    network queue state, read through accessors instead of a `Debug_Dump` hierarchy
- RA's `BStart`/`BEnd` benchmark instrumentation is write-only since `Benchmarks()` went away
  - Either delete it or make the overlay its reader

## Other

- Delete the `[WinHotkeys]` section from the `REDALERT.INI` in the local CLion build dirs
  (`cmake-build-debug-ra/src/ra/`, `cmake-build-strict-ra-clang/src/ra/`)
  - An older build saved scancodes there; the hotkeys now load as Windows key codes, so those
    entries come back unbound until the section is removed and the game saves it again
