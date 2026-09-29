// Command & Conquer Red Alert(tm)
// Copyright 2025 Electronic Arts Inc.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CNC_RED_ALERT_ENGINE_GFX_PALETTE_COLOR_H_
#define CNC_RED_ALERT_ENGINE_GFX_PALETTE_COLOR_H_

// A palette entry: one color at 8 bits a component. Originally Westwood's
// RGB.H (Joe L. Bostic, December 1995), which held the VGA DAC's 6-bit guns.

#include <cstdint>

namespace engine::gfx {

class HsvColor;

// Returns the 6-bit VGA gun 'gun' (0..63) at 8 bits, copying its top bits into
// the bottom two so that 0 stays 0 and 63 becomes 255, as the VGA DAC showed
// it. Palette files store 6-bit guns and are expanded this way on loading; it
// is (gun << 2) | (gun >> 4), written arithmetically.
[[nodiscard]] constexpr uint8_t ExpandGun(const int gun) {
  return static_cast<uint8_t>((gun * 4) + (gun / 16));
}

// One palette color: red, green and blue, 0..255 each, as the screen shows
// them.
//
// Example:
//   constexpr engine::gfx::PaletteColor kEmber(255, 80, 80);
//   const engine::gfx::PaletteColor dimmed =
//       kEmber.FadedToward(128, engine::gfx::kBlackColor);
class PaletteColor {
 public:
  // Black.
  constexpr PaletteColor() = default;

  constexpr PaletteColor(const uint8_t red, const uint8_t green,
                         const uint8_t blue) noexcept
      : red_(red), green_(green), blue_(blue) {}

  // Returns this color as hue, saturation and value, each 0..255, on the
  // hexagonal color wheel; see HsvColor for the scale. A grey has hue and
  // saturation 0.
  [[nodiscard]] HsvColor ToHsv() const;

  // The brightest component.
  static constexpr uint8_t kMaxComponent = 255;
  // The brightest VGA gun; 63, not 64, since this is a maximum, not a range.
  static constexpr int kMaxGun = 63;

  // Returns this color moved proportionately toward 'target', as palette
  // fades do.
  //
  // 'ratio' is how far to go, from 0 (no change) to 255 (becomes 'target');
  // values outside 0..255 are clamped. Each component rounds down, so one
  // rising toward the target reaches it only at 255.
  [[nodiscard]] PaletteColor FadedToward(int ratio,
                                         const PaletteColor& target) const;

  // Returns the squared Euclidean distance between this color and 'other'
  // (0..3 * 255 * 255). Used to find the closest palette match without square
  // roots; 0 means the colors are identical.
  [[nodiscard]] int SquaredDistance(const PaletteColor& other) const;

  [[nodiscard]] constexpr int red() const { return red_; }
  [[nodiscard]] constexpr int green() const { return green_; }
  [[nodiscard]] constexpr int blue() const { return blue_; }

  friend constexpr bool operator==(const PaletteColor&,
                                   const PaletteColor&) = default;

 private:
  uint8_t red_{0};
  uint8_t green_{0};
  uint8_t blue_{0};
};

// A palette of 256 entries stays 768 bytes.
static_assert(sizeof(PaletteColor) == 3);

// Black, the target of fades to black.
inline constexpr PaletteColor kBlackColor(0, 0, 0);

// Full-intensity white.
inline constexpr PaletteColor kWhiteColor(PaletteColor::kMaxComponent,
                                          PaletteColor::kMaxComponent,
                                          PaletteColor::kMaxComponent);

// The darkest grey the VGA could show (gun 1). The games briefly set an
// all-near-black palette so that the black one after it differs in every
// entry.
inline constexpr PaletteColor kNearBlackColor(4, 4, 4);

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_PALETTE_COLOR_H_
