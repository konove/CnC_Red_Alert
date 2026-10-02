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

/* $Header: /CounterStrike/INLINE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INLINE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 08/21/96 *
 *                                                                                             *
 *                  Last Update : September 30, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Adjacent_Cell -- Calculate the adjacent cell in the direction
 *specified.                  * Adjacent_Cell -- Calculate the adjacent cell in
 *the direction specified.                  * Cell_Coord -- Convert a cell to a
 *coordinate value.                                       * Cell_To_Lepton --
 *Convert a cell distance into a lepton distance.                         *
 *   Cell_X -- Fetch the X cell component from the cell value. * Cell_Y -- Fetch
 *the Y cell component from the cell value specified.                       *
 *   Coord_Add -- Adds coordinates together. * Coord_Fraction -- Discards all
 *but the sub-cell components of the coordinate.             * Coord_Mid --
 *Finds the midpoint between two coordinates.                                  *
 *   Coord_Snap -- Coerce coordinate to refer to center of a cell. * Coord_Sub
 *-- Subtracts one coordinate from another. * Coord_Whole -- Discards the
 *sub-cell components of the coordinate.                        * Coord_X --
 *Fetches the X lepton component from a coordinate value. * Coord_XCell -- Fetch
 *the X cell component from a coordinate value.                        *
 *   Coord_XLepton -- Fetch the X sub-cell lepton component from the coordinate.
 ** Coord_Y -- Fetch the Y lepton component from the coordinate value. *
 *   Coord_YCell -- Fetch the Y cell component from a coordinate. *
 *   Coord_YLepton -- Fetches the Y lepton sub-cell component from the
 *coordinate.             * Dir_Facing -- Convert a DirType into a FacingType
 *value.                                  * Dir_To_16 -- Convert a facing to a
 *0..15 value.                                           * Dir_To_32 -- Convert
 *a DirType into a 0..31 value.                                        *
 *   Dir_To_8 -- Convert a DirType into a value from 0 to 7. * Direction --
 *Calculates the DirType from one cell to another.                             *
 *   Direction -- Determines the facing value from one coordinate to another. *
 *   Direction256 -- Calculate the facing value from one coordinate to another.
 ** Direction8 -- Fetches the direction from one coordinate to another. *
 *   Distance -- Finds the distance between two arbitrary points. * Facing_Dir
 *-- Convert a FacingType into a DirType. * Lepton_To_Cell -- Convert lepton
 *distance to cell distance.                               * Lepton_To_Pixel --
 *Convert a lepton value into pixel value.                               *
 *   Percent_Chance -- Calculate a percentage chance event. * Pixel_To_Lepton --
 *Convert pixel value into lepton equivalent.                            *
 *   Random_Pick -- Pick a random number in a specified range. *
 *   Sim_Percent_Chance -- Calculates a percentage chance event for local
 *events.              * Sim_Random_Pick -- Picks a random number that will not
 *affect the game.                   * Text_String -- Convert a text number into
 *a text pointer.                                 * XYP_COORD -- Convert pixel
 *components into a coordinate value.                            * XYP_Coord --
 *Combine pixel values into a coordinate.                                      *
 *   XY_Cell -- Create a cell from X and Y cell components. * XY_Coord --
 *Convert X Y lepton components into a COORD. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_CORE_INLINE_H_
#define CNC_RED_ALERT_RA_CORE_INLINE_H_

#include <cstdint>
#include <cstdlib>
#include <iterator>

#include "base/array.h"
#include "base/random.h"
#include "engine/file/string_table.h"
#include "game/sim/direction.h"
#include "ra/ai/scenario.h"
#include "ra/assets/assets.h"
#include "ra/core/const.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/packed.h"  // IWYU pragma: export
#include "ra/map/display.h"
#include "ra/map/world.h"

namespace ra {

// Source of random numbers for events that must NOT affect game logic, such
// as cosmetic animations and one-machine-only effects. Because it never
// influences the simulation, it does not need to stay in sync across
// networked machines, and it is not saved. For sync-critical randomness use
// TheScenario().sync_rng_.
inline RandomClass& LocalRandom() {
  static RandomClass rng;
  return rng;
}

/***********************************************************************************************
 * XYP_COORD -- Convert pixel components into a coordinate value. *
 *                                                                                             *
 *    This routine will take the specified pixel components and convert and
 *combine them into  * a coordinate value. *
 *                                                                                             *
 * INPUT:   x,y   -- The X and Y pixel components to coerce into a coordinate
 *value.           *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate value that matches the pixel values
 *specified.         *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/21/1996 JLB : Created. *
 *=============================================================================================*/
