/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Coordinate system utilities: conversions, distance calculations, movement,
// and cell spillage lists. Called frequently during gameplay.

#include "ra/coord.h"

#include <absl/log/check.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>

#include "base/array.h"
#include "base/numeric.h"
#include "base/trig.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/const.h"
#include "ra/defines.h"
#include "ra/display_constants.h"
#include "ra/face.h"
#include "ra/inline.h"
#include "ra/target.h"
#include "tech/rect.h"

std::span<const int16_t> Coord_Spillage_List(const COORDINATE coord,
                                             const Rect& rect,
                                             const bool no_center) {
  if (!rect.Is_Valid()) {
    static constexpr int16_t _list[] = {kRefreshEol};
    return _list;
  }

  const CELL origin_cell = Coord_Cell(coord);
  const LEPTON x = Coord_X(coord);
  const LEPTON y = Coord_Y(coord);

  // Convert the rect from coord-relative to absolute world leptons.
  LEPTON_COMPOSITE left{};
  LEPTON_COMPOSITE top{};
  LEPTON_COMPOSITE right{};
  LEPTON_COMPOSITE bottom{};
  left.Raw = static_cast<LEPTON>(static_cast<int>(x) +
                                 static_cast<int16_t>(Pixel_To_Lepton(rect.X)));
  top.Raw = static_cast<LEPTON>(static_cast<int>(y) +
                                static_cast<int16_t>(Pixel_To_Lepton(rect.Y)));
  right.Raw = left.Raw + Pixel_To_Lepton(rect.Width - 1);
  bottom.Raw = top.Raw + Pixel_To_Lepton(rect.Height - 1);

  const int cell_left = left.Sub.Cell;
  const int cell_right = right.Sub.Cell;
  const int cell_top = top.Sub.Cell;
  const int cell_bottom = bottom.Sub.Cell;

  int count = 0;
  static int16_t offsets[128];
  std::span<int16_t> ptr(offsets);
  for (int yy = cell_top; yy <= cell_bottom; yy++) {
    for (int xx = cell_left; xx <= cell_right; xx++) {
      if (const auto offset =
              static_cast<int16_t>(XY_Cell(xx, yy) - origin_cell);
          !no_center || offset != 0) {
        base::ConsumeFront(ptr) = offset;
        count++;
        if (count + 2 >= std::ssize(offsets)) {
          break;
        }
      }
    }
    if (count + 2 >= std::ssize(offsets)) {
      break;
    }
  }

  ptr.front() = kRefreshEol;
  return offsets;
}

COORDINATE Coord_Move(const COORDINATE start, const DirType facing,
                      const uint16_t distance) {
  auto x = static_cast<int16_t>(Coord_X(start));
  auto y = static_cast<int16_t>(Coord_Y(start));
  base::MovePoint(x, y, static_cast<uint8_t>(facing),
                  static_cast<int16_t>(distance));
  return XY_Coord(static_cast<LEPTON>(x), static_cast<LEPTON>(y));
}

COORDINATE Coord_Scatter(const COORDINATE coord, const int distance,
                         const bool lock) {
  COORDINATE result = Coord_Move(coord, Random_Pick(DIR_N, DIR_MAX),
                                 static_cast<uint16_t>(distance));

  // If the move overflowed the valid coordinate range, discard it.
  if (result & kHighCoordMask) {
    result = coord;
  }

  if (lock) {
    result = Coord_Snap(result);
  }

  return result;
}

// "Dragon Strike" approximation: max + min/2. Fast and avoids sqrt.
int Distance(const COORDINATE coord1, const COORDINATE coord2) {
  int diff1 = Coord_Y(coord1) - Coord_Y(coord2);
  if (diff1 < 0) {
    diff1 = -diff1;
  }

  int diff2 = Coord_X(coord1) - Coord_X(coord2);
  if (diff2 < 0) {
    diff2 = -diff2;
  }

  if (diff1 > diff2) {
    return diff1 + (diff2 / 2);
  }
  return diff2 + (diff1 / 2);
}

int Distance(const TARGET target1, const TARGET target2) {
  return Distance(As_Coord(target1), As_Coord(target2));
}

