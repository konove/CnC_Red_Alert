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

// Westwood .FNT bitmap fonts: a view over the font data, the current font and
// its spacing, and the 16-entry palette that colours the glyphs. Originally the
// Westwood 32-bit font library (FONT.H, Scott K. Bowen, June 1994), which split
// these across SET_FONT.CPP, FONT.CPP and TEXTPRNT.ASM; the drawing itself is
// PixelView::Print() in sdllib/pixel_buffer.cc.

#ifndef CNC_RED_ALERT_SDLLIB_FONT_H_
#define CNC_RED_ALERT_SDLLIB_FONT_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"

// Byte offsets, within the font info block, of the two font-wide metrics the
// game reads.
inline constexpr int kFontInfoMaxHeight = 4;
inline constexpr int kFontInfoMaxWidth = 5;

// Header of a Westwood .FNT file, stored little-endian at the start of the
// font data and read by copying it straight into this struct, so it assumes a
// little-endian host. The *_block fields are byte offsets from the start of the
// font data to the named table.
struct FontHeader {
  uint16_t size;          // Total font file size.
  uint8_t compression;    // Compression method (0 in all shipped fonts).
  uint8_t num_blocks;     // Number of data blocks.
  uint16_t info_block;    // Font-wide info (max glyph height/width).
  uint16_t offset_block;  // Per-glyph data offsets (uint16 each).
  uint16_t width_block;   // Per-glyph widths (uint8 each).
  uint16_t data_block;    // Glyph pixel data (FontView reaches it through
                          // offset_block, not through this field).
  uint16_t height_block;  // Per-glyph heights (uint16 each): drawn rows in
                          // the high byte, blank rows above in the low byte.
};
static_assert(sizeof(FontHeader) == 14,
              "FontHeader must match the on-disk layout");

