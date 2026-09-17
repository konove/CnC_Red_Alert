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

/* $Header:   F:\projects\c&c\vcs\code\findpath.cpv   2.17   16 Oct 1995
 * 16:51:04   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : FINDPATH.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : May 25, 1995   [PWG] *
 *                                                                                             *
 * The path algorithm works by following a LOS path to the target. If it *
 * collides with an impassable spot, it uses an Edge following routine to * get
 *around it. The edge follower moves along the edge in a clockwise or * counter
 *clockwise fashion until finding the destination spot. The * destination is
 *determined by Find_Path. It is the first passable that                       *
 * can be reached (so it will handle the doughnut case, where there is * a
 *passable in the center of an unreachable area). *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Clear_Path_Overlap -- clears the path overlap list * Find_Path
 *-- Find a path from point a to point b. * Find_Path_Cell -- Finds a given cell
 *on a specified path                                  * Follow_Edge -- Follow
 *an edge to get around an impassable spot.                           *
 *   FootClass::Unravel_Loop -- Unravels a loop in the movement path *
 *   Get_New_XY -- Get the new x,y based on current position and direction. *
 *   Optimize_Moves -- Optimize the move list. * Set_Path_Overlap -- Sets the
 *overlap bit for given cell                                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/wwstd.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/externs.h"
#include "td/foot.h"
#include "td/globals.h"
#include "td/inline.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/path_overlap.h"
#include "td/special.h"
#include "td/support.h"
#include "td/team.h"
#include "td/teamtype.h"

/*
**	When an edge search is started, it can be performed CLOCKwise or
**	COUNTERCLOCKwise direction.
*/
constexpr FacingType kClockwise = static_cast<FacingType>(1);
constexpr FacingType kCounterclockwise = static_cast<FacingType>(-1);

/*
**	If defined, diagonal moves are allowed, else no diagonals.
*/
#define DIAGONAL

/*
**	This is the marker to signify the end of the path list.
*/
#define END FACING_NONE

/*
**	Modify this macro so that given two cell values, it will return
**	a value between 0 and 7, with 0 being North and moving
**	clockwise (just like map degrees).
*/
#define CELL_FACING(a, b) Dir_Facing(::Direction((a), (b)))

/*
**	Maximum lookahead cells. Twice this value in bytes will be
**	reserved on the stack. The smaller this number, the faster the
*processing.
*/
#define MAX_MLIST_SIZE 300
#define THREAT_THRESHOLD 5

#ifdef NEVER
typedef enum {
  FACING_N,   // North
  FACING_NE,  // North-East
  FACING_E,   // East
  FACING_SE,  // South-East
  FACING_S,   // South
  FACING_SW,  // South-West
  FACING_W,   // West
  FACING_NW,  // North-West

  FACING_COUNT  // Total of 8 directions (0..7).
} FacingType;
#endif

/*-------------------------------------------------------------------------*/
static bool DrawPath;

static inline FacingType Opposite(FacingType face) { return face + 4; }

static inline void Draw_Cell_Point(CELL cell, bool passable, int threat_stage,
                                   int overide = 0) {
  if (DrawPath) {
    if (!Debug_Find_Path) {
      int x = 0;
      int y = 0;

      if (Map.Coord_To_Pixel(Cell_Coord(cell), x, y)) {
        if (threat_stage > 2) {
          SeenBuff.Put_Pixel(x, y, passable ? kLtGreen : kRed);
        } else {
          SeenBuff.Put_Pixel(
              x, y,
              static_cast<unsigned char>(passable ? 9 + threat_stage : kRed));
        }
      }
    } else {
      const int x = cell % 64;
      const int y = cell / 64;
      if (!overide) {
        SeenBuff.Put_Pixel(64 + (x * 3) + 1, 8 + (y * 3) + 1,
                           passable ? kWhite : kBlack);
      } else {
        SeenBuff.Put_Pixel(64 + (x * 3) + 1, 8 + (y * 3) + 1,
                           static_cast<unsigned char>(overide));
      }
    }
  }
}

static FacingType Next_Direction(const FacingType current,
                                 const FacingType delta) {
  const FacingType result = current + delta;
#ifndef DIAGONAL
  result = static_cast<FacingType>(result & 0x06);
#endif
  return result;
}

/*=========================================================================*/
/* Define a couple of variables which are private to the module they are   */
/*      declared in.                                                       */
/*=========================================================================*/
// One bit per cell needs no partial word, see td/path_overlap.h.
static_assert(MAP_CELL_TOTAL % 32 == 0);
static uint32_t MainOverlap[MAP_CELL_TOTAL / 32];   // main path
static uint32_t LeftOverlap[MAP_CELL_TOTAL / 32];   // left path
static uint32_t RightOverlap[MAP_CELL_TOTAL / 32];  // right path

// static CELL MoveMask = 0;
static CELL DestLocation;

/*
**	The overlap lists above are bitmaps with one bit per map cell, marking
**	which cells a path has already entered. Every access goes through these
**	helpers, so the "cell is on the map" invariant lives in one place -
**	callers get their cells by stepping to an adjacent cell, which has no
**	notion of the map edge.
**
**	The original picked bit (cell & 31) - 1, which is -1 for the first cell of
**	each word. On x86 SHL masks its count to 5 bits, so that cell used bit 31:
**	a rotation of the bits within the same word. Every read and write goes
**	through these helpers and the buffers are only cleared or copied whole, so
**	any one-to-one bit choice gives identical paths; the plain cell & 31 used
**	here (as in RA) avoids the undefined negative shift.
*/
static bool Is_Overlapped(const PathType* path, CELL cell) {
  DCHECK(cell >= 0 && cell < MAP_CELL_TOTAL);
  return IsOverlapped(path->Overlap, cell);
}

static void Set_Overlap(const PathType* path, CELL cell) {
  DCHECK(cell >= 0 && cell < MAP_CELL_TOTAL);
  SetOverlap(path->Overlap, cell);
}

static void Clear_Overlap(const PathType* path, CELL cell) {
  DCHECK(cell >= 0 && cell < MAP_CELL_TOTAL);
  ClearOverlap(path->Overlap, cell);
}

