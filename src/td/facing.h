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

/* $Header:   F:\projects\c&c\vcs\code\facing.h_v   1.14   16 Oct 1995 16:46:02
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_FACING_H_
#define CNC_RED_ALERT_TD_FACING_H_

#include "td/defines.h"

/*
**	This is a general facing handler class. It is used in those cases where
*facing needs to be *	kept track of, but there could also be an associated
*desired facing. The current facing *	is supposed to transition to the desired
*state over time. Using this class facilitates this *	processing as well as
*isolating the rest of the code from the internals.
*/
class FacingClass {
 public:
  // Field-wise saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(CurrentFacing, DesiredFacing);
  }

  FacingClass();
  // a facing reads and assigns as its direction.
  // NOLINTNEXTLINE(*-explicit-constructor)
  FacingClass(DirType dir) { CurrentFacing = DesiredFacing = dir; }

  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator DirType() const { return CurrentFacing; }

  [[nodiscard]] DirType Current() const { return CurrentFacing; }
  [[nodiscard]] DirType Desired() const { return DesiredFacing; }

  bool Set_Desired(DirType facing);
  bool Set_Current(DirType facing);

  void Set(DirType facing) {
    Set_Current(facing);
    Set_Desired(facing);
  }

  [[nodiscard]] DirType Get() const { return CurrentFacing; }

  [[nodiscard]] bool Is_Rotating() const {
    return DesiredFacing != CurrentFacing;
  }

  [[nodiscard]] int Difference() const {
    return static_cast<signed char>(static_cast<unsigned char>(DesiredFacing) -
                                    static_cast<unsigned char>(CurrentFacing));
  }
  [[nodiscard]] int Difference(DirType facing) const {
    return static_cast<signed char>(static_cast<signed char>(facing) -
                                    static_cast<signed char>(CurrentFacing));
  }
  bool Rotation_Adjust(int rate);

 private:
  DirType CurrentFacing;
  DirType DesiredFacing;
};

#endif  // CNC_RED_ALERT_TD_FACING_H_
