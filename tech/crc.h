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

// CRC checksum computation utilities.

#ifndef CNC_RED_ALERT_TECH_CRC_H_
#define CNC_RED_ALERT_TECH_CRC_H_

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

// Modern constexpr CRC accumulator engine for computing checksums.
//
// Type-safe replacement for CRCEngine that avoids union type punning.
// Processes data via update() methods and accumulates a running checksum.
// The value returned is not a true CRC but shares similar error-detection
// characteristics while being faster to compute.
//
// Example:
//   CrcEngine crc;
//   crc.update(buffer, length);
//   uint32_t checksum = crc.value();
class CrcEngine {
 public:
  explicit constexpr CrcEngine(const uint32_t initial = 0) noexcept
      : crc_(initial) {}

  // Submits a single byte to the accumulator.
  constexpr CrcEngine& Update(const uint8_t datum) noexcept {
    buffer_[index_++] = datum;
    if (index_ == sizeof(uint32_t)) {
      crc_ = std::rotl(crc_, 1) + CurrentBufferAsInt();
      index_ = 0;
      buffer_.fill(0);
    }
    return *this;
  }

  // Submits a string view to the accumulator.
  constexpr CrcEngine& Update(const std::string_view str) noexcept {
    for (const char c : str) {
      Update(static_cast<uint8_t>(c));
    }
    return *this;
  }

  // Submits a span of bytes to the accumulator.
  constexpr CrcEngine& Update(const std::span<const uint8_t> data) noexcept {
    for (const auto& byte : data) {
      Update(byte);
    }
    return *this;
  }

  // Returns the current CRC value.
  [[nodiscard]] constexpr uint32_t Value() const noexcept {
    if (index_ > 0) {
      return std::rotl(crc_, 1) + CurrentBufferAsInt();
    }
    return crc_;
  }

  // Computes CRC of a string in a single call.
  [[nodiscard]] static constexpr uint32_t Compute(
      const std::string_view str) noexcept {
    return CrcEngine().Update(str).Value();
  }

  // Computes CRC of a byte span in a single call.
  [[nodiscard]] static constexpr uint32_t Compute(
      const std::span<const uint8_t> data) noexcept {
    return CrcEngine().Update(data).Value();
  }

 private:
  uint32_t crc_;
  size_t index_ = 0;
  std::array<uint8_t, 4> buffer_{};

  // ReSharper disable once CppDFAUnreachableFunctionCall
  [[nodiscard]] constexpr uint32_t CurrentBufferAsInt() const noexcept {
    return static_cast<uint32_t>(buffer_[0]) |
           static_cast<uint32_t>(buffer_[1]) << 8 |
           static_cast<uint32_t>(buffer_[2]) << 16 |
           static_cast<uint32_t>(buffer_[3]) << 24;
  }
};

#endif  // CNC_RED_ALERT_TECH_CRC_H_
