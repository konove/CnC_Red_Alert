#include "sdllib/font.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <set>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/log.h"

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

void CheckFontStyle(const FontStyle& style, const FontUse use,
                    const std::source_location location) {
  const FontStyle current = CurrentFontStyle();
  const bool same_font = style.font.data().data() == current.font.data().data();
  const bool same_x = style.x_spacing == current.x_spacing;
  const bool same_y = style.y_spacing == current.y_spacing;
  const bool same_palette =
      std::ranges::equal(std::span(style.palette).subspan(2),
                         std::span(current.palette).subspan(2));
  const bool matches =
      same_font && (use != FontUse::kWidth || same_x) &&
      (use != FontUse::kHeight || same_y) &&
      (use != FontUse::kPrint || (same_x && same_y && same_palette));
  if (matches) {
    return;
  }

  // Reported once per site: a mismatch inside a drawing loop would otherwise
  // repeat every frame.
  static std::set<std::pair<std::string, uint32_t>> reported;
  if (!reported.emplace(location.file_name(), location.line()).second) {
    return;
  }
  LOG(ERROR) << "Font style at " << location.file_name() << ":"
             << location.line() << " differs from the font globals: font "
             << style.font.data().size() << " bytes (globals "
             << current.font.data().size() << "), x spacing " << style.x_spacing
             << " (" << current.x_spacing << "), y spacing " << style.y_spacing
             << " (" << current.y_spacing << "), palette "
             << (same_palette ? "same" : "different");
}

int FontMaxHeight(const FontStyle& style, const std::source_location location) {
  CheckFontStyle(style, FontUse::kGlyphs, location);
  return style.font.MaxHeight();
}

int FontMaxWidth(const FontStyle& style, const std::source_location location) {
  CheckFontStyle(style, FontUse::kGlyphs, location);
  return style.font.MaxWidth();
}

int FontLineHeight(const FontStyle& style,
                   const std::source_location location) {
  CheckFontStyle(style, FontUse::kHeight, location);
  return style.font.MaxHeight() + style.y_spacing;
}

namespace {

// Returns how far printing character in style moves the pen.
int Advance(const FontStyle& style, const char character) {
  return style.font.GlyphWidth(static_cast<uint8_t>(character)) +
         style.x_spacing;
}

}  // namespace

int CharPixelWidth(const FontStyle& style, const char character,
                   const std::source_location location) {
  CheckFontStyle(style, FontUse::kWidth, location);
  return Advance(style, character);
}

int StringPixelWidth(const FontStyle& style, const char* text,
                     const std::source_location location) {
  CheckFontStyle(style, FontUse::kWidth, location);
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
