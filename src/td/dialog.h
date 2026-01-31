#ifndef CNC_RED_ALERT_TD_DIALOG_H_
#define CNC_RED_ALERT_TD_DIALOG_H_

#include "sdllib/include/wwstd.h"
#include "td/defines.h"
#include "td/jshell.h"

int Format_Window_String(char* string, int maxlinelen, int& width, int& height);
extern void Dialog_Box(int x, int y, int w, int h);
void Conquer_Clip_Text_Print(const char*, unsigned x, unsigned y, unsigned fore,
                             unsigned back = TBLACK,
                             TextPrintType flag = TPF_8POINT | TPF_DROPSHADOW,
                             unsigned width = -1, const int* tabs = nullptr);
void Draw_Box(int x, int y, int w, int h, BoxStyleEnum up, bool filled);
int __cdecl Dialog_Message(char* errormsg, ...);
void Window_Box(WindowNumberType window, BoxStyleEnum style);
void Fancy_Text_Print(const char* text, unsigned x, unsigned y, unsigned fore,
                      unsigned back, TextPrintType flag, ...);
void Fancy_Text_Print(int text, unsigned x, unsigned y, unsigned fore,
                      unsigned back, TextPrintType flag, ...);
void Simple_Text_Print(const char* text, unsigned x, unsigned y, unsigned fore,
                       unsigned back, TextPrintType flag);

#endif  // CNC_RED_ALERT_TD_DIALOG_H_
