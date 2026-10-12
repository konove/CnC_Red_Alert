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

/* $Header: /CounterStrike/STAGE.H 1     3/03/97 10:25a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_GAME_SIM_STAGE_COUNTER_H_
#define CNC_RED_ALERT_GAME_SIM_STAGE_COUNTER_H_

#include <cstdint>

#include "engine/platform/timer.h"
#include "game/core/game_clock.h"

namespace game {

// An animation stage that counts up by one every `rate` game frames: what
// drives an object's smoke, walking, flapping and rocket flames, a cloak
// fading in and out, and a factory's progress. The owner decides what a stage
// means (usually a shape number) and when to wrap it; the stage itself only
// ever counts up. The countdown runs on the game's frame clock, so a stage
// keeps time even in a frame where AI() is not called.
//
// Example:
//   StageCounter animation;
//   animation.SetRate(3);
//   if (animation.AI()) { /* the stage went up: redraw */ }
class StageCounter {
 public:
  StageCounter() : countdown_(0) {}

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(stage_, countdown_, rate_);
  }

  [[nodiscard]] int stage() const { return stage_; }
  [[nodiscard]] int rate() const { return rate_; }
  void set_stage(const int stage) { stage_ = stage; }
  // Sets the game frames a stage lasts and restarts the countdown to the next
  // one; 0 stops the animation.
  void SetRate(const int rate) {
    countdown_.Set(rate);
    rate_ = rate;
  }
  // Returns whether the next AI() will move on a stage.
  [[nodiscard]] bool IsAboutToChange() const {
    return countdown_.IsFinished() && rate_ != 0;
  }
  // Moves on a stage when the countdown has run out, and restarts it. Call it
  // once a game frame. Returns whether the stage changed, so the owner knows
  // to redraw.
  bool AI() {
    if (IsAboutToChange()) {
      stage_++;
      countdown_.Set(rate_);
      return true;
    }
    return false;
  }

 private:
  // The animation stage of the object, counted from 0.
  int32_t stage_{0};

  // Counts down the game frames to the next stage. When it is finished the
  // stage goes up by one and it starts over.
  engine::platform::Timer<FrameTickSource> countdown_;

  // The game frames a stage lasts, which the countdown is restarted with; 0
  // when the stage is not animating.
  int32_t rate_{0};
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_STAGE_COUNTER_H_
