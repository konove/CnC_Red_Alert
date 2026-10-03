#ifndef CNC_RED_ALERT_ENGINE_GFX_FADING_TABLE_H_
#define CNC_RED_ALERT_ENGINE_GFX_FADING_TABLE_H_

#include <array>
#include <cstdint>

#include "engine/gfx/palette.h"

// A remap table with one entry per palette color, giving the index of the
// color to draw in its place.
using FadingTable = std::array<uint8_t, 256>;

// Returns the table that fades every palette color toward `color` by `frac`
// (0..255, where 255 is fully faded; larger values count as 255). Each entry
// is the closest-matching color on the path toward `color`, found by
// nearest-neighbor search over `palette`. Color 0, transparent black, stays
// itself. `color` must be a palette index (0..255).
FadingTable BuildFadingTable(const engine::gfx::Palette& palette, int color,
                             int frac);

#endif  // CNC_RED_ALERT_ENGINE_GFX_FADING_TABLE_H_
