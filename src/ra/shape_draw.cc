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

// File: Shape drawing, bounding-box measurement and radar icon rendering.

#include "ra/shape_draw.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <span>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/rect.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/shape_blitter.h"
#include "engine/gfx/text_window.h"
#include "engine/gfx/wwstd.h"
#include "ra/defines.h"
#include "ra/display.h"
#include "ra/face.h"
#include "ra/game_clock.h"
#include "ra/interpal.h"
#include "ra/keyframe.h"
#include "ra/screen.h"

namespace {

// Draws the width x height shape `pixels` into `out`, scaled by `scale` (24.8
// fixed point, 0x100 = 1.0; zero draws nothing) and turned to `rotation`, and
// returns the part of `out` that holds it: a (2 * width) x (2 * height) image
// with the shape's centre at its centre, so a quarter turn of a non-square
// shape still fits. Pixel 0 is transparent, and whatever falls outside the
// doubled frame is cropped.
std::span<uint8_t> ScaleRotateShape(const std::span<const uint8_t> pixels,
                                    const int width, const int height,
                                    const std::span<uint8_t> out,
                                    const DirType rotation,
                                    const int32_t scale) {
  const int out_width = width * 2;
  const int out_height = height * 2;
  const auto out_size = base::ToSize(out_width) * base::ToSize(out_height);
  CHECK_LE(out_size, out.size());
  const auto dst = out.first(out_size);
  std::ranges::fill(dst, uint8_t{0});
  if (scale == 0) {
    return dst;
  }

  // Walks the destination rather than the source: every destination pixel is
  // mapped back through the inverse transform to the shape pixel it came
  // from. Walking the source instead would scatter its pixels and leave holes
  // wherever the scale stretches the image.
  //
  // The matrix below is [[sin, cos], [-cos, sin]], a rotation by `angle`
  // minus a quarter turn, so converting the facing negates it and adds that
  // quarter turn back. Angles are in 256ths of a circle, like DirType.
  const auto angle =
      static_cast<uint8_t>(256 - static_cast<int>(rotation) + 64);
  const double radians = angle * 2.0 * std::numbers::pi / 256.0;
  const double cos_a = std::cos(radians);
  const double sin_a = std::sin(radians);

  // scale is 24.8 fixed point, so 256 / scale is its reciprocal, which is
  // what the destination-to-source direction needs. The matrix's determinant
  // is 1, so undoing the scale is this single multiply.
  const double inv_scale = 256.0 / scale;
  const double src_cx = width / 2.0;
  const double src_cy = height / 2.0;

  for (int y = 0; y < out_height; y++) {
    const double ry = y - height;
    const double row_x = cos_a * ry;
    const double row_y = sin_a * ry;
    for (int x = 0; x < out_width; x++) {
      const double rx = x - width;
      const int bx =
          static_cast<int>((((sin_a * rx) + row_x) * inv_scale) + src_cx);
      const int by =
          static_cast<int>((((-cos_a * rx) + row_y) * inv_scale) + src_cy);

      // Destination pixels that map outside the shape stay transparent.
      if (bx >= 0 && bx < width && by >= 0 && by < height) {
        const uint8_t pixel = base::At(pixels, (by * width) + bx);
        if (pixel != 0) {
          base::At(dst, (y * out_width) + x) = pixel;
        }
      }
    }
  }
  return dst;
}

}  // namespace

