// File: ScriptedPlayer, a list of key presses and clicks played into the
// game as it ends its frames.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_PLAYER_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_PLAYER_H_

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/platform/manual_clock.h"
#include "engine/window/display.h"
#include "engine/window/display_observer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"

namespace engine::window {

// Thrown by a ScriptedPlayer whose script has run out while the game is still
// waiting for input. An exception, which the style guide otherwise avoids,
// because it is the only way out of a modal loop that has no other exit: it
// unwinds the loop and fails the test that ran it, where a flag would leave
// the test hanging until its timeout.
class ScriptOverrun : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
  ~ScriptOverrun() override;

  // Copyable, as a thrown exception has to be.
  ScriptOverrun(const ScriptOverrun&) = default;
  ScriptOverrun& operator=(const ScriptOverrun&) = default;
  ScriptOverrun(ScriptOverrun&&) = default;
  ScriptOverrun& operator=(ScriptOverrun&&) = default;
};

// Plays the part of the person at the keyboard for code that blocks in its
// own loop: a dialog, a menu, a fade. Those loops end a frame on every turn
// (ServiceRealTime() in the games), so the script is written against frames:
// frame 1 is the first one the Display ends after the player is made. On each
// frame the player moves its clock one tick, so the loop's timers run at the
// 60 frames a second they expect however fast the machine is, and then queues
// that frame's keys and clicks for the loop to read on its next turn.
//
// Queue the whole script before calling the code under test, since that call
// does not return until the script has driven it to its end. A loop still
// running `frame_budget` frames after the last step throws ScriptOverrun, as
// does one that polls for input 100,000 times without ending a frame.
//
// It attaches itself to the Display as its observer and detaches when
// destroyed; the Display, the devices and the clock must outlive it.
//
// Example:
//   ScriptedPlayer player(display, keyboard, mouse, clock);
//   player.At(2).Click(120, 80).At(5).Key(KN_RETURN);
//   RunTheDialog();
//   EXPECT_TRUE(player.finished());
class ScriptedPlayer final : public DisplayObserver {
 public:
  // How long a loop may keep running after the last step: ten seconds of
  // frames.
  static constexpr int kDefaultFrameBudget = 600;

  ScriptedPlayer(Display& display ABSL_ATTRIBUTE_LIFETIME_BOUND,
                 KeyBuffer& keyboard ABSL_ATTRIBUTE_LIFETIME_BOUND,
                 HardwareCursor& mouse ABSL_ATTRIBUTE_LIFETIME_BOUND,
                 engine::platform::ManualClock& clock
                     ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~ScriptedPlayer() override;

  ScriptedPlayer(const ScriptedPlayer&) = delete;
  ScriptedPlayer& operator=(const ScriptedPlayer&) = delete;
  ScriptedPlayer(ScriptedPlayer&&) = delete;
  ScriptedPlayer& operator=(ScriptedPlayer&&) = delete;

  // Makes `frame` the frame the steps queued after this call happen on.
  // Frames count from 1 and a script is written in order: `frame` must not be
  // earlier than the one before it. Steps queued before any At() happen on
  // frame 1.
  ScriptedPlayer& At(int frame) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues a press of `key`, a bare key number, and its release.
  ScriptedPlayer& Key(KeyNumber key) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues a move of the pointer to `x`, `y`, in game pixels.
  ScriptedPlayer& MoveTo(int x, int y) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues a move to `x`, `y` and a click of `button` there: the press and
  // the release.
  ScriptedPlayer& Click(int x, int y, MouseButton button = MouseButton::kLeft)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues a move to `x`, `y` and one half of a click there, for a drag:
  // Press(), MoveTo() on later frames, then Release().
  ScriptedPlayer& Press(int x, int y, MouseButton button = MouseButton::kLeft)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  ScriptedPlayer& Release(int x, int y, MouseButton button = MouseButton::kLeft)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues a call of `action`, for a look at the screen or the game's state
  // while the loop under test is still running.
  ScriptedPlayer& Do(std::function<void()> action)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Sets how many frames a loop may run past the last step before
  // ScriptOverrun is thrown.
  void set_frame_budget(int frames) { frame_budget_ = frames; }

  // The number of frames the Display has ended since the player was made.
  [[nodiscard]] int frame() const { return frame_; }
  // Whether every step has been played.
  [[nodiscard]] bool finished() const { return next_step_ == steps_.size(); }

  void OnFrameEnd() override;
  void OnPump() override;

 private:
  struct Step {
    int frame;  // the frame it happens on, from 1
    std::function<void()> action;
  };

  // Moves the pointer and queues one half of a click where it now is.
  void PutClick(MouseButton button, bool release, int x, int y);

  Display* display_;
  KeyBuffer* keyboard_;
  HardwareCursor* mouse_;
  engine::platform::ManualClock* clock_;

  // In frame order; steps of one frame keep the order they were queued in.
  std::vector<Step> steps_;
  size_t next_step_ = 0;  // index of the first step not yet played
  int queue_frame_ = 1;   // the frame At() last named
  int frame_ = 0;         // frames ended so far
  int frame_budget_ = kDefaultFrameBudget;
  int pumps_since_frame_ = 0;
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_PLAYER_H_
