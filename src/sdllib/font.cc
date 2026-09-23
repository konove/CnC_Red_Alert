#include "sdllib/font.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "base/array.h"

int g_font_x_spacing;
int g_font_y_spacing;
int g_font_max_width;
int g_font_max_height;
std::span<const std::byte> g_font;

uint8_t g_font_palette[16]{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};

std::span<const std::byte> SetFont(std::span<const std::byte> font) {
  const auto previous_font = g_font;

  if (!font.empty()) {
    g_font = font;

    // Cached for the dialog and menu layout code, which reads the metrics
    // without a FontView.
    const FontView view(font);
    g_font_max_height = view.MaxHeight();
    g_font_max_width = view.MaxWidth();
  }

  return previous_font;
}

int CharPixelWidth(const char character) {
  return FontView(g_font).GlyphWidth(static_cast<uint8_t>(character)) +
         g_font_x_spacing;
}

int StringPixelWidth(const char* text) {
  if (!text) {
    return 0;
  }

  const FontView font(g_font);
  int widest_line = 0;
  int line_width = 0;  // Width of the line measured so far.
  for (const char character : std::string_view(text)) {
    // '\r' is the game's line break; see the declaration.
    if (character == '\r') {
      widest_line = std::max(widest_line, line_width);
      line_width = 0;
    } else {
      line_width +=
          font.GlyphWidth(static_cast<uint8_t>(character)) + g_font_x_spacing;
    }
  }
  return std::max(widest_line, line_width);
}

void Set_Font_Palette_Range(std::span<const uint8_t> palette, int start_idx,
                            int end_idx) {
  auto palette8 = palette.begin();

  // Wrap into the table like the original assembly, which masked both with
  // 0x0F; unlike the mask, % leaves a negative index negative, and the check
  // below rejects it.
  start_idx %= 16;
  end_idx %= 16;

  if (start_idx < 0 || end_idx < start_idx ||
      end_idx - start_idx + 1 > std::ssize(palette)) {
    return;
  }
  for (int i = start_idx; i <= end_idx; ++i) {
    base::At(g_font_palette, i) = *palette8++;
  }
}

void* Get_Font_Palette_Ptr() { return g_font_palette; }
std::span<const uint8_t> Get_Font_Palette() { return g_font_palette; }
