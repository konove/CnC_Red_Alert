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
// page its pixels, and locking the surface a page borrows them from.
//
// The primitives all clip with the Cohen-Sutherland outcodes of
// base/clip.h and then walk whole rows.

#include "engine/gfx/pixel_buffer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "base/array.h"
#include "base/clip.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/rect.h"
#include "base/types.h"
#include "engine/gfx/font.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_surface.h"

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
  // true of exactly the buffers that borrow a surface's pixels.
  return buffer_ != nullptr && buffer_->HasSurface();
}

void PixelView::PutPixel(const int x, const int y,
                         const engine::gfx::PaletteIndex color) {
  if (Lock()) {
    PutPixelLocked(x, y, color);
    Unlock();
  }
}

void PixelView::PutPixelLocked(const int x, const int y,
                               const engine::gfx::PaletteIndex color) {
  if (x >= 0 && y >= 0 && x < width() && y < height()) {
    base::At(pixels(), base::ToSize(x + (y * stride()))) = color.byte();
  }
}

engine::gfx::PaletteIndex PixelView::GetPixel(const int x, const int y) {
  engine::gfx::PaletteIndex color;
  if (Lock()) {
    color = GetPixelLocked(x, y);
    Unlock();
  }
  return color;
}

engine::gfx::PaletteIndex PixelView::GetPixelLocked(const int x, const int y) {
  if (x < 0 || y < 0 || x >= width() || y >= height()) {
    return {};
  }

  const base::ssize dst_area = stride();
  const auto dst_offset = pixels().begin() + x + (y * dst_area);

  return engine::gfx::PaletteIndex(*dst_offset);
}

void PixelView::Clear(const engine::gfx::PaletteIndex color) {
  if (Lock()) {
    ClearLocked(color);
    Unlock();
  }
}

void PixelView::ClearLocked(const engine::gfx::PaletteIndex color) {
  const uint8_t pixel = color.byte();
  const base::ssize dst_area = stride();
  auto dst_offset = pixels().begin();

  const int pixel_count = width();

  // A view with no lines, such as a page that was never attached, has nothing
  // to fill.
  for (int line = 0; line < height(); line++) {
    std::fill_n(dst_offset, pixel_count, pixel);
    dst_offset += dst_area;
  }
}

void PixelView::CopyToBuffer(const base::Rect& source,
                             const std::span<uint8_t> dest) {
  if (Lock()) {
    CopyToBufferLocked(source, dest);
    Unlock();
  }
}

