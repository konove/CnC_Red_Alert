// Tiberian Dawn's map cell: the 16-bit word that indexes the 64 x 64 map. In
// a header of its own so that light headers such as td/core/target.h can name
// it without td/core/defines.h.

#ifndef CNC_RED_ALERT_TD_CORE_CELL_H_
#define CNC_RED_ALERT_TD_CORE_CELL_H_

#include <compare>
#include <cstdint>
#include <type_traits>

#include "td/core/coordinate.h"

namespace td {

// A map cell, packed as y * 64 + x (6 bits per axis) in a signed 16-bit word.
// index() is that word, the index into the map's cell array. Negative values
// are sentinels ("no cell"); None() is the usual one. The REFRESH_EOL-
// terminated offset lists are int16_t deltas, not cells: Offset() applies
// them.
//
// Unlike Red Alert's Cell, nothing here masks a component: FromXY() ORs them
// unmasked, and y() is every bit above X, so a sentinel's y() is huge.
//
// Cell{} is cell 0, the map's top left corner, not "no cell".
//
// Example:
//   const Cell cell = Cell::FromXY(5, 3);        // index() 197
//   const Cell below = cell.Offset(MAP_CELL_W);  // (5, 4)
//   const Coordinate center = cell.Center();     // 0x03800580
class Cell {
 public:
  // Combines X and Y components without masking either: Y is shifted over X
  // and the two are ORed, so an X of 64 or more sets Y bits (FromXY(64, 0) is
  // cell (0, 1)) and a negative X sets every high bit (FromXY(-1, y) is
  // None()). The word keeps its low 16 bits.
  static constexpr Cell FromXY(int x, int y) noexcept {
    return FromIndex(static_cast<int>((static_cast<unsigned>(y) << kAxisBits) |
                                      static_cast<unsigned>(x)));
  }

  // Wraps a map index or a sentinel. The index keeps its low 16 bits, read as
  // signed, as the old int16_t cell word did.
  static constexpr Cell FromIndex(int index) noexcept {
    Cell cell{};
    cell.value_ = static_cast<int16_t>(index);
    return cell;
  }

  // The usual "no cell" sentinel, index -1.
  static constexpr Cell None() noexcept { return FromIndex(-1); }

  // Returns the cell a coordinate lies in: the Y cell byte shifted over the
  // whole X cell byte, as the original assembly's OR did. An X cell of 64 or
  // more (off the map) therefore sets Y bits.
  static constexpr Cell Containing(Coordinate coord) noexcept {
    return FromXY(coord.cell_x(), coord.cell_y());
  }

  // Returns the coordinate of this cell's center: each component is the cell
  // in the high byte and the lepton offset 0x80 in the low byte. Each
  // component keeps the low 8 bits of x() or y(), so None() is the center of
  // cell (63, 255), 0xFF803F80.
  [[nodiscard]] constexpr Coordinate Center() const noexcept {
    // Unsigned, so a sentinel's huge y() wraps instead of overflowing; only
    // the low 16 bits reach the coordinate.
    return Coordinate::FromLeptons(
        static_cast<int>((static_cast<unsigned>(x()) * kLeptonsPerCell) +
                         kHalfCellLeptons),
        static_cast<int>((static_cast<unsigned>(y()) * kLeptonsPerCell) +
                         kHalfCellLeptons));
  }

  // Returns the word, sign-extended: the map index, or a negative sentinel.
  // Saves, INI files, network events and the editor carry this number; the
  // multiplayer CRC and the per-frame log do not.
  [[nodiscard]] constexpr int index() const noexcept { return value_; }

  // Returns the X component, in 0..63; a sentinel reads as its low 6 bits
  // (None() is 63).
  [[nodiscard]] constexpr int x() const noexcept {
    return static_cast<int>(static_cast<unsigned>(index()) & kAxisMask);
  }

  // Returns every bit above X: the Y component, 0..63 for a map cell. Nothing
  // masks it, and a negative index is sign-extended first, so a sentinel
  // reads as a huge row: None() is 67108863, not 63. Off-map positive
  // indices read past 63 too (REFRESH_EOL is 511).
  [[nodiscard]] constexpr int y() const noexcept {
    return static_cast<int>(static_cast<unsigned>(index()) >> kAxisBits);
  }

  // True for any negative index: a sentinel, not a map cell.
  [[nodiscard]] constexpr bool is_none() const noexcept { return value_ < 0; }

  // Returns the cell `delta` indices away: MAP_CELL_W is one row down, 1 one
  // column right (running into the next row off the right edge). The sum
  // keeps its low 16 bits, as the old int16_t arithmetic did.
  [[nodiscard]] constexpr Cell Offset(int delta) const noexcept {
    return FromIndex(value_ + delta);
  }

  // Cells compare by their signed index, so every sentinel sorts before
  // cell 0.
  friend constexpr auto operator<=>(Cell, Cell) noexcept = default;

  // Saves and loads the word, 2 bytes little-endian.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(value_);
  }

 private:
  // Each component is 6 bits: X in the low bits, Y above it.
  static constexpr unsigned kAxisBits = 6;
  static constexpr unsigned kAxisMask = (1U << kAxisBits) - 1;
  // A cell is 256 leptons on a side.
  static constexpr unsigned kLeptonsPerCell = 256;
  static constexpr unsigned kHalfCellLeptons = kLeptonsPerCell / 2;

  int16_t value_;  // No initializer: EventClass's union needs a trivial type.
};

static_assert(sizeof(Cell) == 2);
static_assert(std::is_trivially_copyable_v<Cell>);
static_assert(std::is_trivially_default_constructible_v<Cell>);
static_assert(std::is_standard_layout_v<Cell>);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_CELL_H_
