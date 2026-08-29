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

#include "base/types.h"
#include <cstdint>
#include <cstring>

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
  explicit FontView(const void* data)
      : font_(static_cast<const uint8_t*>(data)) {
    FontHeader header;
    std::memcpy(&header, font_, sizeof(header));
    info_ = font_ + header.info_block;
    offsets_ = font_ + header.offset_block;
    widths_ = font_ + header.width_block;
    heights_ = font_ + header.height_block;
  }

  // Tallest glyph in pixels; the height of one text line.
  int MaxHeight() const { return info_[kFontInfoMaxHeight]; }
  // Widest glyph in pixels.
  int MaxWidth() const { return info_[kFontInfoMaxWidth]; }

  // Width in pixels of the glyph for character ch.
  int GlyphWidth(uint8_t ch) const { return widths_[ch]; }

  // Number of drawn pixel rows in the glyph.
  int GlyphHeight(uint8_t ch) const { return PackedHeight(ch) >> 8; }
  // Number of blank rows between the top of the line and the drawn rows.
  int GlyphBlankRowsAbove(uint8_t ch) const { return PackedHeight(ch) & 0xFF; }

  // The glyph's pixel data: two 4-bit palette indices per byte, low nibble
  // first, GlyphWidth x GlyphHeight pixels.
  const uint8_t* GlyphData(uint8_t ch) const {
    return font_ + ReadWord(offsets_ + base::ssize{2} * ch);
  }

 private:
  // Reads a little-endian uint16 with no alignment requirement.
  static uint16_t ReadWord(const uint8_t* data) {
    uint16_t value;
    std::memcpy(&value, data, sizeof(value));
    return value;
  }

  // Blank rows above the glyph in the low byte, drawn rows in the high byte.
  int PackedHeight(uint8_t ch) const {
    return ReadWord(heights_ + base::ssize{2} * ch);
  }

  const uint8_t* font_;     // Start of the font data.
  const uint8_t* info_;     // Font-wide info block.
  const uint8_t* offsets_;  // Per-glyph data offset table (uint16, unaligned).
  const uint8_t* widths_;   // Per-glyph width table (uint8).
  const uint8_t*
      heights_;  // Per-glyph packed height table (uint16, unaligned).
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
const void* Set_Font(const void* new_font);

/*=========================================================================*/
/* The following prototypes are for the file: FONT.CPP
 */
/*=========================================================================*/

int Char_Pixel_Width(char chr);
unsigned int String_Pixel_Width(const char* string);

/*=========================================================================*/
/* The following prototypes are for the file: TEXTPRNT.ASM
 */
/*=========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

void Set_Font_Palette_Range(const void* palette, int start_idx, int end_idx);

#ifdef TD
void* Get_Font_Palette_Ptr();
#endif

#ifdef __cplusplus
}
#endif

// Sets all 16 font color entries (indices 0 through 15).
inline void Set_Font_Palette(const void* palette) {
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

extern "C" const void* FontPtr;
// Maps 4-bit glyph pixel values to screen colours; see font.cc.
extern uint8_t FontPalette[16];

#endif  // CNC_RED_ALERT_SDLLIB_FONT_H_
