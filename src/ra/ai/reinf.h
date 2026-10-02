#ifndef CNC_RED_ALERT_RA_AI_REINF_H_
#define CNC_RED_ALERT_RA_AI_REINF_H_

#include "ra/ai/teamtype.h"
#include "ra/core/defines.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"

namespace ra {

bool Do_Reinforcements(TeamTypeClass* team);
bool Create_Special_Reinforcement(const HouseClass* house,
                                  const TechnoTypeClass* type,
                                  const TechnoTypeClass* another,
                                  TeamMissionType mission = TMISSION_NONE,
                                  int argument = 0);
int Create_Air_Reinforcement(HouseClass* house, AircraftType air, int number,
                             MissionType mission, Target tarcom, Target navcom,
                             InfantryType passenger = INFANTRY_NONE);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_REINF_H_
