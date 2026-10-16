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

// File: TypeTally, the per-type counts behind the internet game
// statistics.
//
// Originally UTRACKER.CPP by Steve Tall, June 3-7, 1996.

#include "game/sim/utracker.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/numeric.h"

namespace game {

TypeTally::TypeTally(int type_count)
    : counts_(base::ToSize(type_count)), UnitCount(type_count) {
  Clear();
}

TypeTally::~TypeTally() = default;

void TypeTally::Increment(int type) { counts_.at(base::ToSize(type))++; }

void TypeTally::Decrement(int type) { counts_.at(base::ToSize(type))--; }

void TypeTally::Clear() { std::ranges::fill(counts_, 0); }

int TypeTally::Count(int type) const { return counts_.at(base::ToSize(type)); }

std::vector<std::byte> TypeTally::NetworkBytes() const {
  std::vector<std::byte> bytes;
  bytes.reserve(counts_.size() * sizeof(int32_t));
  for (const int32_t count : counts_) {
    auto value = static_cast<uint32_t>(count);
    if constexpr (std::endian::native == std::endian::little) {
      value = std::byteswap(value);
    }
    const auto big_endian = std::bit_cast<std::array<std::byte, 4>>(value);
    bytes.insert(bytes.end(), big_endian.begin(), big_endian.end());
  }
  return bytes;
}

}  // namespace game
