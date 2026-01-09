#ifndef CNC_RED_ALERT_TD_COMBAT_H_
#define CNC_RED_ALERT_TD_COMBAT_H_

int Modify_Damage(int damage, WarheadType warhead, ArmorType armor,
                  int distance);
void Explosion_Damage(COORDINATE coord, unsigned strength, TechnoClass *source,
                      WarheadType warhead);

#endif  // CNC_RED_ALERT_TD_COMBAT_H_