inline COORDINATE XYP_COORD(int x, int y) noexcept {
  return XY_Coord(Pixel_To_Lepton(x), Pixel_To_Lepton(y));
}

/***********************************************************************************************
 * XYP_Coord -- Combine pixel values into a coordinate. *
 *                                                                                             *
 *    This will convert X and Y pixel values into a coordinate. Primarily this
 *is used for     * converting mouse clicks into coordinate values. *
 *                                                                                             *
 * INPUT:   x,y   -- The X and Y pixel coordinates to convert. Origin is upper
 *left corner.    *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate that most closely corresponds to the
 *pixel values      * specified. *
 *                                                                                             *
 * WARNINGS:   The coordinate is relative to the upper left corner (0,0). To
 *conver the        * coordinate to a game relative one, it must be biased by
 *the display coordinate  * of the tactical map and the screen position of the
 *tactical display.            *
 *                                                                                             *
 * HISTORY: * 08/23/1996 JLB : Created. *
 *=============================================================================================*/
inline COORDINATE XYP_Coord(int x, int y) {
  return XY_Coord(Pixel_To_Lepton(x), Pixel_To_Lepton(y));
}

/***********************************************************************************************
 * Direction256 -- Calculate the facing value from one coordinate to another. *
 *                                                                                             *
 *    This will calculate the facing from the first coordinate to the second. *
 *                                                                                             *
 * INPUT:   coord1   -- The first coordinate that facing will be calculated
 *from.              *
 *                                                                                             *
 *          coord2   -- The second coordinate that facing will be calcuated to.
 **
 *                                                                                             *
 * OUTPUT:  Returns with the DirType that is the facing from coord1 to coord2. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline game::DirType Direction256(COORDINATE coord1, COORDINATE coord2) {
  return game::Desired_Facing256(Coord_X(coord1), Coord_Y(coord1),
                                 Coord_X(coord2), Coord_Y(coord2));
}

/***********************************************************************************************
 * Direction -- Determines the facing value from one coordinate to another. *
 *                                                                                             *
 *    This will determine the DirType from the first coordinate to the second. *
 *                                                                                             *
 * INPUT:   coord1   -- The first coordinate that facing will be calculated
 *from.              *
 *                                                                                             *
 *          coord2   -- The second coordinate to calculate facing to. *
 *                                                                                             *
 * OUTPUT:  Returns with the DirType that represents the facing from coordinate
 *1 to coordinate*
 *          2. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline game::DirType Direction(COORDINATE coord1, COORDINATE coord2) {
  return game::Desired_Facing256(Coord_X(coord1), Coord_Y(coord1),
                                 Coord_X(coord2), Coord_Y(coord2));
}

/***********************************************************************************************
 * Direction8 -- Fetches the direction from one coordinate to another. *
 *                                                                                             *
 *    This will calculate the facing from the first coordinate to the second.
 *The return value * is of limited accuracy, but the calculation is fast. *
 *                                                                                             *
 * INPUT:   coord1   -- The coordinate to calculate the facing from. *
 *                                                                                             *
 *          coord2   -- The coordinate to figure the facing to. *
 *                                                                                             *
 * OUTPUT:  Returns with the DirType to get from coordinate 1 to coordinate 2. *
 *                                                                                             *
 * WARNINGS:   The return DirType is only accurate to the 8 primary compass rose
 *directions.   *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline game::DirType Direction8(COORDINATE coord1, COORDINATE coord2) {
  return game::Desired_Facing8(Coord_X(coord1), Coord_Y(coord1),
                               Coord_X(coord2), Coord_Y(coord2));
}

/***********************************************************************************************
 * Adjacent_Cell -- Calculate the adjacent cell in the direction specified. *
 *                                                                                             *
 *    This will coerce the coordinate specified so that it will refer to the
 *immediately       * adjacent cell in the direction specified. *
 *                                                                                             *
 * INPUT:   coord -- The coordinate to calculate the adjacency from. *
 *                                                                                             *
 *          dir   -- The direction to travel to calculate the adjacent cell. *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate the refers to the adjacent cell in the
 *direciton       * specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline COORDINATE Adjacent_Cell(COORDINATE coord, game::FacingType dir) {
  return Coord_Snap(Coord_Add(
      AdjacentCoord.at(game::AsFacing(static_cast<int>(dir))), coord));
}

/***********************************************************************************************
 * Adjacent_Cell -- Calculate the adjacent cell in the direction specified. *
 *                                                                                             *
 *    This will coerce the coordinate specified so that it will refer to the
 *immediately       * adjacent cell in the direction specified. *
 *                                                                                             *
 * INPUT:   coord -- The coordinate to calculate the adjacency from. *
 *                                                                                             *
 *          dir   -- The direction to travel to calculate the adjacent cell. *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate the refers to the adjacent cell in the
 *direciton       * specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline COORDINATE Adjacent_Cell(COORDINATE coord, game::DirType dir) {
  return Adjacent_Cell(coord, game::Dir_Facing(dir));
}

/***********************************************************************************************
 * Adjacent_Cell -- Calculate the adjacent cell in the direction specified. *
 *                                                                                             *
 *    This routine will take the specified cell and coerce it to refer to the
 *immediately      * adjacent cell in the direction specified. *
 *                                                                                             *
 * INPUT:   cell  -- The cell to coerce into an adjacent cell. *
 *                                                                                             *
 *          dir   -- The direction to determine the adjacent cell. *
 *                                                                                             *
 * OUTPUT:  Returns with the cell value that represents the adjacent cell in the
 *direction     * specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline CELL Adjacent_Cell(CELL cell, game::FacingType dir) {
  // Wrapped like the COORDINATE overload above, so that FACING_NONE (-1) does
  // not index before the start of the table.
  return static_cast<CELL>(
      cell + AdjacentCell.at(game::AsFacing(static_cast<int>(dir))));
}

/***********************************************************************************************
 * Adjacent_Cell -- Calculate the adjacent cell in the direction specified. *
 *                                                                                             *
 *    This routine will take the specified cell and coerce it to refer to the
 *immediately      * adjacent cell in the direction specified. *
 *                                                                                             *
 * INPUT:   cell  -- The cell to coerce into an adjacent cell. *
 *                                                                                             *
 *          dir   -- The direction to determine the adjacent cell. *
 *                                                                                             *
 * OUTPUT:  Returns with the cell value that represents the adjacent cell in the
 *direction     * specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline CELL Adjacent_Cell(CELL cell, game::DirType dir) {
  return static_cast<CELL>(cell + AdjacentCell.at(game::Dir_Facing(dir)));
}

/***********************************************************************************************
 * Text_String -- Convert a text number into a text pointer. *
 *                                                                                             *
 *    This routine will convert text numbers (as generated elsewhere) into an
 *actual text      * pointer that can be used for normal purposes. *
 *                                                                                             *
 * INPUT:   string   -- The text number to extract a pointer to. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the text that represents the text number
 *specified.      *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline const char* Text_String(int index) {
  if (index < 0 && -index < std::ssize(TheWorld().name_override())) {
    return base::At(std::span(TheWorld().name_override()), -index - 1).c_str();
  }

  if (index < 1000) {
    return Extract_String(TheAssets().system_strings(), index).data();
  }
  return Extract_String(TheAssets().debug_strings(), index - 1000).data();
}

/***********************************************************************************************
 * Random_Pick -- Pick a random number in a specified range. *
 *                                                                                             *
 *    This routine is used to pick a game influencing random number between
 *(inclusive) the    * range specified. *
 *                                                                                             *
 * INPUT:   a  -- Low limit of range to pick from. *
 *                                                                                             *
 *          b  -- High limit of range to pick from. *
 *                                                                                             *
 * OUTPUT:  Returns with a random number picked between (inclusive) the range of
 *values        * specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/30/1996 JLB : Created. *
 *=============================================================================================*/
