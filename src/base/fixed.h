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

#ifndef CNC_RED_ALERT_BASE_FIXED_H_
#define CNC_RED_ALERT_BASE_FIXED_H_

#include <compare>
#include <cstdint>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"

// The fixed-point number type Red Alert's rules, ratios and percentages use.

// An unsigned 8.8 fixed-point number: a whole part of 0-255 and a fraction in
// 1/256 steps. Integer math gives the same answer on every machine, which is
// what keeps multiplayer games in sync.
//
// It cannot hold a negative value and detects no overflow or underflow; the
// raw 16 bits simply wrap.
//
// A binary operator with an int operand returns int, not Fixed, as C's
// promotion would if int were the wider type, so `f * 300` can exceed 255.
// That result is rounded to the nearest whole number (halves up) instead of
// truncated. For a Fixed result, make both operands Fixed.
//
// The int operands must not be negative. The original computed several
// operators in unsigned arithmetic; this port rounds a negative result towards
// zero instead of down. They are int, not unsigned, because an unsigned
// constructor in the original made a literal `0` ambiguous with its string
// constructor.
//
// Example:
//   const Fixed ratio = Fixed::FromString("75%");  // 192/256
//   const int cost = 400 * ratio;                  // 300
class Fixed {
 public:
  // Default-constructs to zero.
  constexpr Fixed() noexcept : raw_(0) {}

  // Constructs numerator / denominator, truncated to a multiple of 1/256:
  // Fixed(3, 4) is 0.75, Fixed(1, 3) is 85/256. A zero denominator gives zero.
  constexpr Fixed(const int numerator, const int denominator) noexcept
      : raw_(denominator == 0
                 ? uint16_t{0}
                 : static_cast<uint16_t>(ToRaw(numerator) / denominator)) {}

  // Constructs the whole number `value`. An int argument keeps only its low
  // 8 bits, as the original's constructor did.
  explicit Fixed(const uint8_t value) noexcept
      : raw_(static_cast<uint16_t>(value << 8)) {}

  // Parses an INI value. Text ending in '%' is a percentage ("75%" is 0.75);
  // anything else is a decimal ("1.5", ".5", "3"). Leading whitespace is
  // skipped, each number is read up to its first non-digit, and text with no
  // digits reads as zero. The fraction is truncated to 1/256 steps: ".016" is
  // 4/256, not 0.016.
  static Fixed FromString(std::string_view text);

  // As above, for C strings from INI buffers and tokenizers; a null pointer
  // parses as zero.
  static Fixed FromString(const char* text) {
    if (text == nullptr) {
      return {};
    }
    return FromString(std::string_view(text));
  }

  // Returns the value rounded to the nearest whole number, halves up.
  [[nodiscard]] int ToInt() const { return (raw_ + kRoundingBias) / 256; }

  // The whole part, and the fraction in 1/256ths.
  [[nodiscard]] int whole() const { return raw_ >> 8; }
  [[nodiscard]] int fraction() const { return raw_ & 0xFF; }

  // Resets the value to zero.
  void Clear() { raw_ = 0; }

