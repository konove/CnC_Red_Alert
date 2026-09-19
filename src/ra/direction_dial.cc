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

#include "ra/direction_dial.h"

#include <iterator>

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

namespace {

// Where each facing points on a unit circle, in tenths, north first and going
// clockwise to match FacingType; y grows downward. A 45-degree angle meets the
// circle at (.707, .707), written as 7.
constexpr int kCompassTenths[8][2] = {{0, -10}, {7, -7}, {10, 0},  {7, 7},
                                      {0, 10},  {-7, 7}, {-10, 0}, {-7, -7}};

}  // namespace

DirectionDial::DirectionDial(const int id, const int x, const int y,
                             const int width, const int height,
                             const DirType initial_direction)
    : ControlClass(static_cast<unsigned>(id), x, y, width, height,
                   kLeftPress | kLeftHeld | kLeftRelease, true),
      center_x_(X + (Width / 2)),
      center_y_(Y + (Height / 2)),
      direction_(initial_direction),
      facing_(Dir_Facing(direction_)) {
  // The decorations sit at 8/10 of the radius and the hand reaches 6/10 of
  // it, with Width/2 as the x radius and Height/2 as the y radius. Integer
  // division truncates toward zero, so this lands on the same pixels as the
  // original's 32 separate assignments.
  for (int facing = 0; facing < std::ssize(kCompassTenths); ++facing) {
    const auto& offset = base::At(kCompassTenths, facing);
    auto& point = base::At(decoration_points_, facing);
    point[0] = center_x_ + (width * offset[0] * 8 / 2 / 100);
    point[1] = center_y_ + (height * offset[1] * 8 / 2 / 100);
    auto& tip = base::At(hand_tips_, facing);
    tip[0] = center_x_ + (width * offset[0] * 6 / 2 / 100);
    tip[1] = center_y_ + (height * offset[1] * 6 / 2 / 100);
  }
}

bool DirectionDial::Action(const unsigned flags, KeyNumType& key) {
  // We might end up clearing the event bits. Make sure that the sticky
  // process is properly updated anyway: it is what makes StuckOn this dial
  // from the press to the release.
  Sticky_Process(flags);

  // Follow the mouse on a press, or while the button is held after a press on
  // the dial (StuckOn); a drag that starts elsewhere must not turn it.
  if (flags & kLeftPress || (flags & kLeftHeld && StuckOn == this)) {
    const FacingType old_facing = facing_;
    // Get the new dial position, snapped to one of the eight directions.
    direction_ =
        Desired_Facing8(center_x_, center_y_, Get_Mouse_X(), Get_Mouse_Y());
    facing_ = Dir_Facing(direction_);

    if (facing_ != old_facing) {
      // Report the change: ControlClass::Action notifies the peer, replaces
      // `key` with the button ID for the owner and asks for a redraw.
      ControlClass::Action(flags, key);
    } else {
      // The dial has not moved; kill the event so the owner sees nothing.
      // ControlClass::Action leaves `key` alone when given no flags, so it
      // has to be cleared here.
      key = KN_NONE;
      ControlClass::Action(0, key);
    }
    return true;
  }
  // Otherwise nothing changed. A release ends the drag and is swallowed.
  if (flags & kLeftRelease) {
    key = KN_NONE;
  }
  return ControlClass::Action(0, key);
}

bool DirectionDial::Draw_Me(const bool forced) {
  // Redraw only if the parent says a redraw is needed.
  if (!ControlClass::Draw_Me(forced)) {
    return false;
  }
  const RemapControlType* scheme = Get_Color_Scheme();

  // Hide the mouse while drawing on the visible page, so the software cursor
  // does not save and restore pixels the drawing is changing.
  const bool on_screen = LogicPage == &SeenBuff;
  if (on_screen) {
    Hide_Mouse();
  }

  // Draw the background and the eight decorations.
  Draw_Box(X, Y, Width, Height, BOXSTYLE_DOWN, true);
  for (const auto& point : decoration_points_) {
    Draw_Box(point[0] - 1, point[1] - 1, 3, 3, BOXSTYLE_RAISED, false);
  }

  // Draw the hand's shadow one pixel down and right, then the hand.
  const auto& tip = base::At(hand_tips_, static_cast<int>(facing_));
  LogicPage->Draw_Line(center_x_ + 1, center_y_ + 1, tip[0] + 1, tip[1] + 1,
                       scheme->Shadow);
  LogicPage->Draw_Line(center_x_, center_y_, tip[0], tip[1], scheme->Highlight);

  if (on_screen) {
    Show_Mouse();
  }
  return true;
}

DirType DirectionDial::direction() const { return direction_; }

void DirectionDial::set_direction(const DirType direction) {
  direction_ = direction;
  facing_ = Dir_Facing(direction_);
  Flag_To_Redraw();
}
