// File: Clipping a point or a rectangle to a window. Integer geometry only,
// used by the drawing primitives, by the shape blitter and by the map code
// that clips a cell rectangle for itself.

#ifndef CNC_RED_ALERT_BASE_CLIP_H_
#define CNC_RED_ALERT_BASE_CLIP_H_

#include <cstdint>

#include "base/attributes.h"
#include "base/flags.h"
#include "base/numeric.h"

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

#endif  // CNC_RED_ALERT_BASE_CLIP_H_
