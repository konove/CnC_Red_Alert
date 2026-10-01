#include "td/assets/palette.h"

#include "engine/gfx/palette.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/palette_fade.h"
#include "td/ui/input.h"

namespace td {

engine::gfx::Palette CurrentPalette;

// Shows `palette` on the display and recolors the mouse cursor, which SDL
// draws in the colors it had when the cursor was made.
static void ShowPalette(const engine::gfx::Palette& palette) {
  engine::window::TheDisplay().SetPalette(palette);
  TheMouse().UpdatePalette();
}

void Fade_Palette_To(const engine::gfx::Palette& palette, int fade,
                     void (*callback)()) {
  engine::window::FadePalette(CurrentPalette, palette, fade, ShowPalette,
                              callback);
  Set_Palette(palette);
}

void Set_Palette(const engine::gfx::Palette& palette) {
  CurrentPalette = palette;
  ShowPalette(palette);
}

}  // namespace td
