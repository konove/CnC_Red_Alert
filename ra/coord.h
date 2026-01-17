#ifndef CNC_RED_ALERT_RA_COORD_H_
#define CNC_RED_ALERT_RA_COORD_H_

#include "ra/defines.h"
#include "ra/face.h"
#include "tech/rect.h"

short const* Coord_Spillage_List(COORDINATE coord, Rect const& rect,
                                 bool nocenter = true);
void Normal_Move_Point(short& x, short& y, DirType dir,
                       unsigned short distance);
void Move_Point(short& x, short& y, DirType dir, unsigned short distance);
COORDINATE Coord_Move(COORDINATE start, DirType facing,
                      unsigned short distance);
COORDINATE Coord_Scatter(COORDINATE coord, unsigned distance,
                         bool lock = false);
DirType Direction(CELL cell1, CELL cell2);
DirType Direction(COORDINATE coord1, COORDINATE coord2);
DirType Direction256(COORDINATE coord1, COORDINATE coord2);
DirType Direction8(COORDINATE coord1, COORDINATE coord2);
int Distance(COORDINATE coord1, COORDINATE coord2);
int Distance(TARGET target1, TARGET target2);
short const* Coord_Spillage_List(COORDINATE coord, int maxsize);
CELL Coord_Cell(COORDINATE coord);

#endif  // CNC_RED_ALERT_RA_COORD_H_