void CC_Draw_Shape(PixelView& view, const std::span<const std::byte> shapefile,
                   const int shape_num, const int x, const int y,
                   const WindowNumberType window, ShapeFlags flags,
                   std::span<const uint8_t> fading_data,
                   std::span<const uint8_t> ghostdata, const DirType rotation,
                   const int32_t scale) {
  // Special kludge for E3 to prevent crashes
  //
  // Callers that ask for ghosting or fading without supplying the table get
  // the display class's default rather than a null dereference.
  if (base::Any(flags & SHAPE_GHOST) && ghostdata.empty()) {
    ghostdata = DisplayClass::SpecialGhost;
  }
  if (base::Any(flags & SHAPE_FADING) && fading_data.empty()) {
    fading_data = DisplayClass::FadingShade;
  }

  static std::vector<uint8_t> x_buffer(kShapeBufferSize);

  if (!shapefile.empty() && shape_num != -1) {
    int width = Get_Build_Frame_Width(shapefile);
    int height = Get_Build_Frame_Height(shapefile);

    // In WIn95, build shape returns a pointer to the shape not its size
    const auto shape_pointer =
        Build_Frame(shapefile, static_cast<uint16_t>(shape_num), shape_buffer);
    if (!shape_pointer.empty()) {
      PixelView draw_window(
          view.buffer(),
          base::At(base::At(WindowList, static_cast<int>(window)), kWindowX) +
              view.x_pos(),
          base::At(base::At(WindowList, static_cast<int>(window)), kWindowY) +
              view.y_pos(),
          base::At(base::At(WindowList, static_cast<int>(window)),
                   kWindowWidth),
          base::At(base::At(WindowList, static_cast<int>(window)),
                   kWindowHeight));
      auto buffer = shape_pointer;

      // Rotation and scale handler.
      // 0x0100 is 1.0 in the 24.8 fixed point scale, so this is "no rotation
      // and no scaling" -- the common case, which skips the slow path below.
      if (rotation != DIR_N || scale != 0x0100) {
        buffer = ScaleRotateShape(shape_pointer, width, height, x_buffer,
                                  rotation, scale);
        width *= 2;
        height *= 2;
      }

      // Special shadow drawing code (used for aircraft and bullets). Both bits
      // together mean a shadow; either one alone means something else.
      constexpr auto kShadowMask = SHAPE_FADING | SHAPE_PREDATOR;
      const auto shadow_bits = flags & kShadowMask;
      if (shadow_bits == kShadowMask) {
        flags = flags & ~kShadowMask;
        flags = flags | SHAPE_GHOST;
        ghostdata = DisplayClass::SpecialGhost;
      }

      // The predator (cloaking) effect samples the screen at an offset that
      // walks with the frame counter, which is what makes it shimmer. Objects
      // on the right half of the window walk it the other way, so that two
      // cloaked objects side by side do not ripple in lockstep.
      int pred_offset = static_cast<int>(CurrentFrame());

      if (x > base::At(base::At(WindowList, static_cast<int>(window)),
                       kWindowWidth) *
                  4) {
        pred_offset = -pred_offset;
      }

      if (draw_window.Lock()) {
        const engine::gfx::ShapeEffects effects{
            .ghost_table = ghostdata,
            .fading_table = fading_data,
            .predator_offset = pred_offset,
        };
        engine::gfx::DrawShapeFrame(x, y, width, height, std::as_bytes(buffer),
                                    draw_window, flags | SHAPE_TRANS, effects);
        draw_window.Unlock();
      }
    }
  }
}

Rect Shape_Dimensions(const std::span<const std::byte> shapedata,
                      const int shape_num) {
  Rect rect;

  if (shapedata.empty() || shape_num < 0 ||
      std::cmp_greater_equal(shape_num, Get_Build_Frame_Count(shapedata))) {
    return rect;
  }

  const auto sh =
      Build_Frame(shapedata, static_cast<uint16_t>(shape_num), shape_buffer);
  if (sh.empty()) {
    return rect;
  }
  const auto shape = sh;

  const int width = Get_Build_Frame_Width(shapedata);
  const int height = Get_Build_Frame_Height(shapedata);

  // Four scans, one per edge, each narrowing the search area for the next:
  // the top scan also gives a first guess at the left edge, the bottom scan
  // hands the right scan a starting column, and so on.
  rect.X = 0;
  rect.Y = 0;
  int x_limit = width - 1;
  const int y_limit = height - 1;

  // Find top edge of the shape.
  for (int y = 0; y <= y_limit; y++) {
    for (int x = 0; x <= x_limit; x++) {
      if (base::At(shape, base::ToSize((y * width) + x)) != 0) {
        rect.Y = y;
        rect.X = x;
        // Pushing y past the limit breaks the outer loop too -- the first row
        // holding any pixel is the top edge, so there is nothing left to scan.
        y = y_limit + 1;
        break;
      }
    }
  }

  // Find bottom edge of the shape.
  for (int y = y_limit; y >= rect.Y; y--) {
    for (int x = x_limit; x >= 0; x--) {
      if (base::At(shape, base::ToSize((y * width) + x)) != 0) {
        rect.Height = y - rect.Y + 1;
        x_limit = x;
        y = rect.Y - 1;
        break;
      }
    }
  }

  // Find left edge of the shape.
  for (int x = 0; x < rect.X; x++) {
    for (int y = rect.Y; y < rect.Y + rect.Height; y++) {
      if (base::At(shape, base::ToSize((y * width) + x)) != 0) {
        rect.X = x;
        x = rect.X;
        break;
      }
    }
  }

  // Find the right edge of the shape.
  for (int x = width - 1; x >= x_limit; x--) {
    for (int y = rect.Y; y < rect.Y + rect.Height; y++) {
      if (base::At(shape, base::ToSize((y * width) + x)) != 0) {
        rect.Width = x - rect.X + 1;
        x = x_limit - 1;
        break;
      }
    }
  }

  // Normalize the rectangle around the center of the shape.
  rect.X -= width / 2;
  rect.Y -= height / 2;

  // Return with the minimum rectangle that encloses the shape.
  return rect;
}

