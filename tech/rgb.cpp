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

#include "tech/rgb.h"

#include <algorithm>
#include <cmath>

#include "hsv.h"

void RGBClass::Adjust(const int ratio, RGBClass const& target) {
  // FIX: The original code used (ratio & 0xFF), which caused the fade
  // to wrap around to 0 if the value exceeded 255 (e.g., 256 became 0).
  //
  // We use std::clamp to ensure that any value >= 255 stays at 100% fade,
  // and any value < 0 stays at 0%. This prevents visual glitching.
  const int clamped_ratio = std::clamp(ratio, 0, 255);

  // Convert [0, 255] integer range to [0.0, 1.0] floating point range
  // for the linear interpolation function.
  const float t = static_cast<float>(clamped_ratio) / 255.0f;

  // std::lerp (Linear Interpolation) calculates: a + t * (b - a)
  //
  // Note: We cast the result back to uint8_t. The values are safe because
  // our inputs are bounded [0, 63] and t is [0.0, 1.0].
  red_ = static_cast<uint8_t>(
      std::lerp(static_cast<float>(red_), static_cast<float>(target.red_), t));
  green_ = static_cast<uint8_t>(std::lerp(
      static_cast<float>(green_), static_cast<float>(target.green_), t));
  blue_ = static_cast<uint8_t>(std::lerp(static_cast<float>(blue_),
                                         static_cast<float>(target.blue_), t));
}

int RGBClass::Difference(RGBClass const& other) const {
  auto diff_sq = [](const int a, const int b) {
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
  const int value = max_val;  // Value (Brightness)
  const int delta = max_val - min_val;

  int hue = 0;
  int saturation = 0;

  if (delta > 0 && value > 0) {
    // Calculate Saturation (0-255)
    saturation = (delta * 255) / value;

    // Calculate Hue Segment (0-6 scale) using integer math.
    // We calculate (Segment * Delta) + Offset to avoid early division/precision
    // loss. Formula: Hue = Segment + (Main - Other) / Delta
    int segment_numerator = 0;

    if (max_val == r) {
      // Segment 0: Red is dominant.
      // We interpret this as (Green - Blue).
      // If G < B, this wraps to negative (Magenta), handled by +6 later.
      segment_numerator = g - b;
    } else if (max_val == g) {
      // Segment 2: Green is dominant. Offset is 2.0 * delta
      segment_numerator = (2 * delta) + (b - r);
    } else {
      // Segment 4: Blue is dominant. Offset is 4.0 * delta
      segment_numerator = (4 * delta) + (r - g);
    }

    // Convert to 0-255 range.
    // Current 'segment_numerator' is in range [-Delta, 5*Delta] roughly.
    // We want to map 6.0 units (full circle) -> 255 units.
    // Scale factor: 255 / 6 = 42.5.
    //
    // Logic: (Numerator * 255) / (Delta * 6)
    hue = (segment_numerator * 255) / (delta * 6);

    // Handle negative wrap-around (Red-Magenta zone)
    if (hue < 0) {
      hue += 255;
    }
  }

  // We know these values are in [0, 255], so the cast is safe.
  return HSVClass(static_cast<uint8_t>(hue), static_cast<uint8_t>(saturation),
                  static_cast<uint8_t>(value));
}
