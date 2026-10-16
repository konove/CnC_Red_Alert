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

// File: Flasher's countdown.
//
// Originally FLASHER.CPP by Joe L. Bostic, started May 28, 1994; made
// independent of the object it flashes on June 20, 1994.

#include "game/sim/flasher.h"

#include <algorithm>
#include <cstdint>

namespace game {

void Flasher::Start(const int frames) {
  frames_left_ = static_cast<int8_t>(std::clamp(frames, 0, kMaxFrames));
  if (frames_left_ == 0) {
    // Process() only clears the flag while it counts down.
    brightened_ = false;
  }
}

bool Flasher::Process() {
  if (frames_left_ == 0) {
    return false;
  }
  --frames_left_;
  // The parity of what is left decides the frame, so a count of 1 ends the
  // flash on an unbrightened frame.
  brightened_ = frames_left_ % 2 != 0;
  return true;
}

}  // namespace game
