# TODO

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

## Debugging

- Add a Dear ImGui debug overlay to replace the deleted monochrome debug screen
  - `engine/window` presents through `SDL_Renderer` (`engine/window/ww_win.cc`,
    `engine/gfx/pixel_buffer.cc`), so the `imgui_impl_sdl2` + `imgui_impl_sdlrenderer2` backends
    drop in: feed `SDL_PollEvent` and draw before `SDL_RenderPresent`
  - A cheat-key window would show what the mono pages used to: selected object, house, logic/FPS and
    network queue state, read through accessors instead of a `Debug_Dump` hierarchy
- RA's `BStart`/`BEnd` benchmark instrumentation is write-only since `Benchmarks()` went away
  - Either delete it or make the overlay its reader
