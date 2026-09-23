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

// File: The out-of-line members of PixelView and PixelBuffer: attaching a view
// to a page, the drawing primitives that work on the locked pixels, giving a
// page its pixels, and - for the one page the window shows - the SDL surface
// and textures behind it and the presenting done through them.
//
// The primitives all clip with the Cohen-Sutherland outcodes of base/clip.h
// and then walk whole rows.

#include "sdllib/pixel_buffer.h"

#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "base/array.h"
#include "base/clip.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/types.h"
#include "sdllib/bitmap.h"
#include "sdllib/display.h"
#include "sdllib/font.h"
#include "sdllib/ww_win.h"

PixelView::PixelView(PixelBuffer* buffer, const int x, const int y,
                     const int width, const int height) {
  Attach(buffer, x, y, width, height);
}

void PixelView::Attach(PixelBuffer* buffer, int x, int y, int width,
                       int height) {
  // Clamp the corner into the buffer. A buffer that Init() has not sized yet
  // has no last pixel to clamp to, so the corner stays at the origin and the
  // width and height below come out zero; Screen builds its views against
  // such pages and attaches them again once the video mode is known.
  x = std::clamp(x, 0, std::max(buffer->width() - 1, 0));
  y = std::clamp(y, 0, std::max(buffer->height() - 1, 0));

  if (x + width > buffer->width()) {
    width = buffer->width() - x;
  }

  if (y + height > buffer->height()) {
    height = buffer->height() - y;
  }

  /*======================================================================*/
  /* Get a pointer to the top left edge of the buffer.
   */
  /*======================================================================*/
  offset_ =
      buffer->bytes().empty()
          ? nullptr
          : buffer->bytes()
                .subspan(base::ToSize((static_cast<base::ssize>(
                                           buffer->width() + buffer->pitch()) *
                                       y) +
                                      x))
                .data();

  /*======================================================================*/
  /* Copy over all of the variables that we need to store.
   */
  /*======================================================================*/
  x_pos_ = x;
  y_pos_ = y;
  x_add_ = buffer->width() - width;
  width_ = width;
  height_ = height;
  pitch_ = buffer->pitch();
  buffer_ = buffer;
}

std::span<uint8_t> PixelView::pixels() {
  if (buffer_ == nullptr) {
    return {};
  }
  return buffer_->bytes().subspan(base::ToSize((y_pos_ * stride()) + x_pos_));
}

bool PixelView::Lock() {
  if (buffer_ == nullptr) {
    return false;
  }

  if (!buffer_->LockSurface()) {
    return false;
  }

  Attach(buffer_, x_pos_, y_pos_, width_, height_);
  return true;
}

// Not const although it writes no member: releasing the buffer's lock is
// the whole point, and it ends this view's access to the pixels.
void PixelView::Unlock() {
  if (buffer_ != nullptr) {
    buffer_->UnlockSurface();
  }
}

int PixelView::lock_count() const {
  return buffer_ == nullptr ? 0 : buffer_->lock_count();
}

bool PixelView::NeedsLock() const {
  // Named for the DirectDraw surfaces this used to mean; callers read it as
  // "do the pixels have to be locked before they can be touched", which is
  // true of exactly the window's surface.
  return buffer_ != nullptr && buffer_->IsWindowSurface();
}

void PixelView::PutPixel(const int x, const int y, const uint8_t color) {
  if (Lock()) {
    PutPixelLocked(x, y, color);
    Unlock();
  }
}

void PixelView::PutPixelLocked(const int x, const int y, const uint8_t color) {
  if (x >= 0 && y >= 0 && x < width() && y < height()) {
    base::At(pixels(), base::ToSize(x + (y * stride()))) = color;
  }
}

int PixelView::GetPixel(const int x, const int y) {
  int return_code = 0;
  if (Lock()) {
    return_code = GetPixelLocked(x, y);
    Unlock();
  }
  return return_code;
}

int PixelView::GetPixelLocked(const int x, const int y) {
  if (x < 0 || y < 0 || x >= width() || y >= height()) {
    return 0;
  }

  const base::ssize dst_area = stride();
  const auto dst_offset = pixels().begin() + x + (y * dst_area);

  return *dst_offset;
}

void PixelView::Clear(const uint8_t color) {
  if (Lock()) {
    ClearLocked(color);
    Unlock();
  }
}

void PixelView::ClearLocked(const uint8_t color) {
  const base::ssize dst_area = stride();
  auto dst_offset = pixels().begin();

  const int pixel_count = width();
  int line_count = height();

  // fill lines
  do {
    std::fill_n(dst_offset, pixel_count, color);
    dst_offset += dst_area;
  } while (--line_count);
}

void PixelView::CopyToBuffer(const int src_x, const int src_y, const int width,
                             const int height, const std::span<uint8_t> dest) {
  if (Lock()) {
    CopyToBufferLocked(src_x, src_y, width, height, dest);
    Unlock();
  }
}

