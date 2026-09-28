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

#ifndef CNC_RED_ALERT_ENGINE_GFX_HSV_H_
#define CNC_RED_ALERT_ENGINE_GFX_HSV_H_

// Colors as hue, saturation and value, the form the games' options dialogs
// use to apply the brightness, color, tint and contrast sliders to a palette.
// Originally Westwood's HSV.H (Joe L. Bostic, December 1995).

#include <cstdint>

class PaletteColor;

// A color as hue, saturation and value, each in 0..255.
//
// The hue runs once around the six-sided color wheel over 0..255, so 255 is
// the same red as 0 (see base::HsvToRgb8()). The value is the brightest gun
// and the saturation how far the dimmest gun falls below it, relative to
// the value.
//
// Red Alert's options dialog converts each palette entry with
// PaletteColor::ToHSV(), scales the components and assigns the result back.
//
// Example:
//   const HsvColor hsv = palette.at(index).ToHSV();
//   palette.at(index) = HsvColor(hsv.Hue_Component(), 0,
//   hsv.Value_Component());
class HsvColor {
 public:
  HsvColor(uint8_t hue, uint8_t saturation, uint8_t value) noexcept
      : Hue(hue), Saturation(saturation), Value(value) {}

  // Returns the RGB color that most closely matches this one. The conversion
  // truncates each gun to the 6 bits PaletteColor keeps, so ToHSV() followed by
  // this conversion can move a gun down by a step or two.
  //
  // Implicit so the options dialog can assign an HSV color straight into a
  // palette entry.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator PaletteColor() const;

  [[nodiscard]] int Hue_Component() const { return Hue; }
  [[nodiscard]] int Saturation_Component() const { return Saturation; }
  [[nodiscard]] int Value_Component() const { return Value; }

 private:
  uint8_t Hue;
  uint8_t Saturation;
  uint8_t Value;
};

// HSV/RGB conversions for colors stored as VGA 6-bit guns (0..63), with the
// HSV components in 0..255 as in HsvColor. Unlike HsvColor, both directions
// round rather than truncate, so a round trip moves a gun by at most one step.
// Only Tiberian Dawn's options dialog uses these; it keeps its palette as raw
// bytes rather than PaletteColor.

// Converts 'r', 'g' and 'b' (each 0..63) and stores the hue, saturation and
// value in '*h', '*s' and '*v' (each 0..255). A grey has hue and saturation 0.
void Convert_RGB_To_HSV(int r, int g, int b, int* h, int* s, int* v);

// Converts 'h', 's' and 'v' (each 0..255, clamped if not) and stores the guns
// in '*r', '*g' and '*b' (each 0..63).
void Convert_HSV_To_RGB(int h, int s, int v, int* r, int* g, int* b);

#endif  // CNC_RED_ALERT_ENGINE_GFX_HSV_H_
