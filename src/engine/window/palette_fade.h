// File: FadePalette(), the palette fade both games run between screens.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_PALETTE_FADE_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_PALETTE_FADE_H_

#include "engine/gfx/palette.h"

namespace engine::window {

// Blends from `from` to `to` over `ticks` ticks of SystemTicks(). Each step
// passes the blended palette to `show`, then calls `step`, or presents a
// frame when `step` is nullptr. Steps follow the tick, since the blend only
// changes when it does; the last step shows `to` exactly. Does nothing when
// `ticks` is not positive or `from` already equals `to`: the caller still
// makes `to` current either way.
void FadePalette(const gfx::Palette& from, const gfx::Palette& to, int ticks,
                 void (*show)(const gfx::Palette&), void (*step)());

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_PALETTE_FADE_H_
