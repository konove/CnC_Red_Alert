#ifndef CNC_RED_ALERT_TD_AI_REINF_H_
#define CNC_RED_ALERT_TD_AI_REINF_H_

#include "td/ai/teamtype.h"
#include "td/core/defines.h"
#include "td/objects/house.h"
#include "td/objects/type.h"

bool Do_Reinforcements(const TeamTypeClass* team);
bool Create_Special_Reinforcement(const HouseClass* house,
                                  const TechnoTypeClass* type,
                                  const TechnoTypeClass* another,
                                  TeamMissionType mission = TMISSION_NONE,
                                  int argument = 0);
int Create_Air_Reinforcement(HouseClass* house, AircraftType air, int number,
                             MissionType mission, TARGET tarcom, TARGET navcom);

#endif  // CNC_RED_ALERT_TD_AI_REINF_H_
