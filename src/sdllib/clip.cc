#include "sdllib/clip.h"

#include "base/numeric.h"

bool ClipRect(int& x, int& y, int& width, int& height, int clip_width,
              int clip_height) {
  int x0 = x;
  int y0 = y;
  int x1 = x + width;
  int y1 = y + height;

  const OutCode near_corner = OutCodeOf(x0, y0, clip_width, clip_height);
  const OutCode far_corner =
      OutCodeOf(x1, y1, clip_width + 1, clip_height + 1);

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