/***************************************************************************
 * Point_Relative_To_Line -- Relation between a point and a line           *
 *                                                                         *
 *      If a point is on a line then the following function holds true:    *
 *      (x - x2)(z1 - z2) = (z - z2)(x1 - x2) given x,z a point on the     *
 *      line (x1,z1),(x2,z2).                                              *
 *      If the right side is > then the left side then the point is on one *
 *      side of the line and if the right side is < the the left side, then*
 *      the point is on the other side of the line.  By subtracting one side*
 *      from the other we can determine on what side (if any) the point is on*
 *      by testing the side of the resulting subtraction.                  *
 *                                                                         *
 * INPUT:                                                                  *
 *      int   x    - x pos of point.                                       *
 *      int   z    - z pos of point.                                       *
 *      int   x1 - x pos of first end of line segment.                     *
 *      int   z1 - z pos of first end of line segment.                     *
 *      int   x1 - x pos of second end of line segment.                    *
 *      int   z1 - z pos of second end of line segment.                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *   Assuming (x1,z1) is north, (x2,z2) is south:                          *
 *       0 : point is on line.                                             *
 *       > 0 : point is east of line.                                      *
 *       < 0 : point is west of line.                                      *
 *                                                                         *
 * WARNINGS:                                                               *
 *    Remember that int means that is assumes 16 bits of persision.        *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/28/1994 SKB : Created.                                             *
 *=========================================================================*/
static int Point_Relative_To_Line(int x, int z, int x1, int z1, int x2,
                                  int z2) {
  return static_cast<int>(
      ((static_cast<int64_t>(x) - static_cast<int64_t>(x2)) *
       (static_cast<int64_t>(z1) - static_cast<int64_t>(z2))) -
      ((static_cast<int64_t>(z) - static_cast<int64_t>(z2)) *
       (static_cast<int64_t>(x1) - static_cast<int64_t>(x2))));
}

/***************************************************************************
 * FootClass::Unravel_Loop -- Unravels a loop in the movement path         *
 *                                                                         *
 * While in the midst of the Follow Edge logic, it is possible (due to the *
 * fact that we support diagonal movement) to begin looping around a       *
 * column of some type.  The Unravel loop function will scan backward      *
 * through the list and fixup the path to try to prevent the loop.         *
 *                                                                         *
 * INPUT:      path   -   pointer to the generated path so we can pull the *
 *                         commands out of it.                             *
 *               cell   -   the cell we tried to enter that generated the  *
 *                        double overlap condition.                        *
 *               dir    -   the direction we tried to enter from when we   *
 *                        generated the double overlap condition           *
 *               startx -   the start x position of this path segment      *
 *               starty - the start y position of this path segment        *
 *               destx    - the dest x position for this path segment      *
 *               desty    - the dest y position for this path segment      *
 *                                                                         *
 * OUTPUT:      true    - loop has been sucessfully unravelled             *
 *               FALSE  - loop can not be unravelled so abort follow edge  *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/25/1995 PWG : Created.                                             *
 *=========================================================================*/
bool FootClass::Unravel_Loop(PathType* path, CELL& cell, FacingType& dir,
                             int sx, int sy, int dx, int dy,
                             MoveType threshhold) {
  /*
  ** Walk back to the actual cell before we advanced our position
  */
  FacingType curr_dir = dir;
  CELL curr_pos = Adjacent_Cell(cell, Opposite(curr_dir));
  int idx = path->Length;                      // start at the last position
  int list = idx - 1;
  bool last_was_line = false;

  /*
  ** loop backward through the list searching for a point that is
  ** on the line.  If the point was a diagonal move then adjust
  ** it.
  */
  while (idx) {
    const int checkx = Cell_X(curr_pos);
    const int checky = Cell_Y(curr_pos);

    if (!Point_Relative_To_Line(checkx, checky, sx, sy, dx, dy) ||
        last_was_line) {
      /*
      ** We have now found a point on the line.  Now we must check to see
      ** if we left the line on a diagonal.  If we did then we need to fix
      ** it up.
      */
      if (idx > 1 && static_cast<int>(curr_dir) % 2 != 0 &&
          curr_pos != path->LastFixup) {
        cell = curr_pos;
        dir = base::At(path->Command, base::ToSize(list - 1));
        path->Length = idx;
        path->LastFixup = curr_pos;
        Draw_Cell_Point(curr_pos, true, -1, kCyan);
        return true;
      }

      last_was_line = !last_was_line;
    }

    /*
    ** Since this cell will not be in the list, then pull out its cost
    */
    path->Cost -= Passable_Cell(
        curr_pos, base::At(path->Command, base::ToSize(list)), -1, threshhold);

    /*
    ** Remove this cells flag from the overlap list for the path
    */
    Clear_Overlap(path, curr_pos);

    /*
    ** Mark cell on the map
    */
    Draw_Cell_Point(curr_pos, true, -1, kLtCyan);

    /*
    ** Adjust to the next list position and direction.
    */
    curr_dir = base::At(path->Command, base::ToSize(list--));
    curr_pos = Adjacent_Cell(curr_pos, Opposite(curr_dir));
    idx--;
  }

  /*
  ** If we can't modify the list to eliminate the problem, then we have
  ** a larger problem in that we have deleted all of the cells in the
  ** list.
  */
  return false;
}

/***************************************************************************
 * Register_Cell -- registers a cell on our path and check for backtrack   *
 *                                                                         *
 * This function adds a new cell to our path.  If the cell has already     *
 * been recorded as part of our path, then this function moves back down   *
 * the list truncating it at the point we registered that cell.  This      *
 * function will elliminate all backtracking from the list.                *
 *                                                                         *
 * INPUT:      long   * list - the list to set the overlap bit for         *
 *               CELL  cell    - the cell to mark on the overlap list      *
 *                                                                         *
 * OUTPUT:     BOOL - true if bit has been set, FALSE if bit already set   *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/23/1995 PWG : Created.                                             *
 *=========================================================================*/
