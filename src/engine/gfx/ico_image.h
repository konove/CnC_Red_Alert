// File: Decodes a Windows icon file (.ICO) to RGBA pixels, for the window
// icon on platforms that do not take it from the executable's resources.

#ifndef CNC_RED_ALERT_ENGINE_GFX_ICO_IMAGE_H_
#define CNC_RED_ALERT_ENGINE_GFX_ICO_IMAGE_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace engine::gfx {

// One image of an icon file, `width` x `height` pixels, top row first, four
// bytes per pixel in R, G, B, A order (SDL_PIXELFORMAT_RGBA32). A pixel the
// icon's mask leaves out has alpha 0, every other alpha 255.
struct IcoImage {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> rgba;
};

// Decodes the largest image in `ico_file`, preferring the one with the most
// colors among images of the same size. Only uncompressed 1-, 4- and 8-bit
// paletted images are read, which is what both games' icons hold; PNG and
// true-color entries are skipped. Returns nullopt if the file is not an
// icon, or holds no image it can read, or the image it picks is corrupt: its
// data runs past the end of the file, or a pixel names a color its palette
// lacks.
std::optional<IcoImage> DecodeIco(std::span<const std::byte> ico_file);

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_ICO_IMAGE_H_
