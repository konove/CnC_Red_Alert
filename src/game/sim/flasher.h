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

// File: FlasherClass, the flash that shows an object has been picked as a
// target.
//
// Originally FLASHER.H by Joe L. Bostic, started May 28, 1994.

#ifndef CNC_RED_ALERT_GAME_SIM_FLASHER_H_
#define CNC_RED_ALERT_GAME_SIM_FLASHER_H_

#include <algorithm>
#include <cstdint>

namespace game {

// Makes an object flash when it is clicked as a target: for a number of game
// frames it is drawn brightened on every other one.
//
// Example:
//   flasher.Start(7);
//   if (flasher.Process()) { /* redraw, brightened if IsBrightened() */ }
class FlasherClass {
 public:
  // The longest flash, in frames: the most FlashCount's seven bits hold.
  static constexpr int kMaxCount = 127;

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    uint8_t flash_count = FlashCount;
    bool is_blushing = IsBlushing;
    ar(flash_count, is_blushing);
    if constexpr (Archive::kIsReading) {
      // FlashCount is a 7-bit field; a larger count is a damaged save, and
      // storing it would silently drop its high bit.
      if (flash_count > kMaxCount) {
        ar.Fail("invalid flash count");
        return;
      }
      FlashCount = flash_count;
      IsBlushing = is_blushing;
    }
  }

  // Starts flashing for `count` calls of Process(), replacing what was left
  // of an earlier flash. A count past kMaxCount flashes for kMaxCount; a
  // count of 0 or less stops the flash.
  void Start(int count) {
    FlashCount = static_cast<uint8_t>(std::clamp(count, 0, kMaxCount));
    if (FlashCount == 0) {
      // Process() only clears the flag while it counts down.
      IsBlushing = false;
    }
  }

  // Steps the flash by one game frame; the owner calls it once a frame.
  // Returns true when the object has to be redrawn: on every frame of the
  // flash, the one that ends it included.
  bool Process();

  // Whether the object is to be drawn brightened on this frame.
  [[nodiscard]] bool IsBrightened() const { return IsBlushing; }

 private:
  // The Process() calls left in the flash. It counts down to zero and stops
  // there; Process() brightens the object whenever the count it leaves is odd.
  uint8_t FlashCount : 7 {0};

  // Whether the object is drawn brightened, as feedback to the player that
  // it was targeted: true on every other frame of the flash, false once the
  // flash is over.
  bool IsBlushing : 1 {false};
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_FLASHER_H_
