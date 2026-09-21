#include "ra/input.h"

#include <memory>

#include "ra/jshell.h"
#include "sdllib/keyboard.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_mouse.h"

namespace {
// The cursor's hot-spot box, in pixels. The original picked a square large
// enough for every mouse shape the game draws.
constexpr int kCursorWidth = 48;
constexpr int kCursorHeight = 48;
}  // namespace

Input::Input() : keyboard_(std::make_unique<KeyboardClass>()) {
  ActiveKeyboard = keyboard_.get();
}

Input::~Input() { ActiveKeyboard = nullptr; }

void Input::InstallMouse(PixelView& page) {
  mouse_ = std::make_unique<WWMouseClass>(&page, kCursorWidth, kCursorHeight);
}

void Input::RemoveMouse() { mouse_.reset(); }
