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
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "base/types.h"

// Conversion between Fixed and the decimal text INI files hold.

// Parses leading digits from a string_view into an int. Returns 0 on failure.
static int ParseInt(const std::string_view s) {
  int value = 0;
  // from_chars takes a begin/end pair, not a terminated C string.
  // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
  std::from_chars(s.data(), s.substr(s.size()).data(), value);
  return value;
}

Fixed Fixed::FromString(const std::string_view str_in) {
  const auto str = absl::StripLeadingAsciiWhitespace(str_in);
  if (str.empty()) {
    return {};
  }

  // Percentage: "75%" → 75 * 256 / 100 ≈ 0.75 in 8.8 fixed point.
  if (str.ends_with('%')) {
    Fixed result;
    result.raw_ = static_cast<uint16_t>(
        int64_t{ParseInt(str.substr(0, str.size() - 1))} * 256 / 100);
    return result;
  }

  // Parse decimal number (e.g., "1.5", ".016", "3").
  Fixed result;
  const auto dot = str.find('.');
  const auto whole_part = str.substr(0, dot);
  if (!whole_part.empty()) {
    // Only the low byte fits, as with the original's (unsigned char) cast.
    result.raw_ =
        static_cast<uint16_t>(static_cast<uint8_t>(ParseInt(whole_part)) << 8);
  }

  if (dot != std::string_view::npos && dot + 1 < str.size()) {
    // Every multiple of 1/256 is exact in eight decimal places (1/256 is
    // 0.00390625), so digits past the eighth can never change the truncated
    // result; dropping them keeps the numerator and base within int.
    constexpr std::string_view::size_type kMaxFractionDigits = 8;
    const auto frac_part = str.substr(dot + 1, kMaxFractionDigits);
    // The digits are a numerator over 10^digits: ".016" is 16/1000.
    int frac = 0;
    // from_chars takes a begin/end pair, not a terminated C string.
    // NOLINTBEGIN(bugprone-suspicious-stringview-data-usage)
    const auto [ptr, ec] = std::from_chars(
        frac_part.data(), frac_part.substr(frac_part.size()).data(), frac);
    // NOLINTEND(bugprone-suspicious-stringview-data-usage)
    int base = 1;
    for (base::ssize i = 0; i < ptr - frac_part.data(); ++i) {
      base *= 10;
    }
    result.raw_ |= static_cast<uint16_t>(int64_t{256} * frac / base);
  }

  return result;
}

std::string Fixed::AsString() const {
  if (fraction() == 0) {
    return std::to_string(whole());
  }

  // 1/256 is 0.00390625, so eight decimal places hold every fraction
  // exactly, and FromString() reads the text back to the same value.
  constexpr int kHundredMillionthsPer256th = 390625;
  const int frac = fraction() * kHundredMillionthsPer256th;

  // Strip trailing zeros from the fractional part.
  std::string result = absl::StrFormat("%d.%08d", whole(), frac);
  const auto last_nonzero = result.find_last_not_of('0');
  if (last_nonzero != std::string::npos) {
    result.erase(last_nonzero + 1);
  }
  return result;
}
