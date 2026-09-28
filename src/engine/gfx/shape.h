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

// The Westwood shape format and the flags that shape draws take. Originally
// the Westwood 32-bit library's SHAPE.H (Bill Randolph, May 1994).
//
// A shape file is a block of frames: a count, a table of offsets, and the
// frames, each a Shape_Type header followed by its pixels. The game draws its
// sprites from the keyframe format instead (Build_Frame()); the shape format
// is left holding the mouse cursors (MOUSE.SHP), which Extract_Shape() pulls
// apart for the hardware cursor.
//
// The file also owns the shape staging buffer, the scratch memory Build_Frame()
// decompresses frames into and that Tiberian Dawn reuses for reading INI text.

#ifndef CNC_RED_ALERT_ENGINE_GFX_SHAPE_H_
#define CNC_RED_ALERT_ENGINE_GFX_SHAPE_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/attributes.h"
#include "base/flags.h"

// The ShapeType values a shape header can carry: how its pixels are stored.
// Only kMakeshapeNormal is drawn anywhere (the hardware cursor rejects the
// rest).
inline constexpr uint32_t kMakeshapeNormal =
    0x0000;  // 256-color, LCW-compressed
inline constexpr uint32_t kMakeshapeCompact =
    0x0001;  // 16-color, with its color table in the header
inline constexpr uint32_t kMakeshapeNocomp = 0x0002;    // Uncompressed
inline constexpr uint32_t kMakeshapeVariable = 0x0004;  // Fewer than 16 colors

// How a shape is drawn. The low byte moves or transforms the shape, the high
// byte picks the effects; DrawShapeFrame() (engine/gfx/shape_blitter.h) acts
// on SHAPE_CENTER, SHAPE_TRANS, SHAPE_GHOST, SHAPE_FADING and SHAPE_PREDATOR,
// and the tables the effects need travel beside the flags in ShapeEffects.
// The other bits come from the Westwood library and nothing reads them.
enum class CNC_FLAG_ENUM ShapeFlags_Type : uint32_t {
  SHAPE_NORMAL = 0x0000,    // Drawn at x,y as stored
  SHAPE_HORZ_REV = 0x0001,  // Flipped horizontally
  SHAPE_VERT_REV = 0x0002,  // Flipped vertically
  SHAPE_SCALING = 0x0004,   // Scaled
  // Coordinates are relative to the window rather than the page. The games'
  // draw functions always draw into the window's view, so this bit changes
  // nothing; SHAPE_VIEWPORT_REL is the library's other name for it.
  SHAPE_VIEWPORT_REL = 0x0010,
  SHAPE_WIN_REL = 0x0010,
  SHAPE_CENTER = 0x0020,  // x,y name the shape's center, not its top left
  SHAPE_TRANS = 0x0040,   // Color 0 is transparent
  // Every drawn pixel goes through ShapeEffects::fading_table. Together with
  // SHAPE_PREDATOR it asks the games' draw functions for a shadow instead.
  SHAPE_FADING = 0x0100,
  SHAPE_PREDATOR = 0x0200,  // Cloaked: shows a shimmering copy of the page
  SHAPE_COMPACT = 0x0400,   // Reserved by the library; never set
  SHAPE_PRIORITY = 0x0800,  // Drawn through the library's priority system
  SHAPE_GHOST = 0x1000,     // Translucent through ShapeEffects::ghost_table
  SHAPE_SHADOW = 0x2000,
  SHAPE_PARTIAL = 0x4000,
  SHAPE_COLOR = 0x8000  // Colors remapped through a table
};
using enum ShapeFlags_Type;
template <>
inline constexpr bool base::kIsFlagEnum<ShapeFlags_Type> = true;

// The header in front of each frame of a shape file, as stored (packed, so
// the multi-byte fields are unaligned). The hardware cursor copies the first
// ten bytes of it out of a frame that Extract_Shape() returned.
#pragma pack(push, 1)
struct Shape_Type {
  uint16_t ShapeType;            // One of the kMakeshape* values
  unsigned char Height;          // Height of the shape in scan lines
  uint16_t Width;                // Width in bytes, one per pixel at 256 colors
  unsigned char OriginalHeight;  // Original height in scan lines
  uint16_t ShapeSize;            // Size of the frame, this header included
  uint16_t DataLength;           // Size of the pixel data once decompressed
  unsigned char Colortable[16];  // kMakeshapeCompact only: the 16 colors
};

// A shape file: the frame count, then one offset per frame. Extract_Shape()
// reads the same layout from a byte span rather than through this struct.
struct ShapeBlock_Type {
  uint16_t NumShapes;
  // One per frame, each counted from the start of this array rather than of
  // the block. A flexible array, since the offsets follow the count in the
  // file image. NOLINTNEXTLINE(clang-diagnostic-c99-extensions)
  uint32_t Offsets[];
};
#pragma pack(pop)

// The shape staging buffer, set by Set_Shape_Buffer(): three views of the
// same memory. Empty (nullptr and 0) until the game's startup sets it and
// after Tiberian Dawn's shutdown clears it.
extern int ShapeBufferSize;
extern char* ShapeBuffer;
extern std::span<uint8_t> ShapeBufferBytes;

// Returns the number of frames in the shape file `buffer`, or 0 when it is too
// short to hold the count.
int Extract_Shape_Count(std::span<const std::byte> buffer);

// Returns frame `shape` of the shape file `buffer`, its Shape_Type header
// included, as a view into `buffer`. Returns an empty span for a frame number
// out of range and for any offset, header or size that points outside
// `buffer`.
std::span<const std::byte> Extract_Shape(std::span<const std::byte> buffer,
                                         int shape);

// Makes `buffer` the shape staging buffer, updating all three globals above.
// The caller owns the memory and must keep it alive until it sets another
// buffer (an empty one clears them).
void Set_Shape_Buffer(std::span<uint8_t> buffer);

#endif  // CNC_RED_ALERT_ENGINE_GFX_SHAPE_H_