void PixelView::CopyToBufferLocked(const int src_x, const int src_y,
                                   const int width, const int height,
                                   std::span<uint8_t> dest) {
  int dst_x0 = 0;
  int dst_y0 = 0;

  // clip src
  int src_x0 = src_x;
  int src_y0 = src_y;
  int src_x1 = src_x + width;
  int src_y1 = src_y + height;

  const OutCode code0 = OutCodeOf(src_x0, src_y0, width_, height_);
  const OutCode code1 = OutCodeOf(src_x1, src_y1, width_ + 1, height_ + 1);

  // outside
  if (base::Any(code0 & code1)) {
    return;
  }

  if (base::Any(code0 | code1)) {
    // apply clip
    if (base::Any(code0 & OutCode::kLeft)) {
      dst_x0 -= src_x0;
      src_x0 = 0;
    }
    if (base::Any(code1 & OutCode::kRight)) {
      src_x1 = width_;
    }
    if (base::Any(code0 & OutCode::kAbove)) {
      dst_y0 -= src_y0;
      src_y0 = 0;
    }
    if (base::Any(code1 & OutCode::kBelow)) {
      src_y1 = height_;
    }
  }

  const base::ssize src_area = stride();
  auto src_offset = pixels().begin() + src_x0 + (src_y0 * src_area);

  auto dst_offset =
      dest.begin() + dst_x0 + (static_cast<base::ssize>(dst_y0) * width);

  if (src_x1 <= src_x0 || src_y1 <= src_y0) {
    return;
  }

  if (std::to_address(src_offset) == std::to_address(dst_offset)) {
    return;
  }

  const int pixel_count = src_x1 - src_x0;
  int line_count = src_y1 - src_y0;

  // copy lines
  do {
    std::copy_n(src_offset, pixel_count, dst_offset);
    src_offset += src_area;
    dst_offset += width;
  } while (--line_count);
}

void PixelView::CopyFromBuffer(const int dst_x, const int dst_y,
                               const int width, const int height,
                               const std::span<const uint8_t> source) {
  if (Lock()) {
    CopyFromBufferLocked(dst_x, dst_y, width, height, source);
    Unlock();
  }
}

void PixelView::CopyFromBufferLocked(const int dst_x, const int dst_y,
                                     const int width, const int height,
                                     std::span<const uint8_t> source) {
  int src_x0 = 0;
  int src_y0 = 0;

  // clip dest
  int dst_x0 = dst_x;
  int dst_y0 = dst_y;
  int dst_x1 = dst_x + width;
  int dst_y1 = dst_y + height;

  const OutCode code0 = OutCodeOf(dst_x0, dst_y0, width_, height_);
  const OutCode code1 = OutCodeOf(dst_x1, dst_y1, width_ + 1, height_ + 1);

  // outside
  if (base::Any(code0 & code1)) {
    return;
  }

  if (base::Any(code0 | code1)) {
    // apply clip
    if (base::Any(code0 & OutCode::kLeft)) {
      src_x0 -= dst_x0;
      dst_x0 = 0;
    }
    if (base::Any(code1 & OutCode::kRight)) {
      dst_x1 = width_;
    }
    if (base::Any(code0 & OutCode::kAbove)) {
      src_y0 -= dst_y0;
      dst_y0 = 0;
    }
    if (base::Any(code1 & OutCode::kBelow)) {
      dst_y1 = height_;
    }
  }

  auto src_offset =
      source.begin() + src_x0 + (static_cast<base::ssize>(src_y0) * width);

  const base::ssize dst_area = stride();
  auto dst_offset = pixels().begin() + dst_x0 + (dst_y0 * dst_area);

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return;
  }

  if (std::to_address(src_offset) == std::to_address(dst_offset)) {
    return;
  }

  const int pixel_count = dst_x1 - dst_x0;
  int line_count = dst_y1 - dst_y0;

  // copy lines
  do {
    std::copy_n(src_offset, pixel_count, dst_offset);
    src_offset += width;
    dst_offset += dst_area;
  } while (--line_count);
}

void PixelView::BlitTo(PixelView& dest, const int src_x, const int src_y,
                       const int dst_x, const int dst_y, const int width,
                       const int height, const bool transparent) {
  if (Lock()) {
    if (dest.Lock()) {
      BlitToLocked(dest, src_x, src_y, dst_x, dst_y, width, height,
                   transparent);
      dest.Unlock();
    }
    Unlock();
  }
}

