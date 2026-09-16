# Plan: enable `bugprone-signed-bitwise`

Written 2026-09-15 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the row in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

## Context

The row was skipped on 2026-09-12 with 3,715 reports: about 1,800 were OR'd flag enumerators in the
unit, building, infantry and terrain data tables and the rest deliberate bit manipulation. The
review itself named the useful version of the change, "making the flag enums unsigned", and deferred
it to a flag-type redesign. This plan is that version, without the redesign: the flags keep their
names and their layouts, and only their types change.

The check flags every bitwise operator (`& | ^ ~ << >>` and the compound forms) that has an operand
of signed integer type, including the shift count. Since C++20 both shift directions are defined on
signed values (`P0907`), so the check is no longer guarding against undefined behavior; what it
enforces is the rule `CLAUDE.md` already states: bit patterns and flags live in unsigned types, and
`int` is for numbers. The findings therefore split into flag sets stored in `int`, which the project
wants unsigned anyway, and arithmetic written with shift operators, which the project wants written
as arithmetic.

A fresh isolated sweep on 2026-09-15 over the 502 `.cc` translation units with
`IgnorePositiveIntegerLiterals: true` reports **3,363 sites** (TD 2,101, RA 902, tech 172, sdllib
156, winvq 27, base 5), 73 of them the unary `~`. Classified by the flagged operand:

| Operand                                                                   | Sites |
| ------------------------------------------------------------------------- | ----- |
| Flag constant (`HOUSEF_*`, `THEATERF_*`, `STRUCTF_*`, `kHouseFlag*`, ...) | 1,909 |
| Variable or member holding a mask, or a number masked with a literal      | 493   |
| Left operand of a shift                                                   | 328   |
| Shift count (`1 << house`, `1L << Class->Type`, `value >> bits`)          | 230   |
| Call result (`XYP_COORD(x, y)` rows, `Get_Pixel() & 0xFF`)                | 160   |
| Parenthesized arithmetic (`(x + 3) & ~3`, `(Head + 1) & (size - 1)`)      | 83    |
| `~` on a signed constant or variable, and the complemented operand        | 133   |
| `1 << 5` style flag expressions (positive literal shifted by a literal)   | 26    |

The 1,909 constants are 945 `HOUSEF_`, 509 `THEATERF_`, 281 `kTheaterFlag`, 67 `STRUCTF_`, 20
`WWKEY_`, 16 `kHouseFlag`, 16 `OF_`, 16 `EF_` and a tail of `UNITF_`, `AIRCRAFTF_`, `SHAPE_`,
`VQAHDF_`, `kZoneFlag`, `kButton*`, `TPF_` and `BLIT_`.

## Design

Every site takes the first rule that applies.

**1. Flag constants are unsigned (about 1,900 sites from about 130 definitions).** TD's
`1 << HOUSE_X` macros in `td/defines.h` (67 `HOUSEF_`, `THEATERF_`, `STRUCTF_`, `UNITF_`,
`AIRCRAFTF_` definitions) become `inline constexpr` constants of `uint32_t` or `uint64_t`, named the
way RA already names them (`kHouseFlagMulti1`, `kTheaterFlagWinter`, `kStructFlagWeap`); RA's `int`
flag constants (`kHouseFlag*`, `kTheaterFlag*`, `kZoneFlag*`) and the `WWKEY_*_BIT` key modifiers
become `uint32_t`. Fields and parameters that hold those sets follow: RA's
`BuildingTypeClass::Theater` and `HouseClass::Allies`, the `theater_mask` locals and the key
modifier masks. The data-table rows are untouched: an OR of unsigned constants is unsigned.

**2. A bit from an index goes through `base::Bit<T>(index)` (about 150 sites).** `1 << house`,
`1L << Class->Type`, `1U << PlayerPtr->Class->House` and the `1 << spot_index` forms become
`base::Bit<uint32_t>(house)` (or `uint64_t` for the 64-bit scans). The helper in `base/numeric.h`
takes the `int` or enum index, checks it against the width in debug builds, and does the one cast to
an unsigned shift count. Nothing else in the tree casts a shift count.

**3. Arithmetic is written as arithmetic (about 350 sites).** `x >> 1` on a width, height, length or
count is `x / 2`; `x << 1` and `x << 3` are `x * 2` and `x * 8`; `120 << factor` for the resolution
factor is `120 * scale` with the scale computed once. These values are non-negative, so the result
is unchanged. Right shifts of values that can be negative (the fixed-point trig in `base/trig.h`,
the ADPCM decoders, the delay average) are floor divisions and stay as shifts under
`NOLINT(bugprone-signed-bitwise)` with a comment saying so; `/` would round toward zero instead.

**4. Bit patterns live in unsigned types (about 600 sites).** Locals and members that hold a mask or
packed bits (`flags`, `anim_flags`, `index` in the overlay-shape and direction-mask code, the
`code0`/`code1` clip codes, `modemstatus`, `composite`) become `uint32_t`, `unsigned` or the flag
enum's type; packing helpers such as `XYP_COORD`, `XY_Cell` and the cell/target accessors cast their
signed components once and combine in `uint32_t`; a number masked to extract a field
(`Frame & 0x01`, `Cell_Number() & 0x01`, `speed &= 0xFF`) is `% 2` or an explicit `uint32_t` cast at
the extraction; `(x + 3) & ~3` alignments cast the size once or use `base::ToSize`.

**5. `1 << 5` is `1U << 5`.** The save-flag bit tests and the two `1L << (placement + index)`
menu-bitfield probes.

`IgnorePositiveIntegerLiterals: true` is set for the check: `flags & 0xFF` on an unsigned `flags` is
the intended spelling, and the option only exempts the literal, never a signed variable next to it.

**Cascades.** New `uint32_t` flag fields trip `clang-diagnostic-sign-conversion` at every signed use
and `readability-implicit-bool-conversion` where a mask was tested as a condition; both are fixed in
the same pass. Widened compare sites between a flag and an `int` take
`modernize-use-integer-sign-comparison`'s fix-it.

## Steps

1. `base::Bit` with a test; TD's flag macros to unsigned constants (a scripted rename of the uses);
   RA's flag constants, the key modifier bits and their holder fields to `uint32_t`. Re-sweep.
2. The remaining sites by area, in parallel over disjoint file sets: shared libraries (`base`,
   `sdllib`, `tech`, `winvq`), TD gameplay, RA gameplay, both games' UI code.
3. Re-sweep the check alone until quiet; it doubles as the compile check.
4. Full check set over the changed units for the cascades; repeat until quiet.
5. Remove the name from `.clang-tidy` (`hicpp-signed-bitwise` stays excluded: it does nothing on
   LLVM 23 and is recorded as Legacy); strict build; CTest; both smoke scripts with every fixture.

## Verification

```sh
xargs -P 8 -I{} sh -c 'n=$(echo "{}" | sed "s|.*/src/||; s|/|__|g"); \
    clang-tidy -p cmake-build-strict-ra-clang --quiet --checks="-*,bugprone-signed-bitwise" \
    --config="{CheckOptions: {bugprone-signed-bitwise.IgnorePositiveIntegerLiterals: true}}" "{}" \
    > "$OUT/$n.log" 2>/dev/null' < tus.txt   # target: no signed-bitwise lines in any log
```

Then the strict build in the foreground, CTest, `tools/ra_saveload_smoke.sh` (plain and
`--load-fixture`) and `tools/td_saveload_smoke.sh` with every fixture. The smoke scripts compare a
loaded game against a continuous one, so a rounding change in the gameplay arithmetic shows up as a
divergence.
