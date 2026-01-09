#ifndef CNC_RED_ALERT_TD_INI_H_
#define CNC_RED_ALERT_TD_INI_H_

#include "td/defines.h"
#include "td/object.h"

void Set_Scenario_Name(char *buf, int scenario, ScenarioPlayerType player,
                       ScenarioDirType dir = SCEN_DIR_NONE,
                       ScenarioVarType var = SCEN_VAR_NONE);
void Write_Scenario_Ini(char *root);
bool Read_Scenario_Ini(char *root, bool fresh = true);
int Scan_Place_Object(ObjectClass *obj, CELL cell);

#endif  // CNC_RED_ALERT_TD_INI_H_
