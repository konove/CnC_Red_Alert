// File: Door, the opening and closing animation of a building's door.

#ifndef CNC_RED_ALERT_GAME_SIM_DOOR_H_
#define CNC_RED_ALERT_GAME_SIM_DOOR_H_

#include <cstdint>

namespace game {

// The state of a door that opens and closes over a number of animation
// frames. It counts its own game frames: AI() must be called once a frame,
// and says when the owner has to redraw.
//
// A door opened with a `frame_count` of 3 shows Frame() 0, 1, 2 while opening
// and stays at 2 once open. Each frame is a stage of the animation; the last
// one is shown for a stage before the door counts as open.
//
// A door that turns round part-way carries on from the frame it shows, so it
// is closed or open again sooner than after a whole animation.
//
// Example:
//   game::Door door;
//   door.Open(/*frames_per_stage=*/2, /*frame_count=*/9);
//   while (!door.IsOpen()) door.AI();
class Door {
 public:
  Door() = default;

  // Saved-game support. A door loaded with values no door can have (a
  // damaged save) is closed and still.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(stage_, countdown_, frames_per_stage_, frame_count_, state_);
    if (Archive::kIsReading && !IsConsistent()) {
      *this = Door();
    }
  }

  // Advances the animation one game frame; finishes opening or closing, and
  // stops the animation, once the last stage is reached. Returns whether the
  // door moved on a stage, so its owner needs redrawing.
  bool AI();

  // Returns the frame of the door's animation to draw: 0 when closed, up to
  // `frame_count` - 1 when open.
  [[nodiscard]] int Frame() const;

  [[nodiscard]] bool IsOpening() const { return state_ == State::kOpening; }

  // Starts opening a closed or closing door, advancing one stage every
  // `frames_per_stage` calls of AI(). A closed door is open
  // `frames_per_stage` * `frame_count` calls later, a closing one sooner by
  // the frames it has not yet closed. Both arguments must be positive, or the
  // door never opens. Returns false, and changes nothing, if it is already
  // open or opening.
  bool Open(int frames_per_stage, int frame_count);

  // Starts closing an open or opening door, as Open() opens one. Returns
  // false if it is already closed or closing.
  bool Close(int frames_per_stage, int frame_count);

  [[nodiscard]] bool IsOpen() const { return state_ == State::kOpen; }
  [[nodiscard]] bool IsClosed() const { return state_ == State::kClosed; }

 private:
  // What the door is doing. A saved game stores the number.
  enum class State {
    kClosed = 0,
    kOpening = 1,
    kOpen = 2,
    kClosing = 3,
  };

  // Returns whether the members hold what Open(), Close() and AI() can leave
  // in them. The archive reads `state_` as a number without checking it.
  [[nodiscard]] bool IsConsistent() const;

  // Starts the animation towards `state` (opening or closing) at the stage
  // that shows the frame the door shows now.
  void StartAnimation(State state, int frames_per_stage, int frame_count);

  // The stage the opening or closing in progress has reached, from 0.
  int32_t stage_ = 0;

  // The calls of AI() left until the next stage.
  int32_t countdown_ = 0;

  // The calls of AI() a stage of the current, or the last, animation lasts.
  int32_t frames_per_stage_ = 0;

  // The number of frames the current, or the last, door animation shows,
  // which is also the stage at which the opening or closing is finished.
  int32_t frame_count_ = 0;

  State state_ = State::kClosed;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_DOOR_H_
