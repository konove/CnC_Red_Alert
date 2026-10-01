#ifndef CNC_RED_ALERT_TD_CORE_FACE_H_
#define CNC_RED_ALERT_TD_CORE_FACE_H_

#include "td/core/defines.h"

namespace td {

DirType Desired_Facing8(int x1, int y1, int x2, int y2);
DirType Desired_Facing256(int srcx, int srcy, int dstx, int dsty);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_FACE_H_