bool FootClass::Register_Cell(PathType* path, CELL cell, FacingType dir,
                              int cost, MoveType threshhold) {

  /*
  ** See if this point has already been registered as on the list.  If so
  ** we need to truncate the list back to this point and register the
  ** new direction.
  */
  if (Is_Overlapped(path, cell)) {
    /*
    ** If this is not a case of immediate back tracking then handle
    ** by searching the list to see what we find.  However is this is
    ** an immediate back track, then pop of the last direction
    ** and unflag the cell we are in (not the cell we are moving to).
    ** Note: That we do not check for a zero length cell because we
    ** could not have a duplicate unless there are cells in the list.
    */

    if (base::At(path->Command, base::ToSize(path->Length - 1)) ==
        Opposite(dir)) {
      const CELL pos = Adjacent_Cell(cell, Opposite(dir));
      Clear_Overlap(path, pos);
      path->Length--;
      Draw_Cell_Point(pos, true, -1, kBlue);
    } else {
      /*
      ** If this overlap is in the same place as we had our last overlap
      ** then we are in a loop condition.  We need to signify that we
      ** cannot register this cell.
      */
      if (path->LastOverlap == cell) {
        return false;
      }
      path->LastOverlap = cell;

      CELL pos = path->Start;
      int newlen = 0;
      int idx = 0;
      size_t list = 0;

      /*
      ** Note that the cell has to be in this list, so theres no sense
      ** in checking whether we found it (famous last words).
      **
      ** PWG 8/16/95 - However there is no sense searching the list if
      **               the cell we have overlapped on is the cell we
      **               started in.
      */

      if (pos != cell) {
        while (idx < path->Length) {
          pos = Adjacent_Cell(pos, base::At(path->Command, list));
          if (pos == cell) {
            idx++;
            list++;
            break;
          }
          idx++;
          list++;
        }
        newlen = idx;
      }

      /*
      ** Now we are pointing at the next command in the list.  From here on
      ** out we need to unmark the fact that we have entered these cells and
      ** adjust the cost of our path to reflect that we have not entered
      ** then.
      */
      while (idx < path->Length) {
        pos = Adjacent_Cell(pos, base::At(path->Command, list));
        path->Cost -=
            Passable_Cell(pos, base::At(path->Command, list), -1, threshhold);
        Clear_Overlap(path, pos);
        Draw_Cell_Point(pos, true, -1, kLtBlue);
        idx++;
        list++;
      }
      path->Length = newlen;
    }
  } else {
    /*
    ** Now we need to register the new direction, updating the cell structure
    ** and the cost.
    */
    const int cpos = path->Length++;
    base::At(path->Command, base::ToSize(cpos)) =
        dir;                    // save of the direction we moved
    path->Cost += cost;         // figure new cost for cell
    Set_Overlap(path, cell);    // mark the we have entered point
  }
  return true;
}
#ifdef OBSOLETE
bool FootClass::Register_Cell(PathType* path, CELL cell, FacingType dir,
                              int cost, MoveType threshhold) {
  FacingType* list;
  int pos = cell >> 5;
  int bit = (cell & 31) - 1;
  int idx;

  /*
  ** See if this point has already been registered as on the list.  If so
  ** we need to truncate the list back to this point and register the
  ** new direction.
  */
  if (path->Overlap[pos] & (1 << bit)) {
    /*
    ** If this is not a case of immediate back tracking then handle
    ** by searching the list to see what we find.  However is this is
    ** an immediate back track, then pop of the last direction
    ** and unflag the cell we are in (not the cell we are moving to).
    ** Note: That we do not check for a zero length cell because we
    ** could not have a duplicate unless there are cells in the list.
    */

    if (path->Command[base::ToSize(path->Length - 1)] == Opposite(dir)) {
      CELL pos = Adjacent_Cell(cell, Opposite(dir));
      path->Overlap[pos >> 5] &= ~(1 << ((pos & 31) - 1));
      path->Length--;
      Draw_Cell_Point(pos, true, -1, kBlue);
    } else {
      /*
      ** If this overlap is in the same place as we had our last overlap
      ** then we are in a loop condition.  We need to signify that we
      ** cannot register this cell.
      */
      if (path->LastOverlap == cell) {
        return (false);
      } else {
        path->LastOverlap = cell;
      }

      CELL pos = path->Start;
      int newlen = 0;

      /*
      ** Note that the cell has to be in this list, so theres no sense
      ** in checking whether we found it (famous last words)
      */
      for (idx = 0, list = path->Command; idx < path->Length; idx++, list++) {
        pos = Adjacent_Cell(pos, path->Command[list]);
        if (pos == cell) {
          idx++;
          list++;
          break;
        }
      }
      newlen = idx;

      /*
      ** Now we are pointing at the next command in the list.  From here on
      ** out we need to unmark the fact that we have entered these cells and
      ** adjust the cost of our path to reflect that we have not entered
      ** then.
      */
      while (idx < path->Length) {
        pos = Adjacent_Cell(pos, path->Command[list]);
        path->Cost -= Passable_Cell(pos, path->Command[list], -1, threshhold);
        path->Overlap[pos >> 5] &= ~(1 << ((pos & 31) - 1));
        Draw_Cell_Point(pos, true, -1, kLtBlue);
        idx++;
        list++;
      }
      path->Length = newlen;
    }
  } else {
    /*
    ** Now we need to register the new direction, updating the cell structure
    ** and the cost.
    */
    int cpos = path->Length++;
    path->Command[base::ToSize(cpos)] = dir;  // save of the direction we moved
    path->Cost += cost;                // figure new cost for cell
    path->Overlap[pos] |= (1 << bit);  // mark the we have entered point
  }
  return (true);
}
#endif

/***********************************************************************************************
 * Find_Path -- Find a path from point a to point b. *
 *                                                                                             *
 * INPUT:      int source x,y, int destination x,y, char *final moves * array to
 *store moves, int maximum moves we may attempt                          *
 *                                                                                             *
 * OUTPUT:     int number of moves it took (IMPOSSIBLE_MOVES if we could * not
 *reach the destination                                                       *
 *                                                                                             *
 * WARNINGS:   This algorithm assumes that the target is NOT situated * inside
 *an impassable. If this case may arise, the do-while                      *
 *             statement inside the inner while (true) must be changed * to
 *include a check to se if the next_x,y is equal to the                        *
 *             dest_x,y. If it is, then return(IMPOSSIBLE_MOVES). *
 *                                                                                             *
 * HISTORY: * 07/08/1991  CY : Created. *
 *=============================================================================================*/
