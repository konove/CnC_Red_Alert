// The bit layout of Red Alert's 16-bit lepton word (cell byte over sub-cell
// byte), with the lepton/pixel conversions that share it. A lepton (256 to a
// cell) is a plain int everywhere else; these helpers wrap it to the 16-bit
// word. The other packed words have their own classes: ra::Cell
// (ra/core/cell.h), ra::Coordinate (ra/core/coordinate.h) and ra::Target
// (ra/core/target.h).

#ifndef CNC_RED_ALERT_RA_CORE_PACKED_H_
#define CNC_RED_ALERT_RA_CORE_PACKED_H_

#include <cstdint>

#include "game/core/display_constants.h"

namespace ra {

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

// Converts a distance in cells to leptons. Only the low 8 bits of the cell
// count fit.
constexpr int Cell_To_Lepton(int cell_distance) noexcept {
  return Make_Lepton(cell_distance, 0);
}

// Converts a distance in leptons to the nearest whole number of cells. The
// distance keeps its low 16 bits, so -1 is 0xFFFF and rounds to 256 cells.
constexpr int Lepton_To_Cell(int lepton_distance) noexcept {
  return Lepton_Cell(lepton_distance) +
         (Lepton_Fraction(lepton_distance) >= game::kCellWidthLeptons / 2 ? 1
                                                                          : 0);
}

// Converts a lepton distance to the nearest whole number of pixels. The
// distance keeps its low 16 bits, read as signed, so 0xFFFF is -1 lepton.
// Precision is lost: a value converted to pixels and back usually differs.
constexpr int Lepton_To_Pixel(int lepton) noexcept {
  return ((static_cast<int>(static_cast<int16_t>(lepton)) *
           game::kCellWidthPixels) +
          (game::kCellWidthLeptons / 2)) /
         game::kCellWidthLeptons;
}

// Converts a pixel distance to the nearest lepton distance, as a 16-bit
// lepton word in 0..65535: a negative pixel count wraps.
constexpr int Pixel_To_Lepton(int pixel) noexcept {
  return static_cast<uint16_t>(
      ((pixel * game::kCellWidthLeptons) + (game::kCellWidthPixels / 2)) /
      game::kCellWidthPixels);
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_PACKED_H_
