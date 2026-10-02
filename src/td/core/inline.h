#ifndef CNC_RED_ALERT_TD_CORE_INLINE_H_
#define CNC_RED_ALERT_TD_CORE_INLINE_H_

#include <concepts>
#include <cstdint>

#include "base/array.h"
#include "game/core/display_constants.h"
#include "game/sim/direction.h"
#include "td/core/const.h"
#include "td/core/defines.h"

namespace td {

// Extracts the lower 16 bits (low word) from an integer.
template <std::integral T>
[[nodiscard]] constexpr uint16_t LowWord(T value) {
  return static_cast<uint16_t>(value);
}

// Extracts the upper 16 bits (high word) from a 32-bit integer view.
//
// Note: This casts the input to uint32_t before shifting, mirroring the
// original macro's behavior of discarding bits > 32.
template <std::integral T>
[[nodiscard]] constexpr uint16_t HighWord(T value) {
  return static_cast<uint16_t>(static_cast<uint32_t>(value) >> 16);
}

// Combines two 16-bit words into a 32-bit integer.
//
// Replacement for: MAKE_LONG(a, b)
// WARNING: The original macro defined the order as (High, Low), which is
// opposite to the standard Windows MAKELONG(Low, High).
//
// We explicitly name parameters 'high' and 'low' to prevent confusion.
[[nodiscard]] constexpr int32_t MakeLong(uint16_t high, uint16_t low) {
  // Use unsigned math for the shift to prevent Undefined Behavior,
  // then cast back to the legacy int32_t type.
  return static_cast<int32_t>(static_cast<uint32_t>(high) << 16 |
                              static_cast<uint32_t>(low));
}

/*
**	Inline miscellaneous functions.
*/
// Packs a pixel offset into a Coordinate, truncating each component to whole
// leptons (XYP_Coord rounds instead). Negative offsets wrap in their 16-bit
// component, as the packed sum always did.
constexpr Coordinate Pixel_Offset_Coord(int x, int y) noexcept {
  const auto x_lepton =
      static_cast<uint32_t>((x * ICON_LEPTON_W) / CELL_PIXEL_W);
  const auto y_lepton =
      static_cast<uint32_t>((y * ICON_LEPTON_H) / CELL_PIXEL_H);
  return Coordinate::FromPacked((y_lepton << 16) + x_lepton);
}
inline int Cell_To_Lepton(int cell) { return cell * 256; }
inline int Lepton_To_Cell(int lepton) {
  return static_cast<unsigned>(lepton + 0x0080) >> 8;
}
inline CELL XY_Cell(int x, int y) {
  return static_cast<CELL>(static_cast<unsigned>(y) << 6 |
                           static_cast<unsigned>(x));
}
inline int Cell_X(CELL cell) {
  return static_cast<int>(static_cast<unsigned>(cell) & 0x3F);
}
inline int Cell_Y(CELL cell) {
  return static_cast<int>(static_cast<unsigned>(cell) >> 6);
}
// The coordinate of the cell's center: each component is the cell index in
// the high byte and the lepton offset 0x80 in the low byte.
inline Coordinate Cell_Coord(CELL cell) {
  return Coordinate::FromLeptons((Cell_X(cell) * 256) + 0x80,
                                 (Cell_Y(cell) * 256) + 0x80);
}
inline Coordinate XYPixel_Coord(int x, int y) {
  return Coordinate::FromLeptons(x * ICON_LEPTON_W / ICON_PIXEL_W,
                                 y * ICON_LEPTON_H / ICON_PIXEL_H);
}
inline game::DirType Direction256(Coordinate coord1, Coordinate coord2) {
  return game::Desired_Facing256(coord1.x(), coord1.y(), coord2.x(),
                                 coord2.y());
}
inline game::DirType Direction(Coordinate coord1, Coordinate coord2) {
  return game::Desired_Facing256(coord1.x(), coord1.y(), coord2.x(),
                                 coord2.y());
}
inline game::DirType Direction8(Coordinate coord1, Coordinate coord2) {
  return game::Desired_Facing8(coord1.x(), coord1.y(), coord2.x(), coord2.y());
}
inline game::DirType Direction(CELL cell1, CELL cell2) {
  return game::Desired_Facing8(Cell_X(cell1), Cell_Y(cell1), Cell_X(cell2),
                               Cell_Y(cell2));
}
inline Coordinate Adjacent_Cell(Coordinate coord, game::FacingType dir) {
  return (AdjacentCoord.at(game::AsFacing(static_cast<int>(dir))) + coord)
      .Snapped();
}
inline Coordinate Adjacent_Cell(Coordinate coord, game::DirType dir) {
  return Adjacent_Cell(coord, game::Dir_Facing(dir));
}
inline CELL Adjacent_Cell(CELL cell, game::FacingType dir) {
  // Masked like the Coordinate overload above, so that FACING_NONE (-1) does
  // not index before the start of the table.
  return static_cast<CELL>(
      cell + AdjacentCell.at(game::AsFacing(static_cast<int>(dir))));
}
inline CELL Adjacent_Cell(CELL cell, game::DirType dir) {
  return static_cast<CELL>(cell + AdjacentCell.at(game::Dir_Facing(dir)));
}
inline int Lepton_To_Pixel(int lepton) {
  return ((lepton * ICON_PIXEL_W) + (ICON_LEPTON_W / 2)) / ICON_LEPTON_W;
}
inline int Pixel_To_Lepton(int pixel) {
  return ((pixel * ICON_LEPTON_W) + (ICON_PIXEL_W / 2)) / ICON_PIXEL_W;
}
inline Coordinate XYP_Coord(int x, int y) {
  return Coordinate::FromLeptons(Pixel_To_Lepton(x), Pixel_To_Lepton(y));
}

[[nodiscard]] constexpr CELL Coord_Cell(const Coordinate coord) noexcept {
  // Capture the 'High Word' processing:
  // ((coord >> 16) & 0xFF00) clears the 'AL' equivalent (bits 23-16).
  // Then we shift that right by 2.
  const uint32_t processed_high = (coord.packed() >> 16 & 0xFF00) >> 2;

  // Capture 'BH' (bits 15-8 of original)
  const uint8_t original_mid_byte = coord.packed() >> 8 & 0xFF;

  // Combine them.
  // Note: The 'OR' only affects the bottom 8 bits because 'original_mid_byte'
  // is 8-bit.
  return static_cast<CELL>(processed_high | original_mid_byte);
}

/***********************************************************************************************
 * Distance -- Determines the lepton distance between two coordinates. *
 *                                                                                             *
 *    This routine is used to determine the distance between two coordinates. It
 *uses the      * Dragon Strike method of distance determination and thus it is
 *very fast.                 *
 *                                                                                             *
 * INPUT:   coord1   -- First coordinate. *
 *                                                                                             *
 *          coord2   -- Second coordinate. *
 *                                                                                             *
 * OUTPUT:  Returns the lepton distance between the two coordinates. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/27/1994 JLB : Created. *
 *=============================================================================================*/
inline int Distance(Coordinate coord1, Coordinate coord2) {
  int diff1 = coord1.y() - coord2.y();
  if (diff1 < 0) {
    diff1 = -diff1;
  }
  int diff2 = coord1.x() - coord2.x();
  if (diff2 < 0) {
    diff2 = -diff2;
  }
  if (diff1 > diff2) {
    return diff1 + (diff2 / 2);
  }
  return diff2 + (diff1 / 2);
}

/***********************************************************************************************
 * Distance -- Determines the cell distance between two cells. *
 *                                                                                             *
 *    Use this routine to determine the distance between the two cells
 *specified. The distance * is returned in cells. *
 *                                                                                             *
 * INPUT:   cell1, cell2   -- The two cells to determine the distance between. *
 *                                                                                             *
 * OUTPUT:  Returns with the distance between the two cells in units of cell
 *size.             *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/23/1994 JLB : Created. *
 *=============================================================================================*/
inline int Distance(CELL coord1, CELL coord2) {
  int diff1 = Cell_Y(coord1) - Cell_Y(coord2);
  if (diff1 < 0) {
    diff1 = -diff1;
  }
  int diff2 = Cell_X(coord1) - Cell_X(coord2);
  if (diff2 < 0) {
    diff2 = -diff2;
  }
  if (diff1 > diff2) {
    return diff1 + (diff2 / 2);
  }
  return diff2 + (diff1 / 2);
}

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_INLINE_H_
