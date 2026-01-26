#ifndef CNC_RED_ALERT_TD_SCENARIO_H_
#define CNC_RED_ALERT_TD_SCENARIO_H_

#include "td/defines.h"

bool End_Game();
bool Read_Scenario(char* root);
bool Start_Scenario(char* root, bool briefing = true);
HousesType Select_House();
void Clear_Scenario();
void Do_Briefing(char const* text);
void Do_Lose();
void Do_Win();
void Do_Restart();
void Fill_In_Data();
bool Restate_Mission(char const* name, int right_btn, int left_btn);

#endif  // CNC_RED_ALERT_TD_SCENARIO_H_
