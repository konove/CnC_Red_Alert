// The bit layouts of Red Alert's packed map and target words: CELL (7-bit X
// and Y), LEPTON (cell byte over sub-cell byte), COORDINATE (X lepton word
// low, Y lepton word high) and TARGET (8-bit RTTI kind over a 24-bit value).

#ifndef CNC_RED_ALERT_RA_CORE_PACKED_H_
#define CNC_RED_ALERT_RA_CORE_PACKED_H_

#include <bit>
#include <cstdint>

#include "game/core/display_constants.h"
#include "ra/core/defines.h"

namespace ra {

// Combines X and Y lepton components into a coordinate.
inline COORDINATE XY_Coord(LEPTON x, LEPTON y) noexcept {
  COORD_COMPOSITE coord{};

  coord.Sub.X.Raw = x;
  coord.Sub.Y.Raw = y;
  return coord.Coord;
}

// Returns the sub-cell coordinate of a cell's center.
inline COORDINATE CellCenterFraction() noexcept {
  return XY_Coord(CELL_LEPTON_W / 2, CELL_LEPTON_H / 2);
}

// Returns the X cell component of a coordinate.
inline CELL Coord_XCell(COORDINATE coord) {
  return std::bit_cast<COORD_COMPOSITE>(coord).Sub.X.Sub.Cell;
}

// Returns the Y cell component of a coordinate.
inline CELL Coord_YCell(COORDINATE coord) {
  return std::bit_cast<COORD_COMPOSITE>(coord).Sub.Y.Sub.Cell;
}

// Combines X and Y cell components into a cell. Each keeps its low 7 bits, so
// out-of-range components wrap.
inline CELL XY_Cell(int x, int y) {
  CELL_COMPOSITE cell{};
  cell.Cell = 0;
  cell.Sub.X = static_cast<uint16_t>(x);
  cell.Sub.Y = static_cast<uint16_t>(y);
  return cell.Cell;
}

// Converts a distance in cells to leptons. Only the low 8 bits of the cell
// count fit.
inline LEPTON Cell_To_Lepton(int cell_distance) {
  LEPTON_COMPOSITE lepton{};
  lepton.Sub.Cell = static_cast<unsigned char>(cell_distance);
  lepton.Sub.Lepton = 0;
  return lepton.Raw;
}

// Converts a distance in leptons to the nearest whole number of cells.
inline int Lepton_To_Cell(LEPTON lepton_distance) {
  const auto lepton = std::bit_cast<LEPTON_COMPOSITE>(lepton_distance);
  if (lepton.Sub.Lepton >= CELL_LEPTON_W / 2) {
    return lepton.Sub.Cell + 1;
  }
  return lepton.Sub.Cell;
}

// Returns the X lepton component of a coordinate.
inline LEPTON Coord_X(COORDINATE coord) {
  return std::bit_cast<COORD_COMPOSITE>(coord).Sub.X.Raw;
}

// Returns the Y lepton component of a coordinate.
inline LEPTON Coord_Y(COORDINATE coord) {
  return std::bit_cast<COORD_COMPOSITE>(coord).Sub.Y.Raw;
}

// Returns the X component of a cell.
inline int Cell_X(CELL cell) {
  return std::bit_cast<CELL_COMPOSITE>(cell).Sub.X;
}

// Returns the Y component of a cell.
inline int Cell_Y(CELL cell) {
  return std::bit_cast<CELL_COMPOSITE>(cell).Sub.Y;
}

// Returns the X position within its cell, 0 at the cell's left edge.
inline int Coord_XLepton(COORDINATE coord) {
  return std::bit_cast<COORD_COMPOSITE>(coord).Sub.X.Sub.Lepton;
}

// Returns the Y position within its cell, 0 at the cell's top edge.
inline int Coord_YLepton(COORDINATE coord) {
  return std::bit_cast<COORD_COMPOSITE>(coord).Sub.Y.Sub.Lepton;
}

// Returns the cell a coordinate lies in.
inline CELL Coord_Cell(COORDINATE coord) {
  COORD_COMPOSITE cc{};
  cc.Coord = coord;
  CELL_COMPOSITE cell{};
  cell.Cell = 0;
  cell.Sub.X = cc.Sub.X.Sub.Cell;
  cell.Sub.Y = cc.Sub.Y.Sub.Cell;
  return cell.Cell;
}

// Returns the coordinate of a cell's center.
inline COORDINATE Cell_Coord(CELL cell) {
  COORD_COMPOSITE coord{};
  const auto cell_xy = std::bit_cast<CELL_COMPOSITE>(cell);

  coord.Sub.X.Sub.Cell = static_cast<unsigned char>(cell_xy.Sub.X);
  coord.Sub.X.Sub.Lepton = static_cast<unsigned char>((CELL_LEPTON_W / 2));
  coord.Sub.Y.Sub.Cell = static_cast<unsigned char>(cell_xy.Sub.Y);
  coord.Sub.Y.Sub.Lepton = static_cast<unsigned char>((CELL_LEPTON_W / 2));
  return coord.Coord;
}

// Moves a coordinate to the center of its cell.
inline COORDINATE Coord_Snap(COORDINATE coord) {
  auto composite = std::bit_cast<COORD_COMPOSITE>(coord);
  composite.Sub.X.Sub.Lepton = CELL_LEPTON_W / 2;
  composite.Sub.Y.Sub.Lepton = CELL_LEPTON_W / 2;
  return composite.Coord;
}

// Keeps only the position within the cell, dropping the cell components.
inline COORDINATE Coord_Fraction(COORDINATE coord) {
  auto composite = std::bit_cast<COORD_COMPOSITE>(coord);
  composite.Sub.X.Sub.Cell = 0;
  composite.Sub.Y.Sub.Cell = 0;
  return composite.Coord;
}

// Moves a coordinate to the upper left corner of its cell.
inline COORDINATE Coord_Whole(COORDINATE coord) {
  auto composite = std::bit_cast<COORD_COMPOSITE>(coord);
  composite.Sub.X.Sub.Lepton = 0;
  composite.Sub.Y.Sub.Lepton = 0;
  return composite.Coord;
}

// Adds two coordinates component by component; each component wraps at 16
// bits, so a coordinate can carry a negative offset.
inline COORDINATE Coord_Add(COORDINATE a, COORDINATE b) {
  COORD_COMPOSITE coord{};
  const auto lhs = std::bit_cast<COORD_COMPOSITE>(a);
  const auto rhs = std::bit_cast<COORD_COMPOSITE>(b);

  coord.Sub.X.Raw = static_cast<LEPTON>(static_cast<int16_t>(lhs.Sub.X.Raw) +
                                        static_cast<int16_t>(rhs.Sub.X.Raw));
  coord.Sub.Y.Raw = static_cast<LEPTON>(static_cast<int16_t>(lhs.Sub.Y.Raw) +
                                        static_cast<int16_t>(rhs.Sub.Y.Raw));
  return coord.Coord;
}

// Subtracts coord2 from coord1 component by component, wrapping at 16 bits.
inline COORDINATE Coord_Sub(COORDINATE coord1, COORDINATE coord2) {
  COORD_COMPOSITE coord{};
  const auto lhs = std::bit_cast<COORD_COMPOSITE>(coord1);
  const auto rhs = std::bit_cast<COORD_COMPOSITE>(coord2);

  coord.Sub.X.Raw = static_cast<LEPTON>(static_cast<int16_t>(lhs.Sub.X.Raw) -
                                        static_cast<int16_t>(rhs.Sub.X.Raw));
  coord.Sub.Y.Raw = static_cast<LEPTON>(static_cast<int16_t>(lhs.Sub.Y.Raw) -
                                        static_cast<int16_t>(rhs.Sub.Y.Raw));
  return coord.Coord;
}

// Returns the midpoint of two coordinates.
inline COORDINATE Coord_Mid(COORDINATE coord1, COORDINATE coord2) {
  COORD_COMPOSITE coord{};
  const auto first = std::bit_cast<COORD_COMPOSITE>(coord1);
  const auto second = std::bit_cast<COORD_COMPOSITE>(coord2);

  coord.Sub.X.Raw =
      static_cast<LEPTON>((int{first.Sub.X.Raw} + int{second.Sub.X.Raw}) / 2);
  coord.Sub.Y.Raw =
      static_cast<LEPTON>((int{first.Sub.Y.Raw} + int{second.Sub.Y.Raw}) / 2);
  return coord.Coord;
}

// Returns the kind of object a target refers to.
inline RTTIType Target_Kind(TARGET a) {
  return static_cast<RTTIType>(std::bit_cast<TARGET_COMPOSITE>(a).Sub.Exponent);
}

// Returns the target's value: a heap index, cell or coordinate by kind.
inline int Target_Value(TARGET a) {
  const TARGET_COMPOSITE composite{.Target = a};
  return static_cast<int>(composite.Sub.Mantissa);
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_PACKED_H_
