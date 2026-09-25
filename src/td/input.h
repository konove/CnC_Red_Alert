// File: Input, the keyboard and the mouse the player plays with.

#ifndef CNC_RED_ALERT_TD_INPUT_H_
#define CNC_RED_ALERT_TD_INPUT_H_

#include <memory>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"

class PixelView;
class WWMouseClass;

// The two devices the player plays with. The keyboard exists for the whole
// of a Game, because the startup path reads keys before there is a window;
// the mouse cursor needs a page to draw itself over, so it is created by
// InstallMouse() once the video mode is set and destroyed by ShutDownEngine()
// before the pages go.
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
  WWMouseClass* mouse() ABSL_ATTRIBUTE_LIFETIME_BOUND { return mouse_.get(); }

  // Creates the cursor over the given page. Called once the video mode is
  // set; calling it again replaces the cursor.
  void InstallMouse(PixelView& page);

  // Destroys the cursor. ShutDownEngine() calls this before the video pages go.
  void RemoveMouse();

 private:
  engine::window::KeyBuffer keyboard_;
  std::unique_ptr<WWMouseClass> mouse_;
};

// Returns the Input that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Input& TheInput() { return base::Installed<Input>::Get(); }

// Shorthands for the two devices.
inline engine::window::KeyBuffer& TheKeyboard() {
  return TheInput().keyboard();
}
inline WWMouseClass* TheMouse() { return TheInput().mouse(); }

#endif  // CNC_RED_ALERT_TD_INPUT_H_
