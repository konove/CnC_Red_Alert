#include "sdllib/font.h"

#include <algorithm>
#include <cstdint>

int FontXSpacing;
int FontYSpacing;
char FontWidth;
char FontHeight;
const void* FontPtr;

// Maps 4-bit glyph pixel values to screen colors. Defaults to the identity
// mapping; Buffer_Print installs the fore/background per call and
// Set_Font_Palette_Range() installs multi-colour font palettes.
uint8_t FontPalette[16]{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};

const void* Set_Font(const void* new_font) {
  const void* old_font = FontPtr;

  if (new_font) {
    FontPtr = new_font;

    // Refresh the font metric globals the rest of the system reads.
    const FontView font(new_font);
    FontHeight = static_cast<char>(font.MaxHeight());
    FontWidth = static_cast<char>(font.MaxWidth());
  }

  return old_font;
}

int Char_Pixel_Width(const char chr) {
  return FontView(FontPtr).GlyphWidth(static_cast<uint8_t>(chr)) + FontXSpacing;
}

unsigned int String_Pixel_Width(const char* string) {
  if (!string) {
    return 0;
  }

  const FontView font(FontPtr);
  int largest = 0;  // Largest recorded line width of the string.
  int width = 0;    // Working accumulator of the current line's width.
  while (*string) {
    if (*string == '\r') {
      string++;
      largest = std::max(largest, width);
      width = 0;
    } else {
      width += font.GlyphWidth(static_cast<uint8_t>(*string++)) + FontXSpacing;
    }
  }
  return std::max(largest, width);
}

void Set_Font_Palette_Range(const void* palette, int start_idx, int end_idx) {
  const auto* palette8 = static_cast<const uint8_t*>(palette);

  start_idx &= 0xF;
  end_idx &= 0xF;

  for (int i = start_idx; i <= end_idx; ++i) {
    FontPalette[i] = *palette8++;
  }
}

extern "C" void* Get_Font_Palette_Ptr() { return FontPalette; }