PathType* FootClass::Find_Path(CELL dest, std::span<FacingType> final_moves,
                               int maxlen, MoveType threshhold) {
  const CELL source = Coord_Cell(Coord);  // Source expressed as cell
  static PathType path;             // Main path control.
  FacingType newdir = FACING_NONE;  // Tentative facing value.

  bool left = false;
  bool                // Was leftward path legal?
      right = false;  // Was rightward path legal?

  int len = 0;          // Length of detour command list.
  int unit_threat = 0;  // Calculated unit threat rating
  FacingType moves_left[MAX_MLIST_SIZE + 2];
  FacingType                            // Counterclockwise move list.
      moves_right[MAX_MLIST_SIZE + 2];  // Clockwise move list.
  PathType pleft;
  PathType pright;                            // Path control structures.
  PathType* which = nullptr;                  // Which path to actually use.
  int threat = 0;
  int threat_stage = 0;

  /*
  ** If we have been provided an illegal place to store our final moves
  ** then forget it.
  */
  if (final_moves.empty() || maxlen <= 0) {
    return nullptr;
  }
  //	IsFindPath = true;

  /*
  ** Set the draw path variable to draw the path of the selected unit
  ** if necessary.
  */
  if (!Debug_Find_Path) {
    DrawPath = IsSelected && Special.IsShowPath;
  } else {
    DrawPath = IsSelected;
  }
  Debug_Draw_Map("Initial Draw", source, dest, false);

  //	MoveMask = flags;
  if (Team && Team->Class->IsRoundAbout) {
    unit_threat = Team ? Team->Risk : Risk();
    threat_stage = 0;
    threat = 0;
  } else {
    unit_threat = threat = -1;
  }

  DestLocation = dest;

  /*
  ** Initialize the path structure so that we can keep track of the
  ** path.
  */
  path.Start = source;
  path.Cost = 0;
  path.Length = 0;
  path.Command = final_moves;
  base::At(path.Command, 0) = END;
  path.Overlap = MainOverlap;
  path.LastOverlap = -1;
  path.LastFixup = -1;

  std::ranges::fill(path.Overlap, 0);

  /*
  ** Clear the over lap list and then make sure that our starting position is
  *marked
  ** on the overlap list.  (Otherwise the harvesters will drive in circles... )
  */
  //	memset(path.Overlap, 0, 512);
  Set_Overlap(&path, source);

  CELL startcell = source;  // Cell we started in

  /*
  **	Account for trailing end of list command, so reduce the maximum
  **	allowed legal commands to reflect this.
  */
  maxlen = std::min(maxlen, static_cast<int>(final_moves.size()));
  maxlen--;

  /*
  **	As long as there is room to put commands in the movement command list,
  ** then put commands in it.  We build the path using the following
  ** methodology.
  **
  ** 1. Scan through the desired strait line path until we eiter hit an
  **    impassable or have created a valid path.
  **
  ** 2. If we have hit an impassable, walk through the impassable to make
  **    sure that there is a passable on the other side.  If there is not
  **    and we can not change the impassable, then this list is dead.
  **
  ** 3. Walk around the impassable on both the left and right edges and
  **    take the shorter of the two paths.
  **
  ** 4. Taking the new location as our start location start again with
  **    step #1.
  */
  while (path.Length < maxlen) {
    bool restart_outer_loop = false;

    /*
    **	Have we reached the destination already?  If so abort any further
    **	command building.
    */
    if (startcell == dest) {
      break;
    }

    /*
    **	Find the absolute correct direction to reach the next straight
    ** line cell and what cell it is.
    */
    const FacingType direction =
        CELL_FACING(startcell, dest);  // Working direction of look ahead.
    CELL next = Adjacent_Cell(startcell, direction);  // Next cell to enter

    /*
    **	If we can move here, then make this our next move.
    */
    const int cost = Passable_Cell(next, direction, threat,
                                   threshhold);  // Cost to enter the square
    if (cost) {
      Draw_Cell_Point(next, true, threat_stage);
      Register_Cell(&path, next, direction, cost, threshhold);
    } else {
      if (Debug_Find_Path && DrawPath) {
        Debug_Draw_Map("Walk Through Obstacle", startcell, dest, true);
      }
      Draw_Cell_Point(next, false, threat_stage);

      /*
      **	If the impassable location is actually the destination,
      **	then stop here and consider this "good enough".
      */
      if (next == dest) {
        break;
      }

      /*
      **	We could not move to the next cell, so follow through the
      **	impassable until we find a passable spot that can be reached.
      ** Once we find a passable, figure out the shortest path to it.
      ** Since we have variable passable conditions this is not as
      ** simple as it used to be.  The limiter loop below allows us to
      ** step through ten donuts before we give up.
      */
      for (int limiter = 0; limiter < 5; limiter++) {
        /*
        **	Get the next passable position by zipping through the
        ** impassable positions until a passable position is found
        **	or the destination is reached.
        */
        for (;;) {
          /*
          **	Move one step closer toward destination.
          */
          newdir = CELL_FACING(next, dest);
          next = Adjacent_Cell(next, newdir);

          /*
          ** If the cell is passable then we have been completely
          ** sucessful.  If the cell is not passable then continue.
          */
          if (Passable_Cell(next, FACING_NONE, threat, threshhold) ||
              next == dest) {
            Draw_Cell_Point(next, true, threat_stage);
            break;
          }
          Draw_Cell_Point(next, false, threat_stage);

          /*
          **	If we reached destination while in this loop, we
          **	know that either the destination is impassible (if
          **	we are ignoring) or that we need to up our threat
          ** tolerance and try again.
          */
          if (next == dest) {
            if (threat != -1) {
              switch (threat_stage++) {
                case 0:
                  threat = unit_threat / 2;
                  break;

                case 1:
                  threat += unit_threat;
                  break;

                case 2:
                  threat = -1;
                  break;
                default:
                  break;
              }
              restart_outer_loop = true;
              break;
            }
            goto end_of_list;
          }
        }

        if (restart_outer_loop) {
          break;
        }

        /*
        **	Try to find a path to the passable position by following
        **	the edge of the blocking object in both CLOCKwise and
        **	COUNTERCLOCKwise fashions.
        */

        Debug_Draw_Map("Follow left edge", startcell, next, true);
        pleft = path;
        pleft.Command = moves_left;
        pleft.Overlap = LeftOverlap;
        std::ranges::copy(path.Command.first(base::ToSize(path.Length)),
                          pleft.Command.begin());
        std::ranges::copy(path.Overlap, pleft.Overlap.begin());
        left =
            Follow_Edge(startcell, next, &pleft, kCounterclockwise, direction,
                        threat, threat_stage, sizeof(moves_left), threshhold);

        /*
        ** If we are in debug mode then let us know how well our left path
        ** did.
        */
        if (Debug_Find_Path && DrawPath) {
          Fancy_Text_Print("   Left", 0, 92, kWhite, kBlack, TPF_6POINT);
          Fancy_Text_Print("Total Steps", 0, 100, kWhite, kBlack, TPF_6POINT);
          if (left) {
            Fancy_Text_Print("    %d", 0, 108, kWhite, kBlack, TPF_6POINT,
                             pleft.Length);
          } else {
            Fancy_Text_Print("   FAIL", 0, 108, kWhite, kBlack, TPF_6POINT);
          }
        }

        Debug_Draw_Map("Follow right edge", startcell, next, true);
        pright = path;
        pright.Command = moves_right;
        pright.Overlap = RightOverlap;
        std::ranges::copy(path.Command.first(base::ToSize(path.Length)),
                          pright.Command.begin());
        std::ranges::copy(path.Overlap, pright.Overlap.begin());
        right =
            Follow_Edge(startcell, next, &pright, kClockwise, direction, threat,
                        threat_stage, sizeof(moves_right), threshhold);
        //				right = Follow_Edge(startcell, next,
        //&pright, kClockwise, direction, threat, threat_stage, follow_len,
        // threshhold);

        /*
        ** If we are in debug mode then let us know how well our right path
        ** did.
        */
        if (Debug_Find_Path && DrawPath) {
          Fancy_Text_Print("  Right", 0, 92, kWhite, kBlack, TPF_6POINT);
          Fancy_Text_Print("Total Steps", 0, 100, kWhite, kBlack, TPF_6POINT);
          if (right) {
            Fancy_Text_Print("    %d", 0, 108, kWhite, kBlack, TPF_6POINT,
                             pright.Length);
          } else {
            Fancy_Text_Print("   FAIL", 0, 108, kWhite, kBlack, TPF_6POINT);
          }
        }

        /*
        **	If we could find a path, break from this loop. Otherwise this
        **	means that we have found a "hole" of passable terrain that
        **	cannot be reached by normal means. Scan forward looking for
        **	the other side of the "doughnut".
        */
        if (left || right) {
          break;
        }

        /*
        **	If no path can be found to the intermediate cell, then
        **	presume we have found a doughnut of some sort. Scan
        **	forward until the next impassable is found and then
        **	process this loop again.
        */
        do {
          /*
          **	If we reached destination while in this loop, we
          **	know that either the destination is impassible (if
          **	we are ignoring) or that we need to up our threat
          ** tolerance and try again.
          */
          if (next == dest) {
            if (threat != -1) {
              switch (threat_stage++) {
                case 0:
                  threat = unit_threat / 2;
                  break;

                case 1:
                  threat += unit_threat;
                  break;

                case 2:
                  threat = -1;
                  break;
                default:
                  break;
              }
              restart_outer_loop = true;
              break;
            }
            goto end_of_list;
          }

          newdir = CELL_FACING(next, dest);
          next = Adjacent_Cell(next, newdir);
        } while (Passable_Cell(next, newdir, threat, threshhold));
      }

      if (restart_outer_loop) {
        continue;
      }

      if (!left && !right) {
        break;
      }

      /*
      **	We found a path around the impassable locations, so figure out
      **	which one was the smallest and copy those moves into the
      **	path.Command array.
      */
      which = &pleft;
      if (right) {
        which = &pright;
        if (left) {
          if (pleft.Length < pright.Length) {
            which = &pleft;
          } else {
            which = &pright;
          }
        }
      }

      /*
      **	Record as much as possible of the shorter of the two
      **	paths. The trailing EOL command is not copied because
      **	this may not be the end of the find path logic.
      */
      len = which->Length;
      len = std::min(len, maxlen);
      if (len > 0) {
        std::ranges::copy(which->Overlap, path.Overlap.begin());
        std::ranges::copy(which->Command.first(base::ToSize(len)),
                          path.Command.begin());
        path.Length = len;
        path.Cost = which->Cost;
        path.LastOverlap = -1;
        path.LastFixup = -1;
      } else {
        break;
      }
      Debug_Draw_Map("Walking to next obstacle", next, dest, true);
    }
    startcell = next;
  }

end_of_list:
  /*
  **	Poke in the stop command.
  */
  if (path.Length < maxlen) {
    base::At(path.Command, base::ToSize(path.Length++)) = END;
  }
  if (Debug_Find_Path && DrawPath) {
    Map.Flag_To_Redraw(true);
  }
/*
**	Optimize the move list but only necessary if
**	diagonal moves are allowed.
*/
#ifdef DIAGONAL
  Optimize_Moves(&path, threshhold);
#endif
  if (Debug_Find_Path && DrawPath) {
    Debug_Draw_Map("Final Generated Path", startcell, dest, false);
    Debug_Draw_Path(&path);
    Get_Key_Num();
  }
  //	IsFindPath = false;
  return &path;
}

