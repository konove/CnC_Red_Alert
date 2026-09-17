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

/***************************************************************************
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Font and text print 32 bit library       *
 *                                                                         *
 *                    File Name : FONT.H                                   *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : June 27, 1994                            *
 *                                                                         *
 *                  Last Update : June 29, 1994   [SKB]                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   VVPC::Text_Print -- Text print into a virtual viewport.               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_FONT_H_
#define CNC_RED_ALERT_SDLLIB_FONT_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "base/buffer.h"

//////////////////////////////////////// Defines
/////////////////////////////////////////////

// Byte offsets into the font info block.
inline constexpr int kFontInfoMaxHeight = 4;
inline constexpr int kFontInfoMaxWidth = 5;

// Header of a Westwood .FNT file, stored little-endian at the start of the
// font data. The *_block fields are byte offsets from the start of the font
// data to the named table.
struct FontHeader {
  uint16_t size;          // Total font file size.
  uint8_t compression;    // Compression method (0 in all shipped fonts).
  uint8_t num_blocks;     // Number of data blocks.
  uint16_t info_block;    // Font-wide info (max glyph height/width).
  uint16_t offset_block;  // Per-glyph data offsets (uint16 each).
  uint16_t width_block;   // Per-glyph widths (uint8 each).
  uint16_t data_block;    // Glyph pixel data.
  uint16_t height_block;  // Per-glyph packed heights (uint16 each).
};
static_assert(sizeof(FontHeader) == 14,
              "FontHeader must match the on-disk layout");

// Non-owning view over Westwood .FNT font data. Provides typed access to the
// per-glyph metric tables, which are byte-packed and unaligned in the blob.
// Cheap to construct and copy; the font data must outlive the view.
//
// Example:
//   FontView font(FontPtr);
//   int width = font.GlyphWidth(ch);
class FontView {
 public:
  // data must point at a complete font file; the view reads the header
  // eagerly and the metric tables lazily.
  explicit FontView(
      std::span<const std::byte> data ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : font_(data) {
    if (font_.size() < sizeof(FontHeader)) {
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

  [[nodiscard]] int MaxHeight() const {
    return Byte(info_, kFontInfoMaxHeight);
  }
  [[nodiscard]] int MaxWidth() const { return Byte(info_, kFontInfoMaxWidth); }
  [[nodiscard]] int GlyphWidth(uint8_t ch) const { return Byte(widths_, ch); }
  [[nodiscard]] int GlyphHeight(uint8_t ch) const {
    return PackedHeight(ch) / 256;
  }
  [[nodiscard]] int GlyphBlankRowsAbove(uint8_t ch) const {
    return PackedHeight(ch) % 256;
  }
  // Returns the complete packed glyph, or an empty span for malformed data.
  [[nodiscard]] std::span<const std::byte> GlyphData(uint8_t ch) const {
    const auto data = Table(ReadWord(offsets_, size_t{2} * ch));
    const auto size = ((static_cast<size_t>(GlyphWidth(ch)) + 1) / 2) *
                      static_cast<size_t>(GlyphHeight(ch));
    return size <= data.size() ? data.first(size)
                               : std::span<const std::byte>{};
  }

 private:
  static uint8_t Byte(std::span<const std::byte> data, size_t offset) {
    return offset < data.size()
               ? std::to_integer<uint8_t>(base::At(data, offset))
               : 0;
  }
  static uint16_t ReadWord(std::span<const std::byte> data, size_t offset) {
    if (offset > data.size() || data.size() - offset < sizeof(uint16_t)) {
      return 0;
    }
    uint16_t value = 0;
    base::CopyBytes(base::ObjectBytes(value),
                    std::as_bytes(data.subspan(offset)), sizeof(value));
    return value;
  }
  [[nodiscard]] int PackedHeight(uint8_t ch) const {
    return ReadWord(heights_, size_t{2} * ch);
  }
  [[nodiscard]] std::span<const std::byte> Table(size_t offset) const {
    return offset <= font_.size() ? font_.subspan(offset)
                                  : std::span<const std::byte>{};
  }
  std::span<const std::byte> font_;
  std::span<const std::byte> info_;
  std::span<const std::byte> offsets_;
  std::span<const std::byte> widths_;
  std::span<const std::byte> heights_;
};

//////////////////////////////////////// Prototypes
/////////////////////////////////////////////

/*=========================================================================*/
/* The following prototypes are for the file: SET_FONT.CPP
 */
/*=========================================================================*/

// Makes new_font the current font and refreshes the font metric globals.
// Returns the previous font, so callers can restore it. Null new_font leaves
// the current font in place.
std::span<const std::byte> Set_Font(std::span<const std::byte> new_font);

/*=========================================================================*/
/* The following prototypes are for the file: FONT.CPP
 */
/*=========================================================================*/

int Char_Pixel_Width(char chr);
int String_Pixel_Width(const char* string);

/*=========================================================================*/
/* The following prototypes are for the file: TEXTPRNT.ASM
 */
/*=========================================================================*/

void Set_Font_Palette_Range(std::span<const uint8_t> palette, int start_idx,
                            int end_idx);

void* Get_Font_Palette_Ptr();
std::span<const uint8_t> Get_Font_Palette();

// Sets all 16 font color entries (indices 0 through 15).
inline void Set_Font_Palette(std::span<const uint8_t> palette) {
  constexpr int kFirstColor = 0;
  constexpr int kLastColor = 15;
  Set_Font_Palette_Range(palette, kFirstColor, kLastColor);
}

/*=========================================================================*/

//////////////////////////////////////// External varables
//////////////////////////////////////////
extern "C" int FontXSpacing;
extern "C" int FontYSpacing;
extern char FontWidth;
extern char FontHeight;

extern std::span<const std::byte> FontPtr;
// Maps 4-bit glyph pixel values to screen colours; see font.cc.
extern uint8_t FontPalette[16];

#endif  // CNC_RED_ALERT_SDLLIB_FONT_H_
