// File: The Westwood Chat hand-off Tiberian Dawn starts internet games from.

#ifndef CNC_RED_ALERT_TD_INTERNET_H_
#define CNC_RED_ALERT_TD_INTERNET_H_

// Shows the internet menu. Returns whether the player chose to go on to a
// game rather than backing out.
bool Do_The_Internet_Menu_Thang();

// Brings Westwood Chat to the front, launching it when 'can_launch' allows.
// Returns whether it is running afterwards.
bool Spawn_WChat(bool can_launch);

// Reads the settings Westwood Chat left in C&CSPAWN.INI and applies them.
void Check_From_WChat(const char* wchat_name);

#endif  // CNC_RED_ALERT_TD_INTERNET_H_
