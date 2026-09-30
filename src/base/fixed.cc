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

#include "base/fixed.h"

#include <charconv>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "base/types.h"

// Conversion between Fixed and the decimal text INI files hold.

namespace {

struct LeadingInt {
  int value = 0;
  base::ssize length = 0;  // Characters read; zero if there was no number.
};

// Parses the number at the start of `text`, ignoring whatever follows it.
LeadingInt ParseLeadingInt(const std::string_view text) {
  LeadingInt result;
  // from_chars takes a begin/end pair, not a terminated C string.
  const char* const begin = std::to_address(text.begin());
  result.length =
      std::from_chars(begin, std::to_address(text.end()), result.value).ptr -
      begin;
  return result;
}

}  // namespace

Fixed Fixed::FromString(const std::string_view text) {
  // Empty text, and any part with no digits, reads as zero.
  const auto trimmed = absl::StripLeadingAsciiWhitespace(text);

  // Percentage: "75%" is 75/100.
  if (trimmed.ends_with('%')) {
    return {ParseLeadingInt(trimmed.substr(0, trimmed.size() - 1)).value, 100};
  }

  // Decimal: "1.5", ".016", "3". Only the low byte of the whole part fits, as
  // with the original's (unsigned char) cast.
  const auto dot = trimmed.find('.');
  Fixed result(
      static_cast<uint8_t>(ParseLeadingInt(trimmed.substr(0, dot)).value));
  if (dot != std::string_view::npos) {
    // Every multiple of 1/256 is exact in eight decimal places (1/256 is
    // 0.00390625), so digits past the eighth can never change the truncated
    // result; dropping them keeps the numerator and denominator within int.
    constexpr std::string_view::size_type kMaxFractionDigits = 8;
    // The digits are a numerator over 10^digits: ".016" is 16/1000.
    const LeadingInt numerator =
        ParseLeadingInt(trimmed.substr(dot + 1, kMaxFractionDigits));
    int denominator = 1;
    for (base::ssize i = 0; i < numerator.length; ++i) {
      denominator *= 10;
    }
    result.raw_ |= Fixed(numerator.value, denominator).raw_;
  }
  return result;
}

std::string Fixed::ToString() const {
  if (fraction() == 0) {
    return std::to_string(whole());
  }

  // 1/256 is 0.00390625, so eight decimal places hold every fraction
  // exactly, and FromString() reads the text back to the same value.
  constexpr int kHundredMillionthsPer256th = 390625;
  const int hundred_millionths = fraction() * kHundredMillionthsPer256th;

  // Strip trailing zeros from the fractional part.
  std::string result = absl::StrFormat("%d.%08d", whole(), hundred_millionths);
  // The '.' stops the search, so there always is a last non-zero.
  result.erase(result.find_last_not_of('0') + 1);
  return result;
}
