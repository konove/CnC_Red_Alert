#ifndef CNC_RED_ALERT_TD_NULLDLG_H_
#define CNC_RED_ALERT_TD_NULLDLG_H_

#include "td/defines.h"

int Init_Null_Modem(SerialSettingsType* settings);
void Shutdown_Modem(void);
void Modem_Signoff(void);
int Test_Null_Modem(void);
int Reconnect_Modem(void);
void Destroy_Null_Connection(int id, int error);
GameType Select_Serial_Dialog(void);
int Com_Scenario_Dialog(void);
int Com_Show_Scenario_Dialog(void);

void Smart_Printf(char* format, ...);
void Hex_Dump_Data(char* buffer, int length);
void itoh(int i, char* s);

#endif  // CNC_RED_ALERT_TD_NULLDLG_H_
