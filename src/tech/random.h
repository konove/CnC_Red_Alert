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

#ifndef CNC_RED_ALERT_TECH_RANDOM_H_
#define CNC_RED_ALERT_TECH_RANDOM_H_

#include <cstdint>

// A deterministic linear congruential pseudo-random number generator.
//
// The exact recurrence must never change. Multiplayer peers advance the
// simulation in lockstep, and saved games and replays reproduce the same draw
// sequence, so any divergence in the numbers produced causes desync. Only the
// upper 15 bits of each draw are significant (values 0..32767).
//
// Example:
//   RandomClass rng(seed);
//   int roll = rng.InRange(1, 6);
class RandomClass {
 public:
  explicit RandomClass(uint32_t seed = 0);

  // Advances the generator and returns the next raw value in [0, 32767].
  int Next();

  // Returns a uniformly distributed value in the inclusive range [low, high].
  // The span must fit within the 15 significant bits, otherwise the result
  // skews non-random.
  int InRange(int low, int high);

  uint32_t seed() const { return seed_; }
  void set_seed(const uint32_t seed) { seed_ = seed; }

 private:
  // Number of significant random bits produced by each draw.
  static constexpr int kSignificantBits = 15;

  // Mask selecting the low kSignificantBits bits, i.e. the value range of a
  // single draw ([0, kSignificantMask]).
  static constexpr int kSignificantMask = (1 << kSignificantBits) - 1;

  // Coefficients of the linear congruential recurrence (the classic ANSI-C
  // constants). Changing these breaks multiplayer sync and saved games.
  static constexpr uint32_t kMultiplier = 0x41C64E6D;
  static constexpr uint32_t kAddend = 0x00003039;

  // Number of low, least-random bits discarded from the seed on each draw.
  static constexpr int kThrowAwayBits = 10;

  // Generator state. Kept trivially copyable so it can be serialized directly.
  uint32_t seed_;
};

#endif  // CNC_RED_ALERT_TECH_RANDOM_H_
