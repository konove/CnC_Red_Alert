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

// base::DivideWithRound() over the unsigned values the conversions below
// work in. Originally from WIN32LIB/MISC/LIB.CPP.
static unsigned Divide_With_Round(unsigned num, unsigned den) {
  return static_cast<unsigned>(
      base::DivideWithRound(static_cast<int>(num), static_cast<int>(den)));
}

// The conversions work at the 8-bit HSV scale for better precision than the
// 6-bit guns would give.
#define HSV_BASE 255
// The brightest VGA gun; 63, not 64, since this is a maximum, not a range.
#define RGB_BASE 63

void Convert_RGB_To_HSV(unsigned int r, unsigned int g, unsigned int b,
                        unsigned int* h, unsigned int* s, unsigned int* v) {
  // Scale the 6-bit guns up to the 8-bit HSV scale.
  r = Divide_With_Round(r * HSV_BASE, RGB_BASE);
  g = Divide_With_Round(g * HSV_BASE, RGB_BASE);
  b = Divide_With_Round(b * HSV_BASE, RGB_BASE);

  // A grey has no hue; it stays 0 unless there is saturation below.
  *h = 0;

  // The value is the brightest gun, which is the dominant primary.
  *v = r > g ? r : g;
  *v = std::max(b, *v);

  // The dimmest gun is the amount of white mixed in.
  unsigned int m = r < g ? r : g;
  m = std::min(b, m);

  // The saturation is the share of the value that is not white.
  if (*v != 0) {
    *s = Divide_With_Round((*v - m) * HSV_BASE, *v);
  } else {
    *s = 0;
  }

  if (*s != 0) {
    // Each gun's distance below the brightest, as a fraction of the spread
    // between brightest and dimmest scaled to 0..HSV_BASE. The dimmest gun's
    // is HSV_BASE, the brightest's 0. A non-zero saturation means the spread
    // is not zero.
    const unsigned int tmp = *v - m;
    CHECK_NE(tmp, 0U);
    const unsigned int r1 = Divide_With_Round((*v - r) * HSV_BASE, tmp);
    const unsigned int g1 = Divide_With_Round((*v - g) * HSV_BASE, tmp);
    const unsigned int b1 = Divide_With_Round((*v - b) * HSV_BASE, tmp);

    // Find the effect of the second most predominant color: which gun is
    // brightest and which dimmest picks the sextant of the color hexagon, and
    // the middle gun's fraction the position within it. The hue is built at
    // six times its final scale, one HSV_BASE per sextant.
    if (*v == r) {
      if (m == g) {
        *h = (5 * HSV_BASE) + b1;
      } else {
        *h = (1 * HSV_BASE) - g1;
      }
    } else {
      if (*v == g) {
        if (m == b) {
          *h = (1 * HSV_BASE) + r1;
        } else {
          *h = (3 * HSV_BASE) - b1;
        }
      } else {
        // *v == b
        if (m == r) {
          *h = (3 * HSV_BASE) + g1;
        } else {
          *h = (5 * HSV_BASE) - r1;
        }
      }
    }

    // Back to one HSV_BASE for the whole circle. A red whose green and blue
    // tie takes the magenta branch and lands on 6 * HSV_BASE, which becomes
    // HSV_BASE, the same hue as 0.
    *h = Divide_With_Round(*h, 6);
  }
}

void Convert_HSV_To_RGB(unsigned int h, unsigned int s, unsigned int v,
                        unsigned int* r, unsigned int* g, unsigned int* b) {
  const base::Rgb8 color = base::HsvToRgb8(
      static_cast<int>(h), static_cast<int>(s), static_cast<int>(v));

  // HsvToRgb8() works at HSV_BASE; scale each gun down to the VGA RGB_BASE,
  // rounding.
  *r = static_cast<unsigned int>(
      base::DivideWithRound(color.red * RGB_BASE, HSV_BASE));
  *g = static_cast<unsigned int>(
      base::DivideWithRound(color.green * RGB_BASE, HSV_BASE));
  *b = static_cast<unsigned int>(
      base::DivideWithRound(color.blue * RGB_BASE, HSV_BASE));
}