void PixelView::BlitToLocked(PixelView& dest, const int src_x, const int src_y,
                             const int dst_x, const int dst_y, const int width,
                             const int height, const bool transparent) {
  // Only Tiberian Dawn asks for a transparent blit.

  // clip source
  int src_x0 = src_x;
  int src_y0 = src_y;
  int src_width = width;
  int src_height = height;
  if (!ClipRect(src_x0, src_y0, src_width, src_height, width_, height_)) {
    return;
  }
  int src_x1 = src_x0 + src_width;
  int src_y1 = src_y0 + src_height;

  // clip dest
  // Whatever the source clip took off the top and left moves the destination
  // by as much, so the remaining pixels keep their place.
  int dst_x0 = dst_x + (src_x0 - src_x);
  int dst_y0 = dst_y + (src_y0 - src_y);
  int dst_x1 = dst_x0 + (src_x1 - src_x0);
  int dst_y1 = dst_y0 + (src_y1 - src_y0);

  const OutCode code0 = OutCodeOf(dst_x0, dst_y0, dest.width(), dest.height());
  const OutCode code1 =
      OutCodeOf(dst_x1, dst_y1, dest.width() + 1, dest.height() + 1);

  // outside
  if (base::Any(code0 & code1)) {
    return;
  }

  if (base::Any(code0 | code1)) {
    // apply clip
    if (base::Any(code0 & OutCode::kLeft)) {
      src_x0 -= dst_x0;
      dst_x0 = 0;
    }
    if (base::Any(code1 & OutCode::kRight)) {
      src_x1 -= dst_x1 - dest.width();
      dst_x1 = dest.width();
    }
    if (base::Any(code0 & OutCode::kAbove)) {
      src_y0 -= dst_y0;
      dst_y0 = 0;
    }
    if (base::Any(code1 & OutCode::kBelow)) {
      src_y1 -= dst_y1 - dest.height();
      dst_y1 = dest.height();
    }
  }

  const base::ssize src_area = stride();
  auto src_offset = pixels().begin() + src_x0 + (src_y0 * src_area);

  const base::ssize dst_area = dest.stride();
  auto dst_offset = dest.pixels().begin() + dst_x0 + (dst_y0 * dst_area);

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return;
  }

  if (std::to_address(src_offset) == std::to_address(dst_offset)) {
    return;
  }

  const int pixel_count = src_x1 - src_x0;
  int line_count = src_y1 - src_y0;

  if (src_offset < dst_offset) {
    // backward (bottom -> top)
    if (transparent) {
      // copy transparent lines backwards
      src_offset += src_area * (line_count - 1);
      dst_offset += dst_area * (line_count - 1);
      do {
        for (int x = 0; x < pixel_count; x++) {
          if (base::At(pixels(), (src_offset - pixels().begin()) + x)) {
            base::At(dest.pixels(), (dst_offset - dest.pixels().begin()) + x) =
                base::At(pixels(), (src_offset - pixels().begin()) + x);
          }
        }
        src_offset -= src_area;
        dst_offset -= dst_area;
      } while (--line_count);
    } else {
      // copy lines backwards
      src_offset += src_area * (line_count - 1);
      dst_offset += dst_area * (line_count - 1);
      do {
        if (src_offset < dst_offset && dst_offset < src_offset + pixel_count) {
          std::copy_backward(src_offset, src_offset + pixel_count,
                             dst_offset + pixel_count);
        } else {
          std::copy_n(src_offset, pixel_count, dst_offset);
        }
        src_offset -= src_area;
        dst_offset -= dst_area;
      } while (--line_count);
    }
  } else {
    // forward (top-> bottom)
    if (transparent) {
      // copy transparent lines
      do {
        for (int x = 0; x < pixel_count; x++) {
          if (base::At(pixels(), (src_offset - pixels().begin()) + x)) {
            base::At(dest.pixels(), (dst_offset - dest.pixels().begin()) + x) =
                base::At(pixels(), (src_offset - pixels().begin()) + x);
          }
        }
        src_offset += src_area;
        dst_offset += dst_area;
      } while (--line_count);
    } else {
      // copy lines
      do {
        if (src_offset < dst_offset && dst_offset < src_offset + pixel_count) {
          std::copy_backward(src_offset, src_offset + pixel_count,
                             dst_offset + pixel_count);
        } else {
          std::copy_n(src_offset, pixel_count, dst_offset);
        }
        src_offset += src_area;
        dst_offset += dst_area;
      } while (--line_count);
    }
  }
}

void PixelView::Scale(PixelView& dest, const int src_x, const int src_y,
                      const int dst_x, const int dst_y, const int src_width,
                      const int src_height, const int dst_width,
                      const int dst_height, const bool transparent,
                      const std::span<const uint8_t> remap_table) {
  if (Lock()) {
    if (dest.Lock()) {
      ScaleLocked(dest, src_x, src_y, dst_x, dst_y, src_width, src_height,
                  dst_width, dst_height, transparent, remap_table);
      dest.Unlock();
    }
    Unlock();
  }
}

