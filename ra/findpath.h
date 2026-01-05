#ifndef CNC_RED_ALERT_RA_FINDPATH_H_
#define CNC_RED_ALERT_RA_FINDPATH_H_
#include "ra/defines.h"

int Optimize_Moves(PathType *path, int (*callback)(CELL, FacingType),
                   int threshhold);

#endif  // CNC_RED_ALERT_RA_FINDPATH_H_
