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

// File: Flasher, the flash that shows an object has been picked as a
// target.
//
// Originally FLASHER.H by Joe L. Bostic, started May 28, 1994.

#ifndef CNC_RED_ALERT_GAME_SIM_FLASHER_H_
#define CNC_RED_ALERT_GAME_SIM_FLASHER_H_

#include <cstdint>

namespace game {

// Makes an object flash when it is clicked as a target: for a number of game
// frames it is drawn brightened on every other one.
//
// Example:
//   flasher.Start(7);
//   if (flasher.Process()) { /* redraw, brightened if IsBrightened() */ }
class Flasher {
 public:
  // The longest flash, in frames. A save holds the count in one byte, as an
  // int8_t.
  static constexpr int kMaxFrames = 127;

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    // The save still holds the brightened flag after the count. It is derived
    // from the count now, so a load ignores it, which also clears the flag an
    // older Start() could leave set with no frames left.
    bool brightened = IsBrightened();
    ar(frames_left_, brightened);
    if constexpr (Archive::kIsReading) {
      // A negative count, a byte past 127, is a damaged save.
      if (frames_left_ < 0) {
        ar.Fail("invalid flash count");
        *this = Flasher();
      }
    }
  }

  // Starts flashing for `frames` calls of Process(), replacing what was left
  // of an earlier flash. More than kMaxFrames flashes for kMaxFrames; 0 or
  // less stops the flash.
  void Start(int frames);

  // Steps the flash by one game frame; the owner calls it once a frame.
  // Returns true when the object has to be redrawn: on every frame of the
  // flash, the one that ends it included.
  bool Process();

  // Whether the object is to be drawn brightened on this frame, as feedback
  // to the player that it was targeted: while the count left is odd, so on
  // every other frame of the flash and never once it is over.
  [[nodiscard]] bool IsBrightened() const { return frames_left_ % 2 != 0; }

 private:
  // The Process() calls left in the flash. It counts down to zero and stops
  // there.
  int8_t frames_left_{0};
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_FLASHER_H_
