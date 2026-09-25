# TODO

## Bugs

- Fix the colour mix-up in both fading-table builders: `Build_Fading_Table`
  (`engine/gfx/fading_table.cc`) and TD's own copy in `td/support.cc`
  - `targetgreen` reads the red gun, and the blue gun is compared against `idealgreen`
  - Predates the engine folders move and is noted in `docs/CLANG_TIDY_PRIORITIES.md`; fixing it
    changes the fade and remap tables, so compare palettes in both games before and after

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

## Testing

- Move `keyframe_test` from `engine/gfx/` to `src/ra/`
  - It compiles `ra/2keyfram.cc` and includes `ra/keyframe.h`, so it tests game code; it is the only
    test that relies on `tools/check_layers.py` exempting tests from the no-games rule
- Make `ww_win_test` (`engine/window/ww_win_test.cc`) check `SDL_Init` and pair it with `SDL_Quit`
  - A failed init now shows up as a confusing "no events seen" failure
- Rename `ra/profile_buffer_test.cc` to `profile_test.cc`
  - It tests `ra/profile.h`; the misleading name keeps clang-format from treating `ra/profile.h` as
    its main include, and it shares a basename with `engine/file/profile_buffer_test.cc`

## Debugging

- Add a Dear ImGui debug overlay to replace the deleted monochrome debug screen
  - `engine/window` presents through `SDL_Renderer` (`engine/window/ww_win.cc`,
    `engine/window/display.cc`), so the `imgui_impl_sdl2` + `imgui_impl_sdlrenderer2` backends drop
    in: feed `SDL_PollEvent` and draw before `SDL_RenderPresent`
  - A cheat-key window would show what the mono pages used to: selected object, house, logic/FPS and
    network queue state, read through accessors instead of a `Debug_Dump` hierarchy
- RA's `BStart`/`BEnd` benchmark instrumentation is write-only since `Benchmarks()` went away
  - Either delete it or make the overlay its reader
