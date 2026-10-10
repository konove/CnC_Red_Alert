// File: Door, the opening and closing animation of a building's door.

#ifndef CNC_RED_ALERT_GAME_SIM_DOOR_H_
#define CNC_RED_ALERT_GAME_SIM_DOOR_H_

#include <cstdint>

namespace game {

// The state of a door that opens and closes over a number of animation
// stages. It counts its own game frames: AI() must be called once a frame.
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
//   game::Door door;
//   door.OpenDoor(/*frames_per_stage=*/2, /*stage_count=*/10);
//   while (!door.IsDoorOpen()) door.AI();
class Door {
 public:
  Door() = default;

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(stage_, countdown_, frames_per_stage_, final_stage_, state_,
       needs_redraw_);
  }

  // Returns whether the animation has moved on since the flag was last
  // cleared, so the door's owner needs redrawing.
  [[nodiscard]] bool DoorNeedsRedraw() const { return needs_redraw_; }
  // Called by the owner once it has drawn the door (or marked itself to be
  // redrawn); const because the draw routines are.
  void ClearDoorRedraw() const { needs_redraw_ = false; }

  // Advances the animation one game frame; finishes opening or closing, and
  // stops the animation, once the last stage is reached.
  void AI();

  // Returns the frame of the door's animation to draw: 0 when closed, up to
  // `stage_count` - 2 when open (see the class comment).
  [[nodiscard]] int DoorFrame() const;

  [[nodiscard]] bool IsDoorOpening() const { return state_ == State::kOpening; }

  // Starts opening a closed or closing door, advancing one stage every
  // `frames_per_stage` calls of AI(); it is open `frames_per_stage` *
  // (`stage_count` - 1) calls later. `stage_count` must be at least 2 and
  // `frames_per_stage` positive, or the door never opens. Returns false, and
  // changes nothing, if it is already open or opening.
  bool OpenDoor(int frames_per_stage, int stage_count);

  // Starts closing an open or opening door, as OpenDoor() opens one. Returns
  // false if it is already closed or closing.
  bool CloseDoor(int frames_per_stage, int stage_count);

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
  void StartAnimation(State state, int frames_per_stage, int stage_count);

  // The stage the opening or closing in progress has reached, from 0.
  int32_t stage_ = 0;

  // The calls of AI() left until the next stage.
  int32_t countdown_ = 0;

  // The calls of AI() a stage lasts; 0 while the door is open or closed and
  // nothing is animating.
  int32_t frames_per_stage_ = 0;

  // The number of stages of the current door animation, less one: the stage
  // at which the opening or closing is finished.
  int32_t final_stage_ = 0;

  State state_ = State::kClosed;

  // Whether the animation moved on, so the object the door belongs to should
  // be redrawn. Drawing clears it from const draw routines, so it is redraw
  // bookkeeping rather than logical state.
  mutable bool needs_redraw_ = false;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_DOOR_H_
