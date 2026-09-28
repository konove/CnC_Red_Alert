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

#ifndef CNC_RED_ALERT_ENGINE_GFX_RGB_H_
#define CNC_RED_ALERT_ENGINE_GFX_RGB_H_

// A palette entry: one color as the VGA DAC stores it. Originally Westwood's
// RGB.H (Joe L. Bostic, December 1995).

#include <cstdint>

class HsvColor;

// One palette color, held at the VGA DAC's 6 bits per gun (0..63).
//
// The interface speaks 8-bit components: the constructor takes 0..255 and the
// red(), green() and blue() return 0..252. Red Alert's PaletteClass is an array
// of these that is also handed out as the raw 768-byte palette, so the three
// one-byte members are the palette file format.
//
// Example:
//   constexpr PaletteColor kEmber(255, 80, 80);
//   const PaletteColor dimmed = kEmber.FadedToward(128, kBlackColor);
class PaletteColor {
 public:
  // Black.
  constexpr PaletteColor() = default;

  // Takes 8-bit components and keeps their top 6 bits.
  constexpr PaletteColor(const uint8_t red, const uint8_t green,
                         const uint8_t blue) noexcept
      : red_(red >> 2), green_(green >> 2), blue_(blue >> 2) {}

  // Returns this color as hue, saturation and value, each 0..255, computed
  // from the 8-bit red(), green() and blue() on the hexagonal color wheel; see
  // HsvColor for the scale. A grey has hue and saturation 0.
  [[nodiscard]] HsvColor ToHsv() const;

  // The brightest 8-bit component the constructor takes.
  static constexpr uint8_t kMaxComponent = 255;

  // Moves this color proportionately toward 'target', as palette fades do.
  //
  // 'ratio' is how far to go, from 0 (no change) to 255 (becomes 'target');
  // values outside 0..255 are clamped. Each gun rounds down, so one rising
  // toward the target reaches it only at 255.
  void Adjust(int ratio, const PaletteColor& target);

  // Returns a copy of this color adjusted toward 'target'; see Adjust().
  [[nodiscard]] PaletteColor FadedToward(int ratio,
                                         const PaletteColor& target) const;

  // Returns the squared Euclidean distance between this color and 'other',
  // measured on the 6-bit guns (0..3 * 63 * 63). Used to find the closest
  // palette match without square roots; 0 means the colors are identical.
  [[nodiscard]] int SquaredDistance(const PaletteColor& other) const;

  // The guns back at the 8-bit scale: the stored 6 bits shifted up by two,
  // so full intensity reads as 252, not 255.
  //
  // TODO: The '/ 64' term is the original '>> 6', always 0 for a 6-bit gun;
  // '>> 4' would copy the top bits down and make 63 read as 255. Changing it
  // shifts ToHsv() values, closest-color matches and the WOLAPI palette.
  [[nodiscard]] constexpr int red() const { return (red_ * 4) + (red_ / 64); }
  [[nodiscard]] constexpr int green() const {
    return (green_ * 4) + (green_ / 64);
  }
  [[nodiscard]] constexpr int blue() const {
    return (blue_ * 4) + (blue_ / 64);
  }

 private:
  // The guns as the VGA DAC takes them, 0..63. Their order and size are the
  // palette's byte layout.
  uint8_t red_{0};
  uint8_t green_{0};
  uint8_t blue_{0};
};

// Black, the target of fades to black.
inline constexpr PaletteColor kBlackColor(0, 0, 0);

#endif  // CNC_RED_ALERT_ENGINE_GFX_RGB_H_
