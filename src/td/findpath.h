#ifndef CNC_RED_ALERT_TD_FINDPATH_H_
#define CNC_RED_ALERT_TD_FINDPATH_H_

#include "td/defines.h"

int Optimize_Moves(PathType* path, int (*callback)(CELL, FacingType),
                   int threshhold);

#endif  // CNC_RED_ALERT_TD_FINDPATH_H_
