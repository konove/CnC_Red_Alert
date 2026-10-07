#ifndef CNC_RED_ALERT_TD_CORE_INLINE_H_
#define CNC_RED_ALERT_TD_CORE_INLINE_H_

#include <cstdint>

#include "base/array.h"
#include "game/core/cell_size.h"
#include "game/sim/direction.h"
#include "td/core/const.h"
#include "td/core/defines.h"

namespace td {

/*
**	Inline miscellaneous functions.
*/
// Packs a pixel offset into a Coordinate, truncating each component to whole
// leptons (XYP_Coord rounds instead). Negative offsets wrap in their 16-bit
// component, as the packed sum always did.
constexpr Coordinate Pixel_Offset_Coord(int x, int y) noexcept {
  const auto x_lepton = static_cast<uint32_t>((x * game::kCellWidthLeptons) /
                                              game::kCellWidthPixels);
  const auto y_lepton = static_cast<uint32_t>((y * game::kCellHeightLeptons) /
                                              game::kCellHeightPixels);
  return Coordinate::FromPacked((y_lepton << 16) + x_lepton);
}
inline int Cell_To_Lepton(int cell) { return cell * 256; }
inline int Lepton_To_Cell(int lepton) {
  return static_cast<unsigned>(lepton + 0x0080) >> 8;
}
inline Coordinate XYPixel_Coord(int x, int y) {
  return Coordinate::FromLeptons(
      x * game::kCellWidthLeptons / game::kCellWidthPixels,
      y * game::kCellHeightLeptons / game::kCellHeightPixels);
}
inline game::Direction Direction256(Coordinate coord1, Coordinate coord2) {
  return game::Direction256Toward(coord1.x(), coord1.y(), coord2.x(),
                                  coord2.y());
}
inline game::Direction DirectionBetween(Coordinate coord1, Coordinate coord2) {
  return game::Direction256Toward(coord1.x(), coord1.y(), coord2.x(),
                                  coord2.y());
}
inline game::Direction Direction8(Coordinate coord1, Coordinate coord2) {
  return game::Direction8Toward(coord1.x(), coord1.y(), coord2.x(), coord2.y());
}
inline game::Direction DirectionBetween(Cell cell1, Cell cell2) {
  return game::Direction8Toward(cell1.x(), cell1.y(), cell2.x(), cell2.y());
}
inline Coordinate Adjacent_Cell(Coordinate coord, game::Facing dir) {
  return (AdjacentCoord.at(game::AsFacing(static_cast<int>(dir))) + coord)
      .Snapped();
}
inline Coordinate Adjacent_Cell(Coordinate coord, game::Direction dir) {
  return Adjacent_Cell(coord, game::NearestFacing(dir));
}
inline Cell Adjacent_Cell(Cell cell, game::Facing dir) {
  // Masked like the Coordinate overload above, so that Facing::kNone (-1)
  // does not index before the start of the table.
  return cell.Offset(AdjacentCell.at(game::AsFacing(static_cast<int>(dir))));
}
inline Cell Adjacent_Cell(Cell cell, game::Direction dir) {
  return cell.Offset(AdjacentCell.at(game::NearestFacing(dir)));
}
inline int Lepton_To_Pixel(int lepton) {
  return ((lepton * game::kCellWidthPixels) + (game::kCellWidthLeptons / 2)) /
         game::kCellWidthLeptons;
}
inline int Pixel_To_Lepton(int pixel) {
  return ((pixel * game::kCellWidthLeptons) + (game::kCellWidthPixels / 2)) /
         game::kCellWidthPixels;
}
inline Coordinate XYP_Coord(int x, int y) {
  return Coordinate::FromLeptons(Pixel_To_Lepton(x), Pixel_To_Lepton(y));
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
inline int Distance(Cell coord1, Cell coord2) {
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

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_INLINE_H_
