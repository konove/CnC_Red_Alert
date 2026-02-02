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

#include "sdllib/gbuffer.h"

#include <cmath>
#include <cstdint>
#include <numbers>

#include "sdllib/bitmap.h"

// Uses inverse mapping: for each destination pixel, applies the inverse
// rotation and scale to find the corresponding source pixel.
void GraphicBufferClass::Scale_Rotate(const BitmapClass& bmp,
                                      const TPoint2D& pt, const int32_t scale,
                                      const uint8_t angle) {
  if (scale == 0) {
    return;
  }

  const double radians = angle * 2.0 * std::numbers::pi / 256.0;
  const double cos_a = std::cos(radians);
  const double sin_a = std::sin(radians);

  const double inv_S = 256.0 / scale;
  const double cx_bmp = bmp.Width / 2.0;
  const double cy_bmp = bmp.Height / 2.0;

  auto* dst_buf = static_cast<uint8_t*>(Get_Buffer());

  for (int dy = 0; dy < Height; dy++) {
    for (int dx = 0; dx < Width; dx++) {
      const double rx = dx - pt.x;
      const double ry = dy - pt.y;

      // Inverse transform: undo rotation, then undo scale.
      const int bx =
          static_cast<int>((sin_a * rx + cos_a * ry) * inv_S + cx_bmp);
      const int by =
          static_cast<int>((-cos_a * rx + sin_a * ry) * inv_S + cy_bmp);

      if (bx >= 0 && bx < bmp.Width && by >= 0 && by < bmp.Height) {
        const uint8_t pixel = bmp.Data[by * bmp.Width + bx];
        if (pixel != 0) {
          dst_buf[dy * Width + dx] = pixel;
        }
      }
    }
  }
}
