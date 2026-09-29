# Plan: one rounding HSV/RGB pair for both games' options palette

Written 2026-09-28, on the tree at 72791611.

**Status: in progress.**

Measured over all 262,144 6-bit colors, converting to HSV and straight back (the options dialog's
neutral settings, where every slider leaves its component unchanged), counting colors whose guns do
not come back exactly. Scratch program built against `base/hsv.h`.

## Findings

- Red Alert's `OptionsClass::Adjust_Palette()` (`ra/options.cc`) runs every palette entry through
  `PaletteColor::ToHsv()` and `HsvColor::ToPaletteColor()`. `ToHsv()` truncates its saturation and
  hue divisions and reads the guns at `gun * 4` (63 → 252, not 255); `ToPaletteColor()` drops each
  gun's low two bits. **127,769** colors drift, by up to two steps (the original code: 76,340).
- Tiberian Dawn's `Rgb6ToHsv()`/`HsvToRgb6()` (`engine/gfx/hsv_color.cc`) scale 6 ↔ 8 bits with
  rounding and round every division: **21,612** drift, by one step at most.
- A hue computed with one rounding instead of three gets to 19,272. The rest is the 8-bit HSV grid
  itself: 256 hues cannot land on every 6-bit color at high value and saturation. Not worth changing
  TD's output for.
- Both games already share the HSV → RGB half (`base::HsvToRgb8()`); only the RGB → HSV half and the
  6 ↔ 8-bit scaling are duplicated, ~45 lines of sextant logic twice, with a third RGB type
  (`Rgb6`) that exists only because TD keeps its palette as raw bytes. `PaletteColor` has the same
  3-byte layout.
- At neutral settings no conversion is needed at all: the adjusted palette is the original.

## Plan

1. **`base::Rgb8ToHsv()`** in `base/hsv.h`, next to `HsvToRgb8()`: `Rgb6ToHsv()`'s 8-bit core
   (everything after the scale-up), returning a `base::Hsv8`. Tests in `base/hsv_test.cc`.
2. **`PaletteColor` speaks guns and rounds.** Add `PaletteColor::FromGuns(red, green, blue)` (each
   0..63) and `red_gun()`/`green_gun()`/`blue_gun()`. `ToHsv()` scales the guns to 0..255 with
   rounding and calls `base::Rgb8ToHsv()`; `HsvColor::ToPaletteColor()` scales `HsvToRgb8()`'s
   result back with rounding. This is exactly TD's pair, so RA drops from 127,769 to 21,612. Test
   that the round trip over every color is within one step.
3. **TD onto `PaletteColor`.** `td/options.cc` builds a `PaletteColor::FromGuns()` from each palette
   entry and writes back the guns of `ToPaletteColor()`. Delete `Rgb6`, `Rgb6ToHsv()` and
   `HsvToRgb6()`. TD's output must not change: check the new pair against the old functions
   exhaustively (every 6-bit color for RGB → HSV, every HSV triple for HSV → RGB) in a scratch
   program before deleting them.
4. **Neutral settings copy the palette.** Both `Adjust_Palette()`s return the original palette
   unchanged when every slider is at its neutral value (RA `fixed::_1_2`, TD `0x80`), so the art is
   exact there. Off neutral the one-step residue remains, spread over a palette already visibly
   adjusted.
5. Remove the TODO.md entry.

## Verification

- `hsv_test`/`hsv_color_test`/`palette_color_test` pass; strict build clean.
- Scratch comparison: new TD path bit-identical to the old one; RA drift count 21,612.
- Both games start and show the options dialog; moving each slider still changes the palette.
