#include "engine/gfx/font.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

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
