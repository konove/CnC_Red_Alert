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
// An arithmetic operator with an int operand returns int, not fixed, as C's
// promotion would if int were the wider type, so `f * 300` can exceed 255.
// That result is rounded to the nearest whole number (halves up) instead of
// truncated. For a fixed result, make both operands fixed.
//
// The int operands must not be negative. The original computed several
// operators in unsigned arithmetic; this port rounds a negative result towards
// zero instead of down. They are int, not unsigned, because an unsigned
// constructor in the original made a literal `0` ambiguous with its string
// constructor.
//
// Example:
//   const fixed ratio = fixed::FromString("75%");  // 192/256
//   const int cost = 400 * ratio;                  // 300
class fixed {
 public:
  // Default-constructs to zero.
  constexpr fixed() noexcept : raw_(0) {}

  // Constructs numerator / denominator, truncated to a multiple of 1/256:
  // fixed(3, 4) is 0.75, fixed(1, 3) is 85/256. A zero denominator gives zero.
  constexpr fixed(const int numerator, const int denominator) noexcept
      : raw_(denominator == 0
                 ? uint16_t{0}
                 : static_cast<uint16_t>((numerator * 256) / denominator)) {}

  // Constructs the whole number `value`. An int argument keeps only its low
  // 8 bits, as the original's constructor did.
  explicit fixed(const uint8_t value) noexcept
      : raw_(static_cast<uint16_t>(value << 8)) {}

  // Parses an INI value. Text ending in '%' is a percentage ("75%" is 0.75);
  // anything else is a decimal ("1.5", ".5", "3"). Leading whitespace is
  // skipped, each number is read up to its first non-digit, and text with no
  // digits reads as zero. The fraction is truncated to 1/256 steps: ".016" is
  // 4/256, not 0.016.
  static fixed FromString(std::string_view str);

  // As above, for C strings from INI buffers and tokenizers; a null pointer
  // parses as zero.
  static fixed FromString(const char* str) {
    if (str == nullptr) {
      return {};
    }
    return FromString(std::string_view(str));
  }

  // Returns the value rounded to the nearest whole number, halves up.
  [[nodiscard]] int ToInt() const { return (raw_ + kRoundingBias) / 256; }

  // The whole part, and the fraction in 1/256ths.
  [[nodiscard]] uint8_t whole() const { return raw_ >> 8; }
  [[nodiscard]] uint8_t fraction() const { return raw_ & 0xFF; }

  // Resets the value to zero.
  void Clear() { raw_ = 0; }

