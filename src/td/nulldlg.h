#ifndef CNC_RED_ALERT_TD_NULLDLG_H_
#define CNC_RED_ALERT_TD_NULLDLG_H_

#include "td/defines.h"

int Init_Null_Modem(SerialSettingsType* settings);
void Shutdown_Modem();
void Modem_Signoff();
int Test_Null_Modem();
int Reconnect_Modem();
void Destroy_Null_Connection(int id, int error);
GameType Select_Serial_Dialog();
int Com_Scenario_Dialog();
int Com_Show_Scenario_Dialog();

void Smart_Printf(const char* format, ...);
void Hex_Dump_Data(char* buffer, int length);
void itoh(int i, char* s);

#endif  // CNC_RED_ALERT_TD_NULLDLG_H_
