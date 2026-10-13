// The position type the simulation and the user interface share.

#ifndef CNC_RED_ALERT_BASE_POINT_H_
#define CNC_RED_ALERT_BASE_POINT_H_

namespace base {

// A position on a plane where (0, 0) is the north-west corner, so y grows
// southward. The unit is the caller's: pixels, leptons or cells. A Point is
// immutable: to change one, assign a new one.
class Point {
 public:
  // The origin.
  constexpr Point() = default;

  constexpr Point(const int x, const int y) : x_(x), y_(y) {}

  [[nodiscard]] constexpr int x() const { return x_; }
  [[nodiscard]] constexpr int y() const { return y_; }

  friend constexpr bool operator==(const Point&, const Point&) = default;

 private:
  int x_ = 0;
  int y_ = 0;
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_POINT_H_
