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

#include <cctype>
#include <cstring>

#include "absl/strings/str_format.h"

fixed fixed::FromString(const char* ascii) {
  if (ascii == nullptr) {
    return fixed{0};
  }

  const char* whole_part = ascii;

  // Skip leading whitespace.
  while (isspace(*ascii)) {
    ascii++;
  }

  // Check for trailing '%' to detect percentage format.
  const char* suffix = ascii;
  while (isdigit(*suffix)) {
    suffix++;
  }

  fixed result;
  // Percentage: "75%" → 75 * 256 / 100 ≈ 0.75 in 8.8 fixed point.
  if (*suffix == '%') {
    result.raw_ = static_cast<uint16_t>(atoi(ascii) * 256 / 100);
  } else {
    result.raw_ = 0;
    if (whole_part && *whole_part != '.') {
      result.raw_ = static_cast<uint16_t>(atoi(whole_part) << 8);
    }

    const char* decimal = strchr(ascii, '.');
    if (decimal) {
      decimal++;
    }
    if (decimal) {
      const int frac = atoi(decimal);

      int base = 1;
      const char* fptr = decimal;
      while (isdigit(*fptr)) {
        fptr++;
        base *= 10;
      }

      result.raw_ |= static_cast<uint16_t>(256 * frac / base);
    }
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
