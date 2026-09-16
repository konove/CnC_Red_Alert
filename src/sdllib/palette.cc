#include <cstdint>
#include <span>

#include "sdllib/gbuffer.h"
#include "sdllib/ww_mouse.h"

void Do_Set_Palette(std::span<const uint8_t> palette) {
  if (WindowBuffer) {
    WindowBuffer->Update_Palette(palette);
  }

  Update_Mouse_Palette();
}
