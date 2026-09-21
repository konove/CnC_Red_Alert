#include "sdllib/drawbuff.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/types.h"
#include "sdllib/font.h"
#include "sdllib/graphic_buffer.h"
#include "sdllib/misc.h"
#include "sdllib/ww_win.h"

void* MainWindow;

PixelView* LogicPage;
bool AllowHardwareBlitFills = true;
bool OverlappedVideoBlits = true;

PixelBuffer* WindowBuffer = nullptr;

// Cohen-Sutherland outcode of (x, y) against a w by h window: bits for
// left, right, top and bottom.
static inline uint32_t Make_Code(int x, int y, int w, int h) {
  return (x < 0 ? 0b1000U : 0U) | (x >= w ? 0b0100U : 0U) |
         (y < 0 ? 0b0010U : 0U) | (y >= h ? 0b0001U : 0U);
}

int Buffer_Get_Pixel(void* thisptr, int x, int y) {
  auto* vp_dst = static_cast<PixelView*>(thisptr);

  if (x < 0 || y < 0 || x >= vp_dst->width() || y >= vp_dst->height()) {
    return 0;
  }

  const base::ssize dst_area = vp_dst->stride();
  const auto dst_offset = vp_dst->pixels().begin() + x + (y * dst_area);

  return *dst_offset;
}

void Buffer_Clear(void* thisptr, unsigned char color) {
  auto* vp_dst = static_cast<PixelView*>(thisptr);

  const base::ssize dst_area = vp_dst->stride();
  auto dst_offset = vp_dst->pixels().begin();

  const int pixel_count = vp_dst->width();
  int line_count = vp_dst->height();

  // fill lines
  do {
    std::fill_n(dst_offset, pixel_count, color);
    dst_offset += dst_area;
  } while (--line_count);
}

int32_t Buffer_To_Buffer(void* thisptr, int x_pixel, int y_pixel,
                         int pixel_width, int pixel_height,
                         std::span<uint8_t> buff, int32_t /*size*/) {
  auto* vp_src = static_cast<PixelView*>(thisptr);

  int dst_x0 = 0;
  int dst_y0 = 0;

  // clip src
  int src_x0 = x_pixel;
  int src_y0 = y_pixel;
  int src_x1 = x_pixel + pixel_width;
  int src_y1 = y_pixel + pixel_height;

  const uint32_t code0 =
      Make_Code(src_x0, src_y0, vp_src->width(), vp_src->height());
  const uint32_t code1 =
      Make_Code(src_x1, src_y1, vp_src->width() + 1, vp_src->height() + 1);

  // outside
  if (code0 & code1) {
    return 0;  // i'm not sure this actually has a return value...
  }

  if (code0 | code1) {
    // apply clip
    if (code0 & 0b1000) {
      dst_x0 -= src_x0;
      src_x0 = 0;
    }
    if (code1 & 0b0100) {
      src_x1 = vp_src->width();
    }
    if (code0 & 0b0010) {
      dst_y0 -= src_y0;
      src_y0 = 0;
    }
    if (code1 & 0b0001) {
      src_y1 = vp_src->height();
    }
  }

  const base::ssize src_area = vp_src->stride();
  auto src_offset = vp_src->pixels().begin() + src_x0 + (src_y0 * src_area);

  auto dst_offset =
      buff.begin() + dst_x0 + (static_cast<base::ssize>(dst_y0) * pixel_width);

  if (src_x1 <= src_x0 || src_y1 <= src_y0) {
    return 1;
  }

  if (std::to_address(src_offset) == std::to_address(dst_offset)) {
    return 1;
  }

  const int pixel_count = src_x1 - src_x0;
  int line_count = src_y1 - src_y0;

  // copy lines
  do {
    std::copy_n(src_offset, pixel_count, dst_offset);
    src_offset += src_area;
    dst_offset += pixel_width;
  } while (--line_count);

  return 0;
}

