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

#include "jshell/rotbmp.h"

#include <cmath>
#include <numbers>

#include "sdllib/gbuffer.h"

// Inverse-mapping rotation: iterates every destination pixel, computes
// the corresponding source pixel via reverse rotation, and copies it
// if in-bounds and non-transparent.
void Rotate_Bitmap(GraphicViewPortClass* srcvp, GraphicViewPortClass* destvp,
                   int angle) {
  int sw = srcvp->Get_Width();
  int sh = srcvp->Get_Height();
  int dw = destvp->Get_Width();
  int dh = destvp->Get_Height();

  int src_stride = sw + srcvp->Get_XAdd() + srcvp->Get_Pitch();
  int dst_stride = dw + destvp->Get_XAdd() + destvp->Get_Pitch();

  uint8_t* src_buf = srcvp->Get_Offset();
  uint8_t* dst_buf = destvp->Get_Offset();

  // Convert 0-255 angle to radians (full circle = 256 steps, clockwise).
  double radians = angle * 2.0 * std::numbers::pi / 256.0;
  double cos_a = std::cos(radians);
  double sin_a = std::sin(radians);

  // Rotation centers.
  double cx_src = sw / 2.0;
  double cy_src = sh / 2.0;
  double cx_dst = dw / 2.0;
  double cy_dst = dh / 2.0;

  for (int dy = 0; dy < dh; dy++) {
    for (int dx = 0; dx < dw; dx++) {
      // Center-relative destination coordinates.
      double rx = dx - cx_dst;
      double ry = dy - cy_dst;

      // Inverse rotation to find the source pixel.
      int sx = static_cast<int>(cos_a * rx + sin_a * ry + cx_src);
      int sy = static_cast<int>(-sin_a * rx + cos_a * ry + cy_src);

      if (sx >= 0 && sx < sw && sy >= 0 && sy < sh) {
        uint8_t pixel = src_buf[sy * src_stride + sx];
        if (pixel != 0) {
          dst_buf[dy * dst_stride + dx] = pixel;
        }
      }
    }
  }
}
