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

#include "td/coord.h"

#include <algorithm>

#include "td/const.h"
#include "td/display_constants.h"
#include "td/inline.h"
#include "td/jshell.h"
#include "td/rand.h"

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
const short* Coord_Spillage_List(COORDINATE coord, int maxsize) {
  static const short _MoveSpillage[static_cast<int>(FACING_COUNT) + 1][5] = {
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
  static short _manual[10];
  //;	00 = on axis
  //;	01 = below axis
  //;	10 = above axis
  //;	11 = undefined
  static const char _SpillTable[16] = {8, 6, 2, -1, 0,  7,  1,  -1,
                                       4, 5, 3, -1, -1, -1, -1, -1};
  int index = 0;
  int x, y;

  /*
  **	For mondo-enourmo-gigundo objects, use a prebuilt mammoth table
  **	that covers a 5x5 square region.
  */
  if (maxsize > ICON_PIXEL_W * 2) {
    static const short _gigundo[] = {
        -(2 * MAP_CELL_W - 2), -(2 * MAP_CELL_W - 1),
        -(2 * MAP_CELL_W),     -(2 * MAP_CELL_W + 1),
        -(2 * MAP_CELL_W + 2), -(1 * MAP_CELL_W - 2),
        -(1 * MAP_CELL_W - 1), -(1 * MAP_CELL_W),
        -(1 * MAP_CELL_W + 1), -(1 * MAP_CELL_W + 2),
        -(0 * MAP_CELL_W - 2), -(0 * MAP_CELL_W - 1),
        -(0 * MAP_CELL_W),     -(0 * MAP_CELL_W + 1),
        -(0 * MAP_CELL_W + 2), (1 * MAP_CELL_W - 2),
        (1 * MAP_CELL_W - 1),  (1 * MAP_CELL_W),
        (1 * MAP_CELL_W + 1),  (1 * MAP_CELL_W + 2),
        +(2 * MAP_CELL_W - 2), +(2 * MAP_CELL_W - 1),
        +(2 * MAP_CELL_W),     +(2 * MAP_CELL_W + 1),
        +(2 * MAP_CELL_W + 2), REFRESH_EOL};
    return &_gigundo[0];
  }

  /*
  **	For very large objects, build the overlap list by hand. This is time
  *consuming, but *	not nearly as time consuming as drawing even a single
  *cell unnecessarily.
  */
  if (maxsize > ICON_PIXEL_W) {
    maxsize = std::min(maxsize, ICON_PIXEL_W * 2) / 2;

    x = Fixed_To_Cardinal(ICON_PIXEL_W, Coord_XLepton(coord));
    y = Fixed_To_Cardinal(ICON_PIXEL_H, Coord_YLepton(coord));
    int left = x - maxsize;
    int right = x + maxsize;
    int top = y - maxsize;
    int bottom = y + maxsize;

    _manual[index++] = 0;
    if (left < 0) _manual[index++] = -1;
    if (right >= ICON_PIXEL_W) _manual[index++] = 1;
    if (top < 0) _manual[index++] = -MAP_CELL_W;
    if (bottom >= ICON_PIXEL_H) _manual[index++] = MAP_CELL_W;
    if (left < 0 && top < 0) _manual[index++] = -(MAP_CELL_W + 1);
    if (right >= ICON_PIXEL_W && bottom >= ICON_PIXEL_H)
      _manual[index++] = MAP_CELL_W + 1;
    if (left < 0 && bottom >= ICON_PIXEL_H) _manual[index++] = MAP_CELL_W - 1;
    if (right >= ICON_PIXEL_H && top < 0) _manual[index++] = -(MAP_CELL_W - 1);
    _manual[index] = REFRESH_EOL;
    return &_manual[0];
  }

  /*
  **	Determine the number of leptons "leeway" allowed this unit.
  */
  int posval = Pixel2Lepton[(ICON_PIXEL_W - maxsize) / 2];

  x = Coord_XLepton(coord) - 0x0080;
  y = Coord_YLepton(coord) - 0x0080;
  if (y > posval) index |= 0x08;   // Spilling South.
  if (y < -posval) index |= 0x04;  // Spilling North.
  if (x > posval) index |= 0x02;   // Spilling East.
  if (x < -posval) index |= 0x01;  // Spilling West.

  return &_MoveSpillage[_SpillTable[index]][0];
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
COORDINATE Coord_Move(COORDINATE start, DirType dir, unsigned short distance) {
  short x = Coord_X(start);
  short y = Coord_Y(start);

  Move_Point(x, y, dir, distance);
  return XY_Coord(x, y);
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
COORDINATE Coord_Scatter(COORDINATE coord, unsigned distance, bool lock) {
  COORDINATE newcoord;

  newcoord = Coord_Move(coord, Random_Pick(DIR_N, DIR_MAX), distance);

  if (newcoord & 0xC000C000L) newcoord = coord;

  if (lock) {
    newcoord = Coord_Snap(newcoord);
  }

  return newcoord;
}

int calcx(signed short v, short distance) { return (v * distance) >> 7; }

int calcy(signed short v, short distance) { return -((v * distance) >> 7); }

void Move_Point(short& x, short& y, DirType dir, unsigned short distance) {
  static const signed char CosTable[256] = {
      0,    3,    6,    9,    12,   15,   18,   21,   24,   27,   30,
      33,   36,   39,   42,   45,   48,   51,   54,   57,   59,   62,
      65,   67,   70,   73,   75,   78,   80,   82,   85,   87,   89,
      91,   94,   96,   98,   100,  101,  103,  105,  107,  108,  110,
      111,  113,  114,  116,  117,  118,  119,  120,  121,  122,  123,
      123,  124,  125,  125,  126,  126,  126,  126,  126,

      127,  126,  126,  126,  126,  126,  125,  125,  124,  123,  123,
      122,  121,  120,  119,  118,  117,  116,  114,  113,  112,  110,
      108,  107,  105,  103,  102,  100,  98,   96,   94,   91,   89,
      87,   85,   82,   80,   78,   75,   73,   70,   67,   65,   62,
      59,   57,   54,   51,   48,   45,   42,   39,   36,   33,   30,
      27,   24,   21,   18,   15,   12,   9,    6,    3,

      0,    -3,   -6,   -9,   -12,  -15,  -18,  -21,  -24,  -27,  -30,
      -33,  -36,  -39,  -42,  -45,  -48,  -51,  -54,  -57,  -59,  -62,
      -65,  -67,  -70,  -73,  -75,  -78,  -80,  -82,  -85,  -87,  -89,
      -91,  -94,  -96,  -98,  -100, -102, -103, -105, -107, -108, -110,
      -111, -113, -114, -116, -117, -118, -119, -120, -121, -122, -123,
      -123, -124, -125, -125, -126, -126, -126, -126, -126,

      -126, -126, -126, -126, -126, -126, -125, -125, -124, -123, -123,
      -122, -121, -120, -119, -118, -117, -116, -114, -113, -112, -110,
      -108, -107, -105, -103, -102, -100, -98,  -96,  -94,  -91,  -89,
      -87,  -85,  -82,  -80,  -78,  -75,  -73,  -70,  -67,  -65,  -62,
      -59,  -57,  -54,  -51,  -48,  -45,  -42,  -39,  -36,  -33,  -30,
      -27,  -24,  -21,  -18,  -15,  -12,  -9,   -6,   -3,
  };

  static const signed char SinTable[256] = {
      127,  126,  126,  126,  126,  126,  125,  125,  124,  123,  123,
      122,  121,  120,  119,  118,  117,  116,  114,  113,  112,  110,
      108,  107,  105,  103,  102,  100,  98,   96,   94,   91,   89,
      87,   85,   82,   80,   78,   75,   73,   70,   67,   65,   62,
      59,   57,   54,   51,   48,   45,   42,   39,   36,   33,   30,
      27,   24,   21,   18,   15,   12,   9,    6,    3,

      0,    -3,   -6,   -9,   -12,  -15,  -18,  -21,  -24,  -27,  -30,
      -33,  -36,  -39,  -42,  -45,  -48,  -51,  -54,  -57,  -59,  -62,
      -65,  -67,  -70,  -73,  -75,  -78,  -80,  -82,  -85,  -87,  -89,
      -91,  -94,  -96,  -98,  -100, -102, -103, -105, -107, -108, -110,
      -111, -113, -114, -116, -117, -118, -119, -120, -121, -122, -123,
      -123, -124, -125, -125, -126, -126, -126, -126, -126,

      -126, -126, -126, -126, -126, -126, -125, -125, -124, -123, -123,
      -122, -121, -120, -119, -118, -117, -116, -114, -113, -112, -110,
      -108, -107, -105, -103, -102, -100, -98,  -96,  -94,  -91,  -89,
      -87,  -85,  -82,  -80,  -78,  -75,  -73,  -70,  -67,  -65,  -62,
      -59,  -57,  -54,  -51,  -48,  -45,  -42,  -39,  -36,  -33,  -30,
      -27,  -24,  -21,  -18,  -15,  -12,  -9,   -6,   -3,

      0,    3,    6,    9,    12,   15,   18,   21,   24,   27,   30,
      33,   36,   39,   42,   45,   48,   51,   54,   57,   59,   62,
      65,   67,   70,   73,   75,   78,   80,   82,   85,   87,   89,
      91,   94,   96,   98,   100,  101,  103,  105,  107,  108,  110,
      111,  113,  114,  116,  117,  118,  119,  120,  121,  122,  123,
      123,  124,  125,  125,  126,  126,  126,  126,  126,
  };

  x += calcx(CosTable[dir], distance);
  y += calcy(SinTable[dir], distance);
}

unsigned int Cardinal_To_Fixed(unsigned base, unsigned cardinal) {
  if (!base) return 0xFFFF;

  return (cardinal << 8) / base;
}

unsigned int Fixed_To_Cardinal(unsigned base, unsigned fixed) {
  unsigned ret = base * fixed + 0x80;

  if (ret & 0xFF000000) return 0xFFFF;

  return ret >> 8;
}
