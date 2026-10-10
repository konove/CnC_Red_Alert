// File: Door, the opening and closing animation of a building's door.

#ifndef CNC_RED_ALERT_GAME_SIM_DOOR_H_
#define CNC_RED_ALERT_GAME_SIM_DOOR_H_

#include "absl/log/check.h"

namespace game {

// The state of a door that opens and closes over a number of animation
// stages, driven by the game's StageClass. Each game has its own StageClass,
// whose saved layout differs, so the door is a template over it; a game names
// its door as `using DoorClass = game::Door<StageClass>;`.
//
// Stage must provide Graphic_Logic(), Fetch_Stage(), Set_Stage(int) and
// Set_Rate(int), and be serializable.
//
// The `stage_count` a door is opened or closed with is one more than the
// number of frames its animation shows: with 4, DoorFrame() runs 0, 1, 2 while
// opening and stays at 2 once open. The callers' counts allow for it (the war
// factory's door art has one frame fewer than the count it passes).
//
// A door that turns round part-way does not continue from the frame it had
// reached: closing always starts from the fully open frame, and opening from
// the closed one.
//
// Example:
//   DoorClass door;
//   door.OpenDoor(/*frames_per_stage=*/2, /*stage_count=*/10);
//   while (!door.IsDoorOpen()) door.AI();
template <class Stage>
class Door {
 public:
  Door() = default;

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(animation_, final_stage_, state_, needs_redraw_);
  }

  // Returns whether the animation has moved on since the flag was last
  // cleared, so the door's owner needs redrawing.
  [[nodiscard]] bool DoorNeedsRedraw() const { return needs_redraw_; }
  // Called by the owner once it has drawn the door (or marked itself to be
  // redrawn); const because the draw routines are.
  void ClearDoorRedraw() const { needs_redraw_ = false; }

  // Advances the animation one game frame; finishes opening or closing, and
  // stops the animation, once the last stage is reached.
  void AI() {
    if (animation_.Graphic_Logic()) {
      if (animation_.Fetch_Stage() >= final_stage_) {
        animation_.Set_Rate(0);
        if (state_ == State::kOpening) {
          state_ = State::kOpen;
        } else if (state_ == State::kClosing) {
          state_ = State::kClosed;
        }
      }
      needs_redraw_ = true;
    }
  }

  // Returns the frame of the door's animation to draw: 0 when closed, up to
  // `stage_count` - 2 when open (see the class comment).
  [[nodiscard]] int DoorFrame() const {
    switch (state_) {
      case State::kClosing:
        return final_stage_ - 1 - animation_.Fetch_Stage();
      case State::kOpening:
        return animation_.Fetch_Stage();
      case State::kOpen:
        return final_stage_ - 1;
      case State::kClosed:
      default:
        return 0;
    }
  }

  [[nodiscard]] bool IsDoorOpening() const { return state_ == State::kOpening; }

  // Starts opening a closed or closing door, advancing one stage every
  // `frames_per_stage` game frames; it is open `frames_per_stage` *
  // (`stage_count` - 1) frames later. `stage_count` must be at least 2 and
  // `frames_per_stage` nonzero, or the door never opens. Returns false, and
  // changes nothing, if it is already open or opening.
  bool OpenDoor(const int frames_per_stage, const int stage_count) {
    if (state_ != State::kClosed && state_ != State::kClosing) {
      return false;
    }
    StartAnimation(State::kOpening, frames_per_stage, stage_count);
    return true;
  }

  // Starts closing an open or opening door, as OpenDoor() opens one. Returns
  // false if it is already closed or closing.
  bool CloseDoor(const int frames_per_stage, const int stage_count) {
    if (state_ != State::kOpen && state_ != State::kOpening) {
      return false;
    }
    StartAnimation(State::kClosing, frames_per_stage, stage_count);
    return true;
  }

  [[nodiscard]] bool IsDoorOpen() const { return state_ == State::kOpen; }
  [[nodiscard]] bool IsDoorClosed() const { return state_ == State::kClosed; }

 private:
  // What the door is doing. A saved game stores the number.
  enum class State {
    kClosed = 0,
    kOpening = 1,
    kOpen = 2,
    kClosing = 3,
  };

  // Restarts the animation from stage 0 on the way to `state`.
  void StartAnimation(const State state, const int frames_per_stage,
                      const int stage_count) {
    // Fewer than two stages would leave an open door on frame -1, and more
    // than 256 do not fit the saved byte.
    DCHECK(stage_count >= 2 && stage_count <= 256);
    DCHECK_GT(frames_per_stage, 0);
    state_ = state;
    final_stage_ = static_cast<unsigned char>(stage_count - 1);
    animation_.Set_Stage(0);
    animation_.Set_Rate(frames_per_stage);
  }

  // Counts the stages of the opening or closing in progress, from 0; stopped
  // (rate 0) while the door is open or closed.
  Stage animation_;

  // The number of stages of the current door animation, less one: the stage
  // at which the opening or closing is finished.
  unsigned char final_stage_ = 0;

  State state_ = State::kClosed;

  // Whether the animation moved on, so the object the door belongs to should
  // be redrawn. Drawing clears it from const draw routines, so it is redraw
  // bookkeeping rather than logical state.
  mutable bool needs_redraw_ = false;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_DOOR_H_
