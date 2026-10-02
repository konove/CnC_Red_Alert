// Tiberian Dawn's map cell number, in a header of its own so that light
// headers such as td/core/target.h can name it without td/core/defines.h.

#ifndef CNC_RED_ALERT_TD_CORE_CELL_H_
#define CNC_RED_ALERT_TD_CORE_CELL_H_

#include <cstdint>

namespace td {

// A map coordinate with cell resolution: y * 64 + x, 6 bits per axis.
// Negative values are sentinels.
using CELL = int16_t;

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_CELL_H_