int32_t Buffer_To_Page(int dx_pixel, int dy_pixel, int pixel_width,
                       int pixel_height, std::span<const uint8_t> Buffer,
                       void* view) {
  auto* vp_dst = static_cast<PixelView*>(view);

  int src_x0 = 0;
  int src_y0 = 0;

  // clip dest
  int dst_x0 = dx_pixel;
  int dst_y0 = dy_pixel;
  int dst_x1 = dx_pixel + pixel_width;
  int dst_y1 = dy_pixel + pixel_height;

  const uint32_t code0 =
      Make_Code(dst_x0, dst_y0, vp_dst->width(), vp_dst->height());
  const uint32_t code1 =
      Make_Code(dst_x1, dst_y1, vp_dst->width() + 1, vp_dst->height() + 1);

  // outside
  if (code0 & code1) {
    return 0;  // i'm not sure this actually has a return value...
  }

  if (code0 | code1) {
    // apply clip
    if (code0 & 0b1000) {
      src_x0 -= dst_x0;
      dst_x0 = 0;
    }
    if (code1 & 0b0100) {
      dst_x1 = vp_dst->width();
    }
    if (code0 & 0b0010) {
      src_y0 -= dst_y0;
      dst_y0 = 0;
    }
    if (code1 & 0b0001) {
      dst_y1 = vp_dst->height();
    }
  }

  auto src_offset = Buffer.begin() + src_x0 +
                    (static_cast<base::ssize>(src_y0) * pixel_width);

  const base::ssize dst_area = vp_dst->stride();
  auto dst_offset = vp_dst->pixels().begin() + dst_x0 + (dst_y0 * dst_area);

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return 1;
  }

  if (std::to_address(src_offset) == std::to_address(dst_offset)) {
    return 1;
  }

  const int pixel_count = dst_x1 - dst_x0;
  int line_count = dst_y1 - dst_y0;

  // copy lines
  do {
    std::copy_n(src_offset, pixel_count, dst_offset);
    src_offset += pixel_width;
    dst_offset += dst_area;
  } while (--line_count);

  return 0;
}

