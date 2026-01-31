#ifndef CNC_RED_ALERT_TD_INIT_H_
#define CNC_RED_ALERT_TD_INIT_H_

void Uninit_Game();
void Load_Title_Page(bool visible = false);
long Obfuscate(const char* string);
void Anim_Init();
bool Init_Game(int argc, char* argv[]);
bool Select_Game(bool fade = false);
bool Parse_Command_Line(int argc, char* argv[]);
void Parse_INI_File();
int Version_Number();
void Save_Recording_Values();
void Load_Recording_Values();

#endif  // CNC_RED_ALERT_TD_INIT_H_
