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
// PaletteColor::ToHsv(), scales the components and assigns the result back.
//
// Example:
//   const HsvColor hsv = palette.at(index).ToHsv();
//   palette.at(index) = HsvColor(hsv.hue(), 0, hsv.value());
class HsvColor {
 public:
  HsvColor(uint8_t hue, uint8_t saturation, uint8_t value) noexcept
      : hue_(hue), saturation_(saturation), value_(value) {}

  // Returns the RGB color that most closely matches this one. The conversion
  // truncates each gun to the 6 bits PaletteColor keeps, so ToHsv() followed by
  // this conversion can move a gun down by a step or two.
  //
  // Implicit so the options dialog can assign an HSV color straight into a
  // palette entry.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator PaletteColor() const;

  [[nodiscard]] int hue() const { return hue_; }
  [[nodiscard]] int saturation() const { return saturation_; }
  [[nodiscard]] int value() const { return value_; }

 private:
  uint8_t hue_;
  uint8_t saturation_;
  uint8_t value_;
};

// HSV/RGB conversions for colors stored as VGA 6-bit guns (0..63). Unlike
// HsvColor's conversion to PaletteColor, both directions round rather than
// truncate, so a round trip moves a gun by at most one step. Only Tiberian
// Dawn's options dialog uses these; it keeps its palette as raw bytes rather
// than PaletteColor.

// An RGB color as three VGA 6-bit guns, each 0..63.
struct Rgb6 {
  int red;
  int green;
  int blue;
};

// Returns the HSV form of the color with guns 'red', 'green' and 'blue' (each
// 0..63). A grey has hue and saturation 0.
[[nodiscard]] HsvColor Rgb6ToHsv(int red, int green, int blue);

// Returns the 6-bit guns of 'hsv'.
[[nodiscard]] Rgb6 HsvToRgb6(const HsvColor& hsv);

#endif  // CNC_RED_ALERT_ENGINE_GFX_HSV_H_
