#ifndef CNC_RED_ALERT_ENGINE_GFX_PALETTE_H_
#define CNC_RED_ALERT_ENGINE_GFX_PALETTE_H_

// The 256-color palette the games draw through.

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/palette_index.h"

class ByteStream;

namespace engine::gfx {

// Returns the 6-bit VGA gun `gun` (0..63) at 8 bits, copying its top bits into
// the bottom two so that 0 stays 0 and 63 becomes 255, as the VGA DAC showed
// it. Palette::FromVga() expands palette files this way. It is
// (gun << 2) | (gun >> 4), written arithmetically; a gun above 63 wraps.
[[nodiscard]] constexpr uint8_t ExpandGun(const int gun) {
  return static_cast<uint8_t>((gun * 4) + (gun / 16));
}

// 256 PaletteColors, indexed by the 8-bit pixel values of a PixelBuffer.
//
// Palette files store 768 bytes, red, green and blue per entry: .PAL files,
// CPS pictures and WSA animations as the VGA's 6-bit guns (FromVga() and
// ToVga()), PCX pictures at 8 bits (FromRgb() and ToRgb()). Everything else
// works on the colors.
//
// Example:
//   engine::gfx::Palette palette = engine::gfx::Palette::FromVga(file_bytes);
//   palette.at(255) = engine::gfx::kBlackColor;
//   engine::window::TheDisplay().SetPalette(palette);
class Palette {
 public:
  static constexpr int kColorCount = 256;
  // The size of a palette in a file: one byte per component.
  static constexpr int kFileSize = kColorCount * 3;

  // All black.
  constexpr Palette() = default;

  // Every entry `fill`.
  explicit Palette(const PaletteColor& fill) noexcept { colors_.fill(fill); }

  // Returns the palette stored in `vga`, 256 triples of 6-bit guns, each
  // expanded by ExpandGun(). The top two bits of each byte are ignored, as
  // the VGA DAC ignored them.
  [[nodiscard]] static Palette FromVga(
      std::span<const uint8_t, kFileSize> vga) noexcept;

  // Returns this palette as 256 triples of 6-bit guns: each component's top
  // six bits.
  [[nodiscard]] std::array<uint8_t, kFileSize> ToVga() const noexcept;

  // Returns the palette stored in `rgb`, 256 triples of 8-bit components.
  [[nodiscard]] static Palette FromRgb(
      std::span<const uint8_t, kFileSize> rgb) noexcept;

  // Returns this palette as 256 triples of 8-bit components.
  [[nodiscard]] std::array<uint8_t, kFileSize> ToRgb() const noexcept;

  // Returns an entry; indices outside [0, 256) fail in every build.
  //
  // Clang 23 cannot trace the reference through libstdc++ span operations;
  // the span is made from colors_ and never escapes.
  // NOLINTBEGIN(clang-diagnostic-lifetime-safety-lifetimebound-violation)
  [[nodiscard]] PaletteColor& at(const int index)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(std::span(colors_), index);
  }
  [[nodiscard]] const PaletteColor& at(const int index) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(std::span(colors_), index);
  }

  // Returns the entry of slot `index`.
  [[nodiscard]] PaletteColor& at(const PaletteIndex index)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(std::span(colors_), index.value());
  }
  [[nodiscard]] const PaletteColor& at(const PaletteIndex index) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(std::span(colors_), index.value());
  }
  // NOLINTEND(clang-diagnostic-lifetime-safety-lifetimebound-violation)

  [[nodiscard]] std::span<PaletteColor, kColorCount> colors()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return colors_;
  }
  [[nodiscard]] std::span<const PaletteColor, kColorCount> colors() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return colors_;
  }

  friend bool operator==(const Palette&, const Palette&) = default;

 private:
  std::array<PaletteColor, kColorCount> colors_{};
};

// Returns the palette `step` of `steps` of the way from `from` to `to`, as a
// fade shows it: each component moves linearly and rounds toward `from`.
// `steps` must be positive and `step` within 0..steps.
[[nodiscard]] Palette BlendedPalette(const Palette& from, const Palette& to,
                                     int step, int steps);

// Brightens every color of `palette` by the given percentages, each component
// stopping at full intensity, as the games do to their movie and map screen
// palettes.
void BrightenPalette(Palette& palette, int red_percentage, int green_percentage,
                     int blue_percentage);

// Reads a palette in the VGA file format (a .PAL file) from `stream`'s current
// position. Returns nullopt if the stream ends first.
[[nodiscard]] std::optional<Palette> ReadVgaPalette(ByteStream& stream);

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_PALETTE_H_