  // Saved-game support: the raw 8.8 bit pattern is the whole state.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(raw_);
  }

  // In-place arithmetic. Dividing by zero leaves the value unchanged; dividing
  // by 1.0 is skipped as a shortcut.
  fixed& operator*=(const fixed& rvalue) {
    // Divide by 256 to remove extra 8.8 scale factor.
    raw_ = static_cast<uint16_t>((raw_ * rvalue.raw_) / 256);
    return *this;
  }
  fixed& operator*=(const int rvalue) {
    raw_ = static_cast<uint16_t>(raw_ * rvalue);
    return *this;
  }
  fixed& operator/=(const fixed& rvalue) {
    if (rvalue.raw_ != 0 && rvalue.raw_ != 256) {
      raw_ = static_cast<uint16_t>(raw_ * 256 / rvalue.raw_);
    }
    return *this;
  }
  fixed& operator/=(const int rvalue) {
    if (rvalue) {
      raw_ = static_cast<uint16_t>(raw_ / rvalue);
    }
    return *this;
  }
  fixed& operator+=(const fixed& rvalue) {
    raw_ += rvalue.raw_;
    return *this;
  }
  fixed& operator+=(const int rvalue) {
    raw_ += static_cast<uint16_t>(rvalue * 256);
    return *this;
  }
  fixed& operator-=(const fixed& rvalue) {
    raw_ -= rvalue.raw_;
    return *this;
  }
  fixed& operator-=(const int rvalue) {
    raw_ -= static_cast<uint16_t>(rvalue * 256);
    return *this;
  }

  // Binary arithmetic. The original called the int overloads faster than the
  // fixed ones; they return int, rounded as the class comment describes.
  fixed operator*(const fixed& rvalue) const {
    fixed temp = *this;
    // Divide by 256 to remove extra 8.8 scale factor.
    temp.raw_ = static_cast<uint16_t>(temp.raw_ * rvalue.raw_ / 256);
    return temp;
  }
  // Rounds the exact product, so 0.5 * 3 is 2.
  int operator*(const int rvalue) const {
    return ((raw_ * rvalue) + kRoundingBias) / 256;
  }
  fixed operator/(const fixed& rvalue) const {
    fixed temp = *this;
    if (rvalue.raw_ != 0 && rvalue.raw_ != 256) {
      temp.raw_ = static_cast<uint16_t>(temp.raw_ * 256 / rvalue.raw_);
    }
    return temp;
  }
  // Rounds this value first, then divides with truncation: 3.5 / 2 is 4 / 2.
  // Dividing by zero returns the rounded value.
  int operator/(const int rvalue) const {
    if (rvalue != 0) {
      return ToInt() / rvalue;
    }
    return ToInt();
  }
  fixed operator+(const fixed& rvalue) const {
    fixed temp = *this;
    temp += rvalue;
    return temp;
  }
  // Rounds this value first, as does int subtraction below.
  int operator+(const int rvalue) const { return ToInt() + rvalue; }
  fixed operator-(const fixed& rvalue) const {
    fixed temp = *this;
    temp -= rvalue;
    return temp;
  }
  int operator-(const int rvalue) const { return ToInt() - rvalue; }

  // Multiply or divide by a power of two by shifting the raw bits; bits
  // shifted out are lost.
  fixed& operator>>=(const unsigned rvalue) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    raw_ >>= rvalue;
    return *this;
  }
  fixed& operator<<=(const unsigned rvalue) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    raw_ <<= rvalue;
    return *this;
  }
  fixed operator>>(const unsigned rvalue) const {
    fixed temp = *this;
    temp >>= rvalue;
    return temp;
  }
  fixed operator<<(const unsigned rvalue) const {
    fixed temp = *this;
    temp <<= rvalue;
    return temp;
  }

  // Comparison operators (fixed vs fixed).
  bool operator==(const fixed& rvalue) const { return raw_ == rvalue.raw_; }
  bool operator!=(const fixed& rvalue) const { return raw_ != rvalue.raw_; }
  bool operator<(const fixed& rvalue) const { return raw_ < rvalue.raw_; }
  bool operator>(const fixed& rvalue) const { return raw_ > rvalue.raw_; }
  bool operator<=(const fixed& rvalue) const { return raw_ <= rvalue.raw_; }
  bool operator>=(const fixed& rvalue) const { return raw_ >= rvalue.raw_; }
  bool operator!() const { return raw_ == 0; }

  // Comparison with a whole number, which is scaled to 8.8 so the fraction
  // still counts: fixed(1, 2) < 1 holds.
  bool operator<(const int rvalue) const { return raw_ < rvalue * 256; }
  bool operator>(const int rvalue) const { return raw_ > rvalue * 256; }
  bool operator<=(const int rvalue) const { return raw_ <= rvalue * 256; }
  bool operator>=(const int rvalue) const { return raw_ >= rvalue * 256; }
  bool operator==(const int rvalue) const { return raw_ == rvalue * 256; }
  bool operator!=(const int rvalue) const { return raw_ != rvalue * 256; }

  // The same operators with the int on the left (5 * f). The compound ones
  // store the int result back into the int.
  friend int operator*(const int lvalue, const fixed& rvalue) {
    return rvalue * lvalue;
  }
  // Dividing by zero or by 1.0 returns lvalue.
  // TODO: The rounding bias should be half the divisor (rvalue.raw_ / 2), not
  // half of 256, so 1 / fixed(3, 4) gives 2 instead of 1. It is the shipped
  // behaviour of factory production rates and aircraft landing speed.
  friend int32_t operator/(const int32_t lvalue, const fixed& rvalue) {
    if (rvalue.raw_ == 0 || rvalue.raw_ == 256) {
      return lvalue;
    }
    return ((lvalue * 256) + kRoundingBias) / rvalue.raw_;
  }
  friend int operator+(const int lvalue, const fixed& rvalue) {
    return rvalue + lvalue;
  }
  friend int operator-(const int lvalue, const fixed& rvalue) {
    return ((lvalue * 256) - rvalue.raw_ + kRoundingBias) / 256;
  }
  friend bool operator<(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 < rvalue.raw_;
  }
  friend bool operator>(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 > rvalue.raw_;
  }
  friend bool operator<=(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 <= rvalue.raw_;
  }
  friend bool operator>=(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 >= rvalue.raw_;
  }
  friend bool operator==(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 == rvalue.raw_;
  }
  friend bool operator!=(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 != rvalue.raw_;
  }
  friend int operator*=(int& lvalue, const fixed& rvalue) {
    lvalue = lvalue * rvalue;
    return lvalue;
  }
  friend int operator/=(int& lvalue, const fixed& rvalue) {
    lvalue = lvalue / rvalue;
    return lvalue;
  }
  friend int operator+=(int& lvalue, const fixed& rvalue) {
    lvalue = lvalue + rvalue;
    return lvalue;
  }
  friend int operator-=(int& lvalue, const fixed& rvalue) {
    lvalue = lvalue - rvalue;
    return lvalue;
  }

  // Rounding, clamping and inversion, in place. Each returns *this so calls
  // chain: `Get_Fixed(...).Sub_Saturate(1)`.

  // Ceiling: rounds up only if there is a fractional part. Adding 255
  // (not 256) avoids carrying into the whole part when fraction is zero.
  // Values of 255.0 and above are left unchanged, fraction and all, since
  // 256.0 does not fit.
  fixed& Round_Up() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (raw_ < 0xFF00) {
      raw_ += 255;
      raw_ &= 0xFF00;
    }
    return *this;
  }
  // Drops the fraction.
  fixed& Round_Down() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    raw_ &= 0xFF00;
    return *this;
  }
  // Rounds to the nearest whole number, halves up.
  fixed& Round() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (fraction() >= kRoundingBias) {
      Round_Up();
    }
    Round_Down();
    return *this;
  }
  // Caps the value at `cap`.
  fixed& Saturate(const unsigned cap) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (raw_ > cap * 256) {
      raw_ = static_cast<uint16_t>(cap * 256);
    }
    return *this;
  }
  fixed& Saturate(const fixed& cap) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (*this > cap) {
      *this = cap;
    }
    return *this;
  }
  // Caps the value just below `cap`, at cap - 1/256. Volumes use a cap of 1 so
  // that `volume * 256` still fits in a byte. `cap` must be positive: zero
  // wraps round to 255 255/256.
  fixed& Sub_Saturate(const unsigned cap) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (raw_ >= cap * 256) {
      raw_ = static_cast<uint16_t>((cap * 256) - 1);
    }
    return *this;
  }
  fixed& Sub_Saturate(const fixed& cap) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (*this >= cap) {
      raw_ = static_cast<uint16_t>(cap.raw_ - 1);
    }
    return *this;
  }
  // Replaces the value with 1 / value. Zero becomes 1.0, because dividing by
  // zero leaves the dividend alone; Wide_Area_Damage() relies on that to give
  // the centre cell full damage. 1/256 overflows to zero.
  fixed& Inverse() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    *this = fixed(1) / *this;
    return *this;
  }

  // Returns the value as a decimal with trailing zeros dropped ("1.5", "0.75",
  // "3"), the fraction truncated to thousandths. INI files store it, and
  // FromString() reads it back.
  [[nodiscard]] std::string AsString() const;

  // Common fractions, truncated like fixed(n, d): _1_3 is 85/256.
  static const fixed _1_2;  // 1/2
  static const fixed _1_3;  // 1/3
  static const fixed _1_4;  // 1/4
  static const fixed _3_4;  // 3/4
  static const fixed _2_3;  // 2/3

 private:
  // Half the fractional range, added before integer division for
  // round-to-nearest instead of truncation.
  static constexpr int kRoundingBias = 128;

  // 8.8 fixed-point value: high byte is the whole part (0-255),
  // low byte is the fractional part (0-255 representing 0/256 to 255/256).
  uint16_t raw_;
};

// Defined outside the class because a static member of the class's own type
// needs the class complete. constinit keeps them out of static
// initialization order, and inline lets every includer share one definition.
inline constinit const fixed fixed::_1_2{1, 2};
inline constinit const fixed fixed::_1_3{1, 3};
inline constinit const fixed fixed::_1_4{1, 4};
inline constinit const fixed fixed::_3_4{3, 4};
inline constinit const fixed fixed::_2_3{2, 3};

#endif  // CNC_RED_ALERT_BASE_FIXED_H_
