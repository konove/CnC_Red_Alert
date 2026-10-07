/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\coord.cpv   2.18   16 Oct 1995 16:51:24
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : COORD.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : January 7, 1995 [JLB] *
 *                                                                                             *
 * Support code to handle the coordinate system is located in this module. *
 * Routines here will be called QUITE frequently during play and must be * as
 *efficient as possible. *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Cardinal_To_Fixed -- Converts cardinal numbers into a fixed
 *point number.                 * Coord_Move -- Moves a coordinate an arbitrary
 *direction for an arbitrary distance         * Coord_Scatter -- Determines a
 *random coordinate from an anchor point.                     *
 *   Coord_Spillage_List -- Determines the offset list for cell
 *spillage/occupation.           * Fixed_To_Cardinal -- Converts a fixed point
 *number into a cardinal number.                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/core/coord.h"

#include <algorithm>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/cardinal.h"
#include "base/trig.h"
#include "game/core/cell_size.h"
#include "game/sim/direction.h"
#include "magic_enum/magic_enum.hpp"
#include "td/core/const.h"
#include "td/core/defines.h"
#include "td/core/rand.h"

namespace td {

/***********************************************************************************************
 * Coord_Spillage_List -- Determines the offset list for cell
 *spillage/occupation.             *
 *                                                                                             *
 *    This routine will take an arbitrary position and object size and return
 *with a list of   * cell offsets from the current cell for all cells that are
 *overlapped by the object. The  * first cell offset is always zero, so to just
 *get the adjacent spill cell list, add one   * to the return pointer. *
 *                                                                                             *
 * INPUT:   coord -- The coordinate to examine. *
 *                                                                                             *
 *          maxsize -- The maximum width/height of the object (pixels). *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to a spillage list. *
 *                                                                                             *
 * WARNINGS:   The algorithm is limited to working with a maxsize of 48 or less.
 *Larger values * will generate an incomplete overlap list. *
 *                                                                                             *
 * HISTORY: * 11/06/1993 JLB : Created. * 03/25/1994 JLB : Added width
 *optimization.                                                * 04/29/1994 JLB
 *: Converted to C.                                                          *
 *   06/03/1994 JLB : Converted to general purpose spillage functionality. *
 *   01/07/1995 JLB : Manually calculates spillage list for large objects. *
 *=============================================================================================*/
std::span<const int16_t> Coord_Spillage_List(Coordinate coord, int maxsize) {
  static const int16_t
      MoveSpillage[magic_enum::enum_count<game::Facing>() + 1][5] = {
          {0, -MAP_CELL_W, REFRESH_EOL, 0, 0},                   // N
          {0, -MAP_CELL_W, 1, -(MAP_CELL_W - 1), REFRESH_EOL},   // NE
          {0, 1, REFRESH_EOL, 0, 0},                             // E
          {0, 1, MAP_CELL_W, MAP_CELL_W + 1, REFRESH_EOL},       // SE
          {0, MAP_CELL_W, REFRESH_EOL, 0, 0},                    // S
          {0, -1, MAP_CELL_W, MAP_CELL_W - 1, REFRESH_EOL},      // SW
          {0, -1, REFRESH_EOL, 0, 0},                            // W
          {0, -1, -MAP_CELL_W, -(MAP_CELL_W + 1), REFRESH_EOL},  // NW
          {0, REFRESH_EOL, 0, 0, 0}                              // non-moving.
          //		{0, -MAP_CELL_W, -(MAP_CELL_W-1), 1, MAP_CELL_W+1,
          // MAP_CELL_W, MAP_CELL_W-1, -1, -(MAP_CELL_W+1), REFRESH_EOL}
  };
  static int16_t _manual[10];
  //;	00 = on axis
  //;	01 = below axis
  //;	10 = above axis
  //;	11 = undefined
  static const int SpillTable[16] = {8, 6, 2, -1, 0,  7,  1,  -1,
                                     4, 5, 3, -1, -1, -1, -1, -1};
  int index = 0;
  int x = 0;
  int y = 0;

  /*
  **	For mondo-enourmo-gigundo objects, use a prebuilt mammoth table
  **	that covers a 5x5 square region.
  */
  if (maxsize > game::kCellWidthPixels * 2) {
    static const int16_t _gigundo[] = {
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
        +((2 * MAP_CELL_W) + 2), REFRESH_EOL};
    return _gigundo;
  }

  /*
  **	For very large objects, build the overlap list by hand. This is time
  *consuming, but *	not nearly as time consuming as drawing even a single
  *cell unnecessarily.
  */
  if (maxsize > game::kCellWidthPixels) {
    maxsize = std::min(maxsize, game::kCellWidthPixels * 2) / 2;

    x = base::FixedToCardinal(game::kCellWidthPixels, coord.sub_x());
    y = base::FixedToCardinal(game::kCellHeightPixels, coord.sub_y());
    const int left = x - maxsize;
    const int right = x + maxsize;
    const int top = y - maxsize;
    const int bottom = y + maxsize;

    base::At(_manual, index++) = 0;
    if (left < 0) {
      base::At(_manual, index++) = -1;
    }
    if (right >= game::kCellWidthPixels) {
      base::At(_manual, index++) = 1;
    }
    if (top < 0) {
      base::At(_manual, index++) = -MAP_CELL_W;
    }
    if (bottom >= game::kCellHeightPixels) {
      base::At(_manual, index++) = MAP_CELL_W;
    }
    if (left < 0 && top < 0) {
      base::At(_manual, index++) = -(MAP_CELL_W + 1);
    }
    if (right >= game::kCellWidthPixels && bottom >= game::kCellHeightPixels) {
      base::At(_manual, index++) = MAP_CELL_W + 1;
    }
    if (left < 0 && bottom >= game::kCellHeightPixels) {
      base::At(_manual, index++) = MAP_CELL_W - 1;
    }
    if (right >= game::kCellWidthPixels && top < 0) {
      base::At(_manual, index++) = -(MAP_CELL_W - 1);
    }
    base::At(_manual, index) = REFRESH_EOL;
    return _manual;
  }

  /*
  **	Determine the number of leptons "leeway" allowed this unit.
  */
  const int posval =
      base::At(Pixel2Lepton, (game::kCellWidthPixels - maxsize) / 2);

  x = coord.sub_x() - 0x0080;
  y = coord.sub_y() - 0x0080;
  // The north/south and east/west bits are set with "else if" so that opposing
  // pairs can never both be set. That keeps index away from the -1 entries in
  // _SpillTable, which stand for combinations that cannot occur.
  if (y > posval) {
    index += 8;  // Spilling South.
  } else if (y < -posval) {
    index += 4;  // Spilling North.
  }
  if (x > posval) {
    index += 2;  // Spilling East.
  } else if (x < -posval) {
    index += 1;  // Spilling West.
  }

  return base::At(MoveSpillage, base::At(SpillTable, index));
}

/***********************************************************************************************
 * Coord_Move -- Moves a coordinate an arbitrary direction for an arbitrary
 *distance           *
 *                                                                                             *
 *    This function will move a coordinate in a using SIN and COS arithmetic. *
 *                                                                                             *
 * INPUT:   start    -- The starting coordinate. *
 *                                                                                             *
 *          dir      -- The direction to move the coordinate. *
 *                                                                                             *
 *          distance -- The distance to move the coordinate position (in
 *leptons).             *
 *                                                                                             *
 * OUTPUT:  Returns the new coordinate position. *
 *                                                                                             *
 * WARNINGS:   This routine uses multiplies -- use with caution. *
 *                                                                                             *
 * HISTORY: * 05/27/1994 JLB : Created. *
 *=============================================================================================*/
Coordinate Coord_Move(Coordinate start, game::Direction dir,
                      uint16_t distance) {
  auto x = static_cast<int16_t>(start.x());
  auto y = static_cast<int16_t>(start.y());

  base::MovePoint(x, y, static_cast<uint8_t>(dir.value()),
                  static_cast<int16_t>(distance));
  return Coordinate::FromLeptons(x, y);
}

/***********************************************************************************************
 * Coord_Scatter -- Determines a random coordinate from an anchor point. *
 *                                                                                             *
 *    This routine will perform a scatter algorithm on the specified * anchor
 *point in order to return with another coordinate that is * randomly nearby the
 *original. Typical use of this would be for                           * missile
 *targeting. *
 *                                                                                             *
 * INPUT:   coord    -- This is the anchor coordinate. *
 *                                                                                             *
 *          distance -- This is the distance in pixels that the scatter * should
 *fall within.                                                    *
 *                                                                                             *
 *          lock     -- bool; Convert the new coordinate into a center * cell
 *based coordinate?                                                 *
 *                                                                                             *
 * OUTPUT:  Returns with a new coordinate that is nearby the original. *
 *                                                                                             *
 * WARNINGS:   Maximum pixel scatter distance is 255. *
 *                                                                                             *
 * HISTORY: * 02/01/1992 JLB : Created. * 05/13/1992 JLB : Only uses Random(). *
 *=============================================================================================*/
Coordinate Coord_Scatter(Coordinate coord, int distance, bool lock) {
  Coordinate newcoord = Coord_Move(
      coord, Random_Pick(game::Direction::kNorth, kMaxRandomDirection),
      static_cast<uint16_t>(distance));

  if ((newcoord.packed() & kHighCoordMask) != 0) {
    newcoord = coord;
  }

  if (lock) {
    newcoord = newcoord.Snapped();
  }

  return newcoord;
}

}  // namespace td
