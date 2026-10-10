// File: The pixel rectangle of one of the game's text windows.

#ifndef CNC_RED_ALERT_RA_ASSETS_WINDOW_RECT_H_
#define CNC_RED_ALERT_RA_ASSETS_WINDOW_RECT_H_

#include "base/array.h"
#include "engine/gfx/rect.h"
#include "engine/gfx/text_window.h"

namespace ra {

enum class WindowNumberType;

// Returns the rectangle of WindowList row `window`, in page pixels. Red Alert
// stores every column of the row in pixels already.
inline Rect WindowRect(const WindowNumberType window) {
  const auto& row = base::At(WindowList, static_cast<int>(window));
  return Rect{base::At(row, kWindowX), base::At(row, kWindowY),
              base::At(row, kWindowWidth), base::At(row, kWindowHeight)};
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_ASSETS_WINDOW_RECT_H_
