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

// Dial8Class, an eight-way direction dial gadget.
//
// Originally DIAL8.H by Bill Randolph, February 1995.

#ifndef CNC_RED_ALERT_RA_DIAL8_H_
#define CNC_RED_ALERT_RA_DIAL8_H_

#include "ra/control.h"
#include "ra/defines.h"
#include "ra/face.h"
#include "sdllib/keyboard.h"

// A round dial with eight decorations and a hand that the player turns by
// clicking or dragging with the left mouse button. The hand snaps to the eight
// facings. When it moves, the dial reports its button ID as the key, like any
// other control; the owner then reads the new direction with Get_Direction().
// The map editor uses it to set a unit's or building's facing.
class Dial8Class : public ControlClass {
 public:
  // Creates the dial with button ID `id` in the window-relative rectangle
  // `x`, `y`, `w`, `h` (pixels), pointing in direction `dir`.
  Dial8Class(int id, int x, int y, int w, int h, DirType dir);

  // Returns the direction the dial points in, 0-255. After the player turns
  // it, this is one of the eight snapped values Desired_Facing8() returns.
  [[nodiscard]] DirType Get_Direction() const;
  // Points the dial in direction `dir`, 0-255, and schedules a redraw. Does
  // not report a change to the owner.
  void Set_Direction(DirType dir);

  // Draws the dial and its hand if a redraw is pending or `forced` is true.
  // Returns true if it drew.
  bool Draw_Me(bool forced = false) override;

 protected:
  // Turns the dial to follow the mouse while the left button is held after a
  // press on it. Reports a change in facing to the owner through `key`;
  // presses that leave the facing unchanged, and the release, are swallowed.
  // `flags` are the gadget event bits (kLeftPress, ...).
  bool Action(unsigned flags, KeyNumType& key) override;

 private:
  int FaceX;              // x of the dial's centre, in window pixels
  int FaceY;              // y of the dial's centre, in window pixels
  int FacePoint[8][2]{};  // x, y of each decoration, indexed by FacingType
  int FaceLine[8][2]{};   // x, y of the hand's tip, indexed by FacingType
  DirType Direction;      // direction the dial points in, 0-255
  FacingType Facing;      // Direction snapped to one of the eight facings
  FacingType OldFacing;   // last facing reported or set, to detect a change
};

#endif  // CNC_RED_ALERT_RA_DIAL8_H_
