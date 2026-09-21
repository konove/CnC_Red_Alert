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

// File: The one PixelBuffer member that needs floating point. The rest of the
// class lives in drawbuff.cc, and the SDL half in drawbuff_sdl.cc.

#include "sdllib/pixel_buffer.h"

#include <cmath>
#include <cstdint>
#include <numbers>
#include <span>

#include "base/array.h"
#include "base/numeric.h"
#include "sdllib/bitmap.h"

// Walks the destination rather than the source: every destination pixel is
// mapped back through the inverse transform to the bitmap pixel it came
// from. Walking the source instead would scatter its pixels and leave holes
// wherever the scale stretches the image.
void PixelBuffer::DrawScaledRotated(const BitmapClass& bitmap,
                                    const TPoint2D& center, const int32_t scale,
                                    const uint8_t angle) {
  if (scale == 0) {
    return;
  }

  // The game measures angles in 256ths of a circle, like DirType.
  const double radians = angle * 2.0 * std::numbers::pi / 256.0;
  const double cos_a = std::cos(radians);
  const double sin_a = std::sin(radians);

  // scale is 24.8 fixed point, so 256 / scale is its reciprocal, which is
  // what the destination-to-source direction needs.
  const double inv_S = 256.0 / scale;
  const double cx_bmp = bitmap.Width / 2.0;
  const double cy_bmp = bitmap.Height / 2.0;

  // Rows in this buffer are width_ apart: DrawScaledRotated is a member of the
  // buffer rather than of a view, and Init() leaves x_add_ and pitch_ zero
  // for every buffer the games allocate.
  const auto dst_buf = Get_Bytes();

  for (int y2 = 0; y2 < height_; y2++) {
    for (int x2 = 0; x2 < width_; x2++) {
      const double rx = x2 - center.x;
      const double ry = y2 - center.y;

      // The matrix is [[sin, cos], [-cos, sin]], a rotation by angle minus a
      // quarter turn; the caller adds that quarter turn back when it converts
      // its facing. Its determinant is 1, so undoing the scale is the single
      // multiply by inv_S.
      const int bx =
          static_cast<int>((((sin_a * rx) + (cos_a * ry)) * inv_S) + cx_bmp);
      const int by =
          static_cast<int>((((-cos_a * rx) + (sin_a * ry)) * inv_S) + cy_bmp);

      // Destination pixels that map outside the bitmap keep what was there,
      // as does pixel 0, which is the transparent index.
      if (bx >= 0 && bx < bitmap.Width && by >= 0 && by < bitmap.Height) {
        const uint8_t pixel =
            base::At(bitmap.Data, base::ToSize((by * bitmap.Width) + bx));
        if (pixel != 0) {
          base::At(dst_buf, base::ToSize((y2 * width_) + x2)) = pixel;
        }
      }
    }
  }
}
