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
  // Default constructor leaves value uninitialized (required for memcpy-based
  // serialization and NoInit patterns).
  fixed() {}

  // Constructs from a fraction (e.g., fixed(3, 4) = 0.75). Zero denominator
  // yields zero.
  fixed(int numerator, int denominator);

  // Constructs from a whole number (fractional part set to zero).
  explicit fixed(const int value) {
    Data.Composite.Fraction = 0;
    Data.Composite.Whole = static_cast<unsigned char>(value);
  }

  // Parses a decimal string ("1.5") or a percentage string ("75%").
  // A null pointer yields zero.
  explicit fixed(const char* ascii);

  // Returns the value rounded to the nearest whole integer.
  int ToInt() const {
    return (static_cast<unsigned>(Data.Raw) + 256 / 2) / 256;
  }

  // Resets the value to zero.
  void Clear() { Data.Raw = 0; }

  // In-place arithmetic operators.
  fixed& operator*=(const fixed& rvalue) {
    Data.Raw = static_cast<unsigned short>(static_cast<int>(Data.Raw) *
                                           rvalue.Data.Raw / 256);
    return *this;
  }
  fixed& operator*=(const int rvalue) {
    Data.Raw = static_cast<unsigned short>(Data.Raw * rvalue);
    return *this;
  }
  fixed& operator/=(const fixed& rvalue) {
    if (rvalue.Data.Raw != 0 && rvalue.Data.Raw != 256) {
      Data.Raw = static_cast<unsigned short>(static_cast<int>(Data.Raw) * 256 /
                                             rvalue);
    }
    return *this;
  }
  fixed& operator/=(const int rvalue) {
    if (rvalue) {
      Data.Raw =
          static_cast<unsigned short>(static_cast<unsigned>(Data.Raw) / rvalue);
    }
    return *this;
  }
  fixed& operator+=(const fixed& rvalue) {
    Data.Raw += rvalue.Data.Raw;
    return *this;
  }
  fixed& operator+=(const int rvalue) {
    Data.Raw += static_cast<unsigned short>(rvalue * 256);
    return *this;
  }
  fixed& operator-=(const fixed& rvalue) {
    Data.Raw -= rvalue.Data.Raw;
    return *this;
  }
  fixed& operator-=(const int rvalue) {
    Data.Raw -= static_cast<unsigned short>(rvalue * 256);
    return *this;
  }

  // Arithmetic operators. Integer overloads are more efficient than
  // fixed-point and return int rounded to nearest whole value.
  fixed operator*(const fixed& rvalue) const {
    fixed temp = *this;
    temp.Data.Raw =
        static_cast<unsigned short>(static_cast<int>(temp.Data.Raw) *
                                    static_cast<int>(rvalue.Data.Raw) / 256);
    return temp;
  }
  int operator*(const int rvalue) const {
    return (static_cast<unsigned>(Data.Raw) * rvalue + 256 / 2) / 256;
  }
  fixed operator/(const fixed& rvalue) const {
    fixed temp = *this;
    if (rvalue.Data.Raw != 0 && rvalue.Data.Raw != 256) {
      temp.Data.Raw = static_cast<unsigned short>(
          static_cast<int>(temp.Data.Raw) * 256 / rvalue.Data.Raw);
    }
    return temp;
  }
  int operator/(const int rvalue) const {
    if (rvalue) {
      return (static_cast<unsigned>(Data.Raw) + 256 / 2) /
             (static_cast<unsigned>(rvalue) * 256);
    }
    return this->ToInt();
  }
  fixed operator+(const fixed& rvalue) const {
    fixed temp = *this;
    temp += rvalue;
    return temp;
  }
  int operator+(const int rvalue) const {
    return (static_cast<unsigned>(Data.Raw) + 256 / 2) / 256 + rvalue;
  }
  fixed operator-(const fixed& rvalue) const {
    fixed temp = *this;
    temp -= rvalue;
    return temp;
  }
  int operator-(const int rvalue) const {
    return (static_cast<unsigned>(Data.Raw) + 256 / 2) / 256 - rvalue;
  }

  // Avoids MSVC ambiguity between int and fixed overloads.
  int operator*(const unsigned short rvalue) const {
    return *this * static_cast<int>(rvalue);
  }

  // Shift operators for efficient multiply/divide by powers of 2.
  fixed& operator>>=(const unsigned rvalue) {
    Data.Raw >>= rvalue;
    return *this;
  }
  fixed& operator<<=(const unsigned rvalue) {
    Data.Raw <<= rvalue;
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
  bool operator==(const fixed& rvalue) const {
    return Data.Raw == rvalue.Data.Raw;
  }
  bool operator!=(const fixed& rvalue) const {
    return Data.Raw != rvalue.Data.Raw;
  }
  bool operator<(const fixed& rvalue) const {
    return Data.Raw < rvalue.Data.Raw;
  }
  bool operator>(const fixed& rvalue) const {
    return Data.Raw > rvalue.Data.Raw;
  }
  bool operator<=(const fixed& rvalue) const {
    return Data.Raw <= rvalue.Data.Raw;
  }
  bool operator>=(const fixed& rvalue) const {
    return Data.Raw >= rvalue.Data.Raw;
  }
  bool operator!() const { return Data.Raw == 0; }

  // Comparison to integers (scales integer to 8.8 for accurate comparison).
  bool operator<(const int rvalue) const { return Data.Raw < rvalue * 256; }
  bool operator>(const int rvalue) const { return Data.Raw > rvalue * 256; }
  bool operator<=(const int rvalue) const { return Data.Raw <= rvalue * 256; }
  bool operator>=(const int rvalue) const { return Data.Raw >= rvalue * 256; }
  bool operator==(const int rvalue) const { return Data.Raw == rvalue * 256; }
  bool operator!=(const int rvalue) const { return Data.Raw != rvalue * 256; }

  // Commutative friend operators for int-on-left expressions (e.g., 5 * f).
  friend int operator*(const int lvalue, const fixed& rvalue) {
    return rvalue * lvalue;
  }
  friend int operator/(const int lvalue, const fixed& rvalue) {
    if (rvalue.Data.Raw == 0 || rvalue.Data.Raw == 256) {
      return lvalue;
    }
    return (static_cast<unsigned>(lvalue * 256) + 256 / 2) / rvalue.Data.Raw;
  }
  friend int operator+(const int lvalue, const fixed& rvalue) {
    return rvalue + lvalue;
  }
  friend int operator-(const int lvalue, const fixed& rvalue) {
    return (lvalue * 256 - rvalue.Data.Raw + 256 / 2) / 256;
  }
  friend bool operator<(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 < rvalue.Data.Raw;
  }
  friend bool operator>(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 > rvalue.Data.Raw;
  }
  friend bool operator<=(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 <= rvalue.Data.Raw;
  }
  friend bool operator>=(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 >= rvalue.Data.Raw;
  }
  friend bool operator==(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 == rvalue.Data.Raw;
  }
  friend bool operator!=(const unsigned lvalue, const fixed& rvalue) {
    return lvalue * 256 != rvalue.Data.Raw;
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

  // Avoids MSVC ambiguity between int and fixed overloads.
  friend int operator*(const unsigned short lvalue, const fixed& rvalue) {
    return rvalue * static_cast<int>(lvalue);
  }

  // Rounding, clamping, and inversion helpers.
  void Round_Up() {
    Data.Raw += static_cast<unsigned short>(256 - 1);
    Data.Composite.Fraction = 0;
  }
  void Round_Down() { Data.Composite.Fraction = 0; }
  void Round() {
    if (Data.Composite.Fraction >= 256 / 2) {
      Round_Up();
    }
    Round_Down();
  }
  void Saturate(const unsigned cap) {
    if (Data.Raw > cap * 256) {
      Data.Raw = static_cast<unsigned short>(cap * 256);
    }
  }
  void Saturate(const fixed& cap) {
    if (*this > cap) {
      *this = cap;
    }
  }
  void Sub_Saturate(const unsigned cap) {
    if (Data.Raw >= cap * 256) {
      Data.Raw = static_cast<unsigned short>(cap * 256 - 1);
    }
  }
  void Sub_Saturate(const fixed& cap) {
    if (*this >= cap) {
      Data.Raw = static_cast<unsigned short>(cap.Data.Raw - 1);
    }
  }
  void Inverse() { *this = fixed(1) / *this; }

  // Non-member versions that return a modified copy.
  friend fixed Round_Up(const fixed& value) {
    fixed temp = value;
    temp.Round_Up();
    return temp;
  }
  friend fixed Round_Down(const fixed& value) {
    fixed temp = value;
    temp.Round_Down();
    return temp;
  }
  friend fixed Round(const fixed& value) {
    fixed temp = value;
    temp.Round();
    return temp;
  }
  friend fixed Saturate(const fixed& value, const unsigned cap) {
    fixed temp = value;
    temp.Saturate(cap);
    return temp;
  }
  friend fixed Saturate(const fixed& value, const fixed& cap) {
    fixed temp = value;
    temp.Saturate(cap);
    return temp;
  }
  friend fixed Sub_Saturate(const fixed& value, const unsigned cap) {
    fixed temp = value;
    temp.Sub_Saturate(cap);
    return temp;
  }
  friend fixed Sub_Saturate(const fixed& value, const fixed& cap) {
    fixed temp = value;
    temp.Sub_Saturate(cap);
    return temp;
  }
  friend fixed Inverse(const fixed& value) {
    fixed temp = value;
    temp.Inverse();
    return temp;
  }

  // Writes the decimal representation to buffer. Returns the number of
  // characters written (excluding null terminator). If buffer_size is -1,
  // the buffer is assumed to be large enough.
  int To_ASCII(char* buffer, int buffer_size = -1) const;

  // Returns a pointer to a static buffer containing the decimal representation.
  // Valid until the next call.
  const char* As_ASCII() const;

  // Common fixed-point constants.
  static const fixed _1_2;  // 1/2
  static const fixed _1_3;  // 1/3
  static const fixed _1_4;  // 1/4
  static const fixed _3_4;  // 3/4
  static const fixed _2_3;  // 2/3

 private:
  // 8.8 representation: Whole is the high byte, Fraction is the low byte.
  // Raw provides direct access to the full 16-bit value for arithmetic.
  union {
    struct {
      unsigned char
          Fraction;  // Low byte: fractional part (0-255 = 0/256 to 255/256).
      unsigned char Whole;  // High byte: integer part (0-255).
    } Composite;
    unsigned short Raw;  // Full 16-bit value (Whole * 256 + Fraction).
  } Data;
};

#endif  // CNC_RED_ALERT_TECH_FIXED_H_