  // Saved-game support: the raw 8.8 bit pattern is the whole state.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(raw_);
  }

  // In-place arithmetic. Dividing by zero leaves the value unchanged.
  Fixed& operator*=(const Fixed& rhs) {
    raw_ = static_cast<uint16_t>(int64_t{raw_} * rhs.raw_ / 256);
    return *this;
  }
  Fixed& operator/=(const Fixed& rhs) {
    if (rhs.raw_ != 0) {
      raw_ = static_cast<uint16_t>(raw_ * 256 / rhs.raw_);
    }
    return *this;
  }
  Fixed& operator/=(const int rhs) {
    if (rhs) {
      raw_ = static_cast<uint16_t>(raw_ / rhs);
    }
    return *this;
  }
  Fixed& operator+=(const Fixed& rhs) {
    raw_ += rhs.raw_;
    return *this;
  }
  Fixed& operator-=(const Fixed& rhs) {
    raw_ -= rhs.raw_;
    return *this;
  }

  // Binary arithmetic. The original called the int overload faster than the
  // Fixed one; it returns int, rounded as the class comment describes.
  friend Fixed operator*(Fixed lhs, const Fixed& rhs) { return lhs *= rhs; }
  friend Fixed operator/(Fixed lhs, const Fixed& rhs) { return lhs /= rhs; }
  friend Fixed operator+(Fixed lhs, const Fixed& rhs) { return lhs += rhs; }
  friend Fixed operator-(Fixed lhs, const Fixed& rhs) { return lhs -= rhs; }
  // Rounds the exact product, so 0.5 * 3 is 2.
  int operator*(const int rhs) const {
    return static_cast<int>(((int64_t{raw_} * rhs) + kRoundingBias) / 256);
  }

  // Comparison, including with a whole number on either side. The whole
  // number is scaled to 8.8 so the fraction still counts: Fixed(1, 2) < 1.
  bool operator==(const Fixed& rhs) const = default;
  auto operator<=>(const Fixed& rhs) const = default;
  bool operator==(const int rhs) const { return raw_ == ToRaw(rhs); }
  std::strong_ordering operator<=>(const int rhs) const {
    return raw_ <=> ToRaw(rhs);
  }

  // Arithmetic with the int on the left (5 * f, 5 / f); the compound one
  // stores the int result back into the int.
  friend int operator*(const int lhs, const Fixed& rhs) { return rhs * lhs; }
  // Dividing by zero or by 1.0 returns lhs. The original added 128, half of
  // 256, before dividing, which rounds to nearest only when rhs is 0.5:
  // 1 / Fixed(3, 4) came out 2. Half the divisor rounds every quotient,
  // at the cost of factory rates and landing speeds differing slightly from
  // the shipped game.
  friend int operator/(const int lhs, const Fixed& rhs) {
    if (rhs.raw_ == 0 || rhs.raw_ == 256) {
      return lhs;
    }
    return static_cast<int>((ToRaw(lhs) + (rhs.raw_ / 2)) / rhs.raw_);
  }
  friend int& operator*=(int& lhs ABSL_ATTRIBUTE_LIFETIME_BOUND,
                         const Fixed& rhs) {
    lhs = lhs * rhs;
    return lhs;
  }

  // Rounding, clamping and inversion, in place. Each returns *this so calls
  // chain: `Get_Fixed(...).SaturateBelow(1)`.

  // Ceiling: rounds up only if there is a fractional part. Adding 255
  // (not 256) avoids carrying into the whole part when fraction is zero.
  // Values of 255.0 and above are left unchanged, fraction and all, since
  // 256.0 does not fit.
  Fixed& RoundUp() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (raw_ < 0xFF00) {
      raw_ += 255;
      raw_ &= 0xFF00;
    }
    return *this;
  }
  // Caps the value at `cap`.
  Fixed& Saturate(const int cap) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (raw_ > ToRaw(cap)) {
      raw_ = static_cast<uint16_t>(ToRaw(cap));
    }
    return *this;
  }
  // Caps the value just below `cap`, at cap - 1/256. Volumes use a cap of 1 so
  // that `volume * 256` still fits in a byte. `cap` must be positive: zero
  // wraps round to 255 255/256.
  Fixed& SaturateBelow(const int cap) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (raw_ >= ToRaw(cap)) {
      raw_ = static_cast<uint16_t>(ToRaw(cap) - 1);
    }
    return *this;
  }
  // Replaces the value with 1 / value. Zero becomes 1.0, because dividing by
  // zero leaves the dividend alone; Wide_Area_Damage() relies on that to give
  // the centre cell full damage. 1/256 overflows to zero.
  Fixed& Invert() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    *this = Fixed(1) / *this;
    return *this;
  }

  // Returns the value as an exact decimal with trailing zeros dropped ("1.5",
  // "0.75", "3", "0.33203125" for 85/256). INI files store it, and
  // FromString() reads it back unchanged.
  [[nodiscard]] std::string ToString() const;

  // Common fractions, truncated like Fixed(n, d): kThird is 85/256.
  static const Fixed kHalf;           // 1/2
  static const Fixed kThird;          // 1/3
  static const Fixed kQuarter;        // 1/4
  static const Fixed kThreeQuarters;  // 3/4

 private:
  // Half the fractional range, added before integer division for
  // round-to-nearest instead of truncation.
  static constexpr int kRoundingBias = 128;

  // Returns the whole number `value` in 8.8, widened so that no int overflows.
  static constexpr int64_t ToRaw(const int value) {
    return int64_t{value} * 256;
  }

  // 8.8 fixed-point value: high byte is the whole part (0-255),
  // low byte is the fractional part (0-255 representing 0/256 to 255/256).
  uint16_t raw_;
};

// Defined outside the class because a static member of the class's own type
// needs the class complete. constinit keeps them out of static
// initialization order, and inline lets every includer share one definition.
inline constinit const Fixed Fixed::kHalf{1, 2};
inline constinit const Fixed Fixed::kThird{1, 3};
inline constinit const Fixed Fixed::kQuarter{1, 4};
inline constinit const Fixed Fixed::kThreeQuarters{3, 4};

#endif  // CNC_RED_ALERT_BASE_FIXED_H_