void PixelView::ScaleLocked(PixelView& dest, int src_x, int src_y, int dst_x,
                            int dst_y, int src_width, int src_height,
                            int dst_width, int dst_height, bool transparent,
                            std::span<const uint8_t> remap_table) {
  // Check for scale error when to or from size 0,0
  if (dst_width == 0 || dst_height == 0 || src_width == 0 || src_height == 0) {
    return;
  }

  int src_x0 = src_x;
  int src_y0 = src_y;
  int src_x1 = src_x + src_width;
  int src_y1 = src_y + src_height;

  int dst_x0 = dst_x;
  int dst_y0 = dst_y;
  int dst_x1 = dst_x + dst_width;
  int dst_y1 = dst_y + dst_height;

  // clip source
  OutCode code0 = OutCodeOf(src_x0, src_y0, width_, height_);
  OutCode code1 = OutCodeOf(src_x1, src_y1, width_ + 1, height_ + 1);

  // outside
  if (base::Any(code0 & code1)) {
    return;
  }

  if (base::Any(code0 | code1)) {
    // apply clip
    if (base::Any(code0 & OutCode::kLeft)) {
      src_x0 = 0;
      dst_x0 = dst_x + ((src_x0 - src_x) * dst_width / src_width);
    }
    if (base::Any(code1 & OutCode::kRight)) {
      src_x1 = width_;
      dst_x1 = dst_x + ((src_x1 - src_x) * dst_width / src_width);
    }
    if (base::Any(code0 & OutCode::kAbove)) {
      src_y0 = 0;
      dst_y0 = dst_y + ((src_y0 - src_y) * dst_height / src_height);
    }
    if (base::Any(code1 & OutCode::kBelow)) {
      src_y1 = height_;
      dst_y1 = dst_y + ((src_y1 - src_y) * dst_height / src_height);
    }
  }

  // clip dest
  code0 = OutCodeOf(dst_x0, dst_y0, dest.width(), dest.height());
  code1 = OutCodeOf(dst_x1, dst_y1, dest.width() + 1, dest.height() + 1);

  // outside
  if (base::Any(code0 & code1)) {
    return;
  }

  if (base::Any(code0 | code1)) {
    // apply clip
    if (base::Any(code0 & OutCode::kLeft)) {
      dst_x0 = 0;
      src_x0 = src_x + ((dst_x0 - dst_x) * src_width / dst_width);
    }
    if (base::Any(code1 & OutCode::kRight)) {
      dst_x1 = dest.width();
    }
    if (base::Any(code0 & OutCode::kAbove)) {
      src_y0 = src_y + ((dst_y0 - dst_y) * src_height / dst_height);
    }
    if (base::Any(code1 & OutCode::kBelow)) {
      dst_y1 = dest.height();
    }
  }

  // do scale
  const base::ssize src_win_width = stride();
  auto src_offset = pixels().begin() + src_x0 + (src_y0 * src_win_width);

  const base::ssize dst_win_width = dest.stride();
  auto dst_offset = dest.pixels().begin() + dst_x0 + (dst_y0 * dst_win_width);

  const int dy_intr = static_cast<int>(src_height / dst_height * src_win_width);
  const int dy_frac = src_height % dst_height;
  int dy_acc = -dst_height;

  const int dx_frac = (src_width * 65536) / dst_width;

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return;
  }

  int counter_y = dst_y1 - dst_y0;
  const int pixel_count = dst_x1 - dst_x0;

  if (transparent && !remap_table.empty()) {
    do {
      int counter_x = pixel_count;
      int x = 0;
      auto out = dst_offset;
      do {
        const uint8_t pixel =
            base::At(pixels(), (src_offset - pixels().begin()) + (x / 65536));

        if (pixel) {
          *out = base::At(remap_table, pixel);
        }

        x += dx_frac;
        ++out;
      } while (--counter_x);

      src_offset += dy_intr;
      dst_offset += dst_win_width;

      dy_acc += dy_frac;
      if (dy_acc > 0) {
        src_offset += src_win_width;
        dy_acc -= dst_height;
      }
    } while (--counter_y);
  } else if (transparent) {
    // normal scale with transparency
    do {
      int counter_x = pixel_count;
      int x = 0;
      auto out = dst_offset;
      do {
        const uint8_t pixel =
            base::At(pixels(), (src_offset - pixels().begin()) + (x / 65536));

        if (pixel) {
          *out = pixel;
        }

        x += dx_frac;
        ++out;
      } while (--counter_x);

      src_offset += dy_intr;
      dst_offset += dst_win_width;

      dy_acc += dy_frac;
      if (dy_acc > 0) {
        src_offset += src_win_width;
        dy_acc -= dst_height;
      }
    } while (--counter_y);
  } else if (!remap_table.empty()) {
    // normal scale with remap_table
    do {
      int counter_x = pixel_count;
      int x = 0;
      auto out = dst_offset;
      do {
        *out++ = base::At(
            remap_table,
            base::At(pixels(), (src_offset - pixels().begin()) + (x / 65536)));
        x += dx_frac;
      } while (--counter_x);

      src_offset += dy_intr;
      dst_offset += dst_win_width;

      dy_acc += dy_frac;
      if (dy_acc > 0) {
        src_offset += src_win_width;
        dy_acc -= dst_height;
      }
    } while (--counter_y);
  } else {
    // normal scale
    do {
      int counter_x = pixel_count;
      int x = 0;
      auto out = dst_offset;
      do {
        *out++ =
            base::At(pixels(), (src_offset - pixels().begin()) + (x / 65536));
        x += dx_frac;
      } while (--counter_x);

      src_offset += dy_intr;
      dst_offset += dst_win_width;

      dy_acc += dy_frac;
      if (dy_acc > 0) {
        src_offset += src_win_width;
        dy_acc -= dst_height;
      }
    } while (--counter_y);
  }
}

void PixelView::Print(const FontStyle& style, const char* text, const int x,
                      const int y, const int fore_color, const int back_color) {
  if (Lock()) {
    PrintLocked(style, text, x, y, fore_color, back_color);
    Unlock();
  }
}

void PixelView::Print(const FontStyle& style, const int value, const int x,
                      const int y, const int fore_color, const int back_color) {
  Print(style, absl::StrCat(value).c_str(), x, y, fore_color, back_color);
}

