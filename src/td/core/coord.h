#ifndef CNC_RED_ALERT_TD_CORE_COORD_H_
#define CNC_RED_ALERT_TD_CORE_COORD_H_

#include <cstdint>
#include <span>

#include "td/core/defines.h"

namespace td {

COORDINATE Coord_Move(COORDINATE start, game::DirType dir, uint16_t distance);
COORDINATE Coord_Scatter(COORDINATE coord, int distance,
                         bool lock = false);
std::span<const int16_t> Coord_Spillage_List(COORDINATE coord, int maxsize);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_COORD_H_
