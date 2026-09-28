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

// HsvColor's conversion to RGB and Tiberian Dawn's 6-bit HSV/RGB conversions.

#include "engine/gfx/hsv.h"

#include <algorithm>
#include <cstdint>

#include "base/hsv.h"
#include "engine/gfx/rgb.h"

HsvColor::operator PaletteColor() const {
  const base::Rgb8 color = base::HsvToRgb8(hue(), saturation(), value());

  // The PaletteColor constructor drops each gun's low two bits to get the 6-bit
  // VGA value it stores.
  //
  // TODO: Truncating here, and in ToHsv(), makes the round trip lossy: at the
  // options dialog's neutral settings 127,769 of the 262,144 6-bit colors come
  // back a step or two off (the original code: 76,340; TD's rounding pair:
  // 21,612, by one step), so every palette entry the dialog adjusts drifts.
  return {static_cast<uint8_t>(color.red), static_cast<uint8_t>(color.green),
          static_cast<uint8_t>(color.blue)};
}

// The conversions work at the 8-bit HSV scale for better precision than the
// 6-bit guns would give.
constexpr int kMaxComponent = 255;
// The brightest VGA gun; 63, not 64, since this is a maximum, not a range.
constexpr int kMaxGun = 63;

HsvColor Rgb6ToHsv(int red, int green, int blue) {
  // Scale the 6-bit guns up to the 8-bit HSV scale.
  const int r = base::DivideWithRound(red * kMaxComponent, kMaxGun);
  const int g = base::DivideWithRound(green * kMaxComponent, kMaxGun);
  const int b = base::DivideWithRound(blue * kMaxComponent, kMaxGun);

  // The value is the brightest gun, which is the dominant primary; the dimmest
  // is the amount of white mixed in.
  const auto [white, value] = std::minmax({r, g, b});

  // A grey (or black) has no saturation, and so no hue.
  if (value == white) {
    return {0, 0, static_cast<uint8_t>(value)};
  }

  // The saturation is the share of the value that is not white.
  const int saturation =
      base::DivideWithRound((value - white) * kMaxComponent, value);

  // Each gun's distance below the brightest, as a fraction of the spread
  // between brightest and dimmest scaled to 0..kMaxComponent. The dimmest
  // gun's is kMaxComponent, the brightest's 0.
  const int spread = value - white;
  const int red_shortfall =
      base::DivideWithRound((value - r) * kMaxComponent, spread);
  const int green_shortfall =
      base::DivideWithRound((value - g) * kMaxComponent, spread);
  const int blue_shortfall =
      base::DivideWithRound((value - b) * kMaxComponent, spread);

  // Find the effect of the second most predominant color: which gun is
  // brightest and which dimmest picks the sextant of the color hexagon, and
  // the middle gun's shortfall the position within it. The hue is built at
  // six times its final scale, one kMaxComponent per sextant.
  int hue = 0;
  if (value == r) {
    if (white == g) {
      hue = (5 * kMaxComponent) + blue_shortfall;
    } else {
      hue = (1 * kMaxComponent) - green_shortfall;
    }
  } else if (value == g) {
    if (white == b) {
      hue = (1 * kMaxComponent) + red_shortfall;
    } else {
      hue = (3 * kMaxComponent) - blue_shortfall;
    }
  } else {
    // value == b
    if (white == r) {
      hue = (3 * kMaxComponent) + green_shortfall;
    } else {
      hue = (5 * kMaxComponent) - red_shortfall;
    }
  }

  // Back to one kMaxComponent for the whole circle. A red whose green and
  // blue tie takes the magenta branch and lands on 6 * kMaxComponent, which
  // becomes kMaxComponent, the same hue as 0.
  hue = base::DivideWithRound(hue, 6);

  return {static_cast<uint8_t>(hue), static_cast<uint8_t>(saturation),
          static_cast<uint8_t>(value)};
}

Rgb6 HsvToRgb6(const HsvColor& hsv) {
  const base::Rgb8 color =
      base::HsvToRgb8(hsv.hue(), hsv.saturation(), hsv.value());

  // HsvToRgb8() works at kMaxComponent; scale each gun down to the VGA
  // kMaxGun, rounding.
  return {.red = base::DivideWithRound(color.red * kMaxGun, kMaxComponent),
          .green = base::DivideWithRound(color.green * kMaxGun, kMaxComponent),
          .blue = base::DivideWithRound(color.blue * kMaxGun, kMaxComponent)};
}
