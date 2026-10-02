// Red Alert's map cell: the 16-bit word that indexes the 128 x 128 map.

#ifndef CNC_RED_ALERT_RA_CORE_CELL_H_
#define CNC_RED_ALERT_RA_CORE_CELL_H_

#include <compare>
#include <cstdint>
#include <type_traits>

#include "ra/core/coordinate.h"

namespace ra {

// A map cell, packed as y * 128 + x (7 bits per axis) in a signed 16-bit
// word. index() is that word, the index into the map's cell array. Negative
// values are sentinels ("no cell"); None() is the usual one. The
// kRefreshEol-terminated offset lists are int16_t deltas, not cells:
// Offset() applies them.
//
// Cell{} is cell 0, the map's top left corner, not "no cell".
//
// Example:
//   const Cell cell = Cell::FromXY(5, 3);    // index() 389
//   const Cell below = cell.Offset(MAP_CELL_W);  // (5, 4)
//   const Coordinate center = cell.Center();  // 0x03800580
class Cell {
 public:
  // Combines X and Y components. Each keeps its low 7 bits, so out-of-range
  // components wrap: FromXY(-1, 0) is cell 127.
  static constexpr Cell FromXY(int x, int y) noexcept {
    return FromIndex(
        static_cast<int>(((static_cast<unsigned>(y) & kAxisMask) << kAxisBits) |
                         (static_cast<unsigned>(x) & kAxisMask)));
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

  // Returns the cell a coordinate lies in. Each cell byte keeps its low 7
  // bits, so an X cell byte of 128 (off the map) wraps to column 0.
  static constexpr Cell Containing(Coordinate coord) noexcept {
    return FromXY(coord.cell_x(), coord.cell_y());
  }

  // Returns the coordinate of this cell's center. A sentinel reads as its
  // components, so None() is the center of cell (127, 127).
  [[nodiscard]] constexpr Coordinate Center() const noexcept {
    return Coordinate::FromLeptons((x() * kLeptonsPerCell) + kHalfCellLeptons,
                                   (y() * kLeptonsPerCell) + kHalfCellLeptons);
  }

  // Returns the word, sign-extended: the map index, or a negative sentinel.
  // Saves, INI files, the multiplayer CRC and logs carry this number.
  [[nodiscard]] constexpr int index() const noexcept { return value_; }

  // Returns the X component, in 0..127; a sentinel reads as its low 7 bits
  // (None() is 127).
  [[nodiscard]] constexpr int x() const noexcept {
    return static_cast<int>(static_cast<uint16_t>(value_) & kAxisMask);
  }

  // Returns the Y component, in 0..127, the next 7 bits above X; a sentinel
  // reads as those bits (None() is 127).
  [[nodiscard]] constexpr int y() const noexcept {
    return static_cast<int>(
        (static_cast<unsigned>(static_cast<uint16_t>(value_)) >> kAxisBits) &
        kAxisMask);
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
  // Each component is 7 bits: X in the low bits, Y above it.
  static constexpr unsigned kAxisBits = 7;
  static constexpr unsigned kAxisMask = (1U << kAxisBits) - 1;
  // A cell is 256 leptons on a side.
  static constexpr int kLeptonsPerCell = 256;
  static constexpr int kHalfCellLeptons = kLeptonsPerCell / 2;

  int16_t value_;  // No initializer: EventClass's union needs a trivial type.
};

static_assert(sizeof(Cell) == 2);
static_assert(std::is_trivially_copyable_v<Cell>);
static_assert(std::is_trivially_default_constructible_v<Cell>);
static_assert(std::is_standard_layout_v<Cell>);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_CELL_H_
