// File: ScriptedPlayer, a list of key presses and clicks played into the
// game as it ends its frames.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_PLAYER_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_PLAYER_H_

#include <cstddef>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
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
  // A player that moves no clock: the game keeps real time and the frames
  // come as fast as the display presents them. For a run that has to keep
  // pace with something outside it, such as another instance of the game on
  // the network; what frame a step lands on then differs from run to run.
  ScriptedPlayer(Display& display ABSL_ATTRIBUTE_LIFETIME_BOUND,
                 KeyBuffer& keyboard ABSL_ATTRIBUTE_LIFETIME_BOUND,
                 HardwareCursor& mouse ABSL_ATTRIBUTE_LIFETIME_BOUND);
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

  // Queues a press of `key`, a bare key number, and its release. Return,
  // Backspace, Tab, Esc and Space carry the character they type, as they do
  // from a real keyboard; for letters and digits use Type().
  ScriptedPlayer& Key(KeyNumber key) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues `key`, a bare key number, going down and staying down, and its
  // coming up: for Shift, Ctrl and Alt around other steps, and for anything
  // the game asks about as held (IsShiftDown(), KeyBuffer::IsDown()). A key
  // pressed or a button clicked in between carries the modifiers held.
  ScriptedPlayer& KeyDown(KeyNumber key) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  ScriptedPlayer& KeyUp(KeyNumber key) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues the key presses that type `text`, each carrying its character the
  // way SDL's text input delivers it, so an upper-case letter arrives as
  // one. All of them are queued on the same frame; a loop that reads an
  // event a turn takes two turns a character. `text` may hold letters, digits
  // and spaces.
  ScriptedPlayer& Type(std::string_view text) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues a move of the pointer to `x`, `y`, in game pixels.
  ScriptedPlayer& MoveTo(int x, int y) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues a move to `x`, `y` and a click of `button` there: the press and
  // the release.
  ScriptedPlayer& Click(int x, int y, MouseButton button = MouseButton::kLeft)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Queues a move to `x`, `y` and one half of a click there, for a drag:
  // Press(), MoveTo() on later frames, then Release(). The button reads as
  // held down (IsLeftButtonDown()) in between.
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

  // Makes a loop that polls for input `pumps` times without ending a frame
  // get a frame ended for it, so that a loop which only waits for a key --
  // and would never see one, since a script moves on frames -- goes round
  // like the others. 0, the default, ends no frames: such a loop is cut
  // short instead. A scripted run of the whole game sets this; a test of one
  // dialog wants to hear about a loop that does not present.
  void set_idle_pumps_per_frame(int pumps) { idle_pumps_per_frame_ = pumps; }

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

  // Queues a press of `key` that typed `character`, '\0' for none, and the
  // key's release.
  void PutKey(KeyNumber key, char character);

  // Moves the pointer and queues one half of a click where it now is.
  void PutClick(MouseButton button, bool release, int x, int y);

  Display* display_;
  KeyBuffer* keyboard_;
  HardwareCursor* mouse_;
  engine::platform::ManualClock* clock_;  // nullptr: real time

  // In frame order; steps of one frame keep the order they were queued in.
  std::vector<Step> steps_;
  size_t next_step_ = 0;  // index of the first step not yet played
  int queue_frame_ = 1;   // the frame At() last named
  int frame_ = 0;         // frames ended so far
  int frame_budget_ = kDefaultFrameBudget;
  int pumps_since_frame_ = 0;
  // The buttons the player is holding down: each from its Press() to its
  // Release().
  class HeldButtons final : public MouseButtons {
   public:
    [[nodiscard]] bool IsLeftDown() const override;
    [[nodiscard]] bool IsRightDown() const override;

    bool left = false;
    bool right = false;
  };

  // The keys the player is holding down: each from its KeyDown() to its
  // KeyUp().
  class HeldKeys final : public KeyboardState {
   public:
    [[nodiscard]] bool IsShiftDown() const override;
    [[nodiscard]] bool IsCtrlDown() const override;
    [[nodiscard]] bool IsAltDown() const override;
    [[nodiscard]] bool IsKeyDown(int key) const override;

    std::set<int> keys;  // bare key codes
  };

  // What IsLeftButtonDown() and IsRightButtonDown() report while the player
  // exists, and the mouse buttons they reported before, put back when it
  // goes.
  HeldButtons held_;
  MouseButtons* previous_buttons_;
  // The same for the keyboard: IsShiftDown(), KeyBuffer::IsDown() and the
  // modifier bits of a queued key.
  HeldKeys held_keys_;
  KeyboardState* previous_keyboard_;
  int idle_pumps_per_frame_ = 0;  // 0 for never
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_SCRIPTED_PLAYER_H_
