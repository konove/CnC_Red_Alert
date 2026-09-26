# TODO

## Bugs

- Fix the colour mix-up in both fading-table builders: `Build_Fading_Table`
  (`engine/gfx/fading_table.cc`) and TD's own copy in `td/support.cc`
  - `targetgreen` reads the red gun, and the blue gun is compared against `idealgreen`
  - Predates the engine folders move and is noted in `docs/CLANG_TIDY_PRIORITIES.md`; fixing it
    changes the fade and remap tables, so compare palettes in both games before and after
- Make the Chronosphere's synthetic right-click actually end targeting mode
  - `TechnoClass::Take_Damage()` (`ra/techno.cc`) hands `TheMap().AI()` a right-click event, but no
    map layer acts on one there: the tactical gadget takes real right-clicks before `AI()` runs, so
    it has never worked (see `docs/INPUT_EVENTS_PLAN.md`)
  - The call also runs every map layer once with the mouse at (0, 0); cancel the mode directly, as
    `DisplayClass::Mouse_Right_Press()` does, instead
- Make the numeric keypad's digits type in edit boxes and the message line
  - `KeyBuffer::ToAscii()` returns `'\0'` for keypad keys (their SDL keycodes lie above `'z'`); the
    Windows version mapped them through the virtual-key bit, whose dead branches were removed
- Make typed text honour Shift
  - `KeyBuffer::ToAscii()` maps through `SDL_GetKeyFromScancode()`, which ignores modifiers, so edit
    boxes and chat get no capitals or shifted symbols; doing better needs SDL text-input events

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
  - 9 of the sites genuinely need the lock (`Buffer_Frame_To_Page` subspans `dest.pixels()` without
    locking, so its callers must hold one); the other 22 hold it to batch a render loop, since each
    outer unlock also runs `DropScaledFrame()` and arms `Present(false)`
  - `ra/radar.cc:1036` and `:1044` are early-return unwinds inside an outer lock - the shape a guard
    most obviously fixes
- Make `Display::DropScaledFrame()` and `Display::UpdatePalette()` private
  - Only `Display` itself and the tests call them; the games use `PresentScaledFrame()`
- Tidy `Buffer_Frame_To_Page` (`engine/gfx/2keyfbuf.*`) now the uncompressed-shape cache is gone
  - Its source span can be `std::span<const std::byte>`: the cached-header write was the only write
  - Rename `Do_Old_Blit`/`kBlitOld`; there is no "new" blit path any more
- Remove the blank lines left where the `AllSurfaces.SurfacesRestored` blocks were deleted
  - About 44 `while (process) {` loops now open with a blank line, some sites have two in a row
    (`td/nulldlg.cc`, `td/queue.cc`, `ra/goptions.cc`); `git clang-format` does not touch lines that
    were only deleted
- Finish moving the games' input handling onto `engine::window::InputEvent`
  - The map layers' `AI(InputEvent& event, ...)` still open with `KeyNumber& input = event.key;`,
    and the dialogs copy `input = input_event.key;`: transitional aliases that can read the event
  - `GScreenClass::Input(int& x, int& y)` returns the mouse position through out-parameters that
    none of its six callers (`ra/conquer.cc`, `ra/queue.cc`, `td/conquer.cc`, `td/queue.cc`) uses
- Move `engine/window/ww_mouse.h`'s free functions (`Get_Mouse_X()`, `IsLeftButtonDown()`, ...) into
  `engine::window`
- Delete dead code that still spells the old keyboard API
  - `td/msgbox.cc`'s `#ifdef NEVER` block (C-style `KeyNumber` casts, `& 0xFF` masks)
  - Commented-out `kKeyReleaseBit` tests in `td/edit.cc` and `ra/woledit.cc`

## Testing

- Move `keyframe_test` from `engine/gfx/` to `src/ra/`
  - It compiles `ra/2keyfram.cc` and includes `ra/keyframe.h`, so it tests game code; it is the only
    test that relies on `tools/check_layers.py` exempting tests from the no-games rule
- Make `ww_win_test` (`engine/window/ww_win_test.cc`) check `SDL_Init` and pair it with `SDL_Quit`
  - A failed init now shows up as a confusing "no events seen" failure
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

## Debugging

- Add a Dear ImGui debug overlay to replace the deleted monochrome debug screen
  - `engine/window` presents through `SDL_Renderer` (`engine/window/ww_win.cc`,
    `engine/window/display.cc`), so the `imgui_impl_sdl2` + `imgui_impl_sdlrenderer2` backends drop
    in: feed `SDL_PollEvent` and draw before `SDL_RenderPresent`
  - A cheat-key window would show what the mono pages used to: selected object, house, logic/FPS and
    network queue state, read through accessors instead of a `Debug_Dump` hierarchy
- RA's `BStart`/`BEnd` benchmark instrumentation is write-only since `Benchmarks()` went away
  - Either delete it or make the overlay its reader

## Other

- Delete the `[WinHotkeys]` section from the `REDALERT.INI` in the local CLion build dirs
  (`cmake-build-debug-ra/src/ra/`, `cmake-build-strict-ra-clang/src/ra/`)
  - An older build saved scancodes there; the hotkeys now load as Windows key codes, so those
    entries come back unbound until the section is removed and the game saves it again
