# Palette Index Type Plan

**Goal:** A color that names a palette slot is an `engine::gfx::PaletteIndex`, not an `int`, a
`uint8_t` or an `unsigned`. Every drawing function, text printer and widget takes one, and the
sixteen `game::k...` colors are of that type, so a pixel count, a shape number or a player-color id
passed where a color is expected does not compile.

**Spec:** T-187, decided 2026-10-11: a strong type, as a small class (not an `enum class`), so it
can carry helpers and checks. This document is the spec.

## Findings

Measured 2026-10-11 on `7c851531`.

- The sixteen `game::k...` constants (`game/ui/standard_colors.h`) are `int` and used 825 times in
  120 files.
- About 220 parameters are colors by name (`color`, `fore`, `back`, `fcolor`, ...) in 87 files; 88
  of them in headers, as `int` (66), `uint8_t` (13) and `unsigned`/`unsigned char` (13).
- 24 data members hold a color. Twelve are the `unsigned char` fields of RA's `RemapControlType`
  (`BrightColor`, `Color`, `Shadow`, `Background`, `Corners`, `Highlight`, `Box`, `Bright`,
  `Underline`, `Bar`), the rest are in `HouseClass`/`HouseTypeClass` (TD), `TextStyle`,
  `PrintStyle`, the help text, the menus and TD's `Session` color tables.
- The engine is already split: `PixelView::PutPixel`, `DrawLine`, `DrawRect`, `FillRect` and `Clear`
  take `uint8_t`, `Print` takes two `int`, `GetPixel` returns `int`, `BuildFadingTable` takes `int`.
  The game side bridges the two with 18 `static_cast<uint8_t>(color)`.
- Calls into those engine functions: `FillRect` 146, `DrawLine` 97, `PutPixel` 36, `DrawRect` 28,
  plus `Clear` and `Print`.
- `game/ui` reaches colors two ways: `Skin::TextColor` (`int` in TD, `RemapControlType*` in RA) and
  `Skin::Gauge_Fill_Color()` (`unsigned char`).
- `game_ui` links `engine_window`, which links `engine_gfx`, so `standard_colors.h` may include an
  `engine/gfx` header.
- The archive serializes integers and enums (`ArchiveScalar`) and classes with a `Serialize()`
  member.

## The type

`engine/gfx/palette_index.h`, target `engine_gfx`:

```cpp
// The number of a palette slot, 0..255: what a pixel of a page holds.
class PaletteIndex {
 public:
  constexpr PaletteIndex() = default;  // Slot 0.
  constexpr explicit PaletteIndex(uint8_t value) noexcept;

  // For a color that was computed: CHECKs 0..255.
  static constexpr PaletteIndex FromInt(int value);

  [[nodiscard]] constexpr int value() const;
  [[nodiscard]] constexpr uint8_t byte() const;  // To store in a pixel or table.

  // Slot 0, which a transparent blit and a text background skip.
  [[nodiscard]] constexpr bool IsTransparent() const;

  // This color as `table` (a remap or fading table) redraws it.
  [[nodiscard]] PaletteIndex RemappedBy(std::span<const uint8_t, 256> table) const;

  friend constexpr bool operator==(PaletteIndex, PaletteIndex) = default;

  template <class Archive>
  void Serialize(Archive& ar);  // One byte, as the uint8_t it replaces.
};
static_assert(sizeof(PaletteIndex) == 1);
```

No ordering, no arithmetic: `color + 1` and `color < 16` are written through `value()` where the
code means them. `Palette::operator[]` gains a `PaletteIndex` overload.

## What does not change

- **Pixel data.** `std::span<uint8_t>` pixels, shape data, remap tables, fading tables and
  `RemapControlType::RemapTable`/`FontRemap` stay bytes. The type is for one named color crossing an
  interface, not for bulk data.
- **Player colors.** `PlayerColorType`, `Session::color_index_`/`preferred_color_`, the `Color`
  field of the network node and global-packet structs and the `[MultiPlayer] Color` INI key are ids
  into a table of schemes, not palette slots.
- **Option values.** `OptionsClass::Color` (TD) is the saturation setting.
- **The save and network formats.** A `PaletteIndex` is one byte where an `unsigned char` was; an
  `int` member that is saved keeps its width through an explicit `ar` of `value()` or moves to the
  one-byte form with a `kSaveGameVersion` bump and a regenerated fixture. Decide per field in Task
  5/6; the save-fixture tests catch a miss.

## Approach

Compiler-driven, through one transitional crutch. `PaletteIndex` starts with an implicit
`operator int`, marked as transitional. With it the constants can change type first without breaking
a single use, and each signature can then change on its own: a `PaletteIndex` parameter rejects an
`int` argument at once (the constructor is explicit), while a `PaletteIndex` argument still reaches
an `int` parameter that has not been converted yet. Removing the operator in the last task turns
everything that was missed into a compile error.

