#ifndef CNC_RED_ALERT_RA_INIT_H_
#define CNC_RED_ALERT_RA_INIT_H_

#include <span>

void Load_Title_Page(bool visible = false);
void Anim_Init();
bool Init_Game();
bool Select_Game(bool fade = false);
bool Parse_Command_Line(std::span<char*> arguments);
void Parse_INI_File();

#endif  // CNC_RED_ALERT_RA_INIT_H_
