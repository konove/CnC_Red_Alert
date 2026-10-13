// File: Fitting a rectangle to a window, by trimming it (ClipRect)
// or by sliding it (ConfineRect). Integer geometry only, used by the drawing
// primitives, by the shape blitter and by the map code.

#ifndef CNC_RED_ALERT_BASE_CLIP_H_
#define CNC_RED_ALERT_BASE_CLIP_H_

#include <algorithm>
#include <ostream>

#include "base/point.h"
#include "base/rect.h"

namespace base {

// Clips `rect` to the window from (0, 0) to (clip_width, clip_height) and
// returns what is left of it. Nothing is left of a rectangle that lies outside
// the window, only touches its edge or was empty to begin with; the result is
// then the default Rect, so `IsValid()` tells whether there is anything to
// draw.
//
// A caller that has a second rectangle to carry along - a source to match a
// clipped destination, or the reverse - moves it by however much the result's
// corner moved. That does not work for one that scales between the two,
// because each edge then moves the other rectangle by a different amount.
constexpr Rect ClipRect(const Rect& rect, const int clip_width,
                        const int clip_height) {
  const int x0 = std::max(rect.x(), 0);
  const int y0 = std::max(rect.y(), 0);

  // A rectangle outside the window comes out with a width or a height that is
  // not positive, like one that was empty to begin with.
  const Rect clipped{x0, y0, std::min(rect.right(), clip_width) - x0,
                     std::min(rect.bottom(), clip_height) - y0};
  return clipped.IsValid() ? clipped : Rect{};
}

// Where ConfineRect() put a rectangle.
struct ConfinedRect {
  friend constexpr bool operator==(const ConfinedRect&,
                                   const ConfinedRect&) = default;

  // Shows the position and the flag in a failed test expectation.
  friend void PrintTo(const ConfinedRect& confined, std::ostream* os) {
    *os << "{(" << confined.position.x << ", " << confined.position.y << "), "
        << (confined.bounded ? "bounded" : "not bounded") << "}";
  }

  Point position;        // the rectangle's top left corner
  bool bounded = false;  // whether either axis ran into the window's edge
};

// Slides `rect` so that it lies inside `bounds`, moving it as little as it can
// and changing only its position, never its size. A rectangle too big for the
// window is pinned to its top left corner.
//
// Where ClipRect() trims a rectangle to what is visible, this keeps all of it
// and pushes it back inside - which is what the map wants of a tactical view
// or a radar box that has run off the edge of the world.
//
// An axis reports as bounded even when the rectangle already sat where the
// bounding would put it, which happens when it is larger than the window and
// at its corner. Both games' map scrolling reads the result as "the scroll
// hit the edge of the world" and recomputes the distance actually travelled,
// so that is behaviour rather than an accident.
constexpr ConfinedRect ConfineRect(const Rect& rect, const Rect& bounds) {
  ConfinedRect result{.position = {.x = rect.x(), .y = rect.y()}};

  if (rect.x() < bounds.x()) {
    result.position.x = bounds.x();
    result.bounded = true;
  } else if (rect.right() > bounds.right()) {
    result.position.x = std::max(bounds.right() - rect.width(), bounds.x());
    result.bounded = true;
  }

  if (rect.y() < bounds.y()) {
    result.position.y = bounds.y();
    result.bounded = true;
  } else if (rect.bottom() > bounds.bottom()) {
    result.position.y = std::max(bounds.bottom() - rect.height(), bounds.y());
    result.bounded = true;
  }

  return result;
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_CLIP_H_
