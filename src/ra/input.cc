#include "ra/input.h"

#include <memory>

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "ra/jshell.h"

namespace {
// The cursor's hot-spot box, in pixels. The original picked a square large
// enough for every mouse shape the game draws.
constexpr int kCursorWidth = 48;
constexpr int kCursorHeight = 48;
}  // namespace

Input::Input() : keyboard_(std::make_unique<KeyboardClass>()) {
  engine::window::g_active_keyboard = keyboard_.get();
}

Input::~Input() { engine::window::g_active_keyboard = nullptr; }

void Input::InstallMouse(PixelView& page) {
  mouse_ = std::make_unique<WWMouseClass>(&page, kCursorWidth, kCursorHeight);
}

void Input::RemoveMouse() { mouse_.reset(); }
