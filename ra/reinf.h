#ifndef CNC_RED_ALERT_RA_REINF_H_
#define CNC_RED_ALERT_RA_REINF_H_
#include "ra/defines.h"
#include "ra/house.h"
#include "ra/teamtype.h"
#include "ra/type.h"

bool Do_Reinforcements(TeamTypeClass const *team);
bool Create_Special_Reinforcement(HouseClass *house,
                                  TechnoTypeClass const *type,
                                  TechnoTypeClass const *another,
                                  TeamMissionType mission = TMISSION_NONE,
                                  int argument = 0);
int Create_Air_Reinforcement(HouseClass *house, AircraftType air, int number,
                             MissionType mission, TARGET tarcom, TARGET navcom,
                             InfantryType passenger = INFANTRY_NONE);

#endif  // CNC_RED_ALERT_RA_REINF_H_