/***********************************************************************************************
 * Follow_Edge -- Follow an edge to get around an impassable spot. *
 *                                                                                             *
 * INPUT:   start    -- cell to head from *
 *                                                                                             *
 *            target   -- Target cell to head to. *
 *                                                                                             *
 *          path     -- Pointer to path list structure. *
 *                                                                                             *
 *          search   -- Direction of search (1=clock, -1=counterclock). *
 *                                                                                             *
 *          olddir   -- Facing impassible direction from start. *
 *                                                                                             *
 *          callback -- Function pointer for determining if a cell is * passable
 *or not.                                                       *
 *                                                                                             *
 * OUTPUT:  bool: Could a path be found to the desired cell? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/08/1991  CY : Created. * 06/01/1992  JLB : Optimized &
 *commented.                                                  *
 *=============================================================================================*/
bool FootClass::Follow_Edge(CELL start, CELL target, PathType* path,
                            FacingType search, FacingType olddir, int threat,
                            int threat_stage, int max_cells,
                            MoveType threshhold) {
  FacingType newdir =
      FACING_NONE;  // Direction of facing before surrounding cell check.
  CELL oldcell = 0;
  CELL                // Current cell.
      newcell = 0;    // Tentative new cell.
  int cost = 0;       // Working cost value.
  bool online = true;
  int oldval = 0;
  int cellcount = 0;
  bool forceout = false;
  auto firstdir = static_cast<FacingType>(-1);
  CELL firstcell = -1;
  const int startx = Cell_X(start);
  const int starty = Cell_Y(start);
  const int targetx = Cell_X(target);
  const int targety = Cell_Y(target);

  if (!path) {
    return false;
  }
  path->LastOverlap = -1;
  path->LastFixup = -1;

#ifndef DIAGONAL
  /*
  **	The edge following algorithm doesn't "do" diagonals. Force initial
  *facing *	to be an even 90 degree value. Adjust it in the direction it
  *should be *	rotating.
  */
  if (olddir & 0x01) {
    olddir = Next_Direction(olddir, search);
  }
#endif

  oldcell = start;

  /*
  **	Continue until we find our target, find our original starting spot,
  **	or run out of moves.
  */
  while (path->Length < max_cells) {
    /*
    **	Look in all the adjacent cells to determine a passable one that
    **	most closely matches the desired direction (working in the specified
    **	direction).
    */
    newdir = olddir;
    for (;;) {
      bool forcefail = false;  // Is failure forced?

#ifdef DIAGONAL
      /*
      **	Rotate 45/90 degrees in desired direction.
      */
      newdir = Next_Direction(newdir, search);

      /*
      **	If facing a diagonal we must check the next 90 degree location
      **	to make sure that we don't walk right by the destination. This
      **	will happen if the destination it is at the corner edge of an
      **	impassable that we are moving around.
      */
      // The diagonal facings are the odd ones.
      if (static_cast<int>(newdir) % 2 != 0) {
        // int	x,y;

        CELL checkcell = Adjacent_Cell(
            oldcell,
            Next_Direction(newdir, search));  // Non-diagonal check cell.

        if (checkcell == target) {
          /*
          **	This only works if in fact, it is possible to move to the
          **	cell from the current location.
          */
          cost = Passable_Cell(checkcell, Next_Direction(newdir, search),
                               threat, threshhold);
          if (cost) {
            Draw_Cell_Point(checkcell, true, threat_stage);

            /*
            **	YES! The destination is at the corner of an impassable, so
            **	set the direction to point directly at it and then the
            **	scanning will terminate later.
            */
            newdir = Next_Direction(newdir, search);
            newcell = Adjacent_Cell(oldcell, newdir);
            break;
          }
          Draw_Cell_Point(checkcell, false, threat_stage);
        }

        /*
        **	Perform special diagonal check. If the edge follower would cross
        *the *	diagonal or fall on the diagonal line from the source, then
        *consider *	that cell impassible. Otherwise, the find path algorithm
        *will fail *	when there are two impassible locations located on a
        *diagonal *	that is lined up between the source and destination
        *location.
        **
        ** P.S. It might help if you check the right cell rather than using
        **      the value that just happened to be in checkcell.
        */

        checkcell = Adjacent_Cell(oldcell, newdir);

        const int checkx = Cell_X(checkcell);
        const int checky = Cell_Y(checkcell);
        const int checkval = Point_Relative_To_Line(checkx, checky, startx,
                                                    starty, targetx, targety);
        if (checkval && !online) {
          forcefail = (checkval < 0) != (oldval < 0);
        } else {
          forcefail = false;
        }
        /*
        ** The only exception to the above is when we are directly backtracking
        ** because we could be trying to escape from a culdesack!
        */
        if (forcefail && path->Length > 0 &&
            newdir + 4 ==
                base::At(path->Command, base::ToSize(path->Length - 1))) {
          // ST - 12/18/96 5:15PM		if (forcefail &&
          // (FacingType)(newdir ^ 4) == path->Command[base::ToSize(path->Length
          // - 1)]) {
          forcefail = false;
        }
      }

#else
      newdir = Next_Direction(newdir, search * 2);
#endif

      /*
      **	If we have just checked the same heading we started with,
      **	we are surrounded by impassable characters and we exit.
      */
      if (newdir == olddir) {
        return false;
      }

      /*
      **	Get the new cell.
      */
      newcell = Adjacent_Cell(oldcell, newdir);

      /*
      **	If we found a passable position, this is where we should move.
      */
      if (!forcefail) {
        cost = Passable_Cell(newcell, newdir, threat, threshhold);
        if (cost != 0) {
          Draw_Cell_Point(newcell, true, threat_stage);
          break;
        }
      }
      Draw_Cell_Point(newcell, false, threat_stage, forcefail ? kBrown : 0);
      if (newcell == target) {
        forceout = true;
        break;
      }
    }

    /*
    **	Record the direction.
    */
    if (!forceout) {
      /*
      ** Mark the cell because this is where we need to be.  If register
      ** cell fails then the list has been shortened and we need to adjust
      ** the new direction.
      */
      if (!Register_Cell(path, newcell, newdir, cost, threshhold)) {
        /*
        ** The only reason we could not register a cell is that we are in
        ** a looping situation.  So we need to try and unravel the loop if
        ** we can.
        */
        if (!Unravel_Loop(path, newcell, newdir, startx, starty, targetx,
                          targety, threshhold)) {
          return false;
        }
        /*
        ** Since we need to eliminate a diagonal we must pretend the upon
        ** attaining this square, we were moving turned farther in the
        ** search direction then we really were.
        */
        newdir = Next_Direction(
            newdir, static_cast<FacingType>(static_cast<int>(search) * 2));
      }
      /*
      ** Find out which side of the line this cell is on.  If it is on
      ** a side, then store off that side.
      */
      const int newx = Cell_X(newcell);
      const int newy = Cell_Y(newcell);
      const int val =
          Point_Relative_To_Line(newx, newy, startx, starty, targetx, targety);
      if (val) {
        oldval = val;
        online = false;
      } else {
        online = true;
      }
      cellcount++;
      if (cellcount == 100) {
        //				DrawPath = true;
        //				Debug_Find_Path = true;
        //				Debug_Draw_Map("Loop failure", start,
        // target, false); 				Debug_Draw_Path(path);
        return false;
      }
    }

    /*
    **	If we have found the target spot, we are done.
    */
    if (newcell == target) {
      base::At(path->Command, base::ToSize(path->Length)) = END;
      return true;
    }

    /*
    **	If we make a full circle back to our original spot, get out.
    */
    if (newcell == firstcell && newdir == firstdir) {
      return false;
    }

    if (firstcell == -1) {
      firstcell = newcell;
      firstdir = newdir;
    }

/*
**	Because we moved, our facing is now incorrect. We want to face toward
**	the impassable edge we are following (well, not actually toward, but
**	a little past so that we can turn corners). We have to turn 45/90
*degrees *	more than expected in anticipation of the pending 45/90 degree
*turn at *	the start of this loop.
*/
#ifdef DIAGONAL
    olddir = Next_Direction(
        newdir, static_cast<FacingType>(-static_cast<int>(search) * 3));
#else
    olddir = Next_Direction(newdir, (FacingType)(-(int)search * 4));
#endif
    oldcell = newcell;
  }

  /*
  **	The maximum search path is exhausted... abort with a failure.
  */
  return false;
}