template <class T>
T Random_Pick(T a, T b) {
  return T(TheScenario().sync_rng_.InRange(static_cast<int>(a),
                                           static_cast<int>(b)));
}

/***********************************************************************************************
 * Percent_Chance -- Calculate a percentage chance event. *
 *                                                                                             *
 *    This will calculate a percentage chance and return with 'true' as likely
 *as the          * chance value would occur (or less) on a random pick from 1
 *to 100. Thus a                * Percent_Chance(50) would return 'true' 50
 *percent of the time. Percent_Chance(25) would  * return 'true' 25% of the
 *time, etc.                                                      *
 *                                                                                             *
 * INPUT:   percent  -- The percent value to calculate the chance upon. *
 *                                                                                             *
 * OUTPUT:  Returns with 'true' in the same percentage as the percentage number
 *supplied.      *
 *                                                                                             *
 * WARNINGS:   This affects the game syncronization random number generator and
 *should be used * for those events that could affect the game engine. *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline bool Percent_Chance(int percent) {
  return TheScenario().sync_rng_.InRange(0, 99) < percent;
}

/***********************************************************************************************
 * Sim_Random_Pick -- Picks a random number that will not affect the game. *
 *                                                                                             *
 *    Use this routine to pick a random number such that it will be used so that
 *it won't      * actually affect the outcome of the game. It is critical to use
 *this routine for any      * random need that won't be needed on other machines
 *in a multiplayer game. The result     * can be freely used as long as it
 *doesn't affect the outcome of the game.                 *
 *                                                                                             *
 * INPUT:   a  -- Low range of the random number to pick. *
 *                                                                                             *
 *          b  -- High range of the random number to pick. *
 *                                                                                             *
 * OUTPUT:  Returns with a random number between (inclusive) the range limit
 *values            * specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/30/1996 JLB : Created. *
 *=============================================================================================*/
