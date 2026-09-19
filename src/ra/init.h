#ifndef CNC_RED_ALERT_RA_INIT_H_
#define CNC_RED_ALERT_RA_INIT_H_

#include <span>
#include <string_view>

void Load_Title_Page(bool visible = false);
void Anim_Init();
bool Init_Game();
bool Select_Game(bool fade = false);
// Sets the globals the command-line switches control. `arguments` are the
// command-line arguments without the program name. Prints the usage text and
// returns false on "-?" or an invalid "-X" option; true otherwise.
bool Parse_Command_Line(std::span<const std::string_view> arguments);
void Parse_INI_File();

#endif  // CNC_RED_ALERT_RA_INIT_H_
