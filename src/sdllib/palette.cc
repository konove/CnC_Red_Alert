#include <cstdint>
#include <span>

#include "sdllib/graphic_buffer.h"
#include "sdllib/ww_mouse.h"

void SetScreenPalette(std::span<const uint8_t> palette) {
  if (WindowBuffer) {
    WindowBuffer->UpdatePalette(palette);
  }

  Update_Mouse_Palette();
}
