# Coordinate Types: Unions Out, Then Strong Types

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development
> (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the type-punning unions behind Red Alert's map and target words
(`LEPTON_COMPOSITE`, `COORD_COMPOSITE`, `CELL_COMPOSITE`, `TARGET_COMPOSITE`), then replace the
primitive aliases `CELL`, `LEPTON`, `COORDINATE` and `TARGET` in both games with real types.

**Architecture:** Two parts that ship independently. Part A rewrites every union access as shifts
and masks in one small header, pinned by golden-value tests written against the unions first. Part B
turns each alias into a trivially copyable class with named factories and accessors (or, for
`LEPTON`, into plain `int`), one alias per stage, driven by compiler errors.

**Tech stack:** C++23, GoogleTest, the strict clang build, `tools/strict_tu.py`, the save/load smoke
scripts.

## Findings

Measured 2026-10-02 on `d7594d42`.

### The unions

| Union              | Declared                | Uses                                                                          |
| ------------------ | ----------------------- | ----------------------------------------------------------------------------- |
| `LEPTON_COMPOSITE` | `ra/core/defines.h:277` | `ra/core/inline.h` (2), `ra/core/coord.cc:59-62` (refresh-list rect)          |
| `COORD_COMPOSITE`  | `ra/core/defines.h:286` | `ra/core/inline.h` (19), `ra/core/coord.cc:255` (`Coord_Cell`)                |
| `CELL_COMPOSITE`   | `ra/core/defines.h:294` | `ra/core/inline.h` (4), `ra/core/coord.h:36` (`Direction`), `coord.cc:257`    |
| `TARGET_COMPOSITE` | `ra/core/defines.h:320` | `Build_Target`, `ra/objects/target.h` (`Target_Kind`/`Value`, `xTargetClass`) |

All of them are `std::bit_cast` views that depend on implementation-defined layout: bit-field
allocation order and little-endian byte order. Tiberian Dawn already does the same packing with
shifts in `td/core/inline.h:55-185`, so Part A is RA catching up with TD.

The bit-fields also _truncate_, and callers rely on it. The shift versions must mask identically:

- `XY_Cell(x, y)` keeps 7 bits of each (`XY_Cell(-1, 0) == 127`).
- `Cell_X`/`Cell_Y` of the sentinel `-1` read 127/127.
- `Cell_To_Lepton(300)` keeps the low cell byte (`0x2C00`).
- `Build_Target`, `TargetClass(CELL)` and friends keep 24 mantissa bits and 8 exponent bits, so a
  negative value becomes `0xFFFFFF`.
- `Coord_Cell` keeps 7 bits of each cell byte (an X cell byte of `0x80` becomes column 0).

### The aliases

| Alias        | RA                   | TD                   | Width / meaning                                             |
| ------------ | -------------------- | -------------------- | ----------------------------------------------------------- |
| `CELL`       | 756 uses in 92 files | 607 uses in 81 files | `int16_t`; RA packs 7+7 bits (128²), TD 6+6 (64²)           |
| `LEPTON`     | 121 uses in 27 files | — (TD uses `int`)    | `uint16_t`; 256 per cell                                    |
| `COORDINATE` | 409 uses in 73 files | 352 uses in 70 files | `uint32_t`; X word low, Y word high; same layout both games |
| `TARGET`     | 316 uses in 65 files | 281 uses in 55 files | RA `int32_t` 8+24 bits; TD `uint16_t`, different split      |

To size Part B, each RA alias was replaced in a scratch worktree by `struct X { T v; };` (no
conversions, no operators) and all 207 RA translation units were compiled `-fsyntax-only` with the
strict compile database. Unique error sites:

| Alias        | Error sites | Files | Of which comparisons (`==` `!=` `<` ...) | Arithmetic (`+` `-` `*` `/`) |
| ------------ | ----------- | ----- | ---------------------------------------- | ---------------------------- |
| `CELL`¹      | 524         | 62    | 151                                      | 101                          |
| `COORDINATE` | 170         | 41    | 34                                       | 0 (`&`, `\|`: 8)             |
| `LEPTON`     | 176         | 26    | 27                                       | 48                           |
| `TARGET`     | 155         | 30    | 94                                       | 8                            |

¹ With `MAP_CELL_W/H/TOTAL` already turned into `int`; leaving them as `CELL` adds 1,700 cascade
errors. Counts are a floor: an error in a header hides later uses in the same TU.

A defaulted `operator<=>` removes every comparison error, leaving roughly 370 CELL, 140 COORDINATE
and 60 TARGET sites to edit by hand in RA, and a similar number in TD. Most remaining CELL sites are
`Map[cell]` indexing, `cell + offset` neighbour arithmetic (`cell + MAP_CELL_W`, refresh-list
offsets) and the `-1` sentinel (default arguments in `ra/map/display.h:179,195`).

### Constraints

- **Network packets copy `EventClass` as raw bytes.** Its data union (`ra/net/event.h:144`,
  `td/net/event.h`) holds `CELL`, `COORDINATE` and target fields, so every replacement type must be
  trivially copyable, trivially default constructible (no constructor, no default member
  initializer) and keep its size. `xTargetClass` already lives under that rule.
- **Saves are field-wise.** `ArchiveWriter::Field` calls a class's own `Serialize()`, so a class
  that serializes its packed word keeps the on-disk layout. Any layout change (for example `LEPTON`
  fields widening to `int`) bumps `kSaveGameVersion` (`ra/saveload/saveload.h:21`) and TD's
  equivalent.
- **No new `using` aliases.** Per-game classes (`ra::Cell`, `td::Cell`) rather than a shared
  template with an alias per game.

### Out of scope

The other 18 unions in the tree are not map words and want different tools: tagged event and trigger
payloads (`ra/net/event.h`, `ra/ai/taction.h`, `ra/ai/tevent.h`, `ra/ai/teamtype.h`,
`ra/net/session.h` ×4, `ra/ai/carry.h`, the TD counterparts) are `std::variant` candidates; the cell
occupancy `Flag` (`ra/objects/cell.h:207`, `td/objects/cell.h:175`) is a bit-flag set;
`engine/codec/base64.cc` and `engine/crypto/blowfish.cc` are byte views for `std::bit_cast`. Each is
its own plan.

## Global constraints

- Behaviour stays bit-identical: the RA/TD save/load smoke tests and a multiplayer event round trip
  must match before and after every stage.
- Packed layouts are pinned with `static_assert`s (`sizeof`, `std::is_trivially_copyable_v`,
  `std::is_trivially_default_constructible_v`, `std::is_standard_layout_v`).
- New code follows CLAUDE.md: Google naming, `#ifndef` guards matching the path, header comments on
  every declaration, no EA copyright header.
- Each task ends with `tools/strict_tu.py` on the files it touched; each stage ends with a full
  strict build (`--parallel 14`), `ctest`, and both smoke scripts.

## Review focus

1. Negative or out-of-range components passed to the packers (`XY_Cell(-1, y)`,
   `Cell_To_Lepton(300)`) must truncate exactly as the bit-fields did — pinned in Task A1.
2. The `-1` CELL sentinel and `kRefreshEol` read through `Cell_X`/`Cell_Y` — pinned in Task A1.
3. Signed component arithmetic wrapping at 16 bits (`Coord_Add`, `Coord_Sub`, `Coord_Mid`) — pinned
   in Task A1.
4. Targets built from negative values (`Build_Target(kind, -1)`, `TargetClass(CELL)` with a sentinel
   cell) — pinned in Task A1 and Task A3.
5. `sizeof(EventClass)` and the save layout unchanged — `static_assert` in each Part B stage plus
   the smoke scripts.

---

## Part A: Remove the composite unions (RA)

### Task A1: Pin the current encodings with golden tests

**Files:**

- Create: `src/ra/core/packed.h` (empty shell for now: guard, includes, comment)
- Create: `src/ra/core/packed_test.cc`
- Modify: `src/ra/CMakeLists.txt` (new `ra_packed_test` after `ra_search_test`, line ~199)

The tests are written against today's union implementations and must pass before any rewrite. They
include `ra/core/coord.h` and `ra/objects/target.h` only if those build in the test target;
`ra/core/inline.h` pulls in scenario, display and world and does not. So the test first targets the
helpers through `packed.h`, and Step 1 moves the pure packing helpers there, union bodies unchanged.

- [ ] **Step 1: Move the pure helpers, unchanged, into `ra/core/packed.h`**

Move `XY_Coord`, `Coord_XCell`, `Coord_YCell`, `XY_Cell`, `Cell_To_Lepton`, `Lepton_To_Cell`,
`Coord_X`, `Coord_Y`, `Cell_X`, `Cell_Y`, `Coord_XLepton`, `Coord_YLepton`, `Cell_Coord`,
`Coord_Snap`, `Coord_Fraction`, `Coord_Whole`, `Coord_Add`, `Coord_Sub`, `Coord_Mid` from
`ra/core/inline.h`, `Coord_Cell` from `ra/core/coord.cc` (dropping its declaration at
`ra/core/coord.h:54`), and `Target_Kind`/`Target_Value` from `ra/objects/target.h:67-74`. Leave
`Build_Target` and the union declarations in `defines.h` for now. `packed.h`:

```cpp
// The bit layouts of Red Alert's packed map and target words: CELL (7-bit
// X and Y), LEPTON (cell byte over sub-cell byte), COORDINATE (X lepton word
// low, Y lepton word high) and TARGET (8-bit RTTI kind over a 24-bit value).

#ifndef CNC_RED_ALERT_RA_CORE_PACKED_H_
#define CNC_RED_ALERT_RA_CORE_PACKED_H_

#include <bit>
#include <cstdint>

#include "game/core/display_constants.h"
#include "ra/core/defines.h"

namespace ra {
// ... the moved functions, with their existing comments rewritten as one-line
// Google-style declaration comments ...
}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_PACKED_H_
```

`ra/core/inline.h`, `ra/core/coord.h` and `ra/objects/target.h` include `ra/core/packed.h`.

- [ ] **Step 2: Write the golden tests**

```cpp
// Pins the packed CELL, LEPTON, COORDINATE and TARGET encodings, including the
// truncation the original bit-fields performed on out-of-range components.

#include "ra/core/packed.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "ra/core/defines.h"

namespace ra {
namespace {

constexpr COORDINATE kSample = 0x12345678;

TEST(PackedTest, CellPacksSevenBitsPerAxis) {
  EXPECT_EQ(XY_Cell(5, 3), 3 * 128 + 5);
  EXPECT_EQ(Cell_X(XY_Cell(5, 3)), 5);
  EXPECT_EQ(Cell_Y(XY_Cell(5, 3)), 3);
}

TEST(PackedTest, CellTruncatesOutOfRangeComponents) {
  EXPECT_EQ(XY_Cell(-1, 0), 127);
  EXPECT_EQ(XY_Cell(130, 1), 128 + 2);
}

TEST(PackedTest, SentinelCellsReadAsFullComponents) {
  EXPECT_EQ(Cell_X(-1), 127);
  EXPECT_EQ(Cell_Y(-1), 127);
  EXPECT_EQ(Cell_X(kRefreshEol), 127);
  EXPECT_EQ(Cell_Y(kRefreshEol), 127);
}

TEST(PackedTest, CoordinateComponents) {
  EXPECT_EQ(Coord_X(kSample), 0x5678);
  EXPECT_EQ(Coord_Y(kSample), 0x1234);
  EXPECT_EQ(Coord_XCell(kSample), 0x56);
  EXPECT_EQ(Coord_YCell(kSample), 0x12);
  EXPECT_EQ(Coord_XLepton(kSample), 0x78);
  EXPECT_EQ(Coord_YLepton(kSample), 0x34);
  EXPECT_EQ(XY_Coord(0x5678, 0x1234), kSample);
}

TEST(PackedTest, CoordinateRounding) {
  EXPECT_EQ(Coord_Snap(kSample), 0x12805680U);
  EXPECT_EQ(Coord_Fraction(kSample), 0x00340078U);
  EXPECT_EQ(Coord_Whole(kSample), 0x12005600U);
}

TEST(PackedTest, CellCoordinateRoundTrip) {
  EXPECT_EQ(Cell_Coord(XY_Cell(5, 3)), 0x03800580U);
  EXPECT_EQ(Coord_Cell(kSample), XY_Cell(0x56, 0x12));
  EXPECT_EQ(Coord_Cell(0x00008000U), XY_Cell(0, 0));  // X cell byte 128 wraps.
}

TEST(PackedTest, CoordinateArithmeticWrapsPerComponent) {
  EXPECT_EQ(Coord_Add(XY_Coord(0xFFFF, 1), XY_Coord(2, 0xFFFF)), XY_Coord(1, 0));
  EXPECT_EQ(Coord_Sub(XY_Coord(1, 0), XY_Coord(2, 0)), XY_Coord(0xFFFF, 0));
  EXPECT_EQ(Coord_Mid(XY_Coord(0xFFFF, 0), XY_Coord(1, 0)), XY_Coord(0x8000, 0));
}

TEST(PackedTest, LeptonCellConversions) {
  EXPECT_EQ(Cell_To_Lepton(3), 0x0300);
  EXPECT_EQ(Cell_To_Lepton(300), 0x2C00);
  EXPECT_EQ(Lepton_To_Cell(0x017F), 1);
  EXPECT_EQ(Lepton_To_Cell(0x0180), 2);
  EXPECT_EQ(Lepton_To_Cell(0xFF80), 256);
}

TEST(PackedTest, TargetPacksKindOverValue) {
  const TARGET target = Build_Target(RTTI_CELL, 389);
  EXPECT_EQ(static_cast<uint32_t>(target),
            (static_cast<uint32_t>(RTTI_CELL) << 24) | 389U);
  EXPECT_EQ(Target_Kind(target), RTTI_CELL);
  EXPECT_EQ(Target_Value(target), 389);
}

TEST(PackedTest, TargetTruncatesNegativeValues) {
  const TARGET target = Build_Target(RTTI_UNIT, -1);
  EXPECT_EQ(Target_Kind(target), RTTI_UNIT);
  EXPECT_EQ(Target_Value(target), 0xFFFFFF);
  EXPECT_EQ(Build_Target(RTTI_NONE, 0), kTargetNone);
}

}  // namespace
}  // namespace ra
```

CMake, after `ra_search_test`:

```cmake
add_gtest(
    NAME ra_packed_test
    SOURCES core/packed_test.cc
    LIBS base game_core absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_packed_test)
```

- [ ] **Step 3: Build and run; every test passes against the unions**

Run:
`cmake --build build --parallel $JOBS --target ra_packed_test rasdl && ctest --test-dir build -R ra_packed_test --output-on-failure`
Expected: PASS. A failure here means the expected value in the test is wrong, not the code — fix the
test from what the union actually produces.

- [ ] **Step 4: Strict-check and commit**

Run: `tools/strict_tu.py src/ra/core/packed_test.cc src/ra/core/coord.cc` Commit:
`Move RA's coordinate packing helpers to ra/core/packed.h and pin them`

### Task A2: Rewrite the helpers with shifts and masks

**Files:** Modify `src/ra/core/packed.h`, `src/ra/core/coord.h:33-38`, `src/ra/core/coord.cc:55-70`

**Interfaces — produces** (all `constexpr`, `noexcept`, in `packed.h`):

```cpp
inline constexpr int kCellBits = 7;
inline constexpr unsigned kCellMask = (1U << kCellBits) - 1;  // 0x7F

constexpr CELL XY_Cell(int x, int y) noexcept {
  return static_cast<CELL>(((static_cast<unsigned>(y) & kCellMask) << kCellBits) |
                           (static_cast<unsigned>(x) & kCellMask));
}
constexpr int Cell_X(CELL cell) noexcept {
  return static_cast<int>(static_cast<uint16_t>(cell) & kCellMask);
}
constexpr int Cell_Y(CELL cell) noexcept {
  return static_cast<int>((static_cast<uint16_t>(cell) >> kCellBits) & kCellMask);
}

constexpr COORDINATE XY_Coord(LEPTON x, LEPTON y) noexcept {
  return (COORDINATE{y} << 16) | x;
}
constexpr LEPTON Coord_X(COORDINATE coord) noexcept { return static_cast<LEPTON>(coord); }
constexpr LEPTON Coord_Y(COORDINATE coord) noexcept { return static_cast<LEPTON>(coord >> 16); }

// A lepton word is the cell byte over the sub-cell byte.
constexpr int Lepton_Cell(LEPTON lepton) noexcept { return lepton >> 8; }
constexpr int Lepton_Fraction(LEPTON lepton) noexcept { return lepton & 0xFF; }
constexpr LEPTON Make_Lepton(int cell, int fraction) noexcept {
  return static_cast<LEPTON>(((static_cast<unsigned>(cell) & 0xFF) << 8) |
                             (static_cast<unsigned>(fraction) & 0xFF));
}

constexpr CELL Coord_XCell(COORDINATE c) noexcept { return static_cast<CELL>(Lepton_Cell(Coord_X(c))); }
constexpr CELL Coord_YCell(COORDINATE c) noexcept { return static_cast<CELL>(Lepton_Cell(Coord_Y(c))); }
constexpr int Coord_XLepton(COORDINATE c) noexcept { return Lepton_Fraction(Coord_X(c)); }
constexpr int Coord_YLepton(COORDINATE c) noexcept { return Lepton_Fraction(Coord_Y(c)); }
constexpr CELL Coord_Cell(COORDINATE c) noexcept { return XY_Cell(Coord_XCell(c), Coord_YCell(c)); }

constexpr LEPTON Cell_To_Lepton(int cells) noexcept { return Make_Lepton(cells, 0); }
constexpr int Lepton_To_Cell(LEPTON lepton) noexcept {
  return Lepton_Cell(lepton) + (Lepton_Fraction(lepton) >= CELL_LEPTON_W / 2 ? 1 : 0);
}

constexpr COORDINATE Cell_Coord(CELL cell) noexcept {
  return XY_Coord(Make_Lepton(Cell_X(cell), CELL_LEPTON_W / 2),
                  Make_Lepton(Cell_Y(cell), CELL_LEPTON_W / 2));
}
constexpr COORDINATE Coord_Snap(COORDINATE c) noexcept {
  return XY_Coord(Make_Lepton(Lepton_Cell(Coord_X(c)), CELL_LEPTON_W / 2),
                  Make_Lepton(Lepton_Cell(Coord_Y(c)), CELL_LEPTON_W / 2));
}
constexpr COORDINATE Coord_Fraction(COORDINATE c) noexcept { return c & 0x00FF00FFU; }
constexpr COORDINATE Coord_Whole(COORDINATE c) noexcept { return c & 0xFF00FF00U; }

// Components add and subtract as int16_t and wrap at 16 bits.
constexpr COORDINATE Coord_Add(COORDINATE a, COORDINATE b) noexcept {
  return XY_Coord(static_cast<LEPTON>(Coord_X(a) + Coord_X(b)),
                  static_cast<LEPTON>(Coord_Y(a) + Coord_Y(b)));
}
constexpr COORDINATE Coord_Sub(COORDINATE a, COORDINATE b) noexcept {
  return XY_Coord(static_cast<LEPTON>(Coord_X(a) - Coord_X(b)),
                  static_cast<LEPTON>(Coord_Y(a) - Coord_Y(b)));
}
constexpr COORDINATE Coord_Mid(COORDINATE a, COORDINATE b) noexcept {
  return XY_Coord(static_cast<LEPTON>((Coord_X(a) + Coord_X(b)) / 2),
                  static_cast<LEPTON>((Coord_Y(a) + Coord_Y(b)) / 2));
}

inline constexpr int kTargetMantissaBits = 24;
inline constexpr uint32_t kTargetMantissaMask = (1U << kTargetMantissaBits) - 1;

constexpr TARGET Build_Target(RTTIType kind, int value) noexcept {
  return static_cast<TARGET>((static_cast<uint32_t>(kind) << kTargetMantissaBits) |
                             (static_cast<uint32_t>(value) & kTargetMantissaMask));
}
constexpr RTTIType Target_Kind(TARGET target) noexcept {
  return static_cast<RTTIType>(static_cast<uint32_t>(target) >> kTargetMantissaBits);
}
constexpr int Target_Value(TARGET target) noexcept {
  return static_cast<int>(static_cast<uint32_t>(target) & kTargetMantissaMask);
}
```

`Coord_Add`/`Coord_Sub` as written add the unsigned words; the sum modulo 2^16 equals the old
`int16_t` sum modulo 2^16, so the result is the same. `Build_Target` moves from `defines.h` to
`packed.h` (with `kTargetMantissaBits`; `kTargetExponentBits` goes, nothing else reads it), and
`Target_Kind(RTTIType)` stays checked by the existing `EnumCastOutOfRange` analysis because the
shifted value is at most 255.

- [ ] **Step 1: Replace the function bodies in `packed.h` with the code above**
- [ ] **Step 2: Rewrite `Direction(CELL, CELL)` in `coord.h` with `Cell_X`/`Cell_Y`**
- [ ] **Step 3: Rewrite the refresh-list rect in `coord.cc:58-73` with `LEPTON` locals and
      `Lepton_Cell()`**

```cpp
  const auto left = static_cast<LEPTON>(static_cast<int>(x) +
                                        static_cast<int16_t>(Pixel_To_Lepton(rect.X)));
  const auto top = static_cast<LEPTON>(static_cast<int>(y) +
                                       static_cast<int16_t>(Pixel_To_Lepton(rect.Y)));
  const auto right = static_cast<LEPTON>(left + Pixel_To_Lepton(rect.Width - 1));
  const auto bottom = static_cast<LEPTON>(top + Pixel_To_Lepton(rect.Height - 1));

  const int cell_left = Lepton_Cell(left);
  const int cell_right = Lepton_Cell(right);
  const int cell_top = Lepton_Cell(top);
  const int cell_bottom = Lepton_Cell(bottom);
```

(`right.Raw = left.Raw + ...` assigned an `int` into a `uint16_t`, so the explicit cast keeps the
same wrap.)

- [ ] **Step 4: Add compile-time checks next to the tests**

Add to `packed_test.cc` a `static_assert(XY_Cell(5, 3) == 389);` and
`static_assert(Coord_Cell(0x12345678U) == XY_Cell(0x56, 0x12));` so `constexpr` stays true.

- [ ] **Step 5: Run the tests and the smoke scripts**

Run: `cmake --build build --parallel $JOBS && ctest --test-dir build --output-on-failure`, then
`tools/ra_saveload_smoke.sh build/src/ra/rasdl` with each fixture. Expected: all PASS, smoke diffs
empty.

- [ ] **Step 6: Strict-check and commit**

Run: `tools/strict_tu.py src/ra/core/packed_test.cc src/ra/core/coord.cc` Commit:
`Pack RA cells, leptons, coordinates and targets with shifts instead of unions`

### Task A3: Delete the unions and switch `xTargetClass` to a plain word

**Files:** Modify `src/ra/core/defines.h:266-336`, `src/ra/objects/target.h:120-230`,
`src/ra/objects/target.cc:95`

- [ ] **Step 1: Write the failing test for `xTargetClass`**

Add to `ra/objects/target_test.cc` (new; add to whichever existing RA test target already links
`ra_engine`, for example `ra_game_test`'s source list):

```cpp
TEST(TargetClassTest, CellTargetKeepsTwentyFourBits) {
  const TargetClass target(static_cast<CELL>(-1));
  EXPECT_EQ(static_cast<RTTIType>(target), RTTI_CELL);
  EXPECT_EQ(target.Value(), 0xFFFFFFU);
}

TEST(TargetClassTest, DefaultIsInvalid) {
  const TargetClass target;
  EXPECT_FALSE(target.Is_Valid());
  EXPECT_EQ(target.As_Target(), static_cast<TARGET>(kTargetMantissaMask));
}
```

Run it against the union version first: PASS (it pins behaviour; it is not expected to fail).

- [ ] **Step 2: Replace `TARGET_COMPOSITE Target;` with `TARGET Target;`**

Every member goes through `packed.h`: `operator RTTIType()` returns `Target_Kind(Target)`, `Value()`
returns `static_cast<unsigned>(Target_Value(Target))`, `Invalidate()` assigns
`Build_Target(RTTI_NONE, -1)`, `Is_Valid()` compares `Target_Kind(Target) != RTTI_NONE`, and the
`TargetClass` constructors assign `Build_Target(kind, value)`. `operator==` compares the words and
returns `bool`.

- [ ] **Step 3: Delete the four unions and their `static_assert`s from `defines.h`**

Keep the type aliases and rewrite the block comment above them in plain Google style (the `/****`
banner goes).

- [ ] **Step 4: Confirm no union remains and the packet size is unchanged**

Run: `grep -rnw -E 'LEPTON_COMPOSITE|COORD_COMPOSITE|CELL_COMPOSITE|TARGET_COMPOSITE' src` → no
output. Add `static_assert(sizeof(xTargetClass) == sizeof(TARGET));` and
`static_assert(std::is_trivially_default_constructible_v<xTargetClass>);` under the class.

- [ ] **Step 5: Full strict build, ctest, both RA smoke fixtures, commit**

Run: `cmake --build build-strict --parallel 14 && ctest --test-dir build-strict` Commit:
`Delete RA's composite unions`

---

## Part B: Replace the aliases with types

Each stage is one alias in one game, landed and verified before the next. Order is cheapest and most
contained first: `LEPTON` (RA only), `COORDINATE`, `TARGET`, `CELL`.

Every stage follows the same compiler-driven procedure:

1. Write the class (or the `int` replacement) next to its packing helpers, with a unit test of its
   factories, accessors and the truncation cases from Part A.
2. Replace the alias declaration with the class, rebuild, and fix the errors file by file —
   `tools/strict_tu.py` for each file as it compiles. Forks may take disjoint file groups (see the
   parallel-forks memory), reporting signature changes back for the parent to apply.
3. `static_assert` the packet and save layout facts listed in that stage.
4. Full strict build, `ctest`, both smoke scripts for the game, and a two-instance local multiplayer
   game exchanging move, attack, sell and place events. Commit.

### Stage B1: `LEPTON` becomes `int` (RA)

A lepton is a distance with arithmetic everywhere (`+` 28, `<` 15, `/` 9, `-` 9 sites), which is
what an `int` is; TD already uses `int`. The only places the 16-bit width matters are the packing in
`packed.h` (which keeps `uint16_t` internally) and stored fields.

- Before the change, list every implicit narrowing into a `LEPTON`: build the RA TUs with
  `-Wimplicit-int-conversion` and keep the sites whose target is `LEPTON`. Each is a place the old
  code wrapped at 16 bits; decide per site whether the wrap was intended (keep it with an explicit
  `static_cast<uint16_t>`) or a latent bug (document it in the commit).
- `Coord_X`/`Coord_Y` return `int` in `0..65535`.
- `LEPTON` members (`ra/objects/house.h`, `techno.h`, `foot.h`, `type.h`, rules) become `int`; bump
  `kSaveGameVersion`.
- Delete `using LEPTON`.

### Stage B2: `COORDINATE` becomes `Coordinate` (RA, then TD)

```cpp
// A world position in leptons. X and Y are 16-bit words; the high byte of
// each is the cell and the low byte the position within it.
class Coordinate {
 public:
  static constexpr Coordinate FromLeptons(int x, int y) noexcept;
  static constexpr Coordinate FromPacked(uint32_t packed) noexcept;
  static constexpr Coordinate CellCenter(Cell cell) noexcept;  // after B4; Cell_Coord until then

  constexpr uint32_t packed() const noexcept { return packed_; }
  constexpr int x() const noexcept;           // 0..65535
  constexpr int y() const noexcept;
  constexpr Coordinate Snapped() const noexcept;    // Coord_Snap
  constexpr Coordinate Whole() const noexcept;      // Coord_Whole
  constexpr Coordinate Fraction() const noexcept;   // Coord_Fraction
  constexpr explicit operator bool() const noexcept { return packed_ != 0; }

  friend constexpr Coordinate operator+(Coordinate, Coordinate) noexcept;  // Coord_Add
  friend constexpr Coordinate operator-(Coordinate, Coordinate) noexcept;  // Coord_Sub
  friend constexpr bool operator==(Coordinate, Coordinate) = default;

  template <class Archive>
  void Serialize(Archive& ar) { ar.Field(packed_); }

 private:
  uint32_t packed_;  // No initializer: EventClass's union needs a trivial type.
};
static_assert(sizeof(Coordinate) == 4);
static_assert(std::is_trivially_copyable_v<Coordinate> &&
              std::is_trivially_default_constructible_v<Coordinate>);
```

- `if (coord)` (17 sites) keeps working through `explicit operator bool`; `coord & mask` sites (7)
  become `Whole()`/`Fraction()`.
- The free `Coord_*` helpers stay as one-line forwarding wrappers during the stage and are deleted
  at its end, so the stage can land in several commits.
- TD repeats the stage in `td/core/coordinate.h`. TD's `Coord_X` reads the word as **signed**
  (`static_cast<int16_t>`), RA's as unsigned: keep each game's accessor semantics and record the
  difference in the class comment. Whether the two classes can merge into `game::Coordinate` is
  decided after both land, by auditing TD's callers of the signed reading.

### Stage B3: `TARGET` becomes `Target` (RA, then TD)

`xTargetClass` is already the right shape: a trivial wrapper over the packed word. Rename it
`Target`, give it `static constexpr Target Make(RTTIType, int)`, `kind()`, `value()`,
`explicit operator bool`, defaulted `==`, and `Serialize`. `TargetClass`'s constructors become
factories on `Target` (`Target::Of(const AbstractClass*)`, `Target::OfCell(Cell)`, ...), and
`TargetClass` is deleted. The `As_*` free functions become members. `kTargetNone` becomes
`Target{}`'s value; `static_assert(sizeof(Target) == 4)` and the trivial-default check guard the
event union. TD's 16-bit `TARGET` gets its own `td::Target` with TD's bit split.

### Stage B4: `CELL` becomes `Cell` (RA, then TD)

```cpp
// A map cell, packed as y * 128 + x. Negative values are sentinels.
class Cell {
 public:
  static constexpr Cell FromXY(int x, int y) noexcept;  // masks 7 bits, as XY_Cell
  static constexpr Cell FromIndex(int index) noexcept;  // Map[] index or sentinel
  static constexpr Cell None() noexcept { return FromIndex(-1); }

  constexpr int index() const noexcept { return value_; }
  constexpr int x() const noexcept;
  constexpr int y() const noexcept;
  constexpr bool is_none() const noexcept { return value_ < 0; }
  constexpr Cell Offset(int delta) const noexcept;  // cell + MAP_CELL_W and refresh lists

  friend constexpr auto operator<=>(Cell, Cell) = default;

  template <class Archive>
  void Serialize(Archive& ar) { ar.Field(value_); }

 private:
  int16_t value_;  // No initializer: EventClass's union needs a trivial type.
};
```

- First, in its own commit, turn `MAP_CELL_W`, `MAP_CELL_H` and `MAP_CELL_TOTAL` into `int` (they
  are sizes, not cells; leaving them `CELL` produces 1,700 cascade errors).
- `Map[cell]` / `TheMap().at(cell)` gain a `Cell` overload; raw integer indexing stays for loops
  over `0..MAP_CELL_TOTAL`.
- Refresh/occupy lists stay `int16_t` offsets (they are deltas, not cells, terminated by
  `kRefreshEol`); `cell.Offset(delta)` applies them.
- Default arguments `CELL pos = -1` become `Cell pos = Cell::None()`.
- TD's `td::Cell` packs 6 bits per axis; same interface.

## Verification

Per task: the unit tests named in it, `tools/strict_tu.py` on the touched files.

Per stage and at the end of Part A:

- `cmake --build build-strict --parallel 14` with no new findings; `ctest` all green.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` with every fixture: loaded run
  matches the continuous run.
- A two-instance local multiplayer game (both binaries from the same build) that exchanges move,
  attack, sell, place and special-weapon events without a sync error.
- `grep -rnw <alias> src` returns nothing once the stage deletes the alias.

## Progress

| Step       | Status | Commit |
| ---------- | ------ | ------ |
| A1         |        |        |
| A2         |        |        |
| A3         |        |        |
| B1         |        |        |
| B2 RA / TD |        |        |
| B3 RA / TD |        |        |
| B4 RA / TD |        |        |
