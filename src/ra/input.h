// File: Input, the keyboard and the mouse the player plays with.

#ifndef CNC_RED_ALERT_RA_INPUT_H_
#define CNC_RED_ALERT_RA_INPUT_H_

#include <memory>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "ra/jshell.h"

class GraphicViewPortClass;
class WWMouseClass;

// The two devices the player plays with. The keyboard exists for the whole
// of a Game, because the startup path reads keys before there is a window;
// the mouse cursor needs a page to draw itself over, so it is created by
// InstallMouse() once the video mode is set and destroyed by Prog_End()
// before the pages go.
//
// Game owns the one Input; everything else reaches it through TheKeyboard()
// and TheMouse().
//
// Example:
//   if (TheKeyboard().Check()) { ... }
class Input {
 public:
  Input();
  ~Input();

  Input(const Input&) = delete;
  Input& operator=(const Input&) = delete;
  Input(Input&&) = delete;
  Input& operator=(Input&&) = delete;

  KeyboardClass& keyboard() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return *keyboard_;
  }

  // The mouse cursor, or null before InstallMouse() and after RemoveMouse().
  WWMouseClass* mouse() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return mouse_.get();
  }

  // Creates the cursor over the given page. Called once the video mode is
  // set; calling it again replaces the cursor.
  void InstallMouse(GraphicViewPortClass& page);

  // Destroys the cursor. Prog_End() calls this before the video pages go.
  void RemoveMouse();

 private:
  std::unique_ptr<KeyboardClass> keyboard_;
  std::unique_ptr<WWMouseClass> mouse_;
};

// Returns the Input that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Input& TheInput() { return base::Installed<Input>::Get(); }

// Shorthands for the two devices.
inline KeyboardClass& TheKeyboard() { return TheInput().keyboard(); }
inline WWMouseClass* TheMouse() { return TheInput().mouse(); }

#endif  // CNC_RED_ALERT_RA_INPUT_H_