/***********************************************************************************************
 * Optimize_Moves -- Optimize the move list. *
 *                                                                                             *
 * INPUT:      char *moves to optimize *
 *                                                                                             *
 * OUTPUT:     none (list is optimized) *
 *                                                                                             *
 * WARNINGS:   Empty moves are used to hold the place of eliminated * commands.
 *Also, NEVER call this routine with a list that                        *
 *             contains illegal commands. The list MUST be terminated * with a
 *EOL command                                                              *
 *                                                                                             *
 * HISTORY: * 07/08/1991  CY : Created. * 06/01/1992  JLB : Optimized and
 *commented.                                                *
 *=============================================================================================*/
constexpr FacingType kEmptyCommand = static_cast<FacingType>(-2);
int FootClass::Optimize_Moves(PathType* path, MoveType threshhold)
// int Optimize_Moves(PathType *path, int (*callback)(CELL, FacingType), int
// threshold)
{
  /*
  **	Facing command pair adjustment table. Compare the facing difference
  *between *	the two commands. 0 means no optimization is possible. 3 means
  *backtracking *	so eliminate both commands. Any other value adjusts the
  *first command facing.
  */
#ifdef DIAGONAL
  static const base::EnumArray<FacingType, FacingType, kFacingCount> _trans = {
      static_cast<FacingType>(0),  static_cast<FacingType>(0),
      static_cast<FacingType>(1),  static_cast<FacingType>(2),
      static_cast<FacingType>(3),  kEmptyCommand,
      static_cast<FacingType>(-1), static_cast<FacingType>(0)};  // Smoothing.
#else
  static base::EnumArray<FacingType, FacingType, kFacingCount> _trans = {
      (FacingType)0, (FacingType)0, (FacingType)0, (FacingType)2,
      (FacingType)3, kEmptyCommand, (FacingType)0, (FacingType)0};
#endif
  size_t cmd1 = 0;
  size_t cmd2 = 0;
  FacingType newdir = FACING_NONE;  // Tentative new direction for smoothing.

  /*
  **	Abort if there is any illegal parameter.
  */
  if (!path || path->Command.empty()) {
    return 0;
  }

  /*
  **	Optimization loop -- start scanning with the
  **	first pair of commands (if there are at least two
  **	in the command list).
  */
  base::At(path->Command, base::ToSize(path->Length)) =
      END;                            // Force end of list.
  CELL cell = path->Start;            // Working cell (as it moves along path).
  if (path->Length > 1) {
    cmd2 = 1;
    while (base::At(path->Command, cmd2) != END) {
      /*
      **	Set the cmd1 pointer to point to the valid command closest, but
      **	previous to cmd2. Be sure not to go previous to the head of the
      **	command list.
      */
      cmd1 = cmd2 - 1;
      while (base::At(path->Command, cmd1) == kEmptyCommand && cmd1 != 0) {
        cmd1--;
      }

      /*
      **	If there isn't any valid previous command, then bump the
      **	cmd pointers to the next command pair and continue...
      */
      if (base::At(path->Command, cmd1) == kEmptyCommand) {
        cmd2++;
        continue;
      }

      /*
      **	Fetch precalculated command change value. 0 means leave
      **	command set alone, 3 means backtrack and eliminate two
      **	commands. Any other value is new direction and eliminate
      **	one command.
      */
      FacingType newcmd =
          base::At(path->Command, cmd2) -
          base::At(path->Command, cmd1);  // Calculated new optimized command.
      if (newcmd < FACING_N) {
        newcmd = newcmd + FACING_COUNT;
      }
      newcmd = _trans.at(newcmd);

      /*
      **	Check for backtracking. If this occurs, then eliminate the
      **	two commands. This is the easiest optimization.
      */
      if (newcmd == FACING_SE) {
        base::At(path->Command, cmd1) = kEmptyCommand;
        base::At(path->Command, cmd2++) = kEmptyCommand;
        continue;
      }

      /*
      **	If an optimization code was found the process it. The command is
      *a facing *	offset to more directly travel toward the immediate
      *destination cell.
      */
      if (newcmd != FACING_N) {
        /*
        **	Optimizations differ when dealing with diagonals. Especially
        *when dealing *	with diagonals of 90 degrees. In such a case, 90 degree
        *optimizations can *	only be optimized if the intervening cell is
        *passable. The distance travelled *	is the same, but the path is
        *less circuitous.
        */
        // The diagonal facings are the odd ones.
        if (static_cast<int>(base::At(path->Command, cmd1)) % 2 != 0) {
          /*
          **	Diagonal optimizations are always only 45
          **	degree adjustments.
          */
          newdir =
              Next_Direction(base::At(path->Command, cmd1),
                             newcmd < FACING_N ? static_cast<FacingType>(-1)
                                               : static_cast<FacingType>(1));

          /*
          **	Diagonal 90 degree changes can be smoothed, although
          **	the path isn't any shorter.
          */
          if (std::abs(static_cast<int>(newcmd)) == 1) {
            if (Passable_Cell(Adjacent_Cell(cell, newdir), newdir, -1,
                              threshhold)) {
              base::At(path->Command, cmd2) = newdir;
              base::At(path->Command, cmd1) = newdir;
            }
            // BOB 16.12.92
            cell = Adjacent_Cell(cell, base::At(path->Command, cmd1));
            cmd2++;
            continue;
          }
        } else {
          newdir = Next_Direction(base::At(path->Command, cmd1), newcmd);
        }

        /*
        **	Allow shortening turn only on right angle moves that are based
        *on *	90 degrees. Always allow 135 degree optimizations.
        */
        base::At(path->Command, cmd2) = newdir;
        base::At(path->Command, cmd1) = kEmptyCommand;

        /*
        **	Backup what it thinks is the current cell.
        */
        while (base::At(path->Command, cmd1) == kEmptyCommand && cmd1 != 0) {
          cmd1--;
        }
        if (base::At(path->Command, cmd1) != kEmptyCommand) {
          cell = Adjacent_Cell(
              cell, Next_Direction(base::At(path->Command, cmd1), FACING_S));
        } else {
          cell = path->Start;
        }
        continue;
      }

      /*
      **	Since we could not make an optimization, we move our
      **	head pointer forward.
      */
      cell = Adjacent_Cell(cell, base::At(path->Command, cmd1));
      cmd2++;
    }
  }

  /*
  **	Pack the command list to remove any empty command entries.
  */
  cmd1 = 0;
  cmd2 = 0;
  cell = path->Start;
  path->Cost = 0;
  path->Length = 0;
  while (base::At(path->Command, cmd2) != END) {
    if (base::At(path->Command, cmd2) != kEmptyCommand) {
#ifdef NEVER
      if (Debug_ShowPath) {
        int x, y, x1, y1;

        if (Map.Coord_To_Pixel(Cell_Coord(cell), x, y)) {
          Map.Coord_To_Pixel(
              Cell_Coord(Adjacent_Cell(cell, path->Command[cmd2])), x1, y1);
          Set_Logic_Page(SeenBuff);
          LogicPage->Draw_Line(x, y + 8, x1, y1 + 8, kGrey);
        }
      }
#endif

      cell = Adjacent_Cell(cell, base::At(path->Command, cmd2));
      path->Cost +=
          Passable_Cell(cell, base::At(path->Command, cmd2), -1, threshhold);
      path->Length++;
      base::At(path->Command, cmd1++) = base::At(path->Command, cmd2);
    }
    cmd2++;
  }
  path->Length++;
  base::At(path->Command, cmd1) = END;
  return path->Length;
}

