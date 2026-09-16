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

#include "tech/fixed.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"

// Parses leading digits from a string_view into an int. Returns 0 on failure.
static int ParseInt(const std::string_view s) {
  int value = 0;
  // from_chars takes a begin/end pair, not a terminated C string.
  // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
  std::from_chars(s.data(), s.substr(s.size()).data(), value);
  return value;
}

fixed fixed::FromString(const std::string_view str_in) {
  const auto str = absl::StripLeadingAsciiWhitespace(str_in);
  if (str.empty()) {
    return {};
  }

  // Percentage: "75%" → 75 * 256 / 100 ≈ 0.75 in 8.8 fixed point.
  if (str.ends_with('%')) {
    fixed result;
    result.raw_ = static_cast<uint16_t>(
        ParseInt(str.substr(0, str.size() - 1)) * 256 / 100);
    return result;
  }

  // Parse decimal number (e.g., "1.5", ".016", "3").
  fixed result;
  const auto dot = str.find('.');
  const auto whole_part = str.substr(0, dot);
  if (!whole_part.empty()) {
    result.raw_ = static_cast<uint16_t>(ParseInt(whole_part) * 256);
  }

  if (dot != std::string_view::npos && dot + 1 < str.size()) {
    const auto frac_part = str.substr(dot + 1);
    // Count digits parsed to determine the decimal base (10^n).
    int frac = 0;
    // from_chars takes a begin/end pair, not a terminated C string.
    // NOLINTBEGIN(bugprone-suspicious-stringview-data-usage)
    const auto [ptr, ec] = std::from_chars(
        frac_part.data(), frac_part.substr(frac_part.size()).data(), frac);
    // NOLINTEND(bugprone-suspicious-stringview-data-usage)
    int base = 1;
    for (std::ptrdiff_t i = 0; i < ptr - frac_part.data(); ++i) {
      base *= 10;
    }
    result.raw_ |= static_cast<uint16_t>(256 * frac / base);
  }

  return result;
}

std::string fixed::AsString() const {
  if (fraction() == 0) {
    return std::to_string(whole());
  }

  // Convert 8-bit fraction (0-255) to thousandths for decimal display.
  const int frac = fraction() * 1000 / 256;

  // Strip trailing zeros from the fractional part.
  std::string result = absl::StrFormat("%d.%02d", whole(), frac);
  const auto last_nonzero = result.find_last_not_of('0');
  if (last_nonzero != std::string::npos) {
    result.erase(last_nonzero + 1);
  }
  return result;
}
