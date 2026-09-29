// Palette's conversions to and from the VGA file format.

#include "engine/gfx/palette.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "absl/log/check.h"
#include "base/array.h"
#include "engine/gfx/palette_color.h"
#include "engine/stream/byte_stream.h"

namespace engine::gfx {
namespace {

// The levels of a VGA gun, 0..63; the DAC ignored the top two bits.
constexpr int kVgaLevels = 64;

}  // namespace

Palette Palette::FromVga(const std::span<const uint8_t, kVgaSize> vga) noexcept {
  std::array<uint8_t, kVgaSize> rgb{};
  for (int i = 0; i < kVgaSize; ++i) {
    base::At(std::span(rgb), i) = ExpandGun(base::At(vga, i) % kVgaLevels);
  }
  return FromRgb(rgb);
}

std::array<uint8_t, Palette::kVgaSize> Palette::ToVga() const noexcept {
  std::array<uint8_t, kVgaSize> vga = ToRgb();
  for (uint8_t& component : vga) {
    component = static_cast<uint8_t>(component / 4);
  }
  return vga;
}

Palette Palette::FromRgb(
    const std::span<const uint8_t, kVgaSize> rgb) noexcept {
  Palette palette;
  for (int i = 0; i < kColorCount; ++i) {
    palette.at(i) =
        PaletteColor(base::At(rgb, (i * 3) + 0), base::At(rgb, (i * 3) + 1),
                     base::At(rgb, (i * 3) + 2));
  }
  return palette;
}

std::array<uint8_t, Palette::kVgaSize> Palette::ToRgb() const noexcept {
  std::array<uint8_t, kVgaSize> rgb{};
  for (int i = 0; i < kColorCount; ++i) {
    const PaletteColor& color = at(i);
    base::At(std::span(rgb), (i * 3) + 0) = static_cast<uint8_t>(color.red());
    base::At(std::span(rgb), (i * 3) + 1) = static_cast<uint8_t>(color.green());
    base::At(std::span(rgb), (i * 3) + 2) = static_cast<uint8_t>(color.blue());
  }
  return rgb;
}

Palette BlendedPalette(const Palette& from, const Palette& to, const int step,
                       const int steps) {
  CHECK_GT(steps, 0);
  CHECK(step >= 0 && step <= steps);
  const auto blend = [step, steps](const int a, const int b) {
    return static_cast<uint8_t>(a + ((b - a) * step / steps));
  };
  Palette blended;
  for (int i = 0; i < Palette::kColorCount; ++i) {
    const PaletteColor& a = from.at(i);
    const PaletteColor& b = to.at(i);
    blended.at(i) =
        PaletteColor(blend(a.red(), b.red()), blend(a.green(), b.green()),
                     blend(a.blue(), b.blue()));
  }
  return blended;
}

std::optional<Palette> ReadVgaPalette(ByteStream& stream) {
  std::array<uint8_t, Palette::kVgaSize> vga{};
  if (stream.Read(std::span(vga)) != Palette::kVgaSize) {
    return std::nullopt;
  }
  return Palette::FromVga(vga);
}

}  // namespace engine::gfx
