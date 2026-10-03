#ifndef CNC_RED_ALERT_RA_AI_REINF_H_
#define CNC_RED_ALERT_RA_AI_REINF_H_

#include "ra/ai/teamtype.h"
#include "ra/core/defines.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"

namespace ra {

bool Do_Reinforcements(TeamTypeClass* team);
int Create_Air_Reinforcement(HouseClass* house, AircraftType air, int number,
                             MissionType mission, Target tarcom, Target navcom);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_REINF_H_
