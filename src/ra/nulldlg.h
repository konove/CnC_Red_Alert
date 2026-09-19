#ifndef CNC_RED_ALERT_RA_NULLDLG_H_
#define CNC_RED_ALERT_RA_NULLDLG_H_

#include "ra/session.h"

bool Init_Null_Modem(SerialSettingsType* settings);
void Shutdown_Modem();
void Modem_Signoff();
int Test_Null_Modem();
int Reconnect_Modem();
void Destroy_Null_Connection(int id, int error);
GameType Select_Serial_Dialog();
int Com_Scenario_Dialog(bool skirmish = false);
int Com_Show_Scenario_Dialog();

class ModemRegistryEntryClass;
extern ModemRegistryEntryClass* ModemRegistry;

#endif  // CNC_RED_ALERT_RA_NULLDLG_H_