bool Linear_Blit_To_Linear(void* thisptr, void* dest, int x_pixel, int y_pixel,
                           int dx_pixel, int dy_pixel, int pixel_width,
                           int pixel_height, bool trans) {
  // trans seems to only be used by TD

  auto* vp_src = static_cast<PixelView*>(thisptr);
  auto* vp_dst = static_cast<PixelView*>(dest);

  // clip source
  int src_x0 = x_pixel;
  int src_y0 = y_pixel;
  int src_x1 = x_pixel + pixel_width;
  int src_y1 = y_pixel + pixel_height;

  uint32_t code0 = Make_Code(src_x0, src_y0, vp_src->width(), vp_src->height());
  uint32_t code1 =
      Make_Code(src_x1, src_y1, vp_src->width() + 1, vp_src->height() + 1);

  // outside
  if (code0 & code1) {
    return true;
  }

  if (code0 | code1) {
    // apply clip
    if (code0 & 0b1000) {
      src_x0 = 0;
    }
    if (code1 & 0b0100) {
      src_x1 = vp_src->width();
    }
    if (code0 & 0b0010) {
      src_y0 = 0;
    }
    if (code1 & 0b0001) {
      src_y1 = vp_src->height();
    }
  }

  // clip dest
  // Whatever the source clip took off the top and left moves the destination
  // by as much, so the remaining pixels keep their place.
  int dst_x0 = dx_pixel + (src_x0 - x_pixel);
  int dst_y0 = dy_pixel + (src_y0 - y_pixel);
  int dst_x1 = dst_x0 + (src_x1 - src_x0);
  int dst_y1 = dst_y0 + (src_y1 - src_y0);

  code0 = Make_Code(dst_x0, dst_y0, vp_dst->width(), vp_dst->height());
  code1 = Make_Code(dst_x1, dst_y1, vp_dst->width() + 1, vp_dst->height() + 1);

  // outside
  if (code0 & code1) {
    return true;  // i'm not sure this actually has a return value...
  }

  if (code0 | code1) {
    // apply clip
    if (code0 & 0b1000) {
      src_x0 -= dst_x0;
      dst_x0 = 0;
    }
    if (code1 & 0b0100) {
      src_x1 -= dst_x1 - vp_dst->width();
      dst_x1 = vp_dst->width();
    }
    if (code0 & 0b0010) {
      src_y0 -= dst_y0;
      dst_y0 = 0;
    }
    if (code1 & 0b0001) {
      src_y1 -= dst_y1 - vp_dst->height();
      dst_y1 = vp_dst->height();
    }
  }

  const base::ssize src_area = vp_src->stride();
  auto src_offset = vp_src->pixels().begin() + src_x0 + (src_y0 * src_area);

  const base::ssize dst_area = vp_dst->stride();
  auto dst_offset = vp_dst->pixels().begin() + dst_x0 + (dst_y0 * dst_area);

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return true;
  }

  if (std::to_address(src_offset) == std::to_address(dst_offset)) {
    return true;
  }

  const int pixel_count = src_x1 - src_x0;
  int line_count = src_y1 - src_y0;

  if (src_offset < dst_offset) {
    // backward (bottom -> top)
    if (trans) {
      // copy transparent lines backwards
      src_offset += src_area * (line_count - 1);
      dst_offset += dst_area * (line_count - 1);
      do {
        for (int x = 0; x < pixel_count; x++) {
          if (base::At(vp_src->pixels(),
                       (src_offset - vp_src->pixels().begin()) + x)) {
            base::At(vp_dst->pixels(),
                     (dst_offset - vp_dst->pixels().begin()) + x) =
                base::At(vp_src->pixels(),
                         (src_offset - vp_src->pixels().begin()) + x);
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
    if (trans) {
      // copy transparent lines
      do {
        for (int x = 0; x < pixel_count; x++) {
          if (base::At(vp_src->pixels(),
                       (src_offset - vp_src->pixels().begin()) + x)) {
            base::At(vp_dst->pixels(),
                     (dst_offset - vp_dst->pixels().begin()) + x) =
                base::At(vp_src->pixels(),
                         (src_offset - vp_src->pixels().begin()) + x);
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

  return true;
}

bool Linear_Scale_To_Linear(void* thisptr, void* dest, int src_x, int src_y,
                            int dst_x, int dst_y, int src_w, int src_h,
                            int dst_w, int dst_h, bool trans,
                            std::span<const uint8_t> remap) {
  // Check for scale error when to or from size 0,0
  if (dst_w == 0 || dst_h == 0 || src_w == 0 || src_h == 0) {
    return true;
  }

  auto* vp_src = static_cast<PixelView*>(thisptr);
  auto* vp_dst = static_cast<PixelView*>(dest);

  int src_x0 = src_x;
  int src_y0 = src_y;
  int src_x1 = src_x + src_w;
  int src_y1 = src_y + src_h;

  int dst_x0 = dst_x;
  int dst_y0 = dst_y;
  int dst_x1 = dst_x + dst_w;
  int dst_y1 = dst_y + dst_h;

  // clip source
  uint32_t code0 = Make_Code(src_x0, src_y0, vp_src->width(), vp_src->height());
  uint32_t code1 =
      Make_Code(src_x1, src_y1, vp_src->width() + 1, vp_src->height() + 1);

  // outside
  if (code0 & code1) {
    return true;
  }

  if (code0 | code1) {
    // apply clip
    if (code0 & 0b1000) {
      src_x0 = 0;
      dst_x0 = dst_x + ((src_x0 - src_x) * dst_w / src_w);
    }
    if (code1 & 0b0100) {
      src_x1 = vp_src->width();
      dst_x1 = dst_x + ((src_x1 - src_x) * dst_w / src_w);
    }
    if (code0 & 0b0010) {
      src_y0 = 0;
      dst_y0 = dst_y + ((src_y0 - src_y) * dst_h / src_h);
    }
    if (code1 & 0b0001) {
      src_y1 = vp_src->height();
      dst_y1 = dst_y + ((src_y1 - src_y) * dst_h / src_h);
    }
  }

  // clip dest
  code0 = Make_Code(dst_x0, dst_y0, vp_dst->width(), vp_dst->height());
  code1 = Make_Code(dst_x1, dst_y1, vp_dst->width() + 1, vp_dst->height() + 1);

  // outside
  if (code0 & code1) {
    return true;
  }

  if (code0 | code1) {
    // apply clip
    if (code0 & 0b1000) {
      dst_x0 = 0;
      src_x0 = src_x + ((dst_x0 - dst_x) * src_w / dst_w);
    }
    if (code1 & 0b0100) {
      dst_x1 = vp_dst->width();
    }
    if (code0 & 0b0010) {
      src_y0 = src_y + ((dst_y0 - dst_y) * src_h / dst_h);
    }
    if (code1 & 0b0001) {
      dst_y1 = vp_dst->height();
    }
  }

  // do scale
  const base::ssize src_win_width = vp_src->stride();
  auto src_offset =
      vp_src->pixels().begin() + src_x0 + (src_y0 * src_win_width);

  const base::ssize dst_win_width = vp_dst->stride();
  auto dst_offset =
      vp_dst->pixels().begin() + dst_x0 + (dst_y0 * dst_win_width);

  const int dy_intr = static_cast<int>(src_h / dst_h * src_win_width);
  const int dy_frac = src_h % dst_h;
  int dy_acc = -dst_h;

  const int dx_frac = (src_w * 65536) / dst_w;

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return true;
  }

  int counter_y = dst_y1 - dst_y0;
  const int pixel_count = dst_x1 - dst_x0;

  if (trans && !remap.empty()) {
    do {
      int counter_x = pixel_count;
      int x = 0;
      auto out = dst_offset;
      do {
        const uint8_t pixel =
            base::At(vp_src->pixels(),
                     (src_offset - vp_src->pixels().begin()) + (x / 65536));

        if (pixel) {
          *out = base::At(remap, pixel);
        }

        x += dx_frac;
        out++;
      } while (--counter_x);

      src_offset += dy_intr;
      dst_offset += dst_win_width;

      dy_acc += dy_frac;
      if (dy_acc > 0) {
        src_offset += src_win_width;
        dy_acc -= dst_h;
      }
    } while (--counter_y);
  } else if (trans) {
    // normal scale with transparency
    do {
      int counter_x = pixel_count;
      int x = 0;
      auto out = dst_offset;
      do {
        const uint8_t pixel =
            base::At(vp_src->pixels(),
                     (src_offset - vp_src->pixels().begin()) + (x / 65536));

        if (pixel) {
          *out = pixel;
        }

        x += dx_frac;
        out++;
      } while (--counter_x);

      src_offset += dy_intr;
      dst_offset += dst_win_width;

      dy_acc += dy_frac;
      if (dy_acc > 0) {
        src_offset += src_win_width;
        dy_acc -= dst_h;
      }
    } while (--counter_y);
  } else if (!remap.empty()) {
    // normal scale with remap
    do {
      int counter_x = pixel_count;
      int x = 0;
      auto out = dst_offset;
      do {
        *out++ = base::At(
            remap,
            base::At(vp_src->pixels(),
                     (src_offset - vp_src->pixels().begin()) + (x / 65536)));
        x += dx_frac;
      } while (--counter_x);

      src_offset += dy_intr;
      dst_offset += dst_win_width;

      dy_acc += dy_frac;
      if (dy_acc > 0) {
        src_offset += src_win_width;
        dy_acc -= dst_h;
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
            base::At(vp_src->pixels(),
                     (src_offset - vp_src->pixels().begin()) + (x / 65536));
        x += dx_frac;
      } while (--counter_x);

      src_offset += dy_intr;
      dst_offset += dst_win_width;

      dy_acc += dy_frac;
      if (dy_acc > 0) {
        src_offset += src_win_width;
        dy_acc -= dst_h;
      }
    } while (--counter_y);
  }

  return true;
}

void Buffer_Print(void* thisptr, const char* str, int x, int y, int fcolor,
                  int bcolor) {
  if (!str || FontPtr.empty()) {
    return;
  }

  auto* viewport = static_cast<PixelView*>(thisptr);
  const FontView font(FontPtr);

  const int start_x = x;
  const int viewport_width = viewport->width();
  const int viewport_height = viewport->height();
  const base::ssize buffer_stride = viewport->stride();
  auto line_start = viewport->pixels().begin() + (buffer_stride * y);

  const int max_glyph_height = font.MaxHeight();
  y += max_glyph_height;
  if (y > viewport_height) {
    return;
  }

  // Glyph pixels are palette indices into FontPalette: entry 0 is the
  // background (0 also means transparent) and entry 1 the foreground;
  // multi-colour fonts fill entries 2-15 via Set_Font_Palette_Range().
  const auto background = static_cast<uint8_t>(bcolor);
  FontPalette[1] = static_cast<uint8_t>(fcolor);
  FontPalette[0] = background;

  auto next_glyph_start = line_start + x;

  for (const char character : std::string_view(str)) {
    // Unsigned so characters >= 128 index the metric tables correctly.
    const auto ch = static_cast<uint8_t>(character);
    if (ch == '\0') {
      return;
    }

    auto draw_ptr = next_glyph_start;
    const int glyph_width = font.GlyphWidth(ch);

    if (ch == '\n' || ch == '\r' ||
        x + glyph_width + FontXSpacing > viewport_width) {
      // Advance to the next line: '\n' returns to the viewport edge, '\r'
      // and auto-wrap return to the starting column.
      const int line_height = max_glyph_height + FontYSpacing;
      if (viewport_height < y + line_height) {
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

    x += glyph_width + FontXSpacing;
    next_glyph_start = draw_ptr + FontXSpacing + glyph_width;

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
      // case FontPalette[0] already paints it.
      const auto glyph = font.GlyphData(ch);
      if (glyph.empty()) {
        return;
      }
      auto glyph_data = glyph.begin();
      for (int row = 0; row < glyph_height; ++row) {
        int cols_left = glyph_width;
        while (cols_left > 0) {
          const auto pixel_pair = std::to_integer<uint8_t>(*glyph_data++);

          const uint8_t left = base::At(FontPalette, pixel_pair & 0x0F);
          if (left != 0) {
            *draw_ptr = left;
          }
          ++draw_ptr;
          --cols_left;

          if (cols_left > 0) {
            const uint8_t right = base::At(FontPalette, pixel_pair >> 4);
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

void Buffer_Draw_Line(void* thisptr, int sx, int sy, int dx, int dy,
                      unsigned char color) {
  auto* vp_dst = static_cast<PixelView*>(thisptr);

  const int width = vp_dst->width();
  const int height = vp_dst->height();

  // this is different to the original asm, but reused from blits
  const uint32_t code0 = Make_Code(sx, sy, width, height);
  const uint32_t code1 = Make_Code(dx, dy, width, height);

  if (code0 & code1) {
    return;
  }

  if (code0) {
    if (code0 & 0b1000)  // left
    {
      if (dx != sx) {
        sy += -sx * (dy - sy) / (dx - sx);
      }
      sx = 0;
    } else if (code0 & 0b0100)  // right
    {
      if (dx != sx) {
        sy += (width - 1 - sx) * (dy - sy) / (dx - sx);
      }
      sx = width - 1;
    }

    if (code0 & 0b0010)  // top
    {
      if (dy != sy) {
        sx = sx + (-sy * (dx - sx) / (dy - sy));
      }
      sy = 0;
    } else if (code0 & 0b0001)  // bottom
    {
      if (dy != sy) {
        sx = sx + ((height - 1 - sy) * (dx - sx) / (dy - sy));
      }
      sy = height - 1;
    }
  }

  if (code1) {
    if (code1 & 0b1000)  // left
    {
      if (sx != dx) {
        dy = dy + (-dx * (sy - dy) / (sx - dx));
      }
      dx = 0;
    } else if (code1 & 0b0100)  // right
    {
      if (sx != dx) {
        dy = dy + ((width - 1 - dx) * (sy - dy) / (sx - dx));
      }
      dx = width - 1;
    }

    if (code1 & 0b0010)  // top
    {
      if (sy != dy) {
        dx = dx + (-dy * (sx - dx) / (sy - dy));
      }
      dy = 0;
    } else if (code1 & 0b0001)  // bottom
    {
      if (sy != dy) {
        dx = dx + ((height - 1 - dy) * (sx - dx) / (sy - dy));
      }
      dy = height - 1;
    }
  }

  const base::ssize bpr = vp_dst->stride();

  int y_dist = dy - sy;

  if (y_dist == 0) {
    // horizontal
    if (dx < sx) {
      std::swap(dx, sx);
    }

    const int count = dx - sx + 1;
    const auto ptr = vp_dst->pixels().begin() + sx + (bpr * sy);
    std::fill_n(ptr, count, color);

    return;
  }

  // not horizontal
  if (y_dist == 0 || dy < sy) {
    sy = sy + y_dist;
    y_dist = -y_dist;

    std::swap(dx, sx);
  }

  auto ptr = vp_dst->pixels().begin() + sx + (bpr * sy);

  int step = 1;
  int x_dist = dx - sx;

  if (x_dist == 0) {
    // vertical
    int count = y_dist + 1;
    do {
      *ptr = color;
      ptr = ptr + bpr;
    } while (--count);
    return;
  }

  // not vertical
  if (x_dist == 0 || dx < sx) {
    x_dist = -x_dist;
    step = -1;
  }

  if (x_dist < y_dist) {
    int count = y_dist;
    int accum = y_dist / 2;
    while (true) {
      *ptr = color;
      if (--count == 0) {
        break;
      }
      ptr += bpr;

      accum -= x_dist;
      if (accum < 0) {
        accum += y_dist;
        ptr += step;
      }
    }
  } else {
    int count = x_dist;
    int accum = x_dist / 2;
    while (true) {
      *ptr = color;
      if (--count == 0) {
        break;
      }
      ptr = ptr + step;

      accum -= y_dist;
      if (accum < 0) {
        accum += x_dist;
        ptr += bpr;
      }
    }
  }
}

void Buffer_Fill_Rect(void* thisptr, int sx, int sy, int dx, int dy,
                      unsigned char color) {
  auto* vp_dst = static_cast<PixelView*>(thisptr);

  if (sx > dx) {
    std::swap(sx, dx);
  }
  if (sy > dy) {
    std::swap(sy, dy);
  }

  // clamp to bounds
  sx = std::max(sx, 0);
  sy = std::max(sy, 0);

  if (dx >= vp_dst->width()) {
    dx = vp_dst->width() - 1;
  }
  if (dy >= vp_dst->height()) {
    dy = vp_dst->height() - 1;
  }

  // nothing to fill
  if (dx < sx || dy < sy) {
    return;
  }

  const base::ssize dst_area = vp_dst->stride();
  auto dst_offset = vp_dst->pixels().begin() + sx + (sy * dst_area);

  const int pixel_count = dx - sx + 1;
  int line_count = dy - sy + 1;

  // fill lines
  do {
    std::fill_n(dst_offset, pixel_count, color);
    dst_offset += dst_area;
  } while (--line_count);
}

void Buffer_Remap(void* thisptr, int sx, int sy, int width, int height,
                  std::span<const uint8_t> remap) {
  if (remap.empty()) {
    return;
  }

  auto* vp_dst = static_cast<PixelView*>(thisptr);

  // clip
  int dst_x0 = sx;
  int dst_y0 = sy;
  int dst_x1 = sx + width;
  int dst_y1 = sy + height;

  const uint32_t code0 =
      Make_Code(dst_x0, dst_y0, vp_dst->width(), vp_dst->height());
  const uint32_t code1 =
      Make_Code(dst_x1, dst_y1, vp_dst->width() + 1, vp_dst->height() + 1);

  // outside
  if (code0 & code1) {
    return;
  }

  if (code0 | code1) {
    // apply clip
    if (code0 & 0b1000) {
      dst_x0 = 0;
    }
    if (code1 & 0b0100) {
      dst_x1 = vp_dst->width();
    }
    if (code0 & 0b0010) {
      dst_y0 = 0;
    }
    if (code1 & 0b0001) {
      dst_y1 = vp_dst->height();
    }
  }

  const base::ssize dst_area = vp_dst->stride();
  auto dst_offset = vp_dst->pixels().begin() + dst_x0 + (dst_y0 * dst_area);

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return;
  }

  const int pixel_count = dst_x1 - dst_x0;
  int line_count = dst_y1 - dst_y0;

  const int skip = static_cast<int>(dst_area - pixel_count);

  // remap lines
  do {
    for (int x = 0; x < pixel_count; x++) {
      const auto v = base::At(remap, *dst_offset);
      *dst_offset++ = v;
    }
    dst_offset += skip;
  } while (--line_count);
}

// from misc.h, implemented here to share clipping helpers
int Clip_Rect(int* x, int* y, int* dw, int* dh, int width, int height) {
  int x0 = *x;
  int y0 = *y;
  int x1 = *x + *dw;
  int y1 = *y + *dh;

  const uint32_t code0 = Make_Code(x0, y0, width, height);
  const uint32_t code1 = Make_Code(x1, y1, width + 1, height + 1);

  // outside
  if (code0 & code1) {
    return -1;
  }

  if (code0 | code1) {
    // apply clip
    if (code0 & 0b1000) {
      x0 = 0;
    }
    if (code1 & 0b0100) {
      x1 = width;
    }
    if (code0 & 0b0010) {
      y0 = 0;
    }
    if (code1 & 0b0001) {
      y1 = height;
    }

    *x = x0;
    *y = y0;
    *dw = x1 - x0;
    *dh = y1 - y0;
    return 1;
  }

  return 0;
}

PixelView::~PixelView() {
  if (LogicPage == this) {
    LogicPage = nullptr;
  }
}

PixelView* SetLogicPage(PixelView* ptr) {
  std::swap(LogicPage, ptr);
  return ptr;
}

PixelView* SetLogicPage(PixelView& ptr) { return SetLogicPage(&ptr); }

PixelView::PixelView(PixelBuffer* graphic_buff, int x, int y, int w, int h) {
  Attach(graphic_buff, x, y, w, h);
}

void PixelView::DrawRect(int sx, int sy, int dx, int dy, uint8_t color) {
  Lock();
  DrawLine(sx, sy, dx, sy, color);
  DrawLine(sx, dy, dx, dy, color);
  DrawLine(sx, sy, sx, dy, color);
  DrawLine(dx, sy, dx, dy, color);
  Unlock();
}

void PixelView::Attach(PixelBuffer* graphic_buff, int x, int y, int w, int h) {
  if (this == buffer()) {
    return;
  }

  // Clamp the corner into the buffer. A buffer that Init() has not sized yet
  // has no last pixel to clamp to, so the corner stays at the origin and the
  // width and height below come out zero; Screen builds its views against
  // such pages and attaches them again once the video mode is known.
  x = std::clamp(x, 0, std::max(graphic_buff->width() - 1, 0));
  y = std::clamp(y, 0, std::max(graphic_buff->height() - 1, 0));

  if (x + w > graphic_buff->width()) {
    w = graphic_buff->width() - x;
  }

  if (y + h > graphic_buff->height()) {
    h = graphic_buff->height() - y;
  }

  /*======================================================================*/
  /* Get a pointer to the top left edge of the buffer.
   */
  /*======================================================================*/
  offset_ = graphic_buff->Get_Bytes().empty()
                ? nullptr
                : graphic_buff->Get_Bytes()
                      .subspan(base::ToSize(
                          (static_cast<base::ssize>(graphic_buff->width() +
                                                    graphic_buff->pitch()) *
                           y) +
                          x))
                      .data();

  /*======================================================================*/
  /* Copy over all of the variables that we need to store.
   */
  /*======================================================================*/
  x_pos_ = x;
  y_pos_ = y;
  x_add_ = graphic_buff->width() - w;
  width_ = w;
  height_ = h;
  pitch_ = graphic_buff->pitch();
  buffer_ = graphic_buff;
}

PixelBuffer::PixelBuffer(int w, int h, std::span<uint8_t> buffer, int32_t size)
    : PixelBuffer() {
  Init(w, h, buffer, size, GBC_NONE);
}

PixelBuffer::PixelBuffer(int w, int h, std::span<uint8_t> buffer)
    : PixelBuffer(w, h, buffer, w * h) {}

PixelBuffer::PixelBuffer() { buffer_ = this; }

PixelBuffer::~PixelBuffer() {
  ReleaseSurfaces();
  if (WindowBuffer == this) {
    WindowBuffer = nullptr;
  }
}

void PixelBuffer::Init(int w, int h, std::span<uint8_t> buffer, int32_t size,
                       PixelBufferFlags flags) {
  CHECK_GE(w, 0);
  CHECK_GE(h, 0);
  CHECK_GE(size, 0);
  const auto pixel_count = base::ToSize(w) * base::ToSize(h);
  if (!base::Any(flags & GBC_VISIBLE)) {
    CHECK_LE(pixel_count, buffer.empty()
                              ? (size == 0 ? pixel_count : base::ToSize(size))
                              : buffer.size());
  }
  Size = size;
  width_ = w;
  height_ = h;
  pitch_ = 0;
  x_add_ = 0;
  x_pos_ = y_pos_ = 0;

  if (base::Any(flags & GBC_VISIBLE)) {
    CreateDisplaySurface();

    WindowBuffer = this;
  } else {
    // regular allocation
    Allocated = buffer.empty();
    bytes_ = buffer;
    Buffer = buffer.data();

    if (buffer.empty()) {
      if (size == 0) {
        Size = w * h;
      } else {
        Size = size;
      }
      Buffer = new uint8_t[base::ToSize(Size)];
      // This allocation contains exactly Size bytes.
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      bytes_ = std::span(static_cast<uint8_t*>(Buffer), base::ToSize(Size));
    }

    offset_ = static_cast<uint8_t*>(Buffer);
  }
}

void PixelBuffer::ReleaseSurfaces() { DestroyDisplaySurface(); }

void Video_End_Frame() {
  if (WindowBuffer) {
    WindowBuffer->Present(true);
  }
}
