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

/* $Header: /CounterStrike/SHAPEBTN.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SHAPEBTN.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 01/15/95 *
 *                                                                                             *
 *                  Last Update : January 15, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_GAME_UI_SHAPEBTN_H_
#define CNC_RED_ALERT_GAME_UI_SHAPEBTN_H_

#include <cstddef>
#include <span>

#include "engine/gfx/keyframe.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/ui/toggle.h"
#include "game/ui/widget_skin.h"

namespace game {

// A button drawn as frames of a shape file: kUpShape, kDownShape and
// kDisabledShape, or the on/off frame when ReflectButtonState is set.
// `Skin::Draw_Shape(view, shapes, frame, x, y)` draws a frame the way the
// game draws its interface shapes.
// NOLINTBEGIN(portability-template-virtual-member-function): see WidgetSkin.
template <WidgetSkin Skin>
class ShapeButtonClass final : public ToggleClass {
 public:
  ShapeButtonClass() noexcept : ToggleClass(0, 0, 0, 0, 0) {}
  ShapeButtonClass(unsigned id, std::span<const std::byte> shapes, int x, int y)
      : ToggleClass(id, x, y, 0, 0) {
    Set_Shape(shapes);
  }

  bool Draw(PixelView& view, bool forced = false) override {
    if (!ControlClass::Draw(view, forced) || ShapeData.empty()) {
      return false;
    }
    int shapenum = 0;
    if (disabled()) {
      shapenum = kDisabledShape;
    } else if (!ReflectButtonState) {
      shapenum = IsPressed ? kDownShape : kUpShape;
    } else {
      shapenum = IsOn;
    }
    Skin::Draw_Shape(view, ShapeData, shapenum, x(), y());
    return true;
  }

  // Takes the button's size from the shape file's frames.
  void Set_Shape(std::span<const std::byte> data) {
    ShapeData = data;
    if (!ShapeData.empty()) {
      set_width(engine::gfx::KeyframeWidth(ShapeData));
      set_height(engine::gfx::KeyframeHeight(ShapeData));
    }
  }
  std::span<const std::byte> Get_Shape_Data() { return ShapeData; }

  static constexpr int kUpShape = 0;  // Shape to use when button is "up".
  static constexpr int kDownShape =
      kUpShape + 1;  // Shape to use when button is "down".
  static constexpr int kDisabledShape =
      kDownShape + 1;  // Shape to use when button is disabled.

  bool ReflectButtonState : 1 = false;

 protected:
  /*
  **	This points to the shape data file. This file contains the appropriate
  **	shapes for this button in the offsets specified above.
  */
  std::span<const std::byte> ShapeData;
};
// NOLINTEND(portability-template-virtual-member-function)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_SHAPEBTN_H_
