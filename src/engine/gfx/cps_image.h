// File: Reads Westwood's compressed pictures (.CPS): one compressed block
// (engine/codec/compressed_block.h) whose skipped area may hold a palette.

#ifndef CNC_RED_ALERT_ENGINE_GFX_CPS_IMAGE_H_
#define CNC_RED_ALERT_ENGINE_GFX_CPS_IMAGE_H_

#include <cstdint>
#include <vector>

#include "engine/gfx/palette_color.h"

class ByteStream;

namespace engine::gfx {

// A picture is at most one screen, kCpsWidth by kCpsHeight pixels.
inline constexpr int kCpsWidth = 320;
inline constexpr int kCpsHeight = 200;

// A decoded CPS picture.
struct CpsImage {
  // The pixels, row by row, kCpsWidth to a row; empty if the picture could
  // not be read.
  std::vector<uint8_t> pixels;
  // The colors the picture's palette holds whole, from color 0 up: fewer than
  // 256 for a short palette, none for a picture without one.
  std::vector<PaletteColor> colors;
};

// Reads the picture at `file`'s position. Returns an image without pixels if
// the file is damaged or the picture is larger than one screen.
//
// Example:
//   const engine::gfx::CpsImage picture = engine::gfx::ReadCpsImage(*file);
//   std::ranges::copy(picture.colors, palette.colors().begin());
CpsImage ReadCpsImage(ByteStream& file);

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_CPS_IMAGE_H_
