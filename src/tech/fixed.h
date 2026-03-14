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

#ifndef CNC_RED_ALERT_TECH_FIXED_H_
#define CNC_RED_ALERT_TECH_FIXED_H_

#include <cstdint>
#include <string>
#include <string_view>

// Unsigned 8.8 fixed-point number (whole: 0-255, fraction: 1/256 precision).
//
// Does not support negative values or detect overflow/underflow. Operators
// with an integer operand return int (following C upcasting rules, treating
// int as higher precision), rounded to the nearest whole value. To get a
// fixed-point result, ensure both operands are fixed.
//
// Integer parameters are signed to avoid ambiguity with literal "0" (which
// the compiler might otherwise interpret as a null pointer).
class fixed {
 public:
  // Default-constructs to zero.
  fixed() = default;

  // Constructs from a fraction (e.g., fixed(3, 4) = 0.75). Zero denominator
  // yields zero.
  constexpr fixed(const int numerator, const int denominator)
      : raw_(denominator == 0
                 ? uint16_t{0}
                 : static_cast<uint16_t>((numerator << 8) / denominator)) {}

  // Constructs from a whole number (fractional part set to zero).
  explicit fixed(const uint8_t value)
      : raw_(static_cast<uint16_t>(value << 8)) {}

  // Parses a decimal string ("1.5") or a percentage string ("75%").
  static fixed FromString(std::string_view str);

  // Overload for const char* to handle null pointers safely. Callers passing
  // strtok() results or other potentially-null C strings use this overload.
  static fixed FromString(const char* str) {
    if (str == nullptr) return {};
    return FromString(std::string_view(str));
  }

  // Returns the value rounded to the nearest whole integer.
  int ToInt() const { return (raw_ + kRoundingBias) >> 8; }

  // Accessors for the whole and fractional parts.
  uint8_t whole() const { return raw_ >> 8; }
  uint8_t fraction() const { return raw_ & 0xFF; }

  // Resets the value to zero.
  void Clear() { raw_ = 0; }

  // In-place arithmetic operators.
  fixed& operator*=(const fixed& rvalue) {
    // Divide by 256 to remove extra 8.8 scale factor.
    raw_ = static_cast<uint16_t>((raw_ * rvalue.raw_) >> 8);
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
    raw_ += static_cast<uint16_t>(rvalue << 8);
    return *this;
  }
  fixed& operator-=(const fixed& rvalue) {
    raw_ -= rvalue.raw_;
    return *this;
  }
  fixed& operator-=(const int rvalue) {
    raw_ -= static_cast<uint16_t>(rvalue << 8);
    return *this;
  }

  // Arithmetic operators. Integer overloads are more efficient than
  // fixed-point and return int rounded to nearest whole value.
  fixed operator*(const fixed& rvalue) const {
    fixed temp = *this;
    // Divide by 256 to remove extra 8.8 scale factor.
    temp.raw_ = static_cast<uint16_t>(temp.raw_ * rvalue.raw_ / 256);
    return temp;
  }
  int operator*(const int rvalue) const {
    return (raw_ * rvalue + kRoundingBias) / 256;
  }
  fixed operator/(const fixed& rvalue) const {
    fixed temp = *this;
    if (rvalue.raw_ != 0 && rvalue.raw_ != 256) {
      temp.raw_ = static_cast<uint16_t>(temp.raw_ * 256 / rvalue.raw_);
    }
    return temp;
  }
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
  int operator+(const int rvalue) const { return ToInt() + rvalue; }
  fixed operator-(const fixed& rvalue) const {
    fixed temp = *this;
    temp -= rvalue;
    return temp;
  }
  int operator-(const int rvalue) const { return ToInt() - rvalue; }

  // Shift operators for efficient multiply/divide by powers of 2.
  fixed& operator>>=(const unsigned rvalue) {
    raw_ >>= rvalue;
    return *this;
  }
  fixed& operator<<=(const unsigned rvalue) {
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

  // Comparison to integers (scales integer to 8.8 for accurate comparison).
  bool operator<(const int rvalue) const { return raw_ < rvalue << 8; }
  bool operator>(const int rvalue) const { return raw_ > rvalue << 8; }
  bool operator<=(const int rvalue) const { return raw_ <= rvalue << 8; }
  bool operator>=(const int rvalue) const { return raw_ >= rvalue << 8; }
  bool operator==(const int rvalue) const { return raw_ == rvalue << 8; }
  bool operator!=(const int rvalue) const { return raw_ != rvalue << 8; }

  // Commutative friend operators for int-on-left expressions (e.g., 5 * f).
  friend int operator*(const int lvalue, const fixed& rvalue) {
    return rvalue * lvalue;
  }
  friend int32_t operator/(const int32_t lvalue, const fixed& rvalue) {
    if (rvalue.raw_ == 0 || rvalue.raw_ == 256) {
      return lvalue;
    }
    return (lvalue * 256 + kRoundingBias) / rvalue.raw_;
  }
  friend int operator+(const int lvalue, const fixed& rvalue) {
    return rvalue + lvalue;
  }
  friend int operator-(const int lvalue, const fixed& rvalue) {
    return (lvalue * 256 - rvalue.raw_ + kRoundingBias) / 256;
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

  // Rounding, clamping, and inversion helpers.

  // Ceiling: rounds up only if there is a fractional part. Adding 255
  // (not 256) avoids carrying into the whole part when fraction is zero.
  // Values above 255.0 are left unchanged to prevent uint16_t overflow.
  fixed& Round_Up() {
    if (raw_ < 0xFF00) {
      raw_ += 255;
      raw_ &= 0xFF00;
    }
    return *this;
  }
  fixed& Round_Down() {
    raw_ &= 0xFF00;
    return *this;
  }
  fixed& Round() {
    if (fraction() >= kRoundingBias) {
      Round_Up();
    }
    Round_Down();
    return *this;
  }
  fixed& Saturate(const unsigned cap) {
    if (raw_ > cap * 256) {
      raw_ = static_cast<uint16_t>(cap * 256);
    }
    return *this;
  }
  fixed& Saturate(const fixed& cap) {
    if (*this > cap) {
      *this = cap;
    }
    return *this;
  }
  fixed& Sub_Saturate(const unsigned cap) {
    if (raw_ >= cap * 256) {
      raw_ = static_cast<uint16_t>(cap * 256 - 1);
    }
    return *this;
  }
  fixed& Sub_Saturate(const fixed& cap) {
    if (*this >= cap) {
      raw_ = static_cast<uint16_t>(cap.raw_ - 1);
    }
    return *this;
  }
  fixed& Inverse() {
    *this = fixed(1) / *this;
    return *this;
  }

  // Returns the decimal string representation (e.g., "1.5", "0.75", "3").
  std::string AsString() const;

  // Common fixed-point constants (defined after the class is complete).
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
  uint16_t raw_{0};
};

// constinit: compile-time initialization, no global constructor.
// inline: safe for header definitions (ODR).
inline constinit const fixed fixed::_1_2{1, 2};
inline constinit const fixed fixed::_1_3{1, 3};
inline constinit const fixed fixed::_1_4{1, 4};
inline constinit const fixed fixed::_3_4{3, 4};
inline constinit const fixed fixed::_2_3{2, 3};

#endif  // CNC_RED_ALERT_TECH_FIXED_H_
