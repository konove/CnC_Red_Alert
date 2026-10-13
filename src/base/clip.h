// File: Fitting a point or a rectangle to a window, by trimming it (ClipRect)
// or by sliding it (ConfineRect). Integer geometry only, used by the drawing
// primitives, by the shape blitter and by the map code.

#ifndef CNC_RED_ALERT_BASE_CLIP_H_
#define CNC_RED_ALERT_BASE_CLIP_H_

#include <algorithm>
#include <cstdint>

#include "base/attributes.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/point.h"
#include "base/rect.h"

namespace base {

// Which edges of a clipping window a point lies outside of: the
// Cohen-Sutherland outcode. kInside is the empty set, the point is within the
// window.
enum class CNC_FLAG_ENUM OutCode : uint32_t {
  kInside = 0,
  kLeft = 0b1000,
  kRight = 0b0100,
  kAbove = 0b0010,
  kBelow = 0b0001,
};
template <>
inline constexpr bool kIsFlagEnum<OutCode> = true;

// The outcode of (x, y) against the window from (0, 0) to (width, height).
//
// A rectangle is clipped by taking the outcode of its top-left corner and,
// against a window one pixel wider and taller, the outcode of the corner just
// past its bottom-right. That second window is what makes an end coordinate
// equal to the width count as inside: the rectangle owns pixels up to but not
// including it.
constexpr OutCode OutCodeOf(int x, int y, int width, int height) {
  return (x < 0 ? OutCode::kLeft : OutCode::kInside) |
         (x >= width ? OutCode::kRight : OutCode::kInside) |
         (y < 0 ? OutCode::kAbove : OutCode::kInside) |
         (y >= height ? OutCode::kBelow : OutCode::kInside);
}

// Clips `rect` to the window from (0, 0) to (clip_width, clip_height) and
// returns what is left of it. Nothing is left of a rectangle that lies outside
// the window, only touches its edge or was empty to begin with; the result is
// then the default Rect, so `IsValid()` tells whether there is anything to
// draw.
//
// A caller that has a second rectangle to carry along - a source to match a
// clipped destination, or the reverse - moves it by however much the result's
// corner moved. Only one that scales between the two (PixelView::ScaleLocked)
// clips with OutCodeOf() directly, because each edge then moves the other
// rectangle by a different amount.
constexpr Rect ClipRect(const Rect& rect, const int clip_width,
                        const int clip_height) {
  int x0 = rect.x;
  int y0 = rect.y;
  int x1 = rect.right();
  int y1 = rect.bottom();

  const OutCode near_corner = OutCodeOf(x0, y0, clip_width, clip_height);
  const OutCode far_corner = OutCodeOf(x1, y1, clip_width + 1, clip_height + 1);

  // A bit set in both corners puts the whole rectangle past that one edge.
  if (Any(near_corner & far_corner)) {
    return {};
  }

  // Only the near corner can fall off the left or top edge and only the far
  // corner off the right or bottom, so each edge is pulled in from one side.
  if (Any(near_corner & OutCode::kLeft)) {
    x0 = 0;
  }
  if (Any(far_corner & OutCode::kRight)) {
    x1 = clip_width;
  }
  if (Any(near_corner & OutCode::kAbove)) {
    y0 = 0;
  }
  if (Any(far_corner & OutCode::kBelow)) {
    y1 = clip_height;
  }

  // A far corner exactly on the left or top edge passes the outcode test and
  // leaves no pixels.
  const Rect clipped{x0, y0, x1 - x0, y1 - y0};
  return clipped.IsValid() ? clipped : Rect{};
}

// Where ConfineRect() put a rectangle.
struct ConfinedRect {
  Point position;        // the rectangle's top left corner
  bool bounded = false;  // whether either axis ran into the window's edge
};

// Slides `rect` so that it lies inside the window from (0, 0) to (clip_width,
// clip_height), moving it as little as it can and changing only its position,
// never its size. A rectangle too big for the window is pinned to the origin.
//
// Where ClipRect() trims a rectangle to what is visible, this keeps all of it
// and pushes it back inside - which is what the map wants of a tactical view
// or a radar box that has run off the edge of the world.
//
// An axis reports as bounded even when the rectangle already sat where the
// bounding would put it, which happens when it is larger than the window and
// at the origin. Both games' map scrolling reads the result as "the scroll
// hit the edge of the world" and recomputes the distance actually travelled,
// so that is behaviour rather than an accident.
constexpr ConfinedRect ConfineRect(const Rect& rect, const int clip_width,
                                   const int clip_height) {
  ConfinedRect result{.position = {rect.x, rect.y}};

  if (rect.x < 0) {
    result.position.x = 0;
    result.bounded = true;
  } else if (rect.right() > clip_width) {
    result.position.x = std::max(clip_width - rect.width, 0);
    result.bounded = true;
  }

  if (rect.y < 0) {
    result.position.y = 0;
    result.bounded = true;
  } else if (rect.bottom() > clip_height) {
    result.position.y = std::max(clip_height - rect.height, 0);
    result.bounded = true;
  }

  return result;
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_CLIP_H_