CELL FootClass::Safety_Point(CELL src, CELL dst, int start, int max) const {
  const FacingType dir = CELL_FACING(src, dst) + 4 - 1;

  /*
  ** Loop through the different acceptable distances.
  */
  for (int dist = start; dist < max; dist++) {
    /*
    ** Move to the starting location.
    */
    CELL next = dst;

    for (int lp = 0; lp < dist; lp++) {
      next = Adjacent_Cell(next, dir);
    }

    if (static_cast<int>(dir) % 2 != 0) {
      /*
      ** If our direction is diagonal than we need to check
      ** only one side which is as long as both of the old sides
      ** together.
      */
      for (int lp = 0; lp < dist * 2; lp++) {
        next = Adjacent_Cell(next, dir + 3);
        if (Can_Enter_Cell(next) == MOVE_OK) {
          return next;
        }
      }
    } else {
      /*
      ** If our direction is not diagonal than we need to check two
      ** sides so that we are checking a corner like location.
      */
      for (int lp = 0; lp < dist; lp++) {
        next = Adjacent_Cell(next, dir + 2);
        if (Can_Enter_Cell(next) == MOVE_OK) {
          return next;
        }
      }

      for (int lp = 0; lp < dist; lp++) {
        next = Adjacent_Cell(next, dir + 4);
        if (Can_Enter_Cell(next) == MOVE_OK) {
          return next;
        }
      }
    }
  }
  return -1;
}