std::span<const int16_t> Coord_Spillage_List(const COORDINATE coord,
                                             int maxsize) {
  static const int16_t
      kFacingOffsets[static_cast<int>(magic_enum::enum_count<FacingType>()) +
                     1][5] = {
          {0, -MAP_CELL_W, kRefreshEol, 0, 0},                   // N
          {0, -MAP_CELL_W, 1, -(MAP_CELL_W - 1), kRefreshEol},   // NE
          {0, 1, kRefreshEol, 0, 0},                             // E
          {0, 1, MAP_CELL_W, MAP_CELL_W + 1, kRefreshEol},       // SE
          {0, MAP_CELL_W, kRefreshEol, 0, 0},                    // S
          {0, -1, MAP_CELL_W, MAP_CELL_W - 1, kRefreshEol},      // SW
          {0, -1, kRefreshEol, 0, 0},                            // W
          {0, -1, -MAP_CELL_W, -(MAP_CELL_W + 1), kRefreshEol},  // NW
          {0, kRefreshEol, 0, 0, 0}                              // non-moving.
  };
  static int16_t computed_offsets[10];
  // 4-bit index encoding: bit3=south, bit2=north, bit1=east, bit0=west.
  // Maps each spill combination to a kFacingOffsets row index. -1 = invalid.
  static constexpr signed char kSpillToFacing[16] = {
      8, 6, 2, -1, 0, 7, 1, -1, 4, 5, 3, -1, -1, -1, -1, -1};
  int index = 0;
  int x = 0;
  int y = 0;

  // Objects larger than 2 tiles use a prebuilt 5x5 cell region.
  if (maxsize > ICON_PIXEL_W * 2) {
    static constexpr int16_t _gigundo[] = {
        -((2 * MAP_CELL_W) - 2), -((2 * MAP_CELL_W) - 1),
        -(2 * MAP_CELL_W),       -((2 * MAP_CELL_W) + 1),
        -((2 * MAP_CELL_W) + 2), -((1 * MAP_CELL_W) - 2),
        -((1 * MAP_CELL_W) - 1), -(1 * MAP_CELL_W),
        -((1 * MAP_CELL_W) + 1), -((1 * MAP_CELL_W) + 2),
        -((0 * MAP_CELL_W) - 2), -((0 * MAP_CELL_W) - 1),
        -(0 * MAP_CELL_W),       -((0 * MAP_CELL_W) + 1),
        -((0 * MAP_CELL_W) + 2), (1 * MAP_CELL_W - 2),
        (1 * MAP_CELL_W - 1),    (1 * MAP_CELL_W),
        (1 * MAP_CELL_W + 1),    (1 * MAP_CELL_W + 2),
        +((2 * MAP_CELL_W) - 2), +((2 * MAP_CELL_W) - 1),
        +(2 * MAP_CELL_W),       +((2 * MAP_CELL_W) + 1),
        +((2 * MAP_CELL_W) + 2), kRefreshEol};
    return _gigundo;
  }

  // Objects between 1-2 tiles: compute overlap by checking which cell
  // boundaries the bounding box crosses. Cheaper than redundant redraws.
  if (maxsize > ICON_PIXEL_W) {
    maxsize = std::min(maxsize, ICON_PIXEL_W * 2) / 2;

    x = ICON_PIXEL_W * Coord_XLepton(coord) / ICON_LEPTON_W;
    y = ICON_PIXEL_H * Coord_YLepton(coord) / ICON_LEPTON_H;
    const int left = x - maxsize;
    const int right = x + maxsize;
    const int top = y - maxsize;
    const int bottom = y + maxsize;

    base::At(computed_offsets, index++) = 0;
    if (left < 0) {
      base::At(computed_offsets, index++) = -1;
    }
    if (right >= ICON_PIXEL_W) {
      base::At(computed_offsets, index++) = 1;
    }
    if (top < 0) {
      base::At(computed_offsets, index++) = -MAP_CELL_W;
    }
    if (bottom >= ICON_PIXEL_H) {
      base::At(computed_offsets, index++) = MAP_CELL_W;
    }
    if (left < 0 && top < 0) {
      base::At(computed_offsets, index++) = -(MAP_CELL_W + 1);
    }
    if (right >= ICON_PIXEL_W && bottom >= ICON_PIXEL_H) {
      base::At(computed_offsets, index++) = MAP_CELL_W + 1;
    }
    if (left < 0 && bottom >= ICON_PIXEL_H) {
      base::At(computed_offsets, index++) = MAP_CELL_W - 1;
    }
    if (right >= ICON_PIXEL_H && top < 0) {
      base::At(computed_offsets, index++) = -(MAP_CELL_W - 1);
    }
    base::At(computed_offsets, index) = kRefreshEol;
    return computed_offsets;
  }

  // Lepton threshold: how far from cell center before spilling into neighbors.
  const int spill_threshold =
      base::At(Pixel2Lepton, (ICON_PIXEL_W - maxsize) / 2);

  x = Coord_XLepton(coord) - 0x0080;
  y = Coord_YLepton(coord) - 0x0080;
  // The north/south and east/west bits are set with "else if" so that opposing
  // pairs can never both be set. That keeps index away from the -1 entries in
  // kSpillToFacing, which stand for combinations that cannot occur.
  if (y > spill_threshold) {
    index += 8;  // Spilling South.
  } else if (y < -spill_threshold) {
    index += 4;  // Spilling North.
  }
  if (x > spill_threshold) {
    index += 2;  // Spilling East.
  } else if (x < -spill_threshold) {
    index += 1;  // Spilling West.
  }

  return base::At(kFacingOffsets, base::At(kSpillToFacing, index));
}

CELL Coord_Cell(COORDINATE coord) {
  COORD_COMPOSITE cc{};
  cc.Coord = coord;
  CELL_COMPOSITE cell{};
  cell.Cell = 0;
  cell.Sub.X = cc.Sub.X.Sub.Cell;
  cell.Sub.Y = cc.Sub.Y.Sub.Cell;
  return cell.Cell;
}

void List_Copy(const std::span<const int16_t> source, const int len,
               const std::span<int16_t> dest) {
  CHECK_GE(len, 0);
  CHECK_LE(base::ToSize(len), dest.size());
  for (std::size_t i = 0; i < base::ToSize(len) && i < source.size(); ++i) {
    base::At(dest, i) = base::At(source, i);
    if (base::At(source, i) == kRefreshEol) {
      break;
    }
  }
}
