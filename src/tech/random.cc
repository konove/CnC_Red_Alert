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

#include "tech/random.h"

#include <bit>
#include <utility>

RandomClass::RandomClass(const uint32_t seed) : seed_(seed) {}

int RandomClass::Next() {
  // Advance the seed to the next state in the linear congruential sequence,
  // then return the significant bits. The arithmetic is intentionally 32-bit
  // modular: the recurrence and a bit extraction must match across platforms so
  // that the sequence stays identical for multiplayer sync and saved games.
  seed_ = seed_ * kMultiplier + kAddend;
  return static_cast<int>(seed_ >> kThrowAwayBits & kSignificantMask);
}

// This replaces the functionality of IRandom() in the original Westwood
// library.
int RandomClass::InRange(int low, int high) {
  // A null range has only one possible result.
  if (low == high) {
    return low;
  }

  if (low > high) {
    std::swap(low, high);
  }

  // Build an all-ones bitmask just wide enough to cover the magnitude. Only the
  // low kSignificantBits matter, because that is all Next() produces; any
  // higher bits of the magnitude are ignored. Within that window, bit_width
  // gives the highest set bit plus one, so the mask is the smallest 2^n - 1
  // that covers the (windowed) magnitude. The width floors at one bit so the
  // mask is never empty even when the magnitude has no bits inside the window.
  const int magnitude = high - low & kSignificantMask;
  const int high_bit = magnitude == 0 ? 1 : std::bit_width<unsigned>(magnitude);
  const int mask = (1 << high_bit) - 1;

  // Reject-sample masked draws until one lands within the magnitude. Masking to
  // a power-of-two range keeps every draw uniform; rejection then trims the
  // excess without the bias that taking a modulo of the range would introduce.
  int pick = magnitude + 1;
  while (pick > magnitude) {
    pick = Next() & mask;
  }

  // Bias the in-range pick up to the requested starting point.
  return pick + low;
}
