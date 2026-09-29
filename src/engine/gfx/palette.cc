// Palette's conversions to and from the VGA file format.

#include "engine/gfx/palette.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "base/array.h"
#include "engine/gfx/palette_color.h"
#include "engine/stream/byte_stream.h"

namespace engine::gfx {

Palette Palette::FromVga(const std::span<const uint8_t, kVgaSize> vga) noexcept {
  Palette palette;
  for (int i = 0; i < kColorCount; ++i) {
    palette.at(i) = PaletteColor::FromGuns(base::At(vga, (i * 3) + 0),
                                           base::At(vga, (i * 3) + 1),
                                           base::At(vga, (i * 3) + 2));
  }
  return palette;
}

std::array<uint8_t, Palette::kVgaSize> Palette::ToVga() const noexcept {
  std::array<uint8_t, kVgaSize> vga{};
  for (int i = 0; i < kColorCount; ++i) {
    const PaletteColor& color = at(i);
    base::At(std::span(vga), (i * 3) + 0) = static_cast<uint8_t>(color.red_gun());
    base::At(std::span(vga), (i * 3) + 1) = static_cast<uint8_t>(color.green_gun());
    base::At(std::span(vga), (i * 3) + 2) = static_cast<uint8_t>(color.blue_gun());
  }
  return vga;
}

std::optional<Palette> ReadVgaPalette(ByteStream& stream) {
  std::array<uint8_t, Palette::kVgaSize> vga{};
  if (stream.Read(std::span(vga)) != Palette::kVgaSize) {
    return std::nullopt;
  }
  return Palette::FromVga(vga);
}

}  // namespace engine::gfx
