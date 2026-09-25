#include "td/input.h"

#include <memory>

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"

namespace {
// The cursor's hot-spot box, in pixels. The original picked a square large
// enough for every mouse shape the game draws.
constexpr int kCursorWidth = 32;
constexpr int kCursorHeight = 32;
}  // namespace

Input::Input() { engine::window::g_active_keyboard = &keyboard_; }

Input::~Input() { engine::window::g_active_keyboard = nullptr; }

void Input::InstallMouse(PixelView& page) {
  mouse_ = std::make_unique<WWMouseClass>(&page, kCursorWidth, kCursorHeight);
}

void Input::RemoveMouse() { mouse_.reset(); }