void PixelView::PrintLocked(const FontStyle& style, const char* text, int x,
                            int y, const int fore_color, const int back_color) {
  const FontView& font = style.font;
  if (!text || font.data().empty()) {
    return;
  }

  const int start_x = x;
  const base::ssize buffer_stride = stride();
  auto line_start = pixels().begin() + (buffer_stride * y);

  const int max_glyph_height = font.MaxHeight();
  y += max_glyph_height;
  if (y > height_) {
    return;
  }

  // Glyph pixels are indices into the style's palette, with entry 0 the
  // background (0 also means transparent) and entry 1 the foreground.
  const auto background = static_cast<uint8_t>(back_color);
  std::array<uint8_t, 16> palette = style.palette;
  palette.at(1) = static_cast<uint8_t>(fore_color);
  palette.at(0) = background;

  auto next_glyph_start = line_start + x;

  for (const char character : std::string_view(text)) {
    // Unsigned so characters >= 128 index the metric tables correctly.
    const auto ch = static_cast<uint8_t>(character);
    if (ch == '\0') {
      return;
    }

    auto draw_ptr = next_glyph_start;
    const int glyph_width = font.GlyphWidth(ch);

    if (ch == '\n' || ch == '\r' ||
        x + glyph_width + style.x_spacing > width_) {
      // Advance to the next line: '\n' returns to the viewport edge, '\r'
      // and auto-wrap return to the starting column.
      const int line_height = max_glyph_height + style.y_spacing;
      if (height_ < y + line_height) {
        return;  // No room for another line.
      }

      line_start += buffer_stride * line_height;
      y += line_height;
      x = ch == '\n' ? 0 : start_x;
      next_glyph_start = line_start + x;

      if (ch == '\n' || ch == '\r') {
        continue;
      }
      draw_ptr = next_glyph_start;  // The wrapped glyph draws on the new line.
    }

    x += glyph_width + style.x_spacing;
    next_glyph_start = draw_ptr + style.x_spacing + glyph_width;

    // Distance from the end of a glyph row to the start of the next one.
    const int row_skip = static_cast<int>(buffer_stride - glyph_width);
    const int glyph_height = font.GlyphHeight(ch);
    const int blank_rows_above = font.GlyphBlankRowsAbove(ch);
    const int blank_rows_below =
        max_glyph_height - (glyph_height + blank_rows_above);

    // Fill the blank rows above the glyph, or skip them if transparent.
    if (blank_rows_above != 0) {
      if (background == 0) {
        draw_ptr += blank_rows_above * buffer_stride;
      } else {
        for (int row = 0; row < blank_rows_above; ++row) {
          for (int col = 0; col < glyph_width; ++col) {
            *draw_ptr++ = background;
          }
          draw_ptr += row_skip;
        }
      }
    }

    if (glyph_height != 0) {
      // Each glyph byte packs two 4-bit palette indices, low nibble first.
      // Index 0 is transparent unless a background color is set, in which
      // case palette[0] already paints it.
      const auto glyph = font.GlyphData(ch);
      if (glyph.empty()) {
        return;
      }
      auto glyph_data = glyph.begin();
      for (int row = 0; row < glyph_height; ++row) {
        int cols_left = glyph_width;
        while (cols_left > 0) {
          const auto pixel_pair = std::to_integer<uint8_t>(*glyph_data++);

          const uint8_t left = base::At(std::span(palette), pixel_pair & 0x0F);
          if (left != 0) {
            *draw_ptr = left;
          }
          ++draw_ptr;
          --cols_left;

          if (cols_left > 0) {
            const uint8_t right = base::At(std::span(palette), pixel_pair >> 4);
            if (right != 0) {
              *draw_ptr = right;
            }
            ++draw_ptr;
            --cols_left;
          }
        }
        draw_ptr += row_skip;
      }

      // Fill the blank rows below the glyph unless transparent.
      if (blank_rows_below != 0 && background != 0) {
        for (int row = 0; row < blank_rows_below; ++row) {
          for (int col = 0; col < glyph_width; ++col) {
            *draw_ptr++ = background;
          }
          draw_ptr += row_skip;
        }
      }
    }
  }
}

void PixelView::DrawLine(const int x1, const int y1, const int x2, const int y2,
                         const uint8_t color) {
  if (Lock()) {
    DrawLineLocked(x1, y1, x2, y2, color);
    Unlock();
  }
}

void PixelView::DrawRect(const int x1, const int y1, const int x2, const int y2,
                         const uint8_t color) {
  if (!Lock()) {
    return;
  }
  DrawLineLocked(x1, y1, x2, y1, color);
  DrawLineLocked(x1, y2, x2, y2, color);
  DrawLineLocked(x1, y1, x1, y2, color);
  DrawLineLocked(x2, y1, x2, y2, color);
  Unlock();
}

void PixelView::FillRect(const int x1, const int y1, const int x2, const int y2,
                         const uint8_t color) {
  if (Lock()) {
    FillRectLocked(x1, y1, x2, y2, color);
    Unlock();
  }
}