The engine functions get `PaletteIndex` overloads beside the `uint8_t`/`int` ones, so both games
keep building while they migrate; the old overloads go when no caller is left. To count what still
uses the operator or an old overload at any point, tag it `[[deprecated]]` in an overlay header and
compile every TU `-fsyntax-only` (as for earlier member-caller sweeps).

## Global constraints

- Every commit builds with GCC (`build/`) and the strict clang build, and passes `ctest` in both.
- Drawing is unchanged: `tools/ui_check.py` passes without `--accept` after Tasks 3-7.
- `ra_save_fixture_test` and `td_save_fixture_test` pass; where a saved field changes width, bump
  `kSaveGameVersion` and regenerate the fixture in the same commit.
- `tools/net_check.py` passes after Task 5.
- A literal becomes a named constant where one exists (`0` meaning transparent black is
  `game::kTransparentBlack`), otherwise `PaletteIndex{n}`; a computed value goes through
  `PaletteIndex::FromInt()`. No `static_cast<uint8_t>(color)` survives at a call into the engine.
- Format with `git clang-format -f -- <files>`. No commit trailers.

## Tasks

Tasks 2 and 3 were committed in the opposite order: a `PaletteIndex` constant passed to a `uint8_t`
parameter narrows from `int` under clang, so the engine overloads had to be there first.

1. **The type.** `engine/gfx/palette_index.h` with the transitional `operator int`,
   `palette_index_test.cc`, the `Palette::operator[]` overload. One commit.
2. **The constants.** `game::k...` become `inline constexpr engine::gfx::PaletteIndex`; fix what the
   operator does not cover (`switch` labels, template deduction, `std::min`/ternaries mixing a
   constant with an `int`, array initializers of `int`). TD's `kCcOrange`/`kCcTan` and the other
   per-game named colors in `defines.h` follow. One commit.
3. **The engine.** `PaletteIndex` overloads of `PutPixel`, `Clear`, `DrawLine`, `DrawRect`,
   `FillRect`, `Print` (and their `Locked` forms); `GetPixel` returns `PaletteIndex`;
   `BuildFadingTable` takes one; `text_window.h`'s `kWindowFCol`/`kWindowBCol`. Engine tests move to
   the new overloads. One commit.
4. **`game/ui`.** `Skin::Gauge_Fill_Color()` returns `PaletteIndex`; TD's `Skin::TextColor` becomes
   `PaletteIndex` (RA's stays `RemapControlType*`). One commit for both games, as the templates are
   shared.
5. **Red Alert.** Bottom-up: `RemapControlType`'s color fields; `ui/dialog.h` (`TextStyle`,
   `PrintStyle`, `TextStyleFor`, `DrawColorBox`); the printers; `Cell_Color`, `Closest_Color`,
   `FindColor`; radar, power, help, sidebar, vortex; screens, net and editor dialogs; `wol/`. Split
   into commits by folder where a signature does not cross folders.
6. **Tiberian Dawn.** The same order: `HouseClass`/`HouseTypeClass` `Color`/`BrightColor`,
   `Session::graphic_colors_`/`text_colors_`, `ui/dialog.h`, `core/support.h`, menus, help, radar,
   sidebar, the rest.
7. **Remove the crutch.** Delete `operator int` and the `uint8_t`/`int` engine overloads; fix what
   the compiler then finds; delete the "`int` because that is what the drawing functions take"
   comment in `standard_colors.h`; add `PaletteIndex` to CLAUDE.md's Key Files (Graphics) and to the
   Integer Types section as the type for a color. Close T-187.

## Review focus

1. **A player-color id turned into a palette index** (or the reverse). The two are both small
   numbers named `color`; RA's `PlayerColorType` indexes `ColorRemaps`, TD's indexes
   `Session::graphic_colors_`. Follow each value to where it is drawn before typing it.
2. **Sentinels.** `ColorListClass::SelectColor{-1}` and any `color == -1` test mean "none". A
   `PaletteIndex` has no -1: such a value becomes `std::optional<PaletteIndex>`, or stays an `int`
   if it is an id.
3. **Truncation that was relied on.** An `int` color above 255 used to wrap at the `uint8_t`
   parameter; `FromInt()` now CHECKs. Each `FromInt()` call site needs a reason the value is in
   range, or a mask written out where wrapping was the intent.
4. **Background 0.** `Print`'s `back_color` of 0 means "leave the background", which is
   `IsTransparent()`, and `Clear()`'s default stays slot 0.
5. **Saved widths.** See "What does not change".
