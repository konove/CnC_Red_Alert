#ifndef CNC_RED_ALERT_TD_DATA_INI_H_
#define CNC_RED_ALERT_TD_DATA_INI_H_

#include "td/core/defines.h"
#include "td/objects/object.h"

namespace td {

void Set_Scenario_Name(char* buf, int scenario, ScenarioPlayerType player,
                       game::ScenarioDirType dir = game::SCEN_DIR_NONE,
                       ScenarioVarType var = SCEN_VAR_NONE);
void Write_Scenario_Ini(const char* root);
bool Read_Scenario_Ini(const char* root, bool fresh = true);
bool Scan_Place_Object(ObjectClass* obj, Cell cell);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_DATA_INI_H_
