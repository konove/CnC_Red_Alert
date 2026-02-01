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

#ifndef CNC_RED_ALERT_TECH_RGB_H_
#define CNC_RED_ALERT_TECH_RGB_H_

#include <cstdint>

// Forward declaration
class HSVClass;

// RGB color class.
//
// Provides color representation in RGB color space with 6-bit components
// (0-63 range) and conversion to HSV color space.
class RGBClass {
 public:
  // Default constructor (Zero initialization)
  constexpr RGBClass() = default;

  // Handles the bit-shift conversion from 8-bit (0-255) to VGA 6-bit (0-63)
  constexpr RGBClass(const uint8_t red, const uint8_t green, const uint8_t blue)
      : red_(red >> 2), green_(green >> 2), blue_(blue >> 2) {}

  // Converts this RGB color to the HSV color space.
  //
  // The conversion uses a hexagonal color wheel model to determine hue.
  [[nodiscard]] HSVClass ToHSV() const;

  static constexpr uint8_t kMaxValue = 255;

  // Adjusts the current color proportionately toward the 'target' color.
  //
  // This is typically used for fading effects. The 'ratio' argument defines
  // the interpolation step:
  //  - 0:   No change (keeps current color).
  //  - 255: Fully transforms to the 'target' color.
  //
  // If 'ratio' is outside the [0, 255] range, it is clamped.
  void Adjust(int ratio, const RGBClass& target);

  // Returns the squared Euclidean distance between this color and 'other'.
  //
  // This is used to find the closest color match without calculating square
  // roots. A result of 0 indicates the colors are identical.
  int Difference(const RGBClass& other) const;

  // Logic: Restores 6-bit storage to 8-bit range using original bitwise logic.
  [[nodiscard]] constexpr int Red_Component() const {
    return red_ << 2 | red_ >> 6;
  }
  [[nodiscard]] constexpr int Green_Component() const {
    return green_ << 2 | green_ >> 6;
  }
  [[nodiscard]] constexpr int Blue_Component() const {
    return blue_ << 2 | blue_ >> 6;
  }

 private:
  // These hold the actual color gun values in machine dependant scale.
  // Values range from 0 to 63 (VGA standard).

  uint8_t red_{0};
  uint8_t green_{0};
  uint8_t blue_{0};
};

// Common color constant - initialized at compile time.
inline constexpr RGBClass kBlackColor(0, 0, 0);

#endif  // CNC_RED_ALERT_TECH_RGB_H_
