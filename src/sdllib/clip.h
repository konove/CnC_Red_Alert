// File: Clipping a point or a rectangle to a window, shared by the drawing
// primitives and by the map code that clips a cell rectangle for itself.

#ifndef CNC_RED_ALERT_SDLLIB_CLIP_H_
#define CNC_RED_ALERT_SDLLIB_CLIP_H_

#include <cstdint>

#include "base/attributes.h"
#include "base/flags.h"

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
// The drawing primitives clip with OutCodeOf() directly, because each of them
// has a second rectangle - a source, or a destination buffer - to carry along
// by however much came off each edge. This is for a caller with only the one
// rectangle.
bool ClipRect(int& x, int& y, int& width, int& height, int clip_width,
              int clip_height);

#endif  // CNC_RED_ALERT_SDLLIB_CLIP_H_
