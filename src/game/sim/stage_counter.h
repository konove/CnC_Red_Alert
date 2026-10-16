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

// File: StageCounter, the frame counter behind an object's looping animation.
//
// Originally STAGE.H by Joe L. Bostic, started June 17, 1994.

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

  // Saved-game support. The countdown is saved as the frames it has left, so
  // a loaded counter carries on where it was whatever frame the loading game
  // is at; the game clock must be restored before this is read.
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
  // Returns whether AI(), called in this frame, will move on a stage.
  [[nodiscard]] bool IsAboutToChange() const {
    return countdown_.IsFinished() && rate_ != 0;
  }
  // Moves on a stage when the countdown has run out, and restarts it. Call it
  // once a game frame. Returns whether the stage changed, so the owner knows
  // to redraw. However late it is called, it moves on one stage and counts the
  // next one from now: frames that passed without it are not caught up.
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
  engine::platform::Countdown<FrameTickSource> countdown_;

  // The game frames a stage lasts, which the countdown is restarted with; 0
  // when the stage is not animating.
  int32_t rate_{0};
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_STAGE_COUNTER_H_
