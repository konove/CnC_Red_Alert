// Implements ScriptedPlayer: playing each frame's steps into the input
// devices, and giving up on a loop the script cannot end.

#include "engine/window/scripted_player.h"

#include <functional>
#include <utility>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "engine/platform/manual_clock.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"

namespace engine::window {

namespace {

// How many times a loop may poll for input without ending a frame. A loop
// that waits for a key this way gets nothing from a script, which only moves
// on frames; no real loop polls this often between two presents.
constexpr int kMaxPumpsPerFrame = 100'000;

}  // namespace

ScriptOverrun::~ScriptOverrun() = default;

ScriptedPlayer::ScriptedPlayer(Display& display, KeyBuffer& keyboard,
                               HardwareCursor& mouse,
                               engine::platform::ManualClock& clock)
    : display_(&display), keyboard_(&keyboard), mouse_(&mouse), clock_(&clock) {
  display_->set_observer(this);
}

ScriptedPlayer::~ScriptedPlayer() { display_->set_observer(nullptr); }

ScriptedPlayer& ScriptedPlayer::At(const int frame) {
  CHECK_GE(frame, queue_frame_) << "a script is written in frame order";
  queue_frame_ = frame;
  return *this;
}

ScriptedPlayer& ScriptedPlayer::Key(const KeyNumber key) {
  return Do([this, key] {
    keyboard_->PutKey(static_cast<int>(key));
    keyboard_->PutKey(static_cast<int>(key), /*release=*/true);
  });
}

ScriptedPlayer& ScriptedPlayer::MoveTo(const int x, const int y) {
  return Do([this, x, y] { mouse_->RecordPosition(x, y); });
}

ScriptedPlayer& ScriptedPlayer::Click(const int x, const int y,
                                      const MouseButton button) {
  return Do([this, x, y, button] {
    PutClick(button, /*release=*/false, x, y);
    PutClick(button, /*release=*/true, x, y);
  });
}

ScriptedPlayer& ScriptedPlayer::Press(const int x, const int y,
                                      const MouseButton button) {
  return Do(
      [this, x, y, button] { PutClick(button, /*release=*/false, x, y); });
}

ScriptedPlayer& ScriptedPlayer::Release(const int x, const int y,
                                        const MouseButton button) {
  return Do([this, x, y, button] { PutClick(button, /*release=*/true, x, y); });
}

ScriptedPlayer& ScriptedPlayer::Do(std::function<void()> action) {
  steps_.push_back({.frame = queue_frame_, .action = std::move(action)});
  return *this;
}

void ScriptedPlayer::OnFrameEnd() {
  ++frame_;
  pumps_since_frame_ = 0;
  clock_->Advance(1);

  while (next_step_ < steps_.size() && steps_.at(next_step_).frame <= frame_) {
    // Counted as played before it runs: an action may throw.
    const Step& step = steps_.at(next_step_++);
    step.action();
  }

  const int last_step_frame = steps_.empty() ? 0 : steps_.back().frame;
  if (frame_ > last_step_frame + frame_budget_) {
    throw ScriptOverrun(absl::StrFormat(
        "the script ended at frame %d and the game is still waiting for input "
        "at frame %d",
        last_step_frame, frame_));
  }
}

void ScriptedPlayer::OnPump() {
  if (++pumps_since_frame_ > kMaxPumpsPerFrame) {
    pumps_since_frame_ = 0;
    throw ScriptOverrun(absl::StrFormat(
        "the game polled for input %d times after frame %d without ending a "
        "frame; a script only moves when a frame ends",
        kMaxPumpsPerFrame, frame_));
  }
}

void ScriptedPlayer::PutClick(const MouseButton button, const bool release,
                              const int x, const int y) {
  mouse_->RecordPosition(x, y);
  keyboard_->PutClick(button, release, x, y);
}

}  // namespace engine::window
