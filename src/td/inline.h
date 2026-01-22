#ifndef CNC_RED_ALERT_TD_INLINE_H_
#define CNC_RED_ALERT_TD_INLINE_H_

#include <concepts>
#include <cstdint>

#include "td/const.h"
#include "td/defines.h"
#include "td/display_constants.h"
#include "td/face.h"

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
#define XYP_COORD(x, y)                             \
  COORDINATE(((x) * ICON_LEPTON_W) / CELL_PIXEL_W + \
             ((((y) * ICON_LEPTON_H) / CELL_PIXEL_H) << 16))
inline FacingType Dir_Facing(DirType facing) {
  return static_cast<FacingType>(((unsigned char)(facing + 0x10) & 0xFF) >> 5);
}
inline DirType Facing_Dir(FacingType facing) {
  return static_cast<DirType>((int)facing << 5);
}
inline int Cell_To_Lepton(int cell) { return cell << 8; }
inline int Lepton_To_Cell(int lepton) {
  return static_cast<unsigned>(lepton + 0x0080) >> 8;
}
inline CELL XY_Cell(int x, int y) { return static_cast<CELL>(y << 6 | x); }
inline COORDINATE XY_Coord(int x, int y) {
  return static_cast<COORDINATE>(MakeLong(y, x));
}
inline int Coord_X(COORDINATE coord) { return static_cast<short>(LowWord(coord)); }
inline int Coord_Y(COORDINATE coord) { return static_cast<short>(HighWord(coord)); }
inline int Cell_X(CELL cell) { return static_cast<int>((unsigned)cell & 0x3F); }
inline int Cell_Y(CELL cell) { return static_cast<int>((unsigned)cell >> 6); }
inline int Dir_Diff(DirType dir1, DirType dir2) {
  return *(signed char*)&dir2 - *(signed char*)&dir1;
}
inline CELL Coord_XLepton(COORDINATE coord) {
  return *(unsigned char*)&coord;
}
inline CELL Coord_YLepton(COORDINATE coord) {
  return *((unsigned char*)&coord + 2);
}
// inline COORD CellXY_Coord(unsigned x, unsigned y) {return
// (COORD)(MAKE_LONG(y<<8, x<<8));}
inline COORDINATE Coord_Add(COORDINATE coord1, COORDINATE coord2) {
  return static_cast<COORDINATE>(
      MakeLong(*((short*)&coord1 + 1) + *((short*)&coord2 + 1),
               *(short*)&coord1 + *(short*)&coord2));
}
inline COORDINATE Coord_Sub(COORDINATE coord1, COORDINATE coord2) {
  return static_cast<COORDINATE>(
      MakeLong(*((short*)&coord1 + 1) - *((short*)&coord2 + 1),
               *(short*)&coord1 - *(short*)&coord2));
}
inline COORDINATE Coord_Snap(COORDINATE coord) {
  return static_cast<COORDINATE>(
      MakeLong(*((unsigned short*)&coord + 1) & 0xFF00 | 0x80,
               *(unsigned short*)&coord & 0xFF00 | 0x80));
}
inline COORDINATE Coord_Mid(COORDINATE coord1, COORDINATE coord2) {
  return static_cast<COORDINATE>(MakeLong(
      (*((unsigned short*)&coord1 + 1) + *((unsigned short*)&coord2 + 1)) >> 1,
      (*(unsigned short*)&coord1 + *(unsigned short*)&coord2) >> 1));
}
inline COORDINATE Cell_Coord(CELL cell) {
  return static_cast<COORDINATE>(
      MakeLong((cell & 0x0FC0) << 2 | 0x80, (((cell & 0x003F) << 1) + 1) << 7));
}
inline COORDINATE XYPixel_Coord(int x, int y) {
  return static_cast<COORDINATE>(
      MakeLong((int)((long)y * (long)ICON_LEPTON_H /
                     (long)ICON_PIXEL_H) /*+LEPTON_OFFSET_Y*/,
               (int)((long)x * (long)ICON_LEPTON_W /
                     (long)ICON_PIXEL_W) /*+LEPTON_OFFSET_X*/));
}
inline int Facing_To_32(DirType facing) { return Facing32[facing]; }
inline DirType Direction256(COORDINATE coord1, COORDINATE coord2) {
  return Desired_Facing256(Coord_X(coord1), Coord_Y(coord1), Coord_X(coord2),
                           Coord_Y(coord2));
}
inline DirType Direction(COORDINATE coord1, COORDINATE coord2) {
  return Desired_Facing256(Coord_X(coord1), Coord_Y(coord1), Coord_X(coord2),
                           Coord_Y(coord2));
}
inline DirType Direction8(COORDINATE coord1, COORDINATE coord2) {
  return Desired_Facing8(Coord_X(coord1), Coord_Y(coord1), Coord_X(coord2),
                         Coord_Y(coord2));
}
inline DirType Direction(CELL cell1, CELL cell2) {
  return Desired_Facing8(Cell_X(cell1), Cell_Y(cell1), Cell_X(cell2),
                         Cell_Y(cell2));
}
inline COORDINATE Adjacent_Cell(COORDINATE coord, FacingType dir) {
  return Coord_Snap(Coord_Add(AdjacentCoord[dir & 0x07], coord));
}
inline COORDINATE Adjacent_Cell(COORDINATE coord, DirType dir) {
  return Adjacent_Cell(coord, Dir_Facing(dir));
}
inline CELL Adjacent_Cell(CELL cell, FacingType dir) {
  return static_cast<CELL>(cell + AdjacentCell[dir]);
}
inline CELL Adjacent_Cell(CELL cell, DirType dir) {
  return static_cast<CELL>(cell + AdjacentCell[Dir_Facing(dir)]);
}
inline int Lepton_To_Pixel(int lepton) {
  return (lepton * ICON_PIXEL_W + ICON_LEPTON_W / 2) / ICON_LEPTON_W;
}
inline int Pixel_To_Lepton(int pixel) {
  return (pixel * ICON_LEPTON_W + ICON_PIXEL_W / 2) / ICON_PIXEL_W;
}
inline COORDINATE XYP_Coord(int x, int y) {
  return XY_Coord(Pixel_To_Lepton(x), Pixel_To_Lepton(y));
};

