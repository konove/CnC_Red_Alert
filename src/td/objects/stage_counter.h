/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\stage.h_v   2.15   16 Oct 1995 16:45:16
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : STAGE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : June 17, 1994 *
 *                                                                                             *
 *                  Last Update : June 17, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

// File: StageCounter, the frame counter behind an object's looping animation.

#ifndef CNC_RED_ALERT_TD_OBJECTS_STAGE_COUNTER_H_
#define CNC_RED_ALERT_TD_OBJECTS_STAGE_COUNTER_H_

#include <cstdint>

namespace td {

// An animation stage that counts up by one every `rate` calls of
// AI(): what drives an object's smoke, walking, flapping and
// rocket flames, a cloak fading in and out, and a factory's progress. The
// owner decides what a stage means (usually a shape number) and when to wrap
// it; the stage itself only ever counts up.
//
// The members' types are the saved game's layout.
//
// Example:
//   StageCounter animation;
//   animation.SetRate(3);
//   if (animation.AI()) { /* the stage went up: redraw */ }
class StageCounter {
 public:
  StageCounter() = default;

  // Field-wise saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(stage_, countdown_, rate_);
  }

  [[nodiscard]] int stage() const { return stage_; }
  [[nodiscard]] int rate() const { return rate_; }
  // Keeps the low 16 bits of `stage`.
  void set_stage(const int stage) { stage_ = static_cast<uint16_t>(stage); }
  // Sets the calls of AI() a stage lasts and restarts the
  // countdown to the next one; 0 stops the animation. Keeps the low byte of
  // `rate`, as the original's unsigned char parameter did.
  void SetRate(const int rate) {
    rate_ = countdown_ = static_cast<uint8_t>(rate);
  }
  // Counts down a game frame and moves on a stage when the countdown runs
  // out. Call it once a game frame. Returns whether the stage changed, so the
  // owner knows to redraw.
  bool AI() {
    if (rate_) {
      countdown_--;
      if (!countdown_) {
        stage_++;
        countdown_ = rate_;
        return true;
      }
    }
    return false;
  }

 private:
  // The animation stage of the object, counted from 0.
  uint16_t stage_{0};

  // The calls of AI() left until the next stage. At zero the
  // stage goes up by one and the countdown starts over.
  uint8_t countdown_{0};

  // The calls of AI() a stage lasts, which the countdown is
  // restarted with; 0 when the stage is not animating.
  uint8_t rate_{0};
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_OBJECTS_STAGE_COUNTER_H_
