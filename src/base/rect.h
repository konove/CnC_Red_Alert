// Rect: an axis-aligned rectangle of pixels.

#ifndef CNC_RED_ALERT_BASE_RECT_H_
#define CNC_RED_ALERT_BASE_RECT_H_

#include <algorithm>

namespace base {

// A rectangle given by its top left corner and its size. What the corner is
// relative to (a view, a shape's center, a clip window) is up to the function
// that takes it.
//
// Example:
//   view.FillRect(base::Rect{x, y, width, height}, color);
//   view.DrawRect(base::Rect::FromCorners(x1, y1, x2, y2), color);
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

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_RECT_H_
