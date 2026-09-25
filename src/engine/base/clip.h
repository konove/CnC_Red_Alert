// File: Fitting a point or a rectangle to a window, by trimming it (ClipRect)
// or by sliding it (ConfineRect). Integer geometry only, used by the drawing
// primitives, by the shape blitter and by the map code.

#ifndef CNC_RED_ALERT_ENGINE_BASE_CLIP_H_
#define CNC_RED_ALERT_ENGINE_BASE_CLIP_H_

#include <algorithm>
#include <cstdint>

#include "engine/base/attributes.h"
#include "engine/base/flags.h"
#include "engine/base/numeric.h"

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
inline constexpr bool base::kIsFlagEnum<OutCode> = true;

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

// Clips the rectangle at (x, y) sized width by height to the window from
// (0, 0) to (clip_width, clip_height), writing the clipped rectangle back
// through the four references. Returns false, leaving all four alone, when
// the rectangle lies wholly outside the window.
//
// A caller that has a second rectangle to carry along - a source to match a
// clipped destination, or the reverse - clips with OutCodeOf() directly
// instead, because it has to move that rectangle by however much came off
// each edge.
constexpr bool ClipRect(int& x, int& y, int& width, int& height, int clip_width,
                        int clip_height) {
  int x0 = x;
  int y0 = y;
  int x1 = x + width;
  int y1 = y + height;

  const OutCode near_corner = OutCodeOf(x0, y0, clip_width, clip_height);
  const OutCode far_corner = OutCodeOf(x1, y1, clip_width + 1, clip_height + 1);

  // A bit set in both corners puts the whole rectangle past that one edge.
  if (base::Any(near_corner & far_corner)) {
    return false;
  }

  // Only the near corner can fall off the left or top edge and only the far
  // corner off the right or bottom, so each edge is pulled in from one side.
  if (base::Any(near_corner & OutCode::kLeft)) {
    x0 = 0;
  }
  if (base::Any(far_corner & OutCode::kRight)) {
    x1 = clip_width;
  }
  if (base::Any(near_corner & OutCode::kAbove)) {
    y0 = 0;
  }
  if (base::Any(far_corner & OutCode::kBelow)) {
    y1 = clip_height;
  }

  x = x0;
  y = y0;
  width = x1 - x0;
  height = y1 - y0;
  return true;
}

// Slides the width by height rectangle at (x, y) so that it lies inside the
// window from (0, 0) to (clip_width, clip_height), moving it as little as it
// can and changing only its position, never its size. A rectangle too big for
// the window is pinned to the origin. Returns whether either axis had to be
// bounded.
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
constexpr bool ConfineRect(int& x, int& y, int width, int height,
                           int clip_width, int clip_height) {
  bool bounded = false;

  if (x < 0) {
    x = 0;
    bounded = true;
  } else if (x + width > clip_width) {
    x = std::max(clip_width - width, 0);
    bounded = true;
  }

  if (y < 0) {
    y = 0;
    bounded = true;
  } else if (y + height > clip_height) {
    y = std::max(clip_height - height, 0);
    bounded = true;
  }

  return bounded;
}

#endif  // CNC_RED_ALERT_ENGINE_BASE_CLIP_H_
