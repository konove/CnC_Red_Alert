#ifndef CNC_RED_ALERT_TD_COORD_H_
#define CNC_RED_ALERT_TD_COORD_H_

#include <cstdint>
#include <span>

#include "td/defines.h"

COORDINATE Coord_Move(COORDINATE start, DirType dir, uint16_t distance);
COORDINATE Coord_Scatter(COORDINATE coord, int distance,
                         bool lock = false);
std::span<const int16_t> Coord_Spillage_List(COORDINATE coord, int maxsize);

#endif  // CNC_RED_ALERT_TD_COORD_H_
