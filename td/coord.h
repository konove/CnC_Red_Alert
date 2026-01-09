#ifndef CNC_RED_ALERT_TD_COORD_H_
#define CNC_RED_ALERT_TD_COORD_H_

#include "td/defines.h"

void Move_Point(short &x, short &y, DirType dir, unsigned short distance);
COORDINATE Adjacent_Cell(COORDINATE coord, FacingType dir);
COORDINATE Coord_Move(COORDINATE start, DirType facing,
                      unsigned short distance);
COORDINATE Coord_Scatter(COORDINATE coord, unsigned distance,
                         bool lock = false);
DirType Direction(CELL cell1, CELL cell2);
DirType Direction(COORDINATE coord1, COORDINATE coord2);
DirType Direction256(COORDINATE coord1, COORDINATE coord2);
DirType Direction8(COORDINATE coord1, COORDINATE coord2);
int Distance(CELL coord1, CELL coord2);
int Distance(COORDINATE coord1, COORDINATE coord2);
short const *Coord_Spillage_List(COORDINATE coord, int maxsize);
// void Move_Point(unsigned short &x, unsigned short &y, DirType dir, unsigned
// short distance);

#endif  // CNC_RED_ALERT_TD_COORD_H_
