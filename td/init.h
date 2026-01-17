#ifndef CNC_RED_ALERT_TD_INIT_H_
#define CNC_RED_ALERT_TD_INIT_H_

void Uninit_Game(void);
void Load_Title_Page(bool visible = false);
long Obfuscate(char const* string);
void Anim_Init(void);
bool Init_Game(int argc, char* argv[]);
bool Select_Game(bool fade = false);
bool Parse_Command_Line(int argc, char* argv[]);
void Parse_INI_File(void);
int Version_Number(void);
void Save_Recording_Values(void);
void Load_Recording_Values(void);

#endif  // CNC_RED_ALERT_TD_INIT_H_
