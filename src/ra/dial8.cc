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

// DirectionDial, the eight-way facing dial the map editor shows for the
// selected unit or building.
//
// Originally DIAL8.CPP, written by Bill Randolph in 1994-95; the Red Alert copy
// of the file is credited to Joe L. Bostic, July 1996.

#include "ra/dial8.h"

#include "base/array.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/face.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/inline.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"

DirectionDial::DirectionDial(int id, int x, int y, int width, int height,
                             DirType initial_direction)
    : ControlClass(static_cast<unsigned>(id), x, y, width, height,
                   kLeftPress | kLeftHeld | kLeftRelease, true),
      center_x_(X + (Width / 2)),
      center_y_(Y + (Height / 2)),
      direction_(initial_direction),
      facing_(Dir_Facing(direction_)),
      last_facing_(facing_) {
  // The centre and the initial direction are set in the initializer list.
  //
  // Compute the drawing dimensions: a 45-degree angle intersects a unit circle
  // at (.707, .707), written as 7/10 in integer math. The decorations sit at
  // 8/10 of the radius and the hand reaches 6/10 of it, with Width/2 as the x
  // radius and Height/2 as the y radius. Index 0 is north and the points go
  // clockwise, matching FacingType, so Draw_Me() can index by facing_.
  decoration_points_[0][0] = center_x_;
  decoration_points_[0][1] = center_y_ - (height * 8 / 2 / 10);

  decoration_points_[1][0] = center_x_ + (width * 7 * 8 / 2 / 100);
  decoration_points_[1][1] = center_y_ - (height * 7 * 8 / 2 / 100);

  decoration_points_[2][0] = center_x_ + (width * 8 / 2 / 10);
  decoration_points_[2][1] = center_y_;

  decoration_points_[3][0] = center_x_ + (width * 7 * 8 / 2 / 100);
  decoration_points_[3][1] = center_y_ + (height * 7 * 8 / 2 / 100);

  decoration_points_[4][0] = center_x_;
  decoration_points_[4][1] = center_y_ + (height * 8 / 2 / 10);

  decoration_points_[5][0] = center_x_ - (width * 7 * 8 / 2 / 100);
  decoration_points_[5][1] = center_y_ + (height * 7 * 8 / 2 / 100);

  decoration_points_[6][0] = center_x_ - (width * 8 / 2 / 10);
  decoration_points_[6][1] = center_y_;

  decoration_points_[7][0] = center_x_ - (width * 7 * 8 / 2 / 100);
  decoration_points_[7][1] = center_y_ - (height * 7 * 8 / 2 / 100);

  hand_tips_[0][0] = center_x_;
  hand_tips_[0][1] = center_y_ - (height * 6 / 2 / 10);

  hand_tips_[1][0] = center_x_ + (width * 7 * 6 / 2 / 100);
  hand_tips_[1][1] = center_y_ - (height * 7 * 6 / 2 / 100);

  hand_tips_[2][0] = center_x_ + (width * 6 / 2 / 10);
  hand_tips_[2][1] = center_y_;

  hand_tips_[3][0] = center_x_ + (width * 7 * 6 / 2 / 100);
  hand_tips_[3][1] = center_y_ + (height * 7 * 6 / 2 / 100);

  hand_tips_[4][0] = center_x_;
  hand_tips_[4][1] = center_y_ + (height * 6 / 2 / 10);

  hand_tips_[5][0] = center_x_ - (width * 7 * 6 / 2 / 100);
  hand_tips_[5][1] = center_y_ + (height * 7 * 6 / 2 / 100);

  hand_tips_[6][0] = center_x_ - (width * 6 / 2 / 10);
  hand_tips_[6][1] = center_y_;

  hand_tips_[7][0] = center_x_ - (width * 7 * 6 / 2 / 100);
  hand_tips_[7][1] = center_y_ - (height * 7 * 6 / 2 / 100);
}

bool DirectionDial::Action(unsigned flags, KeyNumType& key) {
  // Set by a press on the dial and cleared by the release, so that dragging
  // into the dial with the button already down does not turn it. The dial is
  // sticky: once pressed it receives every mouse event until the release,
  // wherever the pointer goes. A function static, so every dial shares it; the
  // map editor only ever has one.
  static int pressed_on_dial = 0;

  // We might end up clearing the event bits. Make sure that the sticky
  // process is properly updated anyway.
  Sticky_Process(flags);

  if (flags & kLeftPress) {
    pressed_on_dial = 1;
  }

  // If the left button is pressed, or held after a press on the dial, and
  // the dial has changed its direction, invoke the parent Action routine:
  // GadgetClass::Action handles sticky processing and asks for a redraw if
  // any flag bits are set; ControlClass::Action notifies the peer and, when
  // flags are set, replaces `key` with the button ID. With no flags it leaves
  // `key` alone, which is why this function clears it itself before passing
  // 0.
  if (flags & kLeftPress || (flags & kLeftHeld && pressed_on_dial)) {
    // Get the new dial position, snapped to one of the eight directions.
    direction_ =
        Desired_Facing8(center_x_, center_y_, Get_Mouse_X(), Get_Mouse_Y());

    // Convert to a FacingType (0-7).
    facing_ = Dir_Facing(direction_);

    // If it has moved, report the change to the owner (the button ID in
    // `key`) and redraw.
    if (facing_ != last_facing_) {
      last_facing_ = facing_;
      ControlClass::Action(flags, key);
      return true;
    }
    // The dial has not moved; kill the event so the owner sees nothing.
    key = KN_NONE;
    ControlClass::Action(0, key);
    return true;
  }
  // Otherwise nothing changed. A release ends the drag and is swallowed.
  if (flags & kLeftRelease) {
    key = KN_NONE;
    pressed_on_dial = 0;
  }
  return ControlClass::Action(0, key);
}

bool DirectionDial::Draw_Me(bool forced) {
  const RemapControlType* scheme = Get_Color_Scheme();

  // Redraw only if the parent says a redraw is needed.
  if (ControlClass::Draw_Me(forced)) {
    // Hide the mouse while drawing on the visible page, so the software
    // cursor does not save and restore pixels the drawing is changing.
    if (LogicPage == &SeenBuff) {
      Hide_Mouse();
    }

    // Draw the background and the eight decorations.
    Draw_Box(X, Y, Width, Height, BOXSTYLE_DOWN, true);
    for (const auto& point : decoration_points_) {
      Draw_Box(point[0] - 1, point[1] - 1, 3, 3, BOXSTYLE_RAISED, false);
    }

    // Draw the hand's shadow one pixel down and right, then the hand.
    LogicPage->Draw_Line(
        center_x_ + 1, center_y_ + 1,
        base::At(base::At(hand_tips_, static_cast<int>(facing_)), 0) + 1,
        base::At(base::At(hand_tips_, static_cast<int>(facing_)), 1) + 1,
        scheme->Shadow);
    LogicPage->Draw_Line(
        center_x_, center_y_,
        base::At(base::At(hand_tips_, static_cast<int>(facing_)), 0),
        base::At(base::At(hand_tips_, static_cast<int>(facing_)), 1),
        scheme->Highlight);

    // Restore the mouse.
    if (LogicPage == &SeenBuff) {
      Show_Mouse();
    }

    return true;
  }

  return false;
}

DirType DirectionDial::direction() const { return direction_; }

void DirectionDial::set_direction(DirType direction) {
  direction_ = direction;
  facing_ = Dir_Facing(direction_);
  last_facing_ = facing_;
  Flag_To_Redraw();
}
