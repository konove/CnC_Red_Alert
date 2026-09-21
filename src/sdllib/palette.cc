#include <cstdint>
#include <span>

#include "sdllib/gbuffer.h"
#include "sdllib/ww_mouse.h"

void SetScreenPalette(std::span<const uint8_t> palette) {
  if (WindowBuffer) {
    WindowBuffer->UpdatePalette(palette);
  }

  Update_Mouse_Palette();
}
