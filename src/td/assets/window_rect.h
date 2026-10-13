// File: The pixel rectangle of one of the game's text windows.

#ifndef CNC_RED_ALERT_TD_ASSETS_WINDOW_RECT_H_
#define CNC_RED_ALERT_TD_ASSETS_WINDOW_RECT_H_

#include "base/array.h"
#include "base/rect.h"
#include "engine/gfx/text_window.h"

namespace td {

enum class WindowNumberType;

// Returns the rectangle of WindowList row `window`, in page pixels. Tiberian
// Dawn stores a window's x and width in units of eight pixels.
inline base::Rect WindowRect(const WindowNumberType window) {
  constexpr int kWindowUnit = 8;
  const auto& row = base::At(WindowList, static_cast<int>(window));
  return base::Rect{
      base::At(row, kWindowX) * kWindowUnit, base::At(row, kWindowY),
      base::At(row, kWindowWidth) * kWindowUnit, base::At(row, kWindowHeight)};
}

}  // namespace td

#endif  // CNC_RED_ALERT_TD_ASSETS_WINDOW_RECT_H_