template <class T>
T Sim_Random_Pick(T a, T b) {
  return T(LocalRandom().InRange(static_cast<int>(a), static_cast<int>(b)));
}

/***********************************************************************************************
 * Sim_Percent_Chance -- Calculates a percentage chance event for local events.
 **
 *                                                                                             *
 *    This routine is similar to the normal Percent_Chance() routine except that
 *it doesn't    * alter the main random number gerenator sequence. As such, this
 *routine should be used    * for those events that should have a random
 *character, but will either not affect the     * game engine or are only
 *calculated on one machine in a multiplayer game.                 *
 *                                                                                             *
 * INPUT:   percent  -- The percent chance to calculate the possible return of
 *'true' on.      *
 *                                                                                             *
 * OUTPUT:  Returns 'true' with the same percentage chance as the percent number
 *specified.    * A percent value of 50 means 50%, 25 means 25%, etc. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/26/1996 JLB : Created. *
 *=============================================================================================*/
inline bool Sim_Percent_Chance(int percent) {
  return LocalRandom().InRange(0, 99) < percent;
}

/***********************************************************************************************
 * Distance -- Finds the distance between two arbitrary points. *
 *                                                                                             *
 *    This finds the (Dragon Strike) distance between two arbitrary points in
 *flat space.      * It does this by adding 1/2 the smaller absolute axis
 *difference to the other absolute    * axis distance. The result is rough but
 *quick to calculate.                               *
 *                                                                                             *
 * INPUT:   x1,y1 -- Coordinate location for point 1. *
 *                                                                                             *
 *          x2,y2 -- Coordinate location for point 2. *
 *                                                                                             *
 * OUTPUT:  Returns with the rough distance between the two points. The value
 *returned is      * expressed in the same units as the parameters were
 *specified in.                   *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/30/1996 JLB : Created. *
 *=============================================================================================*/
inline int Distance(int x1, int y1, int x2, int y2) {
  int diff1 = y1 - y2;
  if (diff1 < 0) {
    diff1 = -diff1;
  }
  int diff2 = x1 - x2;
  if (diff2 < 0) {
    diff2 = -diff2;
  }
  if (diff1 > diff2) {
    return diff1 + (diff2 / 2);
  }
  return diff2 + (diff1 / 2);
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_INLINE_H_
