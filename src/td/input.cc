#include "td/input.h"

#include <memory>

#include "engine/window/ww_mouse.h"

namespace {
// The cursor's hot-spot box, in pixels. The original picked a square large
// enough for every mouse shape the game draws.
constexpr int kCursorWidth = 32;
constexpr int kCursorHeight = 32;
}  // namespace

Input::Input() = default;

Input::~Input() = default;

void Input::InstallMouse() {
  mouse_ = std::make_unique<HardwareCursor>(kCursorWidth, kCursorHeight);
}

void Input::RemoveMouse() { mouse_.reset(); }
