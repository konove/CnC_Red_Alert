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

/* $Header: /CounterStrike/GAUGE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : GAUGE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic, Maria del Mar McCready Legg *
 *                                                                                             *
 *                   Start Date : 01/15/95 *
 *                                                                                             *
 *                  Last Update : January 15, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_GAME_UI_GAUGE_H_
#define CNC_RED_ALERT_GAME_UI_GAUGE_H_

#include "base/rect.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "game/ui/widget_skin.h"

namespace game {

// A horizontal or vertical bar showing a value from 0 to its maximum, filled
// up to the value and optionally with a thumb the player drags.
// NOLINTBEGIN(portability-template-virtual-member-function): see WidgetSkin.
template <WidgetSkin Skin>
class GaugeClass : public ControlClass {
 public:
  GaugeClass(unsigned id, int x, int y, int w, int h)
      : ControlClass(id, x, y, w, h, kLeftHeld | kLeftPress | kLeftRelease,
                     true),
        IsHorizontal(w > h) {
    // Set the range directly. Set_Maximum and Set_Value are virtual --
    // SliderClass overrides both to recalculate the thumb -- so a constructor
    // cannot reach the override, and they would compare against
    // uninitialised members here anyway. SliderClass's own constructor calls
    // Set_Thumb_Size, which does the recalc.
  }

  bool Draw_Me(PixelView& view, bool forced = false) override {
    if (!ControlClass::Draw_Me(view, forced)) {
      return false;
    }
    Skin::Draw_Gauge_Box(view, X, Y, Width, Height, false);

    // Colourize the inside of the gauge if indicated.
    if (IsColorized) {
      const int middle = Value_To_Pixel(CurValue);
      const engine::gfx::PaletteIndex color = Skin::Gauge_Fill_Color();
      if (IsHorizontal) {
        if (middle >= X + 1) {
          view.FillRect(
              base::Rect::FromCorners(X + 1, Y + 1, middle, Y + Height - 2),
              color);
        }
      } else {
        if (middle >= Y + 1) {
          view.FillRect(
              base::Rect::FromCorners(X + 1, Y + 1, X + Width - 2, middle),
              color);
        }
      }
    }

    if (HasThumb) {
      Draw_Thumb(view);
    }
    return true;
  }

  virtual bool Set_Maximum(int value) {
    if (value != MaxValue) {
      MaxValue = value;
      Flag_To_Redraw();
      return true;
    }
    return false;
  }

  virtual bool Set_Value(int value) {
    value = Bound(value, 0, MaxValue);
    if (value != CurValue) {
      CurValue = value;
      Flag_To_Redraw();
      return true;
    }
    return false;
  }

  [[nodiscard]] virtual int Get_Value() const { return CurValue; }
  virtual void Use_Thumb(bool value) { HasThumb = value; }

  virtual int Thumb_Pixels() { return Skin::kGaugeThumbPixels; }

  /*
  **	If this gauge has a color to the left of the current setting, then this
  **	flag will be true.
  */
  bool IsColorized : 1 {true};

 protected:
  /*
  **	If a thumb is desired, set to true.
  */
  bool HasThumb : 1 {true};

  /*
  **	Is this a horizontal slider?
  */
  bool IsHorizontal : 1;

  int MaxValue{255};  // maximum value (in application units)
  int CurValue{0};    // index of 1st displayed string in box
                      //  (in application units)

  /*
  ** This value records the difference between where the user clicked
  ** and the edge of the thumb, so that the thumb follows the mouse
  ** with the proper offset.
  */
  int ClickDiff{0};

  virtual void Draw_Thumb(PixelView& view) {
    int x = Value_To_Pixel(CurValue);
    if (x + 4 > Value_To_Pixel(MaxValue)) {
      x = Value_To_Pixel(MaxValue) - 2;
    }
    x = x < X ? X : x;
    if (IsHorizontal) {
      Skin::Draw_Gauge_Box(view, x, Y, 4, Height, true);
    } else {
      Skin::Draw_Gauge_Box(view, X, x, Width, 4, true);
    }
  }

  bool Action(unsigned flags, engine::window::KeyNumber& key, int x,
              int y) override {
    // If there's no thumb on this gauge, it's a display-only device.
    if (!HasThumb) {
      return false;
    }

    // We might end up clearing the event bits. Make sure that the sticky
    // process is properly updated anyway.
    Sticky_Process(flags);

    // If the thumb is currently being "dragged around", then update the
    // slider position according to the mouse position. In all other cases,
    // ignore the button being held down.
    if (flags & kLeftPress || (flags & kLeftHeld && StuckOn == this)) {
      // Compute the difference between where we clicked, and the edge of the
      // thumb (only if we clicked on the thumb.)
      if (flags & kLeftPress) {
        const int curpix = Value_To_Pixel(CurValue);
        const int clickpix = IsHorizontal ? TheMouse().x() : TheMouse().y();

        if (clickpix > curpix && clickpix - curpix < Thumb_Pixels()) {
          ClickDiff = clickpix - curpix;
        } else {
          ClickDiff = 0;
        }

        int testval = Pixel_To_Value(IsHorizontal ? TheMouse().x() - ClickDiff
                                                  : TheMouse().y() - ClickDiff);

        // Correct for round-down errors in Pixel_To_Value() and
        // Value_To_Pixel(); make ClickDiff exactly right so that at this
        // point, the mouse position - ClickDiff converts to CurValue.
        while (testval < CurValue && ClickDiff > 0) {
          ClickDiff--;
          testval = Pixel_To_Value(IsHorizontal ? TheMouse().x() - ClickDiff
                                                : TheMouse().y() - ClickDiff);
        }
      }

      // If no change occurred in the gauge, just call Control's Action
      // routine, but turn off the flags so it won't fill in 'key' with the
      // button ID. Thus, no button ID will be returned by Input.
      if (!Set_Value(Pixel_To_Value(IsHorizontal
                                        ? TheMouse().x() - ClickDiff
                                        : TheMouse().y() - ClickDiff))) {
        ControlClass::Action(0, key, x, y);
        key = engine::window::KN_NONE;
        return true;
      }
    } else {
      // Ignore the left mouse button being held down if this gauge is not
      // currently in "sticky" mode. This allows processing of the
      // kLeftPress by any derived classes such that this gauge can be more
      // closely controlled.
      flags &= ~kLeftHeld;
    }
    return ControlClass::Action(flags, key, x, y);
  }

  virtual int Pixel_To_Value(int pixel) {
    int maximum = 0;
    if (IsHorizontal) {
      pixel -= X + 1;
      maximum = Width;
    } else {
      pixel -= Y + 1;
      maximum = Height;
    }
    maximum -= 2;
    pixel = Bound(pixel, 0, maximum);
    return Skin::Scale(MaxValue, pixel, maximum);
  }

  virtual int Value_To_Pixel(int value) {
    int maximum = 0;
    int start = 0;
    if (IsHorizontal) {
      maximum = Width;
      start = X;
    } else {
      maximum = Height;
      start = Y;
    }
    maximum -= 2;
    return start + Skin::Scale(maximum, value, MaxValue);
  }

  // `original` held between `minval` and `maxval`, minval winning if they
  // cross.
  static int Bound(int original, int minval, int maxval) {
    if (original < minval) {
      return minval;
    }
    if (original > maxval) {
      return maxval;
    }
    return original;
  }
};