// Non-owning view over Westwood .FNT font data. Provides typed access to the
// per-glyph metric tables, which are byte-packed and unaligned in the blob.
// Cheap to construct and copy; the font data must outlive the view.
//
// Malformed data never reads out of bounds: a table offset past the end of the
// data, or a glyph entry past the end of its table, reads as 0 (or as an empty
// span for GlyphData()), so a truncated font draws as blank glyphs.
//
// Each glyph is a box GlyphWidth() pixels wide and MaxHeight() rows tall:
// GlyphBlankRowsAbove() empty rows, then GlyphHeight() rows of pixel data, then
// empty rows to the bottom of the box.
//
// Example:
//   FontView font(FontPtr);
//   int width = font.GlyphWidth(ch);
class FontView {
 public:
  // data should hold a complete font file; the view reads the header eagerly
  // and the metric tables lazily. Data shorter than the header gives a view
  // whose every metric is 0.
  explicit FontView(
      std::span<const std::byte> data ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : font_(data) {
    if (std::ssize(font_) < base::ssize{sizeof(FontHeader)}) {
      return;
    }
    FontHeader header{};
    base::CopyBytes(base::ObjectBytes(header), std::as_bytes(font_),
                    sizeof(header));
    info_ = Table(header.info_block);
    offsets_ = Table(header.offset_block);
    widths_ = Table(header.width_block);
    heights_ = Table(header.height_block);
  }

  // Height and width of the tallest and widest glyphs, in pixels.
  [[nodiscard]] int MaxHeight() const {
    return Byte(info_, kFontInfoMaxHeight);
  }
  [[nodiscard]] int MaxWidth() const { return Byte(info_, kFontInfoMaxWidth); }
  // Width of glyph ch in pixels, not counting FontXSpacing.
  [[nodiscard]] int GlyphWidth(uint8_t ch) const { return Byte(widths_, ch); }
  // Number of pixel rows stored for glyph ch.
  [[nodiscard]] int GlyphHeight(uint8_t ch) const {
    return PackedHeight(ch) / 256;
  }
  // Number of empty rows between the top of the line and glyph ch's first
  // stored row.
  [[nodiscard]] int GlyphBlankRowsAbove(uint8_t ch) const {
    return PackedHeight(ch) % 256;
  }
  // Returns glyph ch's pixel rows, or an empty span for malformed data. Each
  // byte packs two 4-bit indices into FontPalette, low nibble first, and each
  // row starts on a byte boundary, so a row is (GlyphWidth() + 1) / 2 bytes.
  [[nodiscard]] std::span<const std::byte> GlyphData(uint8_t ch) const {
    // The offset table holds one uint16 offset from the start of the font data
    // per glyph.
    const auto data = Table(ReadWord(offsets_, base::ssize{2} * ch));
    const base::ssize size =
        base::ssize{(GlyphWidth(ch) + 1) / 2} * GlyphHeight(ch);
    return size <= std::ssize(data) ? data.first(base::ToSize(size))
                                    : std::span<const std::byte>{};
  }

 private:
  // Byte and ReadWord return 0 for an offset outside data; see the class
  // comment.
  static uint8_t Byte(std::span<const std::byte> data, base::ssize offset) {
    return offset < std::ssize(data)
               ? std::to_integer<uint8_t>(base::At(data, offset))
               : 0;
  }
  static uint16_t ReadWord(std::span<const std::byte> data,
                           base::ssize offset) {
    if (std::ssize(data) - offset < base::ssize{sizeof(uint16_t)}) {
      return 0;
    }
    uint16_t value = 0;
    base::CopyBytes(base::ObjectBytes(value),
                    data.subspan(base::ToSize(offset)), sizeof(value));
    return value;
  }
  [[nodiscard]] int PackedHeight(uint8_t ch) const {
    return ReadWord(heights_, base::ssize{2} * ch);
  }
  // Returns the font data from offset to the end, or an empty span if offset
  // is past it. Tables carry no length in the header, so each is bounded only
  // by the end of the data.
  [[nodiscard]] std::span<const std::byte> Table(base::ssize offset) const {
    return offset <= std::ssize(font_) ? font_.subspan(base::ToSize(offset))
                                       : std::span<const std::byte>{};
  }
  std::span<const std::byte> font_;  // The whole font file.
  // The header's tables, each running to the end of font_; empty if the
  // header is missing or points past the end.
  std::span<const std::byte> info_;
  std::span<const std::byte> offsets_;
  std::span<const std::byte> widths_;
  std::span<const std::byte> heights_;
};

// Makes new_font the current font and refreshes FontWidth and FontHeight from
// it. Returns the previous font, so callers can restore it. An empty new_font
// leaves the current font in place (and still returns it).
std::span<const std::byte> Set_Font(std::span<const std::byte> new_font);

// Returns the horizontal distance, in pixels, that printing chr in the current
// font advances by: its glyph width plus FontXSpacing.
int Char_Pixel_Width(char chr);

// Returns the width in pixels of the widest line of string in the current
// font, or 0 for nullptr. Lines are separated by '\r' only, as the game's
// text strings are; a '\n' is measured as a glyph, although PixelView::Print()
// breaks the line on it. Every glyph counts FontXSpacing after it, the last
// one on a line included, as Print() advances.
int String_Pixel_Width(const char* string);

// Copies palette into FontPalette entries start_idx through end_idx
// inclusive. Both indices are taken modulo 16; the call does nothing if the
// range is then negative or reversed, or if palette holds fewer entries than
// the range. Entries 0 and 1 are overwritten by the colours of every
// PixelView::Print(), so only entries 2-15 last.
void Set_Font_Palette_Range(std::span<const uint8_t> palette, int start_idx,
                            int end_idx);

// Both return FontPalette; the pointer form is what the assembly text printer
// used.
void* Get_Font_Palette_Ptr();
std::span<const uint8_t> Get_Font_Palette();

// Sets all 16 font colour entries (indices 0 through 15).
inline void Set_Font_Palette(std::span<const uint8_t> palette) {
  constexpr int kFirstColor = 0;
  constexpr int kLastColor = 15;
  Set_Font_Palette_Range(palette, kFirstColor, kLastColor);
}

// Extra pixels printed after every glyph and between lines. Callers set these
// per font before printing and measuring, and PixelView::Print() and the width
// functions above all read them.
extern int FontXSpacing;
extern int FontYSpacing;

// Width of the widest and height of the tallest glyph in the current font, in
// pixels, as Set_Font() last cached them.
extern int FontWidth;
extern int FontHeight;

// The current font's data, as passed to Set_Font(); empty before the first
// call, and then nothing prints.
extern std::span<const std::byte> FontPtr;

// Maps the 4-bit glyph pixel values to screen colours. Entry 0 is the
// background, and a 0 in the table is transparent: nothing is drawn. Starts
// as the identity mapping; PixelView::Print() sets entries 0 and 1 to its
// background and foreground on every call, and Set_Font_Palette_Range()
// installs the other colours of multi-colour fonts.
extern uint8_t FontPalette[16];

#endif  // CNC_RED_ALERT_SDLLIB_FONT_H_