// Icons are 24x24 pixels in the source art, and the radar draws them at
// zoom_factor pixels per cell -- so each output pixel stands for several source
// ones.
//
// Rather than point-sampling, each output pixel takes the first
// non-transparent of the nine source pixels around its sample point (the
// off_x/off_y offsets). Without that spread, anything thinner than the sample
// step -- walls, most of a structure's outline -- would vanish at radar zoom.
std::vector<unsigned char> Get_Radar_Icon(
    const std::span<const std::byte> shapefile, const int shape_num, int frames,
    const int zoom_factor) {
  static constexpr int off_x[] = {0, 0, -1, 1, 0, -1, 1, -1, 1};
  static constexpr int off_y[] = {0, 0, -1, 1, 0, -1, 1, -1, 1};

  // If there is no shape file, then there can be no radar icon imagery.
  if (shapefile.empty() || zoom_factor <= 0 || zoom_factor > 24) {
    return {};
  }

  // Get the pixel width and height of the frame we built.  This will
  // be used to extract icons and build pixels.
  const int pixel_width = Get_Build_Frame_Width(shapefile);
  const int pixel_height = Get_Build_Frame_Height(shapefile);

  // Find the width and height in icons, adjust these by half an
  // icon because the artists may be sloppy and miss the edge of an
  // icon one way or the other.
  const int icon_width = (pixel_width + 12) / 24;
  const int icon_height = (pixel_height + 12) / 24;

  // If we have been told to build as many frames as possible, then
  // find out how many frames there are to build.
  if (frames == -1) {
    frames = Get_Build_Frame_Count(shapefile);
  }

  // Allocate a position to store our icons.  If the alloc fails then
  // we don't add these icons to the set.
  if (frames < 0) {
    return {};
  }
  std::vector<unsigned char> result(base::ToSize(
      (int64_t{icon_width} * icon_height * zoom_factor * zoom_factor * frames) +
      2));
  auto output = result.begin();
  *output++ = static_cast<unsigned char>(icon_width);
  *output++ = static_cast<unsigned char>(icon_height);
  const int val = 24 / zoom_factor;

  for (int frame_num = 0; frame_num < frames; ++frame_num) {
    // Build the current frame.  If the frame can not be built then we
    // just need to skip past this set of icons and try to build the
    // next frame.
    const auto ptr =
        Build_Frame(shapefile, static_cast<uint16_t>(shape_num + frame_num),
                    TheScreen().sys_mem_page().bytes());
    if (!ptr.empty()) {
      // Loop through the icon width and the icon height building icons
      // into the buffer pointer.  When the getx or gety falls outside of
      // the width and height of the shape, just insert transparent pixels.
      for (int icon_y = 0; icon_y < icon_height; icon_y++) {
        for (int icon_x = 0; icon_x < icon_width; icon_x++) {
          for (int y = 0; y < zoom_factor; y++) {
            for (int x = 0; x < zoom_factor; x++) {
              const int getx = (icon_x * 24) + (x * val) + (zoom_factor / 2);
              const int gety = (icon_y * 24) + (y * val) + (zoom_factor / 2);
              if (getx < pixel_width && gety < pixel_height) {
                unsigned char pixel = 0;
                for (int lp = 0; lp < 9; ++lp) {
                  const int sample_x = getx - base::At(off_x, lp);
                  const int sample_y = gety - base::At(off_y, lp);
                  if (sample_x < 0 || sample_x >= pixel_width || sample_y < 0 ||
                      sample_y >= pixel_height) {
                    continue;
                  }
                  pixel = base::At(
                      ptr, base::ToSize((sample_y * pixel_width) + sample_x));

                  if (pixel == kLtGreen) {
                    pixel = 0;
                  }
                  if (pixel) {
                    break;
                  }
                }
                *output++ = pixel;
              } else {
                *output++ = 0;
              }
            }
          }
        }
      }
    } else {
      output += static_cast<base::ssize>(icon_width) * icon_height *
                zoom_factor * zoom_factor;
    }
  }
  return result;
}
