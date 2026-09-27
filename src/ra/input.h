// File: Input, the keyboard and the mouse the player plays with.

#ifndef CNC_RED_ALERT_RA_INPUT_H_
#define CNC_RED_ALERT_RA_INPUT_H_

#include <memory>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "engine/window/keyboard.h"

class HardwareCursor;

// The two devices the player plays with. The keyboard exists for the whole
// of a Game, because the startup path reads keys before there is a window;
// the mouse cursor grabs the mouse into the window, so it is created by
// InstallMouse() once the video mode is set and destroyed by ShutDownEngine().
//
// Game owns the one Input; everything else reaches it through TheKeyboard()
// and TheMouse().
//
// Example:
//   if (TheKeyboard().Peek()) { ... }
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

  // The mouse cursor, or null before InstallMouse() and after RemoveMouse().
  HardwareCursor* mouse() ABSL_ATTRIBUTE_LIFETIME_BOUND { return mouse_.get(); }

  // Creates the cursor. Called once the video mode is set; calling it again
  // replaces the cursor.
  void InstallMouse();

  // Destroys the cursor, releasing the mouse. ShutDownEngine() calls this.
  void RemoveMouse();

 private:
  engine::window::KeyBuffer keyboard_;
  std::unique_ptr<HardwareCursor> mouse_;
};

// Returns the Input that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Input& TheInput() { return base::Installed<Input>::Get(); }

// Shorthands for the two devices.
inline engine::window::KeyBuffer& TheKeyboard() {
  return TheInput().keyboard();
}
inline HardwareCursor* TheMouse() { return TheInput().mouse(); }

#endif  // CNC_RED_ALERT_RA_INPUT_H_