void PixelView::DrawLineLocked(int x1, int y1, int x2, int y2,
                               const uint8_t color) {
  const int width = width_;
  const int height = height_;

  // this is different to the original asm, but reused from blits
  const OutCode code0 = OutCodeOf(x1, y1, width, height);
  const OutCode code1 = OutCodeOf(x2, y2, width, height);

  if (base::Any(code0 & code1)) {
    return;
  }

  if (base::Any(code0)) {
    if (base::Any(code0 & OutCode::kLeft))  // left
    {
      if (x2 != x1) {
        y1 += -x1 * (y2 - y1) / (x2 - x1);
      }
      x1 = 0;
    } else if (base::Any(code0 & OutCode::kRight))  // right
    {
      if (x2 != x1) {
        y1 += (width - 1 - x1) * (y2 - y1) / (x2 - x1);
      }
      x1 = width - 1;
    }

    if (base::Any(code0 & OutCode::kAbove))  // top
    {
      if (y2 != y1) {
        x1 = x1 + (-y1 * (x2 - x1) / (y2 - y1));
      }
      y1 = 0;
    } else if (base::Any(code0 & OutCode::kBelow))  // bottom
    {
      if (y2 != y1) {
        x1 = x1 + ((height - 1 - y1) * (x2 - x1) / (y2 - y1));
      }
      y1 = height - 1;
    }
  }

  if (base::Any(code1)) {
    if (base::Any(code1 & OutCode::kLeft))  // left
    {
      if (x1 != x2) {
        y2 = y2 + (-x2 * (y1 - y2) / (x1 - x2));
      }
      x2 = 0;
    } else if (base::Any(code1 & OutCode::kRight))  // right
    {
      if (x1 != x2) {
        y2 = y2 + ((width - 1 - x2) * (y1 - y2) / (x1 - x2));
      }
      x2 = width - 1;
    }

    if (base::Any(code1 & OutCode::kAbove))  // top
    {
      if (y1 != y2) {
        x2 = x2 + (-y2 * (x1 - x2) / (y1 - y2));
      }
      y2 = 0;
    } else if (base::Any(code1 & OutCode::kBelow))  // bottom
    {
      if (y1 != y2) {
        x2 = x2 + ((height - 1 - y2) * (x1 - x2) / (y1 - y2));
      }
      y2 = height - 1;
    }
  }

  const base::ssize bpr = stride();

  int y_dist = y2 - y1;

  if (y_dist == 0) {
    // horizontal
    if (x2 < x1) {
      std::swap(x2, x1);
    }

    const int count = x2 - x1 + 1;
    const auto page = pixels().begin() + x1 + (bpr * y1);
    std::fill_n(page, count, color);

    return;
  }

  // not horizontal
  if (y_dist == 0 || y2 < y1) {
    y1 = y1 + y_dist;
    y_dist = -y_dist;

    std::swap(x2, x1);
  }

  auto page = pixels().begin() + x1 + (bpr * y1);

  int step = 1;
  int x_dist = x2 - x1;

  if (x_dist == 0) {
    // vertical
    int count = y_dist + 1;
    do {
      *page = color;
      page = page + bpr;
    } while (--count);
    return;
  }

  // not vertical
  if (x_dist == 0 || x2 < x1) {
    x_dist = -x_dist;
    step = -1;
  }

  if (x_dist < y_dist) {
    int count = y_dist;
    int accum = y_dist / 2;
    while (true) {
      *page = color;
      if (--count == 0) {
        break;
      }
      page += bpr;

      accum -= x_dist;
      if (accum < 0) {
        accum += y_dist;
        page += step;
      }
    }
  } else {
    int count = x_dist;
    int accum = x_dist / 2;
    while (true) {
      *page = color;
      if (--count == 0) {
        break;
      }
      page = page + step;

      accum -= y_dist;
      if (accum < 0) {
        accum += x_dist;
        page += bpr;
      }
    }
  }
}

void PixelView::FillRectLocked(int x1, int y1, int x2, int y2,
                               const uint8_t color) {
  if (x1 > x2) {
    std::swap(x1, x2);
  }
  if (y1 > y2) {
    std::swap(y1, y2);
  }

  // clamp to bounds
  x1 = std::max(x1, 0);
  y1 = std::max(y1, 0);

  if (x2 >= width_) {
    x2 = width_ - 1;
  }
  if (y2 >= height_) {
    y2 = height_ - 1;
  }

  // nothing to fill
  if (x2 < x1 || y2 < y1) {
    return;
  }

  const base::ssize dst_area = stride();
  auto dst_offset = pixels().begin() + x1 + (y1 * dst_area);

  const int pixel_count = x2 - x1 + 1;
  int line_count = y2 - y1 + 1;

  // fill lines
  do {
    std::fill_n(dst_offset, pixel_count, color);
    dst_offset += dst_area;
  } while (--line_count);
}

void PixelView::Remap(const int x1, const int y1, const int width,
                      const int height,
                      const std::span<const uint8_t> remap_table) {
  if (Lock()) {
    RemapLocked(x1, y1, width, height, remap_table);
    Unlock();
  }
}

void PixelView::RemapLocked(const int x1, const int y1, const int width,
                            const int height,
                            const std::span<const uint8_t> remap_table) {
  if (remap_table.empty()) {
    return;
  }

  // clip
  int dst_x0 = x1;
  int dst_y0 = y1;
  int pixel_count = width;
  int line_count = height;
  if (!ClipRect(dst_x0, dst_y0, pixel_count, line_count, width_, height_)) {
    return;
  }

  const base::ssize dst_area = stride();
  auto dst_offset = pixels().begin() + dst_x0 + (dst_y0 * dst_area);

  if (pixel_count <= 0 || line_count <= 0) {
    return;
  }

  const int skip = static_cast<int>(dst_area - pixel_count);

  // Remap one row at a time.
  do {
    for (int x = 0; x < pixel_count; x++) {
      const auto v = base::At(remap_table, *dst_offset);
      *dst_offset++ = v;
    }
    dst_offset += skip;
  } while (--line_count);
}

PixelBuffer::PixelBuffer(const int width, const int height,
                         const std::span<uint8_t> buffer,
                         const int32_t byte_count)
    : PixelBuffer() {
  Init(width, height, buffer, byte_count, BUFFER_NONE);
}

PixelBuffer::PixelBuffer(const int width, const int height,
                         const std::span<uint8_t> buffer)
    : PixelBuffer(width, height, buffer, width * height) {}

PixelBuffer::PixelBuffer() {
  // Attach the view even though there are no pixels yet, so that view() is
  // usable before Init(); the rectangle comes out empty and Init() attaches
  // it again for real.
  whole_.Attach(this, 0, 0, 0, 0);
}

PixelBuffer::~PixelBuffer() {
  ReleaseSurfaces();
  if (HasDisplay() && TheDisplay().window_page() == this) {
    TheDisplay().DetachWindowPage();
  }
}

