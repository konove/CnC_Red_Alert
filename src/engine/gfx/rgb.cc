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

// RGBClass's fades, color distance and conversion to HSV.

#include "engine/gfx/rgb.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "engine/gfx/hsv.h"

void RGBClass::Adjust(const int ratio, const RGBClass& target) {
  // The original masked the ratio with 0xFF, so a fade overshooting to 256
  // wrapped back to no change at all. Clamping keeps it at the target.
  const int clamped_ratio = std::clamp(ratio, 0, 255);

  // The original stepped by ratio / 256 and so never quite reached the
  // target; dividing by 255 makes 255 land on it exactly.
  const float t = static_cast<float>(clamped_ratio) / 255.0F;

  // std::lerp() stays between the two guns for t in [0, 1], so the result
  // fits the 6-bit range; the cast rounds each gun down.
  red_ = static_cast<uint8_t>(
      std::lerp(static_cast<float>(red_), static_cast<float>(target.red_), t));
  green_ = static_cast<uint8_t>(std::lerp(
      static_cast<float>(green_), static_cast<float>(target.green_), t));
  blue_ = static_cast<uint8_t>(std::lerp(static_cast<float>(blue_),
                                         static_cast<float>(target.blue_), t));
}

RGBClass RGBClass::Adjusted(const int ratio, const RGBClass& target) const {
  RGBClass result = *this;
  result.Adjust(ratio, target);
  return result;
}

int RGBClass::Difference(const RGBClass& other) const {
  const auto diff_sq = [](const int a, const int b) {
    const int d = a - b;
    return d * d;
  };

  return diff_sq(red_, other.red_) + diff_sq(green_, other.green_) +
         diff_sq(blue_, other.blue_);
}

HSVClass RGBClass::ToHSV() const {
  const int r = Red_Component();
  const int g = Green_Component();
  const int b = Blue_Component();

  const auto [min_val, max_val] = std::minmax({r, g, b});
  // The value, or brightness, is the brightest gun.
  const int value = max_val;
  const int delta = max_val - min_val;

  int hue = 0;
  int saturation = 0;

  if (delta > 0 && value > 0) {
    // The saturation is the share of the value that is not white, the white
    // being the dimmest gun.
    saturation = delta * 255 / value;

    // The hue in sextants (0..6) is the dominant gun's sextant plus the
    // difference of the other two over the spread. Keep it multiplied by the
    // spread, as a numerator, so the one division comes last.
    int segment_numerator = 0;

    if (max_val == r) {
      // Sextant 0: red is dominant. Towards magenta, where blue exceeds
      // green, this goes negative and wraps round below.
      segment_numerator = g - b;
    } else if (max_val == g) {
      // Sextant 2: green is dominant.
      segment_numerator = (2 * delta) + (b - r);
    } else {
      // Sextant 4: blue is dominant.
      segment_numerator = (4 * delta) + (r - g);
    }

    // Scale the numerator, in [-delta, 5 * delta], so that the six sextants
    // of the circle span 0..255.
    hue = segment_numerator * 255 / (delta * 6);

    // Wrap the red-magenta side, -42..-1, round to the top of the circle.
    if (hue < 0) {
      hue += 255;
    }
  }

  // The components are 8-bit, so all three are in 0..255.
  return {static_cast<uint8_t>(hue), static_cast<uint8_t>(saturation),
          static_cast<uint8_t>(value)};
}
