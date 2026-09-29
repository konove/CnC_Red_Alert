# Plan: keep palettes at 8 bits per gun in memory

Written 2026-09-28, on the tree at 30c71670.

**Status: in progress.** Step 1 done.

The palettes on disk hold 6 bits per gun (0..63, what the VGA DAC took). Both games keep that 6-bit
form in memory, compute fades, slider adjustments and brightened movie palettes in it, and expand to
8 bits only in `Display::SetPalette()`, the last step before SDL. SDL has no 6-bit limit, so every
palette the games _compute_ is quantized to 64 levels for nothing. Loading at 6 bits loses nothing
(the data has no more), but fades have 64 steps instead of 256 and the options sliders round to 1/63
instead of 1/255.

Inventory taken with grep over `src/` on the tree above; counts are sites, not lines.

## Findings

### The one place colors reach the screen

- `engine::window::Display::SetPalette(std::span<const uint8_t>)` (`engine/window/display.cc:240`)
  takes 768 bytes of 6-bit guns and expands each with `gun * 4 + gun / 16`, i.e.
  `(gun << 2) | (gun >> 4)`, so 63 becomes 255. Both games reach it only through their
  `ShowPalette()` (`ra/palette.cc:29`, `td/palette.cc:19`), which also recolor the hardware cursor.
- `PaletteColor::red()`/`green()`/`blue()` expand differently: `gun * 4`, so 63 reads as 252 (the
  `TODO` in `engine/gfx/palette_color.h`). The HSV conversions of `PALETTE_HSV_PLAN.md` use a third
  formula, `DivideWithRound(gun * 255, 63)`, which differs from the display's at 10 of the 64 levels
  (11–15, 48–52) (e.g. 48 → 194 vs 195). Three 6 → 8 expansions for one palette.

### Where palettes come from (all 6-bit)