int FootClass::Passable_Cell(CELL cell, FacingType face, int threat,
                             MoveType threshhold) {
  const MoveType move = Can_Enter_Cell(cell, face);

  if (move < MOVE_MOVING_BLOCK && Distance(cell) > 1) {
    threshhold = MOVE_MOVING_BLOCK;
  }

  if (move > threshhold) {
    return 0;
  }

  if (GameToPlay == GAME_NORMAL) {
    if (threat != -1) {
      if (MapEditClass::Cell_Distance(cell, DestLocation) > THREAT_THRESHOLD) {
        if (MapEditClass::Cell_Threat(cell, Owner()) > threat) {
          return 0;
        }
      }
    }
  }

  static const base::EnumArray<MoveType, int, kMoveCount> _value = {
      1,   //	MOVE_OK
      1,   //	MOVE_CLOAK
      3,   //	MOVE_MOVING_BLOCK
      8,   //	MOVE_DESTROYABLE
      10,  //	MOVE_TEMP
      0    //	MOVE_NO
  };
  return _value.at(move);

#ifdef NEVER
  int can;
  int retval;

  int temp_move_mask = MoveMask;

  if (!House->IsHuman) {
    temp_move_mask &= ~MOVEF_TEMP;
  }

#ifdef NEVER
  if ((!(MoveMask & MOVEF_MOVING_BLOCK)) &&
      Map.Cell_Distance(StartLocation, cell) > 2) {
    temp_move_mask |= MOVEF_MOVING_BLOCK;
  }
#endif

  can = (temp_move_mask & Can_Enter_Cell(cell, face));
  if (can & MOVEF_NO) {
    return (0);
  }

  retval = 1;
  if (can & MOVEF_MOVING_BLOCK) {
    retval += 3;
  }
  if (can & MOVEF_DESTROYABLE) {
    retval += 10;
  }
  if (can & MOVEF_TEMP) {
    retval += 10;
  }

  if (threat != -1) {
    if (Map.Cell_Distance(cell, DestLocation) > THREAT_THRESHOLD) {
      if (Map.Cell_Threat(cell, Owner()) > threat) {
        return (0);
      }
    }
  }

  return (retval);
#endif
}

void FootClass::Debug_Draw_Map(const char* txt, CELL start, CELL dest,
                               bool pause) const {
  if (!Debug_Find_Path || !DrawPath) {
    return;
  }

  if (pause) {
    Get_Key_Num();
  }
  GraphicViewPortClass* page = Set_Logic_Page(SeenBuff);

  VisiblePage.Clear();
  Fancy_Text_Print(txt, 160, 0, kWhite, kBlack, TPF_8POINT | TPF_CENTER);
  for (int x = 0; x < 64; x++) {
    for (int y = 0; y < 64; y++) {
      int color = 0;

      switch (Can_Enter_Cell(XY_Cell(x, y))) {
        case MOVE_OK:
          color = kGreen;
          break;
        case MOVE_MOVING_BLOCK:
          color = kLtGreen;
          break;

        case MOVE_DESTROYABLE:
          color = kYellow;
          break;
        case MOVE_TEMP:
          color = kBrown;
          break;
        case MoveType::MOVE_CLOAK:
        case MoveType::MOVE_NO:
        case MoveType::MOVE_COUNT:
        default:
          color = kRed;
          break;
      }
      if (XY_Cell(x, y) == start) {
        color = kLtBlue;
      }
      if (XY_Cell(x, y) == dest) {
        color = kBlue;
      }
      Fat_Put_Pixel(64 + (x * 3), 8 + (y * 3), static_cast<uint8_t>(color), 3,
                    SeenBuff);
    }
  }
  Set_Logic_Page(page);
}

void FootClass::Debug_Draw_Path(PathType* path) {
  if (!path || path->Command.empty()) {
    return;
  }

  size_t list = 0;
  CELL pos = path->Start;

  for (int idx = 0; idx < path->Length; idx++) {
    pos = Adjacent_Cell(pos, base::At(path->Command, list++));
    Draw_Cell_Point(pos, true, -1, 0);
  }
}