void PixelBuffer::Init(const int width, const int height,
                       const std::span<uint8_t> buffer,
                       const int32_t byte_count, const PixelBufferFlags flags) {
  CHECK_GE(width, 0);
  CHECK_GE(height, 0);
  CHECK_GE(byte_count, 0);
  const auto pixel_count = base::ToSize(width) * base::ToSize(height);
  if (!base::Any(flags & BUFFER_VISIBLE)) {
    CHECK_LE(pixel_count,
             buffer.empty()
                 ? (byte_count == 0 ? pixel_count : base::ToSize(byte_count))
                 : buffer.size());
  }
  width_ = width;
  height_ = height;
  pitch_ = 0;

  if (base::Any(flags & BUFFER_VISIBLE)) {
    // The pixels are the SDL surface's; bytes_ points at them only between
    // LockSurface() and UnlockSurface().
    owned_pixels_.reset();
    bytes_ = {};
    CreateDisplaySurface();
  } else if (buffer.empty()) {
    const auto size = byte_count == 0 ? pixel_count : base::ToSize(byte_count);
    owned_pixels_ = std::make_unique<uint8_t[]>(size);
    // The allocation above holds exactly `size` bytes.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    bytes_ = std::span(owned_pixels_.get(), size);
  } else {
    owned_pixels_.reset();
    bytes_ = buffer;
  }

  whole_.Attach(this, 0, 0, width_, height_);
}

void PixelBuffer::ReleaseSurfaces() { DestroyDisplaySurface(); }

namespace {

// The renderer, which every texture and present in this file needs.
SDL_Renderer* Renderer() {
  return static_cast<SDL_Renderer*>(TheDisplay().renderer());
}

}  // namespace

bool PixelBuffer::LockSurface() {
  if (!palette_surface_) {
    return true;
  }

  if (!lock_count_) {
    if (SDL_LockSurface(static_cast<SDL_Surface*>(palette_surface_)) != 0) {
      return false;
    }
    const auto* surface = static_cast<SDL_Surface*>(palette_surface_);
    // SDL_LockSurface exposes pitch bytes for each of the surface's rows until
    // it is unlocked.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    bytes_ = std::span(static_cast<uint8_t*>(surface->pixels),
                       base::ToSize(surface->pitch) * base::ToSize(surface->h));
    whole_.Attach(this, 0, 0, width_, height_);
  }

  lock_count_++;
  return true;
}

void PixelBuffer::UnlockSurface() {
  if (!palette_surface_ || !lock_count_) {
    return;
  }

  lock_count_--;

  if (!lock_count_) {
    SDL_UnlockSurface(static_cast<SDL_Surface*>(palette_surface_));
    bytes_ = {};
    // The pixels are gone until the next lock; leave no view pointing at them.
    whole_.Attach(this, 0, 0, width_, height_);
    // Content was drawn to palette_surface_ - clear VQA texture to switch back
    // to normal rendering mode
    if (scaled_frame_texture_) {
      DropScaledFrame();
    }
    Present(false);
  }
}

void PixelBuffer::Present(const bool end_frame) const {
  // If VQA texture exists, keep presenting it (for animations like map select
  // that need to preserve the last frame indefinitely)
  if (scaled_frame_texture_) {
    TheDisplay().CancelRedrawTimer();

    if (!end_frame) {
      return;
    }

    // Present the VQA frame
    SDL_RenderClear(Renderer());
    SDL_RenderCopy(Renderer(), static_cast<SDL_Texture*>(scaled_frame_texture_),
                   nullptr, nullptr);
    TheDisplay().PresentFrame();
    SDL_Event_Loop();
    return;
  }

  auto* window_tex = static_cast<SDL_Texture*>(window_texture_);

  // Nothing asked for the frame to end, so leave the drawing in the surface
  // and let the redraw timer present it.
  if (!end_frame) {
    TheDisplay().ArmRedrawTimer();
    return;
  }

  TheDisplay().CancelRedrawTimer();

  // blit from paletted surface
  SDL_Surface* tmp_surf = nullptr;
  SDL_LockTextureToSurface(window_tex, nullptr, &tmp_surf);
  SDL_BlitSurface(static_cast<SDL_Surface*>(palette_surface_), nullptr,
                  tmp_surf, nullptr);
  SDL_UnlockTexture(window_tex);

  // copy to screen
  SDL_RenderClear(Renderer());
  SDL_RenderCopy(Renderer(), window_tex, nullptr, nullptr);
  TheDisplay().PresentFrame();

  // update the event loop here too for now
  SDL_Event_Loop();
}

void PixelBuffer::UpdatePalette(const std::span<const uint8_t> palette) {
  auto* sdl_pal = static_cast<SDL_Surface*>(palette_surface_)->format->palette;
  if (palette.size() / 3 < base::ToSize(sdl_pal->ncolors)) {
    return;
  }
  // SDL owns exactly ncolors entries in the surface palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(sdl_pal->colors, base::ToSize(sdl_pal->ncolors));

  bool changed = false;

  for (int i = 0; i < sdl_pal->ncolors; i++) {
    // convert from 6-bit
    const int new_r = (base::At(palette, base::ToSize((i * 3) + 0)) * 4) +
                      (base::At(palette, base::ToSize((i * 3) + 0)) / 16);
    const int new_g = (base::At(palette, base::ToSize((i * 3) + 1)) * 4) +
                      (base::At(palette, base::ToSize((i * 3) + 1)) / 16);
    const int new_b = (base::At(palette, base::ToSize((i * 3) + 2)) * 4) +
                      (base::At(palette, base::ToSize((i * 3) + 2)) / 16);
    changed = changed ||
              std::cmp_not_equal(base::At(colors, base::ToSize(i)).r, new_r) ||
              std::cmp_not_equal(base::At(colors, base::ToSize(i)).g, new_g) ||
              std::cmp_not_equal(base::At(colors, base::ToSize(i)).b, new_b);
    base::At(colors, base::ToSize(i)).r = static_cast<Uint8>(new_r);
    base::At(colors, base::ToSize(i)).g = static_cast<Uint8>(new_g);
    base::At(colors, base::ToSize(i)).b = static_cast<Uint8>(new_b);
  }

  if (!changed) {
    return;
  }

  // make sure it gets updated
  SDL_SetPaletteColors(sdl_pal, sdl_pal->colors, 0, sdl_pal->ncolors);

  // A scaled frame holds baked colors; the next end of frame presents it.
  if (scaled_frame_texture_) {
    UploadScaledFrame();
  }

  Present(false);
}

