// Implements ScriptedPlayer: playing each frame's steps into the input
// devices, and giving up on a loop the script cannot end.

#include "engine/window/scripted_player.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "absl/strings/ascii.h"
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

// The character the keys that type the same thing on every layout type; '\0'
// for any other key. The games read it from the event: without SDL's keyboard
// there is no layout to look a key up in.
char ControlCharacter(const KeyNumber key) {
  if (key == KN_RETURN) {
    return '\r';
  }
  if (key == KN_BACKSPACE) {
    return '\b';
  }
  if (key == KN_TAB) {
    return '\t';
  }
  if (key == KN_ESC) {
    return '\x1B';
  }
  if (key == KN_SPACE) {
    return ' ';
  }
  return '\0';
}

// The key a US keyboard types `character` with, Shift aside.
KeyNumber KeyThatTypes(const char character) {
  if (absl::ascii_isalpha(static_cast<unsigned char>(character))) {
    return static_cast<KeyNumber>(
        static_cast<int>(KN_A) +
        (absl::ascii_tolower(static_cast<unsigned char>(character)) - 'a'));
  }
  if (character == '0') {
    return KN_0;
  }
  if (absl::ascii_isdigit(static_cast<unsigned char>(character))) {
    return static_cast<KeyNumber>(static_cast<int>(KN_1) + (character - '1'));
  }
  CHECK_EQ(character, ' ') << "Type() takes letters, digits and spaces";
  return KN_SPACE;
}

}  // namespace

ScriptOverrun::~ScriptOverrun() = default;

bool ScriptedPlayer::HeldKeys::IsShiftDown() const {
  return keys.contains(KN_LSHIFT);
}

bool ScriptedPlayer::HeldKeys::IsCtrlDown() const {
  return keys.contains(KN_LCTRL);
}

bool ScriptedPlayer::HeldKeys::IsAltDown() const {
  return keys.contains(KN_LALT);
}

bool ScriptedPlayer::HeldKeys::IsKeyDown(const int key) const {
  return keys.contains(key);
}

bool ScriptedPlayer::HeldButtons::IsLeftDown() const { return left; }

bool ScriptedPlayer::HeldButtons::IsRightDown() const { return right; }

ScriptedPlayer::ScriptedPlayer(Display& display, KeyBuffer& keyboard,
                               HardwareCursor& mouse,
                               engine::platform::ManualClock& clock)
    : display_(&display),
      keyboard_(&keyboard),
      mouse_(&mouse),
      clock_(&clock),
      previous_buttons_(&ExchangeCurrentMouseButtons(held_)),
      previous_keyboard_(&ExchangeCurrentKeyboardState(held_keys_)) {
  display_->set_observer(this);
}

ScriptedPlayer::ScriptedPlayer(Display& display, KeyBuffer& keyboard,
                               HardwareCursor& mouse)
    : display_(&display),
      keyboard_(&keyboard),
      mouse_(&mouse),
      clock_(nullptr),
      previous_buttons_(&ExchangeCurrentMouseButtons(held_)),
      previous_keyboard_(&ExchangeCurrentKeyboardState(held_keys_)) {
  display_->set_observer(this);
}

ScriptedPlayer::~ScriptedPlayer() {
  display_->set_observer(nullptr);
  ExchangeCurrentKeyboardState(*previous_keyboard_);
  ExchangeCurrentMouseButtons(*previous_buttons_);
}

ScriptedPlayer& ScriptedPlayer::At(const int frame) {
  CHECK_GE(frame, queue_frame_) << "a script is written in frame order";
  queue_frame_ = frame;
  return *this;
}

ScriptedPlayer& ScriptedPlayer::Key(const KeyNumber key) {
  return Do([this, key] { PutKey(key, ControlCharacter(key)); });
}

ScriptedPlayer& ScriptedPlayer::KeyDown(const KeyNumber key) {
  return Do([this, key] {
    // Held before it is queued: a Shift going down is itself shifted.
    held_keys_.keys.insert(static_cast<int>(key));
    keyboard_->PutKey(static_cast<int>(key));
  });
}

ScriptedPlayer& ScriptedPlayer::KeyUp(const KeyNumber key) {
  return Do([this, key] {
    held_keys_.keys.erase(static_cast<int>(key));
    keyboard_->PutKey(static_cast<int>(key), /*release=*/true);
  });
}

ScriptedPlayer& ScriptedPlayer::Type(const std::string_view text) {
  return Do([this, typed = std::string(text)] {
    for (const char character : typed) {
      PutKey(KeyThatTypes(character), character);
    }
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
  if (clock_ != nullptr) {
    clock_->Advance(1);
  }

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
  ++pumps_since_frame_;
  if (idle_pumps_per_frame_ > 0 &&
      pumps_since_frame_ >= idle_pumps_per_frame_) {
    // Ending the frame comes back here through OnFrameEnd(), which starts
    // the count again, and through the pump that follows it.
    display_->EndFrame();
    return;
  }
  if (pumps_since_frame_ > kMaxPumpsPerFrame) {
    pumps_since_frame_ = 0;
    throw ScriptOverrun(absl::StrFormat(
        "the game polled for input %d times after frame %d without ending a "
        "frame; a script only moves when a frame ends",
        kMaxPumpsPerFrame, frame_));
  }
}

void ScriptedPlayer::PutKey(const KeyNumber key, const char character) {
  // The press is built here rather than by KeyBuffer::PutKey(), which has no
  // way to be given the character; the modifier bits are the ones it adds.
  auto bits = static_cast<uint32_t>(key);
  if (held_keys_.IsShiftDown()) {
    bits |= kKeyShiftBit;
  }
  if (held_keys_.IsCtrlDown()) {
    bits |= kKeyCtrlBit;
  }
  if (held_keys_.IsAltDown()) {
    bits |= kKeyAltBit;
  }
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  keyboard_->Put({.key = static_cast<KeyNumber>(bits), .character = character});
  keyboard_->PutKey(static_cast<int>(key), /*release=*/true);
}

void ScriptedPlayer::PutClick(const MouseButton button, const bool release,
                              const int x, const int y) {
  if (button == MouseButton::kLeft) {
    held_.left = !release;
  } else if (button == MouseButton::kRight) {
    held_.right = !release;
  }
  mouse_->RecordPosition(x, y);
  keyboard_->PutClick(button, release, x, y);
}

}  // namespace engine::window
