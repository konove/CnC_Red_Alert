# Plan: return fading and translucent tables by value

Written 2026-10-02, on the tree at c83e30e9.

**Status: in progress.**

The builders of the color-remap tables are C functions: the caller passes a
`std::span<unsigned char>` to fill, and gets the same span back. Every caller fills a fixed-size
static array, so the size is known at compile time and the out-parameter only adds size checks, a
`count` that must match the control table, and (for the translucent builders) a `new[]` branch for
an empty span that no caller takes and that leaks. Return the tables as values instead.

## Findings

- **Fading tables** are 256 bytes, one remap entry per palette color. Builders: the engine's
  `Build_Fading_Table()` (`engine/gfx/fading_table.h`, called by RA `display.cc` and `jshell.cc`, TD
  `display.cc` and `jshell.cc`, and its tests); RA's `Conquer_Build_Fading_Table()` and
  `Make_Fading_Table()` (`ra/core/jshell.h`); TD's `Conquer_Build_Fading_Table()`
  (`td/core/support.h`); and the vortex's private `ChronalVortexClass::Build_Fading_Table()`, whose
  tables are cached in a file.
- **Translucent tables** are a 256-byte lookup (source color → fade index, 255 for "not
  translucent") followed by one fading table per control entry. RA builds eight of them in
  `DisplayClass::Init_For_House()` and one in the sidebar; three of them then have a half
  overwritten (`SpecialGhost`, `UnitShadowAir`, `ClockTranslucentTable`). TD's two translucent
  builders have no callers.
- **Consumers** take `std::span<const unsigned char>` (shape drawing, the remap tables, radar
  scaling), so a `std::array` binds to them unchanged. 38 read sites in RA.
- **`Load_Picture()`** (RA) has one caller, `init.cc`, which decompresses PALETTE.CPS into the
  system-memory page, blits it to the hidden page and reads ~20 pixels per player color with
  `GetPixel()`. The screen pages are only scratch space.

## Plan

1. **Engine.** `using FadingTable = std::array<uint8_t, 256>;` and
   `FadingTable BuildFadingTable(const Palette&, int color, int frac)` in
   `engine/gfx/fading_table.h`; a bad `color` is a `CHECK`, not a silent no-op. Update both games'
   callers and the tests.
2. **RA tables.** `ConquerBuildFadingTable()` and `MakeFadingTable()` return `FadingTable`; the
   translucent builders become templates over the control table's size,
   `TranslucentTable<N> BuildTranslucentTable(palette, const std::array<TLucentType, N>&)` with
   `TranslucentTable<N> = std::array<uint8_t, 256 * (N + 1)>`, dropping `count`,
   `TranslucentTableSize()` and the `new[]` branch. The `DisplayClass` and sidebar statics and the
   control tables become `std::array`s. The vortex builder returns a `FadingTable` too.
3. **RA picture.** `std::vector<uint8_t> LoadPicture(std::string_view name)` returns the decoded
   pixels; `init.cc` reads the remap colors from them instead of going through the screen pages.
4. **TD.** Same as step 2 for `Conquer_Build_Fading_Table()` and the `DisplayClass` statics; delete
   the uncalled translucent builders (and `TLucentType` if nothing else uses it). Decide
   `Load_Uncompress()` case by case: its callers draw the picture on the screen page, so the page
   may really be the destination there.

One commit per step, each verified with the plain build, `ctest`, the strict build and cpplint.

## Verification

- The engine tests in `engine/gfx/fading_table_test.cc` and TD's `core/support_test.cc` keep their
  expectations.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` (they render frames through the
  tables).
- Display check by hand: shadows, the chrono clock overlay, translucent animations, the passability
  overlay in the editor.

## Progress

| Step | Commit | Notes |
| ---- | ------ | ----- |
| 1    |        |       |
| 2    |        |       |
| 3    |        |       |
| 4    |        |       |
