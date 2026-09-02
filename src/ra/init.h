#ifndef CNC_RED_ALERT_RA_INIT_H_
#define CNC_RED_ALERT_RA_INIT_H_

#include <cstdint>

void Load_Title_Page(bool visible = false);
uint32_t Obfuscate(const char* string);
void Anim_Init();
bool Init_Game(int argc, char* argv[]);
bool Select_Game(bool fade = false);
bool Parse_Command_Line(int argc, char* argv[]);
void Parse_INI_File();

#endif  // CNC_RED_ALERT_RA_INIT_H_
