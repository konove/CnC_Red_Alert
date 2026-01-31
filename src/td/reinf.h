#ifndef CNC_RED_ALERT_TD_REINF_H_
#define CNC_RED_ALERT_TD_REINF_H_

#include "td/defines.h"
#include "td/house.h"
#include "td/teamtype.h"
#include "td/type.h"

bool Do_Reinforcements(TeamTypeClass* team);
bool Create_Special_Reinforcement(HouseClass* house,
                                  const TechnoTypeClass* type,
                                  const TechnoTypeClass* another,
                                  TeamMissionType mission = TMISSION_NONE,
                                  int argument = 0);
int Create_Air_Reinforcement(HouseClass* house, AircraftType air, int number,
                             MissionType mission, TARGET tarcom, TARGET navcom);

#endif  // CNC_RED_ALERT_TD_REINF_H_
