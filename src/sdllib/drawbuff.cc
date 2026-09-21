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
#include "sdllib/misc.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_win.h"

void* MainWindow;

PixelView* LogicPage;
bool AllowHardwareBlitFills = true;
bool OverlappedVideoBlits = true;

PixelBuffer* WindowBuffer = nullptr;

// Cohen-Sutherland outcode of (x, y) against a width by height window: bits for
// left, right, top and bottom.
static inline uint32_t Make_Code(int x, int y, int width, int height) {
  return (x < 0 ? 0b1000U : 0U) | (x >= width ? 0b0100U : 0U) |
         (y < 0 ? 0b0010U : 0U) | (y >= height ? 0b0001U : 0U);
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

int32_t Buffer_To_Buffer(void* thisptr, int x, int y, int width, int height,
                         std::span<uint8_t> dest, int32_t /*size*/) {
  auto* vp_src = static_cast<PixelView*>(thisptr);

  int dst_x0 = 0;
  int dst_y0 = 0;

  // clip src
  int src_x0 = x;
  int src_y0 = y;
  int src_x1 = x + width;
  int src_y1 = y + height;

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
      dest.begin() + dst_x0 + (static_cast<base::ssize>(dst_y0) * width);

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
    dst_offset += width;
  } while (--line_count);

  return 0;
}

int32_t Buffer_To_Page(int dst_x, int dst_y, int width, int height,
                       std::span<const uint8_t> source, void* view) {
  auto* vp_dst = static_cast<PixelView*>(view);

  int src_x0 = 0;
  int src_y0 = 0;

  // clip dest
  int dst_x0 = dst_x;
  int dst_y0 = dst_y;
  int dst_x1 = dst_x + width;
  int dst_y1 = dst_y + height;

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

  auto src_offset =
      source.begin() + src_x0 + (static_cast<base::ssize>(src_y0) * width);

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
    src_offset += width;
    dst_offset += dst_area;
  } while (--line_count);

  return 0;
}

