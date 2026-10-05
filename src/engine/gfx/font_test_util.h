// File: MakeBlockFont(), a font made in memory for tests that draw text
// without the game's font files.

#ifndef CNC_RED_ALERT_ENGINE_GFX_FONT_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_GFX_FONT_TEST_UTIL_H_

#include <cstddef>
#include <vector>

// Returns a complete font file, in the format FontView reads, whose 256
// glyphs are all the same solid `width` x `height` block of glyph palette
// index 1. Text set in it measures and draws predictably: a string of n
// characters is n * `width` pixels wide and `height` tall, and every pixel of
// it is the text color. `width` and `height` are 1 to 16.
//
// Example:
//   const std::vector<std::byte> font = MakeBlockFont(6, 10);
//   EXPECT_EQ(FontView(font).GlyphWidth('A'), 6);
std::vector<std::byte> MakeBlockFont(int width, int height);

#endif  // CNC_RED_ALERT_ENGINE_GFX_FONT_TEST_UTIL_H_
