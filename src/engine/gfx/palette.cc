// Palette's conversions to and from the VGA file format.

#include "engine/gfx/palette.h"

#include <algorithm>
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

Palette Palette::FromVga(
    const std::span<const uint8_t, kFileSize> vga) noexcept {
  std::array<uint8_t, kFileSize> rgb{};
  for (int i = 0; i < kFileSize; ++i) {
    base::At(std::span(rgb), i) = ExpandGun(base::At(vga, i) % kVgaLevels);
  }
  return FromRgb(rgb);
}

std::array<uint8_t, Palette::kFileSize> Palette::ToVga() const noexcept {
  std::array<uint8_t, kFileSize> vga = ToRgb();
  for (uint8_t& component : vga) {
    component = static_cast<uint8_t>(component / 4);
  }
  return vga;
}

Palette Palette::FromRgb(
    const std::span<const uint8_t, kFileSize> rgb) noexcept {
  Palette palette;
  for (int i = 0; i < kColorCount; ++i) {
    palette.at(i) =
        PaletteColor(base::At(rgb, (i * 3) + 0), base::At(rgb, (i * 3) + 1),
                     base::At(rgb, (i * 3) + 2));
  }
  return palette;
}

std::array<uint8_t, Palette::kFileSize> Palette::ToRgb() const noexcept {
  std::array<uint8_t, kFileSize> rgb{};
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

void BrightenPalette(Palette& palette, const int red_percentage,
                     const int green_percentage, const int blue_percentage) {
  const auto brighten = [](const int component, const int percentage) {
    return static_cast<uint8_t>(
        std::min<int>(component + (component * percentage / 100),
                      PaletteColor::kMaxComponent));
  };
  for (PaletteColor& color : palette.colors()) {
    color = PaletteColor(brighten(color.red(), red_percentage),
                         brighten(color.green(), green_percentage),
                         brighten(color.blue(), blue_percentage));
  }
}

std::optional<Palette> ReadVgaPalette(ByteStream& stream) {
  std::array<uint8_t, Palette::kFileSize> vga{};
  if (stream.Read(std::span(vga)) != Palette::kFileSize) {
    return std::nullopt;
  }
  return Palette::FromVga(vga);
}

}  // namespace engine::gfx
