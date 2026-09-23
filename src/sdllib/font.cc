#include "sdllib/font.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>

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

namespace {

// Returns how far printing character in font moves the pen.
int Advance(const FontView& font, const char character) {
  return font.GlyphWidth(static_cast<uint8_t>(character)) + g_font_x_spacing;
}

}  // namespace

int CharPixelWidth(const char character) {
  return Advance(FontView(g_font), character);
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
      line_width += Advance(font, character);
    }
  }
  return std::max(widest_line, line_width);
}

void SetFontPalette(std::span<const uint8_t> palette) {
  if (std::ssize(palette) < std::ssize(g_font_palette)) {
    return;
  }
  std::ranges::copy(palette.first(std::size(g_font_palette)),
                    std::begin(g_font_palette));
}
