#ifndef CNC_RED_ALERT_RA_NULLDLG_H_
#define CNC_RED_ALERT_RA_NULLDLG_H_

#include "ra/session.h"

int Init_Null_Modem(SerialSettingsType* settings);
void Shutdown_Modem();
void Modem_Signoff();
int Test_Null_Modem();
int Reconnect_Modem();
void Destroy_Null_Connection(int id, int error);
GameType Select_Serial_Dialog();
int Com_Scenario_Dialog(bool skirmish = false);
int Com_Show_Scenario_Dialog();

void Smart_Printf(char* format, ...);
void Hex_Dump_Data(char* buffer, int length);
void itoh(int i, char* s);
void Log_Start_Time(char* string);
void Log_End_Time(char* string);
void Log_Time(char* string);
void Log_Start_Nest_Time(char* string);
void Log_End_Nest_Time(char* string);

#endif  // CNC_RED_ALERT_RA_NULLDLG_H_
