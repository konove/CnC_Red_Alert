// The bit layouts of Red Alert's packed map words: CELL (7-bit X and Y) and
// the lepton word (cell byte over sub-cell byte), with the lepton/pixel
// conversions that share the 16-bit lepton word and the conversions between
// CELL and Coordinate (ra/core/coordinate.h, which holds the coordinate's own
// layout). A lepton (256 to a cell) is a plain int everywhere else; these
// helpers wrap it to the 16-bit word. The target word's layout is
// ra::Target's (ra/core/target.h).

#ifndef CNC_RED_ALERT_RA_CORE_PACKED_H_
#define CNC_RED_ALERT_RA_CORE_PACKED_H_

#include <cstdint>

#include "game/core/display_constants.h"
#include "ra/core/coordinate.h"

namespace ra {

// A map coordinate with cell resolution, the cheaper index into the map than
// a lepton-resolution Coordinate: Y * 128 + X. Negative values are sentinels.
using CELL = int16_t;

// Cell components are 7 bits each: X in the low bits, Y above it.
inline constexpr unsigned kCellBits = 7;
inline constexpr unsigned kCellMask = (1U << kCellBits) - 1;

// Returns the sub-cell coordinate of a cell's center.
constexpr Coordinate CellCenterFraction() noexcept {
  return Coordinate::FromLeptons(CELL_LEPTON_W / 2, CELL_LEPTON_H / 2);
}

// Returns the cell byte of a lepton word: whole cells. The lepton keeps its
// low 16 bits, so -1 is the word 0xFFFF.
constexpr int Lepton_Cell(int lepton) noexcept {
  return static_cast<uint16_t>(lepton) >> 8U;
}

// Returns the sub-cell byte of a lepton word: the position within the cell.
constexpr int Lepton_Fraction(int lepton) noexcept {
  return static_cast<uint16_t>(lepton) & 0xFFU;
}

// Builds a lepton word, in 0..65535, from a cell and a position within it.
// Each keeps its low 8 bits.
constexpr int Make_Lepton(int cell, int fraction) noexcept {
  return static_cast<int>(((static_cast<unsigned>(cell) & 0xFFU) << 8U) |
                          (static_cast<unsigned>(fraction) & 0xFFU));
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
constexpr int Cell_To_Lepton(int cell_distance) noexcept {
  return Make_Lepton(cell_distance, 0);
}

// Converts a distance in leptons to the nearest whole number of cells. The
// distance keeps its low 16 bits, so -1 is 0xFFFF and rounds to 256 cells.
constexpr int Lepton_To_Cell(int lepton_distance) noexcept {
  return Lepton_Cell(lepton_distance) +
         (Lepton_Fraction(lepton_distance) >= CELL_LEPTON_W / 2 ? 1 : 0);
}

// Converts a lepton distance to the nearest whole number of pixels. The
// distance keeps its low 16 bits, read as signed, so 0xFFFF is -1 lepton.
// Precision is lost: a value converted to pixels and back usually differs.
constexpr int Lepton_To_Pixel(int lepton) noexcept {
  return ((static_cast<int>(static_cast<int16_t>(lepton)) * ICON_PIXEL_W) +
          (ICON_LEPTON_W / 2)) /
         ICON_LEPTON_W;
}

// Converts a pixel distance to the nearest lepton distance, as a 16-bit
// lepton word in 0..65535: a negative pixel count wraps.
constexpr int Pixel_To_Lepton(int pixel) noexcept {
  return static_cast<uint16_t>(((pixel * ICON_LEPTON_W) + (ICON_PIXEL_W / 2)) /
                               ICON_PIXEL_W);
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

// Returns the cell a coordinate lies in. Each cell byte keeps its low 7 bits,
// so an X cell byte of 128 (off the map) wraps to column 0.
constexpr CELL Coord_Cell(Coordinate coord) noexcept {
  return XY_Cell(coord.cell_x(), coord.cell_y());
}

// Returns the coordinate of a cell's center.
constexpr Coordinate Cell_Coord(CELL cell) noexcept {
  return Coordinate::FromLeptons(Make_Lepton(Cell_X(cell), CELL_LEPTON_W / 2),
                                 Make_Lepton(Cell_Y(cell), CELL_LEPTON_H / 2));
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_PACKED_H_
