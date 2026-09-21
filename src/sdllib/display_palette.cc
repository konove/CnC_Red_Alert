// Implements Display::SetPalette.
//
// It lives apart from the rest of Display so that only the code that actually
// changes the palette links the mouse cursor, which the palette change has to
// rebuild. Tests that stub the cursor out depend on that separation.

#include <cstdint>
#include <span>

#include "sdllib/display.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_mouse.h"

void Display::SetPalette(std::span<const uint8_t> palette) {
  if (window_page_ != nullptr) {
    window_page_->UpdatePalette(palette);
  }
  Update_Mouse_Palette();
}
