#ifndef CNC_RED_ALERT_TD_CORE_COORD_H_
#define CNC_RED_ALERT_TD_CORE_COORD_H_

#include <cstdint>
#include <span>

#include "td/core/defines.h"

namespace td {

Coordinate Coord_Move(Coordinate start, game::Direction dir, uint16_t distance);
Coordinate Coord_Scatter(Coordinate coord, int distance, bool lock = false);
std::span<const int16_t> Coord_Spillage_List(Coordinate coord, int maxsize);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_COORD_H_
