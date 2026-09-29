#ifndef CNC_RED_ALERT_TD_PALETTE_H_
#define CNC_RED_ALERT_TD_PALETTE_H_

#include "engine/gfx/palette.h"

// The palette on screen.
extern engine::gfx::Palette CurrentPalette;

// Makes `palette` current and shows it at once.
void Set_Palette(const engine::gfx::Palette& palette);

// Blends the screen from CurrentPalette to `palette` over `fade` ticks, then
// makes `palette` current. While fading, calls `callback` once per displayed
// step, or presents the frame itself when callback is nullptr.
void Fade_Palette_To(const engine::gfx::Palette& palette, int fade,
                     void (*callback)());

#endif  // CNC_RED_ALERT_TD_PALETTE_H_
