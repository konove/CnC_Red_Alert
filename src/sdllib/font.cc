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

int FontMaxHeight(const FontStyle& style) { return style.font.MaxHeight(); }

int FontMaxWidth(const FontStyle& style) { return style.font.MaxWidth(); }

int FontLineHeight(const FontStyle& style) {
  return style.font.MaxHeight() + style.y_spacing;
}

namespace {

// Returns how far printing character in style moves the pen.
int Advance(const FontStyle& style, const char character) {
  return style.font.GlyphWidth(static_cast<uint8_t>(character)) +
         style.x_spacing;
}

}  // namespace

int CharPixelWidth(const FontStyle& style, const char character) {
  return Advance(style, character);
}

int StringPixelWidth(const FontStyle& style, const char* text) {
  if (!text) {
    return 0;
  }

  int widest_line = 0;
  int line_width = 0;  // Width of the line measured so far.
  for (const char character : std::string_view(text)) {
    // '\r' is the game's line break; see the declaration.
    if (character == '\r') {
      widest_line = std::max(widest_line, line_width);
      line_width = 0;
    } else {
      line_width += Advance(style, character);
    }
  }
  return std::max(widest_line, line_width);
}

FontStyle CurrentFontStyle() {
  FontStyle style{.font = FontView(g_font),
                  .x_spacing = g_font_x_spacing,
                  .y_spacing = g_font_y_spacing};
  std::ranges::copy(g_font_palette, style.palette.begin());
  return style;
}

int CharPixelWidth(const char character) {
  return CharPixelWidth(CurrentFontStyle(), character);
}

int StringPixelWidth(const char* text) {
  return StringPixelWidth(CurrentFontStyle(), text);
}

void SetFontPalette(std::span<const uint8_t> palette) {
  if (std::ssize(palette) < std::ssize(g_font_palette)) {
    return;
  }
  std::ranges::copy(palette.first(std::size(g_font_palette)),
                    std::begin(g_font_palette));
}
