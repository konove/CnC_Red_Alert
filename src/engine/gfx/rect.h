/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Rect: an axis-aligned rectangle of pixels.

#ifndef CNC_RED_ALERT_ENGINE_GFX_RECT_H_
#define CNC_RED_ALERT_ENGINE_GFX_RECT_H_

#include <algorithm>

// A rectangle given by its top left corner and its size. What the corner is
// relative to (a view, a shape's center, a clip window) is up to the function
// that takes it.
//
// Example:
//   view.FillRect(Rect{x, y, width, height}, color);
//   view.DrawRect(Rect::FromCorners(band_x, band_y, mouse_x, mouse_y), color);
struct Rect {
  // The rectangle that spans two opposite corners, both inclusive and in
  // either order, so it is never empty: equal corners give one pixel.
  [[nodiscard]] static constexpr Rect FromCorners(const int x1, const int y1,
                                                  const int x2, const int y2) {
    const int left = std::min(x1, x2);
    const int top = std::min(y1, y2);
    return Rect{left, top, std::max(x1, x2) - left + 1,
                std::max(y1, y2) - top + 1};
  }

  // Whether the rectangle holds at least one pixel. A default-constructed
  // Rect does not, which makes it the "no rectangle" value.
  [[nodiscard]] constexpr bool IsValid() const {
    return width > 0 && height > 0;
  }

  // One past the last column and row.
  [[nodiscard]] constexpr int right() const { return x + width; }
  [[nodiscard]] constexpr int bottom() const { return y + height; }

  friend constexpr bool operator==(const Rect&, const Rect&) = default;

  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

#endif  // CNC_RED_ALERT_ENGINE_GFX_RECT_H_
