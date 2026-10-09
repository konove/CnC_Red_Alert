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

/* $Header: /CounterStrike/SLIDER.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SLIDER.H *
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

#ifndef CNC_RED_ALERT_GAME_UI_SLIDER_H_
#define CNC_RED_ALERT_GAME_UI_SLIDER_H_

#include <algorithm>
#include <memory>

#include "engine/file/mix_archive.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gauge.h"
#include "game/ui/input.h"
#include "game/ui/shapebtn.h"
#include "game/ui/widget_skin.h"

namespace game {

// A gauge whose thumb covers a range of the value: a scroll bar. A stand-alone
// slider gets plus and minus buttons at its ends; one that belongs to a list
// box is drawn as a sunken bar without them.
// NOLINTBEGIN(portability-template-virtual-member-function): see WidgetSkin.
template <WidgetSkin Skin>
class SliderClass final : public GaugeClass<Skin> {
  using Base = GaugeClass<Skin>;

 public:
  SliderClass(unsigned id, int x, int y, int w, int h,
              bool belong_to_list = false)
      : Base(id, x, y, w, h), BelongToList(belong_to_list) {
    if (!BelongToList) {
      PlusGadget = std::make_unique<ShapeButtonClass<Skin>>(
          id, MixArchive::RetrieveData("BTN-PLUS.SHP"),
          this->X + this->Width + 2, this->Y);
      MinusGadget = std::make_unique<ShapeButtonClass<Skin>>(
          id, MixArchive::RetrieveData("BTN-MINS.SHP"), this->X - 6, this->Y);

      // The buttons are not chained behind the slider. They were, but every
      // dialog then appends the slider to its gadgets, which cut them off
      // again: no dialog has ever shown them.
      PlusGadget->Make_Peer(*this);
      PlusGadget->Flag_To_Redraw();
      MinusGadget->Make_Peer(*this);
      MinusGadget->Flag_To_Redraw();
    }
    Set_Thumb_Size(1);
    Recalc_Thumb();

    // Gauges have at least 2 colors, but sliders should only have one.
    this->IsColorized = false;
  }
  ~SliderClass() override {
    // In creation order; the members alone would go in reverse.
    PlusGadget.reset();
    MinusGadget.reset();
  }
  SliderClass(const SliderClass&) = delete;
  SliderClass& operator=(const SliderClass&) = delete;
  SliderClass(SliderClass&&) = delete;
  SliderClass& operator=(SliderClass&&) = delete;

  void Set_Thumb_Size(int value) {
    Thumb = std::min(value, this->MaxValue);
    Thumb = std::max(Thumb, 1);
    this->Flag_To_Redraw();
    Recalc_Thumb();
  }

  bool Set_Maximum(int value) override {
    if (Base::Set_Maximum(value)) {
      Recalc_Thumb();
      return true;
    }
    return false;
  }

  bool Set_Value(int value) override {
    value = std::min(value, this->MaxValue - Thumb);
    if (Base::Set_Value(value)) {
      Recalc_Thumb();
      return true;
    }
    return false;
  }

  // Moves the value one thumb's length up or down.
  bool Bump(bool up) {
    return Set_Value(up ? this->CurValue - Thumb : this->CurValue + Thumb);
  }

  // Moves the value one unit up or down.
  bool Step(bool up) {
    return Set_Value(up ? this->CurValue - 1 : this->CurValue + 1);
  }

  bool Draw_Me(PixelView& view, bool forced) override {
    // A list-attached slider draws a thumb instead of a gauge body; the
    // stand-alone case still goes through GaugeClass below.
    // NOLINTNEXTLINE(bugprone-parent-virtual-call)
    if (BelongToList && ControlClass::Draw_Me(view, forced)) {
      Skin::Draw_Gauge_Box(view, this->X, this->Y, this->Width, this->Height,
                           false);
      Draw_Thumb(view);
      return true;
    }
    return Base::Draw_Me(view, forced);
  }

  void Peer_To_Peer(unsigned flags, engine::window::KeyNumber& /*unused*/,
                    ControlClass& whom) override {
    if (flags & GadgetClass::kLeftRelease) {
      if (&whom == PlusGadget.get()) {
        Step(false);
      }
      if (&whom == MinusGadget.get()) {
        Step(true);
      }
    }
  }

  int Thumb_Pixels() override { return ThumbSize; }

 protected:
  /*
  **	If the slider bar has been created, these point to the respective
  **	gadgets that it is composed of.
  */
  std::unique_ptr<ShapeButtonClass<Skin>> PlusGadget;
  std::unique_ptr<ShapeButtonClass<Skin>> MinusGadget;

  /*
  **	If I belong to a listbox, I have to draw myself differently...
  */
  bool BelongToList : 1;

  /*
  **	This is the logical size of the thumb. This value is used when drawing
  **	the thumb imagery. It is also the amount that is bumped when the
  **	Bump() function is called.  (This value is in application units.)
  */
  int Thumb = 0;

  /*
  **	This is the current thumb pixel size and starting offset from beginning
  **	of slider region.  (These values are in pixels.)
  */
  int ThumbSize = 0;
  int ThumbStart = 0;  // x or y position for the thumb

  bool Action(unsigned flags, engine::window::KeyNumber& key, int x,
              int y) override {
    // Handle the mouse click in a special way. If the click was not on the
    // thumb, then jump the thumb position one "step" in the appropriate
    // direction. Otherwise, let normal processing take place -- the slider
    // then "sticks" and the thumb moves according to mouse position.
    if (flags & GadgetClass::kLeftPress) {
      int mouse = 0;  // Mouse pixel position.
      int edge = 0;   // Edge of slider.
      if (this->IsHorizontal) {
        mouse = TheMouse().x();
        edge = this->X;
      } else {
        mouse = TheMouse().y();
        edge = this->Y;
      }
      edge += 1;

      // Clicking outside the thumb: invoke parent's Action to process flags
      // etc, but turn off the event & return true so processing stops at this
      // button.
      if (mouse < edge + ThumbStart) {
        Bump(true);
        Base::Action(0, key, x, y);
        key = engine::window::KN_NONE;
        return true;
      }
      if (mouse > edge + ThumbStart + ThumbSize) {
        Bump(false);
        Base::Action(0, key, x, y);
        key = engine::window::KN_NONE;
        return true;
      }
      Base::Action(flags, key, x, y);
      key = engine::window::KN_NONE;
      return true;
    }
    return Base::Action(flags, key, x, y);
  }

  void Draw_Thumb(PixelView& view) override {
    if (this->IsHorizontal) {
      Skin::Draw_Gauge_Box(view, this->X + ThumbStart, this->Y, ThumbSize,
                           this->Height, true);
    } else {
      Skin::Draw_Gauge_Box(view, this->X, this->Y + ThumbStart, this->Width,
                           ThumbSize, true);
    }
  }

 private:
  void Recalc_Thumb() {
    const int length = this->IsHorizontal ? this->Width : this->Height;
    const int size = Skin::Scale(length, Thumb, this->MaxValue);
    ThumbSize = std::max(size, 4);
    const int start = Skin::Scale(length, this->CurValue, this->MaxValue);
    ThumbStart = std::min(start, length - ThumbSize);
  }
};
// NOLINTEND(portability-template-virtual-member-function)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_SLIDER_H_
