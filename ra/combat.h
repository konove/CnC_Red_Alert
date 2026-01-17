#ifndef CNC_RED_ALERT_RA_COMBAT_H_
#define CNC_RED_ALERT_RA_COMBAT_H_
#include "ra/defines.h"
#include "ra/techno.h"

int Modify_Damage(int damage, WarheadType warhead, ArmorType armor,
                  int distance);
void Explosion_Damage(COORDINATE coord, int strength, TechnoClass* source,
                      WarheadType warhead);
AnimType Combat_Anim(int damage, WarheadType warhead, LandType land);
void Wide_Area_Damage(COORDINATE coord, LEPTON radius, int damage,
                      TechnoClass* source, WarheadType warhead);

#endif  // CNC_RED_ALERT_RA_COMBAT_H_