const void* PixelBuffer::palette() const {
  return static_cast<SDL_Surface*>(palette_surface_)->format->palette;
}

void PixelBuffer::CreateDisplaySurface() {
  window_texture_ =
      SDL_CreateTexture(Renderer(), SDL_PIXELFORMAT_RGB888,
                        SDL_TEXTUREACCESS_STREAMING, width_, height_);
  palette_surface_ = SDL_CreateRGBSurface(0, width_, height_, 8, 0, 0, 0, 0);
}

void PixelBuffer::DestroyDisplaySurface() {
  // Only the window page can have armed the timer, and only it has surfaces
  // to release; ~Display cancels anything still pending.
  if (window_texture_ != nullptr && HasDisplay()) {
    TheDisplay().CancelRedrawTimer();
  }
  DropScaledFrame();
  if (window_texture_) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(window_texture_));
    window_texture_ = nullptr;
  }
  if (palette_surface_) {
    SDL_FreeSurface(static_cast<SDL_Surface*>(palette_surface_));
    palette_surface_ = nullptr;
  }
}

void PixelBuffer::PresentScaledFrame(std::span<const uint8_t> frame,
                                     const int width, const int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  const auto frame_width = base::ToSize(width);
  const auto frame_height = base::ToSize(height);
  if (frame_width > frame.size() / frame_height || !palette_surface_) {
    return;
  }
  TheDisplay().CancelRedrawTimer();

  // Create intermediate texture on first use or if size changed
  if (!scaled_frame_texture_ || scaled_frame_width_ != width ||
      scaled_frame_height_ != height) {
    if (scaled_frame_texture_) {
      SDL_DestroyTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
    }
    scaled_frame_texture_ =
        SDL_CreateTexture(Renderer(), SDL_PIXELFORMAT_RGBA32,
                          SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetTextureScaleMode(static_cast<SDL_Texture*>(scaled_frame_texture_),
                            SDL_ScaleModeBest);
    scaled_frame_width_ = width;
    scaled_frame_height_ = height;
  }

  scaled_frame_.assign(
      frame.begin(),
      frame.begin() + static_cast<std::ptrdiff_t>(frame_width * frame_height));
  if (!UploadScaledFrame()) {
    return;
  }

  // Trigger immediate present via Present
  Present(true);
}

bool PixelBuffer::UploadScaledFrame() {
  const auto frame_width = base::ToSize(scaled_frame_width_);
  const std::span<const uint8_t> frame = scaled_frame_;

  // Get the palette already set via UpdatePalette (already 8-bit RGB)
  const auto* sdl_pal =
      static_cast<SDL_Surface*>(palette_surface_)->format->palette;

  // Convert paletted pixels to RGBA and upload to intermediate texture
  void* pixels = nullptr;
  int pitch = 0;
  if (SDL_LockTexture(static_cast<SDL_Texture*>(scaled_frame_texture_), nullptr,
                      &pixels, &pitch) != 0) {
    return false;
  }
  // SDL_LockTexture exposes pitch bytes for each texture row.
  const auto dest =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(static_cast<uint32_t*>(pixels),
                base::ToSize(pitch / 4) * base::ToSize(scaled_frame_height_));
  // SDL owns exactly ncolors entries in the surface palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(sdl_pal->colors, base::ToSize(sdl_pal->ncolors));
  for (int y = 0; y < scaled_frame_height_; y++) {
    for (int x = 0; x < scaled_frame_width_; x++) {
      const uint8_t idx =
          base::At(frame, (base::ToSize(y) * frame_width) + base::ToSize(x));
      // Use palette already converted to 8-bit by UpdatePalette
      const uint8_t r = base::At(colors, idx).r;
      const uint8_t g = base::At(colors, idx).g;
      const uint8_t b = base::At(colors, idx).b;
      base::At(dest,
               (base::ToSize(y) * base::ToSize(pitch / 4)) + base::ToSize(x)) =
          0xFFU << 24U | uint32_t{b} << 16U | uint32_t{g} << 8U | r;
    }
  }
  SDL_UnlockTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
  return true;
}

void PixelBuffer::DropScaledFrame() {
  if (scaled_frame_texture_) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
    scaled_frame_texture_ = nullptr;
    scaled_frame_width_ = 0;
    scaled_frame_height_ = 0;
    scaled_frame_.clear();
  }
}

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

  // Rows are width_ apart: this draws to the whole buffer rather than to a
  // view, and Init() leaves pitch_ zero for every buffer the games allocate.
  const auto dst_buf = bytes();

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
