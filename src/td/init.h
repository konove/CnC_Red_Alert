#ifndef CNC_RED_ALERT_TD_INIT_H_
#define CNC_RED_ALERT_TD_INIT_H_

#include <span>
#include <string_view>

void Uninit_Game();
void Load_Title_Page(bool visible = false);
void Anim_Init();
bool Init_Game();
bool Select_Game(bool fade = false);
// Sets the globals the command-line switches control. `arguments` are the
// command-line arguments without the program name. Returns false when the
// game should not start: the usage text was asked for or a switch is invalid.
bool Parse_Command_Line(std::span<const std::string_view> arguments);
void Parse_INI_File();
int Version_Number();
void Save_Recording_Values();
void Load_Recording_Values();

#endif  // CNC_RED_ALERT_TD_INIT_H_
