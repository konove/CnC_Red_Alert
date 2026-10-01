#include "td/ui/input.h"

namespace {
// The largest cursor shape the mouse accepts, in pixels. The original picked
// a square large enough for every mouse shape the game draws.
constexpr int kCursorWidth = 32;
constexpr int kCursorHeight = 32;
}  // namespace

Input::Input() : mouse_(kCursorWidth, kCursorHeight) {}

Input::~Input() = default;
