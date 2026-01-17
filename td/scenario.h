#ifndef CNC_RED_ALERT_TD_SCENARIO_H_
#define CNC_RED_ALERT_TD_SCENARIO_H_

#include "td/defines.h"

bool End_Game(void);
bool Read_Scenario(char* root);
bool Start_Scenario(char* root, bool briefing = true);
HousesType Select_House(void);
void Clear_Scenario(void);
void Do_Briefing(char const* text);
void Do_Lose(void);
void Do_Win(void);
void Do_Restart(void);
void Fill_In_Data(void);
bool Restate_Mission(char const* name, int button1, int button2);

#endif  // CNC_RED_ALERT_TD_SCENARIO_H_
