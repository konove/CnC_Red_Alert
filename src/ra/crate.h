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

/* $Header: /CounterStrike/CRATE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CRATE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 08/26/96 *
 *                                                                                             *
 *                  Last Update : August 26, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_CRATE_H_
#define CNC_RED_ALERT_RA_CRATE_H_

#include "ra/defines.h"
#include "ra/jshell.h"
#include "tech/ftimer.h"

class CrateClass {
 public:
  CrateClass() = default;

  // Saved-game state; timer reads re-anchor to the restored game frame.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(Cell, CrateTimer);
    if constexpr (Archive::kIsReading) {
      if (Cell < -1 || Cell >= MAP_CELL_TOTAL) {
        ar.Fail("invalid crate cell");
      }
    }
  }
  void Init() { Make_Invalid(); }
  bool Create_Crate(CELL cell);
  [[nodiscard]] bool Is_Here(CELL cell) const {
    return Is_Valid() && cell == Cell;
  }
  bool Remove_It();
  [[nodiscard]] bool Is_Expired() const {
    return Is_Valid() && CrateTimer.IsFinished();
  }
  [[nodiscard]] bool Is_Valid() const { return Cell != -1; }

 private:
  static bool Put_Crate(CELL& cell);
  static bool Get_Crate(CELL cell);

  void Make_Invalid() {
    Cell = -1;
    CrateTimer.Stop();
  }

  Timer<FrameTickSource> CrateTimer;
  CELL Cell = -1;
};

#endif  // CNC_RED_ALERT_RA_CRATE_H_
