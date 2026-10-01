#ifndef CNC_RED_ALERT_RA_SCREENS_MAPSEL_H_
#define CNC_RED_ALERT_RA_SCREENS_MAPSEL_H_

#include "ra/core/defines.h"

namespace ra {

// Plays the mission map and waits for the player to click one of the next
// missions on offer. Returns the variant of the next scenario that the clicked
// mission stands for.
ScenarioVarType ChooseMissionVariant();

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SCREENS_MAPSEL_H_
