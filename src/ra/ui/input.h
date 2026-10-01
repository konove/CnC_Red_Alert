// File: Input, the keyboard and the mouse the player plays with.

#ifndef CNC_RED_ALERT_RA_UI_INPUT_H_
#define CNC_RED_ALERT_RA_UI_INPUT_H_

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"

// The two devices the player plays with. Both exist for the whole of a Game,
// because the startup path reads keys before there is a window; the games grab
// the mouse into the window once it is open (HardwareCursor::GrabMouse()).
//
// Game owns the one Input; everything else reaches it through TheKeyboard()
// and TheMouse().
//
// Example:
//   if (TheKeyboard().Peek()) { ... }
//   TheMouse().Hide();
class Input {
 public:
  Input();
  ~Input();

  Input(const Input&) = delete;
  Input& operator=(const Input&) = delete;
  Input(Input&&) = delete;
  Input& operator=(Input&&) = delete;

  engine::window::KeyBuffer& keyboard() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return keyboard_;
  }

  engine::window::HardwareCursor& mouse() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return mouse_;
  }

 private:
  engine::window::KeyBuffer keyboard_;
  engine::window::HardwareCursor mouse_;
};

// Returns the Input that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Input& TheInput() { return base::Installed<Input>::Get(); }

// Shorthands for the two devices.
inline engine::window::KeyBuffer& TheKeyboard() {
  return TheInput().keyboard();
}
inline engine::window::HardwareCursor& TheMouse() { return TheInput().mouse(); }

#endif  // CNC_RED_ALERT_RA_UI_INPUT_H_
