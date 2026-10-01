#ifndef CNC_RED_ALERT_TD_OBJECTS_COMBAT_H_
#define CNC_RED_ALERT_TD_OBJECTS_COMBAT_H_

#include "td/core/defines.h"
#include "td/objects/techno.h"

int Modify_Damage(int damage, WarheadType warhead, ArmorType armor,
                  int distance);
void Explosion_Damage(COORDINATE coord, int strength, TechnoClass* source,
                      WarheadType warhead);

#endif  // CNC_RED_ALERT_TD_OBJECTS_COMBAT_H_
