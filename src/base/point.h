// The position type the simulation and the user interface share.

#ifndef CNC_RED_ALERT_BASE_POINT_H_
#define CNC_RED_ALERT_BASE_POINT_H_

namespace base {

// A position on a plane where (0, 0) is the north-west corner, so y grows
// southward. The unit is the caller's: pixels, leptons or cells.
struct Point {
  friend constexpr bool operator==(const Point&, const Point&) = default;

  int x = 0;
  int y = 0;
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_POINT_H_