bool Linear_Blit_To_Linear(void* thisptr, void* dest, int src_x, int src_y,
                           int dst_x, int dst_y, int width, int height,
                           bool transparent) {
  // Only Tiberian Dawn asks for a transparent blit.

  auto* vp_src = static_cast<PixelView*>(thisptr);
  auto* vp_dst = static_cast<PixelView*>(dest);

  // clip source
  int src_x0 = src_x;
  int src_y0 = src_y;
  int src_x1 = src_x + width;
  int src_y1 = src_y + height;

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
  int dst_x0 = dst_x + (src_x0 - src_x);
  int dst_y0 = dst_y + (src_y0 - src_y);
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
    if (transparent) {
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
    if (transparent) {
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
                            int dst_x, int dst_y, int src_width, int src_height,
                            int dst_width, int dst_height, bool transparent,
                            std::span<const uint8_t> remap_table) {
  // Check for scale error when to or from size 0,0
  if (dst_width == 0 || dst_height == 0 || src_width == 0 || src_height == 0) {
    return true;
  }

  auto* vp_src = static_cast<PixelView*>(thisptr);
  auto* vp_dst = static_cast<PixelView*>(dest);

  int src_x0 = src_x;
  int src_y0 = src_y;
  int src_x1 = src_x + src_width;
  int src_y1 = src_y + src_height;

  int dst_x0 = dst_x;
  int dst_y0 = dst_y;
  int dst_x1 = dst_x + dst_width;
  int dst_y1 = dst_y + dst_height;

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
      dst_x0 = dst_x + ((src_x0 - src_x) * dst_width / src_width);
    }
    if (code1 & 0b0100) {
      src_x1 = vp_src->width();
      dst_x1 = dst_x + ((src_x1 - src_x) * dst_width / src_width);
    }
    if (code0 & 0b0010) {
      src_y0 = 0;
      dst_y0 = dst_y + ((src_y0 - src_y) * dst_height / src_height);
    }
    if (code1 & 0b0001) {
      src_y1 = vp_src->height();
      dst_y1 = dst_y + ((src_y1 - src_y) * dst_height / src_height);
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
      src_x0 = src_x + ((dst_x0 - dst_x) * src_width / dst_width);
    }
    if (code1 & 0b0100) {
      dst_x1 = vp_dst->width();
    }
    if (code0 & 0b0010) {
      src_y0 = src_y + ((dst_y0 - dst_y) * src_height / dst_height);
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

  const int dy_intr = static_cast<int>(src_height / dst_height * src_win_width);
  const int dy_frac = src_height % dst_height;
  int dy_acc = -dst_height;

  const int dx_frac = (src_width * 65536) / dst_width;

  if (dst_x1 <= dst_x0 || dst_y1 <= dst_y0) {
    return true;
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
            base::At(vp_src->pixels(),
                     (src_offset - vp_src->pixels().begin()) + (x / 65536));

        if (pixel) {
          *out = base::At(remap_table, pixel);
        }

        x += dx_frac;
        out++;
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
            base::At(vp_src->pixels(),
                     (src_offset - vp_src->pixels().begin()) + (x / 65536)));
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
            base::At(vp_src->pixels(),
                     (src_offset - vp_src->pixels().begin()) + (x / 65536));
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

  return true;
}

void Buffer_Print(void* thisptr, const char* text, int x, int y, int fore_color,
                  int back_color) {
  if (!text || FontPtr.empty()) {
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
  const auto background = static_cast<uint8_t>(back_color);
  FontPalette[1] = static_cast<uint8_t>(fore_color);
  FontPalette[0] = background;

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

void Buffer_Draw_Line(void* thisptr, int x1, int y1, int x2, int y2,
                      unsigned char color) {
  auto* vp_dst = static_cast<PixelView*>(thisptr);

  const int width = vp_dst->width();
  const int height = vp_dst->height();

  // this is different to the original asm, but reused from blits
  const uint32_t code0 = Make_Code(x1, y1, width, height);
  const uint32_t code1 = Make_Code(x2, y2, width, height);

  if (code0 & code1) {
    return;
  }

  if (code0) {
    if (code0 & 0b1000)  // left
    {
      if (x2 != x1) {
        y1 += -x1 * (y2 - y1) / (x2 - x1);
      }
      x1 = 0;
    } else if (code0 & 0b0100)  // right
    {
      if (x2 != x1) {
        y1 += (width - 1 - x1) * (y2 - y1) / (x2 - x1);
      }
      x1 = width - 1;
    }

    if (code0 & 0b0010)  // top
    {
      if (y2 != y1) {
        x1 = x1 + (-y1 * (x2 - x1) / (y2 - y1));
      }
      y1 = 0;
    } else if (code0 & 0b0001)  // bottom
    {
      if (y2 != y1) {
        x1 = x1 + ((height - 1 - y1) * (x2 - x1) / (y2 - y1));
      }
      y1 = height - 1;
    }
  }

  if (code1) {
    if (code1 & 0b1000)  // left
    {
      if (x1 != x2) {
        y2 = y2 + (-x2 * (y1 - y2) / (x1 - x2));
      }
      x2 = 0;
    } else if (code1 & 0b0100)  // right
    {
      if (x1 != x2) {
        y2 = y2 + ((width - 1 - x2) * (y1 - y2) / (x1 - x2));
      }
      x2 = width - 1;
    }

    if (code1 & 0b0010)  // top
    {
      if (y1 != y2) {
        x2 = x2 + (-y2 * (x1 - x2) / (y1 - y2));
      }
      y2 = 0;
    } else if (code1 & 0b0001)  // bottom
    {
      if (y1 != y2) {
        x2 = x2 + ((height - 1 - y2) * (x1 - x2) / (y1 - y2));
      }
      y2 = height - 1;
    }
  }

  const base::ssize bpr = vp_dst->stride();

  int y_dist = y2 - y1;

  if (y_dist == 0) {
    // horizontal
    if (x2 < x1) {
      std::swap(x2, x1);
    }

    const int count = x2 - x1 + 1;
    const auto page = vp_dst->pixels().begin() + x1 + (bpr * y1);
    std::fill_n(page, count, color);

    return;
  }

  // not horizontal
  if (y_dist == 0 || y2 < y1) {
    y1 = y1 + y_dist;
    y_dist = -y_dist;

    std::swap(x2, x1);
  }

  auto page = vp_dst->pixels().begin() + x1 + (bpr * y1);

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

void Buffer_Fill_Rect(void* thisptr, int x1, int y1, int x2, int y2,
                      unsigned char color) {
  auto* vp_dst = static_cast<PixelView*>(thisptr);

  if (x1 > x2) {
    std::swap(x1, x2);
  }
  if (y1 > y2) {
    std::swap(y1, y2);
  }

  // clamp to bounds
  x1 = std::max(x1, 0);
  y1 = std::max(y1, 0);

  if (x2 >= vp_dst->width()) {
    x2 = vp_dst->width() - 1;
  }
  if (y2 >= vp_dst->height()) {
    y2 = vp_dst->height() - 1;
  }

  // nothing to fill
  if (x2 < x1 || y2 < y1) {
    return;
  }

  const base::ssize dst_area = vp_dst->stride();
  auto dst_offset = vp_dst->pixels().begin() + x1 + (y1 * dst_area);

  const int pixel_count = x2 - x1 + 1;
  int line_count = y2 - y1 + 1;

  // fill lines
  do {
    std::fill_n(dst_offset, pixel_count, color);
    dst_offset += dst_area;
  } while (--line_count);
}

void Buffer_Remap(void* thisptr, int x1, int y1, int width, int height,
                  std::span<const uint8_t> remap_table) {
  if (remap_table.empty()) {
    return;
  }

  auto* vp_dst = static_cast<PixelView*>(thisptr);

  // clip
  int dst_x0 = x1;
  int dst_y0 = y1;
  int dst_x1 = x1 + width;
  int dst_y1 = y1 + height;

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

  // Remap one row at a time.
  do {
    for (int x = 0; x < pixel_count; x++) {
      const auto v = base::At(remap_table, *dst_offset);
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

PixelView* SetLogicPage(PixelView* page) {
  std::swap(LogicPage, page);
  return page;
}

PixelView* SetLogicPage(PixelView& page) { return SetLogicPage(&page); }

PixelView::PixelView(PixelBuffer* buffer, int x, int y, int width, int height) {
  Attach(buffer, x, y, width, height);
}

void PixelView::DrawRect(int x1, int y1, int x2, int y2, uint8_t color) {
  Lock();
  DrawLine(x1, y1, x2, y1, color);
  DrawLine(x1, y2, x2, y2, color);
  DrawLine(x1, y1, x1, y2, color);
  DrawLine(x2, y1, x2, y2, color);
  Unlock();
}

void PixelView::Attach(PixelBuffer* buffer, int x, int y, int width,
                       int height) {
  if (this == buffer_) {
    return;
  }

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
      buffer->Get_Bytes().empty()
          ? nullptr
          : buffer->Get_Bytes()
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

PixelBuffer::PixelBuffer(int width, int height, std::span<uint8_t> buffer,
                         int32_t byte_count)
    : PixelBuffer() {
  Init(width, height, buffer, byte_count, BUFFER_NONE);
}

PixelBuffer::PixelBuffer(int width, int height, std::span<uint8_t> buffer)
    : PixelBuffer(width, height, buffer, width * height) {}

PixelBuffer::PixelBuffer() { buffer_ = this; }

PixelBuffer::~PixelBuffer() {
  ReleaseSurfaces();
  if (WindowBuffer == this) {
    WindowBuffer = nullptr;
  }
}

void PixelBuffer::Init(int width, int height, std::span<uint8_t> buffer,
                       int32_t byte_count, PixelBufferFlags flags) {
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
  Size = byte_count;
  width_ = width;
  height_ = height;
  pitch_ = 0;
  x_add_ = 0;
  x_pos_ = y_pos_ = 0;

  if (base::Any(flags & BUFFER_VISIBLE)) {
    CreateDisplaySurface();

    WindowBuffer = this;
  } else {
    // regular allocation
    Allocated = buffer.empty();
    bytes_ = buffer;
    Buffer = buffer.data();

    if (buffer.empty()) {
      if (byte_count == 0) {
        Size = width * height;
      } else {
        Size = byte_count;
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
