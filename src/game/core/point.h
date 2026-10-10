// The position type the simulation and the user interface share.

#ifndef CNC_RED_ALERT_GAME_CORE_POINT_H_
#define CNC_RED_ALERT_GAME_CORE_POINT_H_

namespace game {

// A position on a plane where (0, 0) is the north-west corner, so y grows
// southward. The unit is the caller's: pixels, leptons or cells.
struct Point {
  int x = 0;
  int y = 0;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_CORE_POINT_H_