inline CELL Coord_XCell(COORDINATE coord) {
  return *((unsigned char*)&coord + 1);
}
inline CELL Coord_YCell(COORDINATE coord) {
  return *((unsigned char*)&coord + 3);
}
[[nodiscard]] constexpr CELL Coord_Cell(const COORDINATE coord) noexcept {
  // Capture the 'High Word' processing:
  // ((coord >> 16) & 0xFF00) clears the 'AL' equivalent (bits 23-16).
  // Then we shift that right by 2.
  const uint32_t processed_high = (coord >> 16 & 0xFF00) >> 2;

  // Capture 'BH' (bits 15-8 of original)
  const uint8_t original_mid_byte = coord >> 8 & 0xFF;

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
inline int Distance(COORDINATE coord1, COORDINATE coord2) {
  int diff1, diff2;

  diff1 = Coord_Y(coord1) - Coord_Y(coord2);
  if (diff1 < 0) diff1 = -diff1;
  diff2 = Coord_X(coord1) - Coord_X(coord2);
  if (diff2 < 0) diff2 = -diff2;
  if (diff1 > diff2) {
    return diff1 + (diff2 >> 1);
  }
  return diff2 + (diff1 >> 1);
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
  int diff1, diff2;

  diff1 = Cell_Y(coord1) - Cell_Y(coord2);
  if (diff1 < 0) diff1 = -diff1;
  diff2 = Cell_X(coord1) - Cell_X(coord2);
  if (diff2 < 0) diff2 = -diff2;
  if (diff1 > diff2) {
    return diff1 + (diff2 >> 1);
  }
  return diff2 + (diff1 >> 1);
}

#endif  // CNC_RED_ALERT_TD_INLINE_H_
