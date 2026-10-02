// The bit layouts of Red Alert's packed map and target words: CELL (7-bit X
// and Y), LEPTON (cell byte over sub-cell byte), COORDINATE (X lepton word
// low, Y lepton word high) and TARGET (8-bit RTTI kind over a 24-bit value).

#ifndef CNC_RED_ALERT_RA_CORE_PACKED_H_
#define CNC_RED_ALERT_RA_CORE_PACKED_H_

#include <cstdint>

#include "game/core/display_constants.h"
#include "ra/core/defines.h"

namespace ra {

// Cell components are 7 bits each: X in the low bits, Y above it.
inline constexpr unsigned kCellBits = 7;
inline constexpr unsigned kCellMask = (1U << kCellBits) - 1;

// A target's value fills the low 24 bits, its RTTI kind the 8 above.
inline constexpr uint32_t kTargetMantissaMask =
    (uint32_t{1} << kTargetMantissaBits) - 1;

// Combines X and Y lepton components into a coordinate.
constexpr COORDINATE XY_Coord(LEPTON x, LEPTON y) noexcept {
  return (COORDINATE{y} << 16U) | x;
}

// Returns the sub-cell coordinate of a cell's center.
constexpr COORDINATE CellCenterFraction() noexcept {
  return XY_Coord(CELL_LEPTON_W / 2, CELL_LEPTON_H / 2);
}

// Returns the X lepton component of a coordinate.
constexpr LEPTON Coord_X(COORDINATE coord) noexcept {
  return static_cast<LEPTON>(coord);
}

// Returns the Y lepton component of a coordinate.
constexpr LEPTON Coord_Y(COORDINATE coord) noexcept {
  return static_cast<LEPTON>(coord >> 16U);
}

// Returns the cell byte of a lepton word: whole cells.
constexpr int Lepton_Cell(LEPTON lepton) noexcept { return lepton >> 8U; }

// Returns the sub-cell byte of a lepton word: the position within the cell.
constexpr int Lepton_Fraction(LEPTON lepton) noexcept { return lepton & 0xFFU; }

// Builds a lepton word from a cell and a position within it. Each keeps its
// low 8 bits.
constexpr LEPTON Make_Lepton(int cell, int fraction) noexcept {
  return static_cast<LEPTON>(((static_cast<unsigned>(cell) & 0xFFU) << 8U) |
                             (static_cast<unsigned>(fraction) & 0xFFU));
}

// Returns the X cell component of a coordinate.
constexpr CELL Coord_XCell(COORDINATE coord) noexcept {
  return static_cast<CELL>(Lepton_Cell(Coord_X(coord)));
}

// Returns the Y cell component of a coordinate.
constexpr CELL Coord_YCell(COORDINATE coord) noexcept {
  return static_cast<CELL>(Lepton_Cell(Coord_Y(coord)));
}

// Combines X and Y cell components into a cell. Each keeps its low 7 bits, so
// out-of-range components wrap.
constexpr CELL XY_Cell(int x, int y) noexcept {
  return static_cast<CELL>(
      ((static_cast<unsigned>(y) & kCellMask) << kCellBits) |
      (static_cast<unsigned>(x) & kCellMask));
}

// Converts a distance in cells to leptons. Only the low 8 bits of the cell
// count fit.
constexpr LEPTON Cell_To_Lepton(int cell_distance) noexcept {
  return Make_Lepton(cell_distance, 0);
}

// Converts a distance in leptons to the nearest whole number of cells.
constexpr int Lepton_To_Cell(LEPTON lepton_distance) noexcept {
  return Lepton_Cell(lepton_distance) +
         (Lepton_Fraction(lepton_distance) >= CELL_LEPTON_W / 2 ? 1 : 0);
}

// Returns the X component of a cell.
constexpr int Cell_X(CELL cell) noexcept {
  return static_cast<int>(static_cast<uint16_t>(cell) & kCellMask);
}

// Returns the Y component of a cell.
constexpr int Cell_Y(CELL cell) noexcept {
  return static_cast<int>(
      (static_cast<unsigned>(static_cast<uint16_t>(cell)) >> kCellBits) &
      kCellMask);
}

// Returns the X position within its cell, 0 at the cell's left edge.
constexpr int Coord_XLepton(COORDINATE coord) noexcept {
  return Lepton_Fraction(Coord_X(coord));
}

// Returns the Y position within its cell, 0 at the cell's top edge.
constexpr int Coord_YLepton(COORDINATE coord) noexcept {
  return Lepton_Fraction(Coord_Y(coord));
}

// Returns the cell a coordinate lies in.
constexpr CELL Coord_Cell(COORDINATE coord) noexcept {
  return XY_Cell(Coord_XCell(coord), Coord_YCell(coord));
}

// Returns the coordinate of a cell's center.
constexpr COORDINATE Cell_Coord(CELL cell) noexcept {
  return XY_Coord(Make_Lepton(Cell_X(cell), CELL_LEPTON_W / 2),
                  Make_Lepton(Cell_Y(cell), CELL_LEPTON_H / 2));
}

// Moves a coordinate to the center of its cell.
constexpr COORDINATE Coord_Snap(COORDINATE coord) noexcept {
  return XY_Coord(Make_Lepton(Lepton_Cell(Coord_X(coord)), CELL_LEPTON_W / 2),
                  Make_Lepton(Lepton_Cell(Coord_Y(coord)), CELL_LEPTON_H / 2));
}

// Keeps only the position within the cell, dropping the cell components.
constexpr COORDINATE Coord_Fraction(COORDINATE coord) noexcept {
  return coord & 0x00FF00FFU;
}

// Moves a coordinate to the upper left corner of its cell.
constexpr COORDINATE Coord_Whole(COORDINATE coord) noexcept {
  return coord & 0xFF00FF00U;
}

// Adds two coordinates component by component; each component wraps at 16
// bits, so a coordinate can carry a negative offset.
constexpr COORDINATE Coord_Add(COORDINATE a, COORDINATE b) noexcept {
  return XY_Coord(static_cast<LEPTON>(Coord_X(a) + Coord_X(b)),
                  static_cast<LEPTON>(Coord_Y(a) + Coord_Y(b)));
}

// Subtracts coord2 from coord1 component by component, wrapping at 16 bits.
constexpr COORDINATE Coord_Sub(COORDINATE coord1, COORDINATE coord2) noexcept {
  return XY_Coord(static_cast<LEPTON>(Coord_X(coord1) - Coord_X(coord2)),
                  static_cast<LEPTON>(Coord_Y(coord1) - Coord_Y(coord2)));
}

// Returns the midpoint of two coordinates, averaging the unsigned components.
constexpr COORDINATE Coord_Mid(COORDINATE coord1, COORDINATE coord2) noexcept {
  return XY_Coord(static_cast<LEPTON>((Coord_X(coord1) + Coord_X(coord2)) / 2),
                  static_cast<LEPTON>((Coord_Y(coord1) + Coord_Y(coord2)) / 2));
}

// Packs an RTTI kind and a value into a target. The value keeps its low 24
// bits, so -1 becomes 0xFFFFFF. RTTI_NONE with value 0 is kTargetNone.
constexpr TARGET Build_Target(RTTIType kind, int value) noexcept {
  return static_cast<TARGET>(
      (static_cast<uint32_t>(kind) << kTargetMantissaBits) |
      (static_cast<uint32_t>(value) & kTargetMantissaMask));
}

// Returns the kind of object a target refers to.
constexpr RTTIType Target_Kind(TARGET target) noexcept {
  return static_cast<RTTIType>(static_cast<uint32_t>(target) >>
                               kTargetMantissaBits);
}

// Returns the target's value: a heap index, cell or coordinate by kind.
constexpr int Target_Value(TARGET target) noexcept {
  return static_cast<int>(static_cast<uint32_t>(target) & kTargetMantissaMask);
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_PACKED_H_
