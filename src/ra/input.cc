#include "ra/input.h"

#include <memory>

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/ww_mouse.h"

namespace {
// The cursor's hot-spot box, in pixels. The original picked a square large
// enough for every mouse shape the game draws.
constexpr int kCursorWidth = 48;
constexpr int kCursorHeight = 48;
}  // namespace

Input::Input() = default;

Input::~Input() = default;

void Input::InstallMouse(PixelView& page) {
  mouse_ = std::make_unique<WWMouseClass>(&page, kCursorWidth, kCursorHeight);
}

void Input::RemoveMouse() { mouse_.reset(); }
