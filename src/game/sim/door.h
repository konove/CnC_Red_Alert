// File: Door, the opening and closing animation of a building's door.

#ifndef CNC_RED_ALERT_GAME_SIM_DOOR_H_
#define CNC_RED_ALERT_GAME_SIM_DOOR_H_

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
    // A bit-field cannot bind to the archive's reference parameter, so the
    // flag goes through a local.
    bool needs_redraw = needs_redraw_;
    ar(animation_, final_stage_, state_, needs_redraw);
    if constexpr (Archive::kIsReading) {
      needs_redraw_ = needs_redraw;
    }
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
        switch (state_) {
          case kIsOpening:
            state_ = kIsOpen;
            break;
          case kIsClosing:
            state_ = kIsClosed;
            break;
          default:
            break;
        }
      }
      needs_redraw_ = true;
    }
  }

  // Returns the frame of the door's animation to draw: 0 when closed, up to
  // `stage_count` - 2 when open (see the class comment).
  [[nodiscard]] int DoorFrame() const {
    switch (state_) {
      case kIsClosing:
        return final_stage_ - 1 - animation_.Fetch_Stage();
      case kIsClosed:
        return 0;
      case kIsOpening:
        return animation_.Fetch_Stage();
      case kIsOpen:
        return final_stage_ - 1;
      default:
        break;
    }
    return 0;
  }

  [[nodiscard]] bool IsDoorOpening() const { return state_ == kIsOpening; }
  [[nodiscard]] bool IsDoorClosing() const { return state_ == kIsClosing; }

  // Starts opening a closed or closing door, advancing one stage every
  // `frames_per_stage` game frames; it is open `frames_per_stage` *
  // (`stage_count` - 1) frames later. `stage_count` must be at least 2 and
  // `frames_per_stage` nonzero, or the door never opens. Returns false, and
  // changes nothing, if it is already open or opening.
  bool OpenDoor(int frames_per_stage, int stage_count) {
    switch (state_) {
      case kIsClosed:
      case kIsClosing:
        state_ = kIsOpening;
        final_stage_ = static_cast<unsigned char>(stage_count - 1);
        animation_.Set_Stage(0);
        animation_.Set_Rate(frames_per_stage);
        return true;
      default:
        break;
    }
    return false;
  }

  // Starts closing an open or opening door, as OpenDoor() opens one. Returns
  // false if it is already closed or closing.
  bool CloseDoor(int frames_per_stage, int stage_count) {
    switch (state_) {
      case kIsOpen:
      case kIsOpening:
        state_ = kIsClosing;
        final_stage_ = static_cast<unsigned char>(stage_count - 1);
        animation_.Set_Stage(0);
        animation_.Set_Rate(frames_per_stage);
        return true;
      default:
        break;
    }
    return false;
  }

  [[nodiscard]] bool IsDoorOpen() const { return state_ == kIsOpen; }
  [[nodiscard]] bool IsDoorClosed() const { return state_ == kIsClosed; }

 private:
  static constexpr int kIsClosed = 0;   // Door is closed.
  static constexpr int kIsOpening = 1;  // Door is in the process of opening.
  static constexpr int kIsOpen = 2;     // Door is fully open.
  static constexpr int kIsClosing = 3;  // Door is in the process of closing.

  // Counts the stages of the opening or closing in progress, from 0; stopped
  // (rate 0) while the door is open or closed.
  Stage animation_;

  // The number of stages of the current door animation, less one: the stage
  // at which the opening or closing is finished.
  unsigned char final_stage_{0};

  // One of the kIs... values above.
  int state_ = kIsClosed;

  // Whether the animation moved on, so the object the door belongs to should
  // be redrawn. Drawing clears it from const draw routines, so it is redraw
  // bookkeeping rather than logical state.
  mutable bool needs_redraw_ : 1 {false};
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_DOOR_H_