// A gauge filled red up to RedLimit, yellow up to YellowLimit and green
// beyond.
template <WidgetSkin Skin>
class TriColorGaugeClass : public GaugeClass<Skin> {
  using Base = GaugeClass<Skin>;

 public:
  TriColorGaugeClass(unsigned id, int x, int y, int w, int h)
      : Base(id, x, y, w, h) {}

  bool Draw_Me(PixelView& view, bool forced) override {
    // The tri-colour body below replaces GaugeClass's single-colour one, so
    // this takes only the repaint-flag and peer handling from ControlClass.
    // NOLINTNEXTLINE(bugprone-parent-virtual-call)
    if (!ControlClass::Draw_Me(view, forced)) {
      return false;
    }
    const int x = this->X;
    const int y = this->Y;
    const int width = this->Width;
    const int height = this->Height;
    Skin::Draw_Gauge_Box(view, x, y, width, height, this->IsDisabled);

    const int red = this->Value_To_Pixel(RedLimit);
    const int yellow = this->Value_To_Pixel(YellowLimit);
    const int middle = this->Value_To_Pixel(this->CurValue);
    const int cur = this->CurValue;

    if (cur <= RedLimit) {
      if (this->IsHorizontal) {
        view.FillRect(
            base::Rect::FromCorners(x + 1, y + 1, middle, y + height - 2),
            kPink);
      } else {
        view.FillRect(
            base::Rect::FromCorners(x + 1, y + 1, x + width - 2, middle),
            kPink);
      }
    } else if (cur > RedLimit && cur <= YellowLimit) {
      if (this->IsHorizontal) {
        view.FillRect(
            base::Rect::FromCorners(x + 1, y + 1, red, y + height - 2), kPink);
        view.FillRect(
            base::Rect::FromCorners(red, y + 1, middle, y + height - 2),
            kYellow);
      } else {
        view.FillRect(base::Rect::FromCorners(x + 1, y + 1, x + width - 2, red),
                      kPink);
        view.FillRect(
            base::Rect::FromCorners(x + 1, red, x + width - 2, middle),
            kYellow);
      }
    } else if (cur > YellowLimit && cur <= this->MaxValue) {
      if (this->IsHorizontal) {
        view.FillRect(
            base::Rect::FromCorners(x + 1, y + 1, red, y + height - 2), kPink);
        view.FillRect(
            base::Rect::FromCorners(red, y + 1, yellow, y + height - 2),
            kYellow);
        view.FillRect(
            base::Rect::FromCorners(yellow, y + 1, middle, y + height - 2),
            kGreen);
      } else {
        view.FillRect(base::Rect::FromCorners(x + 1, y + 1, x + width - 2, red),
                      kPink);
        view.FillRect(
            base::Rect::FromCorners(x + 1, red, x + width - 2, yellow),
            kYellow);
        view.FillRect(
            base::Rect::FromCorners(x + 1, yellow, x + width - 2, middle),
            kGreen);
      }
    }

    if (this->HasThumb) {
      this->Draw_Thumb(view);
    }
    return true;
  }

  virtual bool Set_Red_Limit(int value) {
    if (value >= 0 && value < this->MaxValue) {
      RedLimit = value;
      this->Flag_To_Redraw();
      return true;
    }
    return false;
  }

  virtual bool Set_Yellow_Limit(int value) {
    if (value >= 0 && value < this->MaxValue) {
      YellowLimit = value;
      this->Flag_To_Redraw();
      return true;
    }
    return false;
  }

 protected:
  int RedLimit{0};     // maximum value for red
  int YellowLimit{0};  // maximum value for yellow
};
// NOLINTEND(portability-template-virtual-member-function)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_GAUGE_H_
