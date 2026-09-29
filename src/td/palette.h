#ifndef CNC_RED_ALERT_TD_PALETTE_H_
#define CNC_RED_ALERT_TD_PALETTE_H_

#include <span>

#include "engine/gfx/palette.h"

extern unsigned char CurrentPalette[3 * 256];

// Transitional, until Tiberian Dawn keeps its palettes as engine::gfx::Palette
// (docs/PALETTE_8BIT_PLAN.md step 3): the palette in the first 768 bytes of
// `vga`, and `palette` written back into them.
engine::gfx::Palette VgaPalette(std::span<const unsigned char> vga);
void StoreVga(const engine::gfx::Palette& palette,
              std::span<unsigned char> vga);

void Set_Palette(std::span<const unsigned char> palette);
void Fade_Palette_To(std::span<const unsigned char> palette, int fade,
                     void (*callback)());

#endif  // CNC_RED_ALERT_TD_PALETTE_H_
