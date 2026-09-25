// Implements Display::SetPalette.
//
// It lives apart from the rest of Display so that only the code that actually
// changes the palette links the mouse cursor, which the palette change has to
// rebuild. Tests that stub the cursor out depend on that separation.

#include <cstdint>
#include <span>

#include "engine/window/display.h"
#include "engine/window/ww_mouse.h"

namespace engine::window {

void Display::SetPalette(std::span<const uint8_t> palette) {
  UpdatePalette(palette);
  Update_Mouse_Palette();
}

}  // namespace engine::window
