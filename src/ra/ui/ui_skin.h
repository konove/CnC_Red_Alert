// File: UiSkin, how Red Alert draws the game::*<Skin> widgets.

#ifndef CNC_RED_ALERT_RA_UI_UI_SKIN_H_
#define CNC_RED_ALERT_RA_UI_UI_SKIN_H_

#include <cstddef>
#include <span>

#include "engine/gfx/pixel_buffer.h"

namespace ra {

// The drawing the shared widgets leave to the game, as static functions the
// widget templates call.
struct UiSkin {
  // Draws frame `frame` of `shapes` at (x, y) in the main window.
  static void Draw_Shape(PixelView& view, std::span<const std::byte> shapes,
                         int frame, int x, int y);
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_UI_UI_SKIN_H_
