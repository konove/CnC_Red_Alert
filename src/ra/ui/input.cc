#include "ra/ui/input.h"

namespace {
// The largest cursor shape the mouse accepts, in pixels. The original picked
// a square large enough for every mouse shape the game draws.
constexpr int kCursorWidth = 48;
constexpr int kCursorHeight = 48;
}  // namespace

Input::Input() : mouse_(kCursorWidth, kCursorHeight) {}

Input::~Input() = default;
