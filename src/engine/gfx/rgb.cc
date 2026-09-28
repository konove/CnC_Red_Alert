// Command & Conquer Red Alert(tm)
// Copyright 2025 Electronic Arts Inc.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// PaletteColor's fades, color distance and conversion to HSV.

#include "engine/gfx/rgb.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "engine/gfx/hsv.h"

namespace engine::gfx {

PaletteColor PaletteColor::FadedToward(const int ratio,
                                       const PaletteColor& target) const {
  // The original masked the ratio with 0xFF, so a fade overshooting to 256
  // wrapped back to no change at all. Clamping keeps it at the target.
  const int clamped_ratio = std::clamp(ratio, 0, 255);

  // The original stepped by ratio / 256 and so never quite reached the
  // target; dividing by 255 makes 255 land on it exactly.
  const float t = static_cast<float>(clamped_ratio) / 255.0F;

  // std::lerp() stays between the two guns for t in [0, 1], so the result
  // fits the 6-bit range; the cast rounds each gun down.
  const auto fade = [t](const uint8_t from, const uint8_t to) {
    return static_cast<uint8_t>(
        std::lerp(static_cast<float>(from), static_cast<float>(to), t));
  };

  PaletteColor result;
  result.red_ = fade(red_, target.red_);
  result.green_ = fade(green_, target.green_);
  result.blue_ = fade(blue_, target.blue_);
  return result;
}

int PaletteColor::SquaredDistance(const PaletteColor& other) const {
  const auto squared_difference = [](const int a, const int b) {
    const int difference = a - b;
    return difference * difference;
  };

  return squared_difference(red_, other.red_) +
         squared_difference(green_, other.green_) +
         squared_difference(blue_, other.blue_);
}

HsvColor PaletteColor::ToHsv() const {
  const int r = red();
  const int g = green();
  const int b = blue();

  // The value, or brightness, is the brightest gun; the dimmest is the white
  // mixed in.
  const auto [white, value] = std::minmax({r, g, b});

  // A grey (or black) has no saturation, and so no hue.
  if (value == white) {
    return {0, 0, static_cast<uint8_t>(value)};
  }

  // The saturation is the share of the value that is not white.
  const int spread = value - white;
  const int saturation = spread * 255 / value;

  // The hue in sextants (0..6) is the dominant gun's sextant plus the
  // difference of the other two over the spread. Keep it multiplied by the
  // spread, as a numerator, so the one division comes last.
  int hue_numerator = 0;
  if (value == r) {
    // Sextant 0: red is dominant. Towards magenta, where blue exceeds green,
    // this goes negative and wraps round below.
    hue_numerator = g - b;
  } else if (value == g) {
    // Sextant 2: green is dominant.
    hue_numerator = (2 * spread) + (b - r);
  } else {
    // Sextant 4: blue is dominant.
    hue_numerator = (4 * spread) + (r - g);
  }

  // Scale the numerator, in [-spread, 5 * spread], so that the six sextants of
  // the circle span 0..255, then wrap the red-magenta side, -42..-1, round to
  // the top of the circle.
  int hue = hue_numerator * 255 / (spread * 6);
  if (hue < 0) {
    hue += 255;
  }

  // The components are 8-bit, so all three are in 0..255.
  return {static_cast<uint8_t>(hue), static_cast<uint8_t>(saturation),
          static_cast<uint8_t>(value)};
}

}  // namespace engine::gfx
