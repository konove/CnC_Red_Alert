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

#ifndef CNC_RED_ALERT_ENGINE_GFX_HSV_COLOR_H_
#define CNC_RED_ALERT_ENGINE_GFX_HSV_COLOR_H_

// Colors as hue, saturation and value, the form the games' options dialogs
// use to apply the brightness, color, tint and contrast sliders to a palette.
// Originally Westwood's HSV.H (Joe L. Bostic, December 1995).

#include <cstdint>

namespace engine::gfx {

class PaletteColor;

// A color as hue, saturation and value, each in 0..255.
//
// The hue runs once around the six-sided color wheel over 0..255, so 255 is
// the same red as 0 (see base::HsvToRgb8()). The value is the brightest
// component and the saturation how far the dimmest falls below it, relative
// to the value.
//
// Both games' options dialogs convert each palette entry with
// PaletteColor::ToHsv(), scale the components and convert the result back
// with ToPaletteColor().
//
// Example:
//   const engine::gfx::HsvColor hsv = palette.at(index).ToHsv();
//   palette.at(index) =
//       engine::gfx::HsvColor(hsv.hue(), 0, hsv.value()).ToPaletteColor();
class HsvColor {
 public:
  HsvColor(const uint8_t hue, const uint8_t saturation,
           const uint8_t value) noexcept
      : hue_(hue), saturation_(saturation), value_(value) {}

  // Returns the palette color that most closely matches this one.
  // PaletteColor::ToHsv() followed by this returns the original color or one
  // up to 4 of 255 off in some components, the precision of this class's own
  // 8-bit hue, saturation and value.
  [[nodiscard]] PaletteColor ToPaletteColor() const;

  [[nodiscard]] int hue() const { return hue_; }
  [[nodiscard]] int saturation() const { return saturation_; }
  [[nodiscard]] int value() const { return value_; }

 private:
  uint8_t hue_;
  uint8_t saturation_;
  uint8_t value_;
};

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_HSV_COLOR_H_