void PixelView::CopyToBufferLocked(const base::Rect& source,
                                   std::span<uint8_t> dest) {
  const int width = source.width;
  int dst_x0 = 0;
  int dst_y0 = 0;

  // clip src
  int src_x0 = source.x;
  int src_y0 = source.y;
  int src_x1 = source.right();
  int src_y1 = source.bottom();

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

void PixelView::CopyFromBuffer(const base::Rect& dest,
                               const std::span<const uint8_t> source) {
  if (Lock()) {
    CopyFromBufferLocked(dest, source);
    Unlock();
  }
}

void PixelView::CopyFromBufferLocked(const base::Rect& dest,
                                     std::span<const uint8_t> source) {
  const int width = dest.width;
  int src_x0 = 0;
  int src_y0 = 0;

  // clip dest
  int dst_x0 = dest.x;
  int dst_y0 = dest.y;
  int dst_x1 = dest.right();
  int dst_y1 = dest.bottom();

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

void PixelView::BlitTo(PixelView& dest, const base::Rect& source,
                       const int dst_x, const int dst_y,
                       const bool transparent) {
  if (Lock()) {
    if (dest.Lock()) {
      BlitToLocked(dest, source, dst_x, dst_y, transparent);
      dest.Unlock();
    }
    Unlock();
  }
}

void PixelView::BlitToLocked(PixelView& dest, const base::Rect& source,
                             const int dst_x, const int dst_y,
                             const bool transparent) {
  // Only Tiberian Dawn asks for a transparent blit.

  // clip source
  const int src_x = source.x;
  const int src_y = source.y;
  int src_x0 = src_x;
  int src_y0 = src_y;
  int src_width = source.width;
  int src_height = source.height;
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
      dst_y0 = 0;
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
                      const int y, const engine::gfx::PaletteIndex fore_color,
                      const engine::gfx::PaletteIndex back_color) {
  if (Lock()) {
    PrintLocked(style, text, x, y, fore_color, back_color);
    Unlock();
  }
}

void PixelView::Print(const FontStyle& style, const int value, const int x,
                      const int y, const engine::gfx::PaletteIndex fore_color,
                      const engine::gfx::PaletteIndex back_color) {
  Print(style, absl::StrCat(value).c_str(), x, y, fore_color, back_color);
}

void PixelView::PrintLocked(const FontStyle& style, const char* text, int x,
                            int y, const engine::gfx::PaletteIndex fore_color,
                            const engine::gfx::PaletteIndex back_color) {
  const FontView& font = style.font;
  if (!text || font.data().empty()) {
    return;
  }
  // Only the bottom is checked per line below; a line starting above the top
  // would draw into whatever memory precedes the pixels.
  if (y < 0) {
    return;
  }

  const int start_x = x;
  const int max_glyph_height = font.MaxHeight();
  if (y + max_glyph_height > height_) {
    return;
  }

  // Glyph pixels are indices into the style's palette, with entry 0 the
  // background (0 also means transparent) and entry 1 the foreground.
  const uint8_t background = back_color.byte();
  std::array<uint8_t, 16> palette = style.palette;
  palette.at(1) = fore_color.byte();
  palette.at(0) = background;

  // The line checks keep every row inside the view, but a glyph can still
  // hang off either side: left of a negative x, or right of a glyph wider
  // than the room left after a wrap. Those columns are clipped.
  const std::span<uint8_t> out = pixels();
  const base::ssize buffer_stride = stride();
  const auto plot = [&](const int px, const int py, const uint8_t color) {
    if (px >= 0 && px < width_) {
      base::At(out, (py * buffer_stride) + px) = color;
    }
  };

  for (const char character : std::string_view(text)) {
    // Unsigned so characters >= 128 index the metric tables correctly.
    const auto ch = static_cast<uint8_t>(character);
    if (ch == '\0') {
      return;
    }

    const int glyph_width = font.GlyphWidth(ch);

    if (ch == '\n' || ch == '\r' ||
        x + glyph_width + style.x_spacing > width_) {
      // Advance to the next line: '\n' returns to the viewport edge, '\r'
      // and auto-wrap return to the starting column.
      const int line_height = max_glyph_height + style.y_spacing;
      if (height_ < y + max_glyph_height + line_height) {
        return;  // No room for another line.
      }

      y += line_height;
      x = ch == '\n' ? 0 : start_x;

      if (ch == '\n' || ch == '\r') {
        continue;
      }
    }

    const int glyph_x = x;
    x += glyph_width + style.x_spacing;

    const int glyph_height = font.GlyphHeight(ch);
    const int blank_rows_above = font.GlyphBlankRowsAbove(ch);
    const int glyph_top = y + blank_rows_above;
    const int glyph_bottom = glyph_top + glyph_height;

    // Fill the blank rows above the glyph unless transparent.
    if (background != 0) {
      for (int row = y; row < glyph_top; ++row) {
        for (int col = 0; col < glyph_width; ++col) {
          plot(glyph_x + col, row, background);
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
      base::ssize data_index = 0;
      for (int row = glyph_top; row < glyph_bottom; ++row) {
        for (int col = 0; col < glyph_width; col += 2) {
          const auto pixel_pair =
              std::to_integer<uint8_t>(base::At(glyph, data_index++));

          const uint8_t left = base::At(std::span(palette), pixel_pair & 0x0F);
          if (left != 0) {
            plot(glyph_x + col, row, left);
          }
          if (col + 1 < glyph_width) {
            const uint8_t right = base::At(std::span(palette), pixel_pair >> 4);
            if (right != 0) {
              plot(glyph_x + col + 1, row, right);
            }
          }
        }
      }

      // Fill the blank rows below the glyph unless transparent.
      if (background != 0) {
        for (int row = glyph_bottom; row < y + max_glyph_height; ++row) {
          for (int col = 0; col < glyph_width; ++col) {
            plot(glyph_x + col, row, background);
          }
        }
      }
    }
  }
}

void PixelView::DrawLine(const int x1, const int y1, const int x2, const int y2,
                         const engine::gfx::PaletteIndex color) {
  if (Lock()) {
    DrawLineLocked(x1, y1, x2, y2, color);
    Unlock();
  }
}

void PixelView::DrawRect(const base::Rect& rect,
                         const engine::gfx::PaletteIndex color) {
  if (!rect.IsValid() || !Lock()) {
    return;
  }
  const int x1 = rect.x;
  const int y1 = rect.y;
  const int x2 = rect.right() - 1;
  const int y2 = rect.bottom() - 1;
  DrawLineLocked(x1, y1, x2, y1, color);
  DrawLineLocked(x1, y2, x2, y2, color);
  DrawLineLocked(x1, y1, x1, y2, color);
  DrawLineLocked(x2, y1, x2, y2, color);
  Unlock();
}

void PixelView::FillRect(const base::Rect& rect,
                         const engine::gfx::PaletteIndex color) {
  if (Lock()) {
    FillRectLocked(rect, color);
    Unlock();
  }
}

void PixelView::DrawLineLocked(int x1, int y1, int x2, int y2,
                               const engine::gfx::PaletteIndex color) {
  const uint8_t pixel = color.byte();
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
    } else if (base::Any(code0 & OutCode::kRight)) {  // right
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
    } else if (base::Any(code0 & OutCode::kBelow)) {  // bottom
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
    } else if (base::Any(code1 & OutCode::kRight)) {  // right
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
    } else if (base::Any(code1 & OutCode::kBelow)) {  // bottom
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
    std::fill_n(page, count, pixel);

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
      *page = pixel;
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
      *page = pixel;
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
      *page = pixel;
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

void PixelView::FillRectLocked(const base::Rect& rect,
                               const engine::gfx::PaletteIndex color) {
  const uint8_t pixel = color.byte();
  // Clamp to the view; x2,y2 are the last column and row filled.
  const int x1 = std::max(rect.x, 0);
  const int y1 = std::max(rect.y, 0);
  const int x2 = std::min(rect.right(), width_) - 1;
  const int y2 = std::min(rect.bottom(), height_) - 1;

  // An empty rectangle, or one that misses the view.
  if (x2 < x1 || y2 < y1) {
    return;
  }

  const base::ssize dst_area = stride();
  auto dst_offset = pixels().begin() + x1 + (y1 * dst_area);

  const int pixel_count = x2 - x1 + 1;
  int line_count = y2 - y1 + 1;

  // fill lines
  do {
    std::fill_n(dst_offset, pixel_count, pixel);
    dst_offset += dst_area;
  } while (--line_count);
}

void PixelView::Remap(const base::Rect& rect,
                      const std::span<const uint8_t> remap_table) {
  if (Lock()) {
    RemapLocked(rect, remap_table);
    Unlock();
  }
}

void PixelView::RemapLocked(const base::Rect& rect,
                            const std::span<const uint8_t> remap_table) {
  if (remap_table.empty()) {
    return;
  }

  // clip
  int dst_x0 = rect.x;
  int dst_y0 = rect.y;
  int pixel_count = rect.width;
  int line_count = rect.height;
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
  Init(width, height, buffer, byte_count);
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

void PixelBuffer::Init(const int width, const int height,
                       const std::span<uint8_t> buffer,
                       const int32_t byte_count) {
  CHECK_GE(width, 0);
  CHECK_GE(height, 0);
  CHECK_GE(byte_count, 0);
  CHECK_EQ(lock_count_, 0);
  const auto pixel_count = base::ToSize(width) * base::ToSize(height);
  const auto size = byte_count == 0 ? pixel_count : base::ToSize(byte_count);
  CHECK_LE(pixel_count, buffer.empty() ? size : buffer.size());
  width_ = width;
  height_ = height;
  pitch_ = 0;
  surface_ = nullptr;

  if (buffer.empty()) {
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

void PixelBuffer::Init(const int width, const int height,
                       PixelSurface& surface) {
  CHECK_GE(width, 0);
  CHECK_GE(height, 0);
  CHECK_EQ(lock_count_, 0);
  width_ = width;
  height_ = height;
  pitch_ = 0;
  surface_ = &surface;
  // The pixels are the surface's; bytes_ points at them only between
  // LockSurface() and UnlockSurface().
  owned_pixels_.reset();
  bytes_ = {};

  whole_.Attach(this, 0, 0, width_, height_);
}

bool PixelBuffer::LockSurface() {
  if (surface_ == nullptr) {
    return true;
  }

  if (lock_count_ == 0) {
    const std::optional<PixelSurface::Pixels> pixels = surface_->Lock();
    if (!pixels.has_value()) {
      return false;
    }
    // A surface smaller than the page would let every primitive run off the
    // end of its rows; that is a mismatch between Init() and the surface.
    CHECK_GE(pixels->pitch, width_);
    CHECK_GE(pixels->bytes.size(),
             base::ToSize(pixels->pitch) * base::ToSize(height_));
    bytes_ = pixels->bytes;
    pitch_ = pixels->pitch - width_;
    whole_.Attach(this, 0, 0, width_, height_);
  }

  lock_count_++;
  return true;
}

void PixelBuffer::UnlockSurface() {
  if (surface_ == nullptr || lock_count_ == 0) {
    return;
  }

  lock_count_--;

  if (lock_count_ == 0) {
    bytes_ = {};
    // The pixels are gone until the next lock; leave no view pointing at them.
    whole_.Attach(this, 0, 0, width_, height_);
    surface_->Unlock();
  }
}
