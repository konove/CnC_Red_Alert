#include "game/ui/input.h"

namespace game {

Input::Input(const int cursor_width, const int cursor_height)
    : mouse_(cursor_width, cursor_height) {}

Input::~Input() = default;

}  // namespace game
