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

/* $Header: /CounterStrike/FACING.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : FACING.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/21/95 *
 *                                                                                             *
 *                  Last Update : March 21, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_FACING_H_
#define CNC_RED_ALERT_RA_FACING_H_
#include "ra/face.h"

/*
**	This is a general facing handler class. It is used in those cases where
*facing needs to be *	kept track of, but there could also be an associated
*desired facing. The current facing *	is supposed to transition to the desired
*state over time. Using this class facilitates this *	processing as well as
*isolating the rest of the code from the internals.
*/
class FacingClass {
 public:
  FacingClass();
  // a facing reads and assigns as its direction.
  // NOLINTNEXTLINE(*-explicit-constructor)
  FacingClass(DirType dir) : CurrentFacing(dir), DesiredFacing(dir) {}

  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator DirType() const { return CurrentFacing; }

  [[nodiscard]] DirType Current() const { return CurrentFacing; }
  [[nodiscard]] DirType Desired() const { return DesiredFacing; }

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(CurrentFacing, DesiredFacing);
  }

  int Set_Desired(DirType facing);
  int Set_Current(DirType facing);

  void Set(DirType facing) {
    Set_Current(facing);
    Set_Desired(facing);
  }

  [[nodiscard]] DirType Get() const { return CurrentFacing; }

  [[nodiscard]] int Is_Rotating() const {
    return DesiredFacing != CurrentFacing;
  }
  [[nodiscard]] int Difference() const {
    return static_cast<signed char>((int)DesiredFacing - (int)CurrentFacing);
  }
  [[nodiscard]] int Difference(DirType facing) const {
    return static_cast<signed char>((int)facing - (int)CurrentFacing);
  }
  int Rotation_Adjust(int rate);

 private:
  DirType CurrentFacing;
  DirType DesiredFacing;
};

#endif  // CNC_RED_ALERT_RA_FACING_H_
