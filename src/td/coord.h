#ifndef CNC_RED_ALERT_TD_COORD_H_
#define CNC_RED_ALERT_TD_COORD_H_

#include "td/defines.h"

void Move_Point(short& x, short& y, DirType dir, unsigned short distance);
COORDINATE Coord_Move(COORDINATE start, DirType facing,
                      unsigned short distance);
COORDINATE Coord_Scatter(COORDINATE coord, unsigned distance,
                         bool lock = false);
short const* Coord_Spillage_List(COORDINATE coord, int maxsize);
// void Move_Point(unsigned short &x, unsigned short &y, DirType dir, unsigned
// short distance);

#endif  // CNC_RED_ALERT_TD_COORD_H_
