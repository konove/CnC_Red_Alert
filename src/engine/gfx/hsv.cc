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

// HSVClass's conversion to RGB and Tiberian Dawn's 6-bit HSV/RGB conversions.

#include "engine/gfx/hsv.h"

#include <algorithm>
#include <cstdint>

#include "absl/log/check.h"
#include "base/hsv.h"
#include "engine/gfx/rgb.h"

HSVClass::operator RGBClass() const {
  const base::Rgb8 color = base::HsvToRgb8(
      Hue_Component(), Saturation_Component(), Value_Component());

  // The RGBClass constructor drops each gun's low two bits to get the 6-bit
  // VGA value it stores.
  //
  // TODO: Truncating here, and in ToHSV(), makes the round trip lossy: at the
  // options dialog's neutral settings 127,769 of the 262,144 6-bit colors come
  // back a step or two off (the original code: 76,340; TD's rounding pair:
  // 21,612, by one step), so every palette entry the dialog adjusts drifts.
  return {static_cast<uint8_t>(color.red), static_cast<uint8_t>(color.green),
          static_cast<uint8_t>(color.blue)};
}

// The conversions work at the 8-bit HSV scale for better precision than the
// 6-bit guns would give.
constexpr int kHsvBase = 255;
// The brightest VGA gun; 63, not 64, since this is a maximum, not a range.
constexpr int kRgbBase = 63;

void Convert_RGB_To_HSV(int r, int g, int b, int* h, int* s, int* v) {
  // Scale the 6-bit guns up to the 8-bit HSV scale.
  r = base::DivideWithRound(r * kHsvBase, kRgbBase);
  g = base::DivideWithRound(g * kHsvBase, kRgbBase);
  b = base::DivideWithRound(b * kHsvBase, kRgbBase);

  // A grey has no hue; it stays 0 unless there is saturation below.
  *h = 0;

  // The value is the brightest gun, which is the dominant primary.
  *v = r > g ? r : g;
  *v = std::max(b, *v);

  // The dimmest gun is the amount of white mixed in.
  int m = r < g ? r : g;
  m = std::min(b, m);

  // The saturation is the share of the value that is not white.
  if (*v != 0) {
    *s = base::DivideWithRound((*v - m) * kHsvBase, *v);
  } else {
    *s = 0;
  }

  if (*s != 0) {
    // Each gun's distance below the brightest, as a fraction of the spread
    // between brightest and dimmest scaled to 0..kHsvBase. The dimmest gun's
    // is kHsvBase, the brightest's 0. A non-zero saturation means the spread
    // is not zero.
    const int tmp = *v - m;
    CHECK_NE(tmp, 0);
    const int r1 = base::DivideWithRound((*v - r) * kHsvBase, tmp);
    const int g1 = base::DivideWithRound((*v - g) * kHsvBase, tmp);
    const int b1 = base::DivideWithRound((*v - b) * kHsvBase, tmp);

    // Find the effect of the second most predominant color: which gun is
    // brightest and which dimmest picks the sextant of the color hexagon, and
    // the middle gun's fraction the position within it. The hue is built at
    // six times its final scale, one kHsvBase per sextant.
    if (*v == r) {
      if (m == g) {
        *h = (5 * kHsvBase) + b1;
      } else {
        *h = (1 * kHsvBase) - g1;
      }
    } else {
      if (*v == g) {
        if (m == b) {
          *h = (1 * kHsvBase) + r1;
        } else {
          *h = (3 * kHsvBase) - b1;
        }
      } else {
        // *v == b
        if (m == r) {
          *h = (3 * kHsvBase) + g1;
        } else {
          *h = (5 * kHsvBase) - r1;
        }
      }
    }

    // Back to one kHsvBase for the whole circle. A red whose green and blue
    // tie takes the magenta branch and lands on 6 * kHsvBase, which becomes
    // kHsvBase, the same hue as 0.
    *h = base::DivideWithRound(*h, 6);
  }
}

void Convert_HSV_To_RGB(int h, int s, int v, int* r, int* g, int* b) {
  const base::Rgb8 color = base::HsvToRgb8(h, s, v);

  // HsvToRgb8() works at kHsvBase; scale each gun down to the VGA kRgbBase,
  // rounding.
  *r = base::DivideWithRound(color.red * kRgbBase, kHsvBase);
  *g = base::DivideWithRound(color.green * kRgbBase, kHsvBase);
  *b = base::DivideWithRound(color.blue * kRgbBase, kHsvBase);
}
