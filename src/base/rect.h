// Rect: an axis-aligned rectangle of pixels.

#ifndef CNC_RED_ALERT_BASE_RECT_H_
#define CNC_RED_ALERT_BASE_RECT_H_

#include <algorithm>

namespace base {

// A rectangle given by its top left corner and its size. What the corner is
// relative to (a view, a shape's center, a clip window) is up to the function
// that takes it. A Rect is immutable: to change one, assign a new one.
//
// Example:
//   view.FillRect(base::Rect{x, y, width, height}, color);
//   view.DrawRect(base::Rect::FromCorners(x1, y1, x2, y2), color);
class Rect {
 public:
  // The rectangle that spans two opposite corners, both inclusive and in
  // either order, so it is never empty: equal corners give one pixel.
  [[nodiscard]] static constexpr Rect FromCorners(const int x1, const int y1,
                                                  const int x2, const int y2) {
    const int left = std::min(x1, x2);
    const int top = std::min(y1, y2);
    return Rect{left, top, std::max(x1, x2) - left + 1,
                std::max(y1, y2) - top + 1};
  }

  // An empty rectangle at the origin.
  constexpr Rect() = default;

  constexpr Rect(const int x, const int y, const int width, const int height)
      : x_(x), y_(y), width_(width), height_(height) {}

  // Whether the rectangle holds at least one pixel. A default-constructed
  // Rect does not, which makes it the "no rectangle" value.
  [[nodiscard]] constexpr bool IsValid() const {
    return width_ > 0 && height_ > 0;
  }

  // The top left corner.
  [[nodiscard]] constexpr int x() const { return x_; }
  [[nodiscard]] constexpr int y() const { return y_; }

  [[nodiscard]] constexpr int width() const { return width_; }
  [[nodiscard]] constexpr int height() const { return height_; }

  // One past the last column and row.
  [[nodiscard]] constexpr int right() const { return x_ + width_; }
  [[nodiscard]] constexpr int bottom() const { return y_ + height_; }

  // Whether the pixel at (`x`, `y`) is inside the rectangle. An empty one,
  // or one with a negative size, contains none.
  [[nodiscard]] constexpr bool Contains(const int x, const int y) const {
    return x >= x_ && x < right() && y >= y_ && y < bottom();
  }

  friend constexpr bool operator==(const Rect&, const Rect&) = default;

 private:
  int x_ = 0;
  int y_ = 0;
  int width_ = 0;
  int height_ = 0;
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_RECT_H_