| Source                          | Sites                                                                                                                                                         |
| ------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Theater `.PAL` files            | RA `ra/init.cc:2323` (TEMPERAT.PAL), `ra/display.cc:435`; TD `td/init.cc:333`                                                                                 |
| Other `.PAL` files              | RA `ra/egos.cc:629` (EGOPAL); TD `td/mapsel.cc:1185,1200`, `td/ending.cc:181,219`                                                                             |
| Palettes inside CPS pictures    | `Load_Uncompress()`/`Load_Picture()` (`ra/jshell.cc:201,251`, TD's copy), ~10 TD callers in `td/ending.cc`, `td/mapsel.cc`, `td/score.cc`                     |
| Palettes inside WSA animations  | `engine/gfx/wsa_animation.cc:122`                                                                                                                             |
| PCX files (8-bit, shifted down) | `ra/nondosstub.cc:231`, `td/nondosstub.cc:258` (`>>= 2`); TD bitmaps `td/winstub.cc:687`                                                                      |
| VQA movie frames                | `VqaFrameView::palette` (`engine/video/vqa/vqa_player.h:67`), masked `&= 63` and brightened in `ra/movie.cc:91`, `td/movie_screen.cc:30`, `td/winstub.cc:511` |
| Palettes built in code          | RA `ra/palettes.h:76` (black/white), `ra/conquer.cc:126` (ember), `ra/sidebar.cc:1183`; TD `td/init.cc:289` (white = 63), black = `0x01`                      |

`ra/vortex.cc:1013`'s `*_VTX.PAL` files are remap tables, not palettes. No palette is serialized in
a save game (the vortex rebuilds its remap tables on load, `ra/vortex.cc:324`) or sent over the
network; step 4 confirms this for TD before switching.

### What assumes 0..63

- **Masks and caps:** fades mask each gun with `& 0x3F` (`ra/palette.cc:57`, `td/palette.cc:39`);
  movie palettes `&= 63`; `Increase_Palette_Luminance(..., cap)` is called with cap 63 at 13 sites
  (`ra/score.cc`, `ra/movie.cc`, `td/mapsel.cc` ×6, `td/score.cc` ×2, `td/ending.cc`, TD movies).
- **Nearest-color searches** build the remap tables from palette distances:
  `engine/gfx/fading_table.cc` (`Build_Fading_Table`), `td/support.cc` (TD's
  `Conquer_Build_Fading_Table`), RA's in `ra/jshell.cc`, the translucent tables in both
  `jshell.cc`s, `ra/vortex.cc`'s `Build_Fading_Table`, the interpolation tables in both
  `interpal.cc`s, `PaletteClass::Closest_Color` (`ra/palette.cc:110`) and
  `PaletteColor::SquaredDistance`. Their arithmetic survives 8 bits (the fading table's `uint16_t`
  products stay below 65,536 and its wraparound is mod 256 either way), but a nearest match can
  change where two candidates are nearly tied. The tables are render-only.
- **8-bit readers of 6-bit guns:** the WOLAPI palette (`ra/wolapiob.cc:3344`, `red()`),
  `ra/installation.cc:255` (tests for black), PCX saving (`engine/gfx/pcx_file.cc:104`, `<< 2`).
- **`PaletteColor(0x3f, 0x3f, 0x3f)`** (`ra/sidebar.cc:1183`) passes 6-bit values to the 8-bit
  constructor, so the cycling colors become grey 15, not white. The original `RGBClass` did the
  same; it only has to keep those entries out of the translucent match, so it is harmless, but it
  should say what it means.

### The palette types

- RA: `PaletteClass` (`ra/palette.h`), 256 `PaletteColor`s, also handed out as raw bytes through
  `bytes()` and implicit `unsigned char*` conversions (13 `bytes()` uses on palettes).
- TD: `std::vector<unsigned char>` of 768 bytes in `ThePalettes()` (`td/palettes.h:56`), plus local
  `unsigned char[768]` arrays; ~20 functions take palettes as `std::span<unsigned char>`
  (`td/score.h`, `td/jshell.h`, `td/support.h`, `td/interpal.h`, `td/winstub.h`, ...).
- Shared engine functions take raw bytes too: `Build_Fading_Table`, `WsaAnimation`, the PCX reader
  and writer, `Display::SetPalette`.

Switching the meaning of the bytes while two games pass them around as untyped `uint8_t` would put
the 6 ↔ 8 conversion at dozens of call sites and make a missed one silent. So the plan first gives
both games one typed palette, then changes what it stores.

## Plan

Each step builds, passes the tests and the save/load smoke tests, and ships on its own. Steps 1–3
change no pixel; step 4 is the switch; step 5 collects the gains.

1. **One 6 → 8 expansion.** Add `engine::gfx::ExpandGun(int gun)` = `(gun << 2) | (gun >> 4)`, the
   display's formula, and use it in `Display::SetPalette()`,
   `PaletteColor::red()`/`green()`/`blue()` (closes that `TODO`; moves the WOLAPI colors and
   `Closest_Color` onto the display's scale) and `PaletteColor::ToHsv()`. Re-pin
   `HsvColorTest.PaletteRoundTripIsWithinOneStep`'s count.
2. **A shared palette type.** Move `PaletteClass`'s storage into `engine::gfx::Palette`
   (`std::array<PaletteColor, 256>` with `at()`, `colors()` and a
   `FromVga(std::span<const uint8_t, 768>)` / `ToVga()` pair for file formats), and have RA's
   `PaletteClass` keep only what is RA-specific (`Set()`, `Adjust()`, `Closest_Color()`). Engine
   functions (`Display::SetPalette`, `Build_Fading_Table`, `WsaAnimation`, PCX) take
   `const Palette&`. Drop the implicit `unsigned char*` conversions.
3. **TD onto `Palette`.** `ThePalettes()`' vectors and the local `unsigned char[768]` arrays become
   `Palette`s; the ~20 raw-span functions take `Palette&`. Loads read into the palette through
   `FromVga()`. Still 6-bit, so pixel-identical; this is the bulk of the mechanical work and could
   go to parallel forks by file.
4. **Store 8 bits.** `PaletteColor` keeps 0..255 (still 3 bytes); `FromVga()` expands with
   `ExpandGun()`, `ToVga()` shifts down; `FromGuns()`/`*_gun()` go. `Display::SetPalette` copies
   straight through. Fix every 0..63 assumption above: the `& 0x3F` and `&= 63` masks go, the
   luminance caps become 255, PCX reads stop shifting down and PCX writes stop shifting up, TD's
   white palette is 255, the sidebar's cycle-color filler says what it means. First confirm no TD
   save or packet carries palette bytes. The screen stays the same for loaded palettes (the display
   already expanded them this way); remap tables may change where nearest matches were near ties.
5. **Collect the precision.** Fades interpolate at 8 bits (256 steps); `Adjust_Palette()` stores
   `HsvToRgb8()`'s result directly and `ToHsv()` reads the guns without scaling, so the scaling
   lambdas and `kMaxGun` go; movie brightening works at 8 bits. Decide whether the neutral-slider
   copy is still worth keeping once the round trip is within 1/255.

## Verification

- Steps 1–3: screenshots of the title screen, a mission and a movie frame in both games match the
  tree before, byte for byte (`SDL_VIDEODRIVER` must be a real driver; headless never loads
  palettes). Unit tests re-pinned only where step 1 changes `red()`.
- Step 4: a scratch program builds every runtime remap table (fading, translucent, interpolation,
  vortex) from the shipped theater palettes before and after and counts changed entries; each change
  is a near-tie to accept or a bug to fix. Headless logic logs and `ra_saveload_smoke.sh` /
  `td_saveload_smoke.sh` unchanged, since palettes are render-only.
- Step 5: the options round trip over all 16,777,216 8-bit colors is within one 8-bit step; a fade
  shows 256 distinct steps.
- Full strict build before each commit.
