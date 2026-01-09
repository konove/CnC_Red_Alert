#ifndef CNC_RED_ALERT_TD_NETDLG_H_
#define CNC_RED_ALERT_TD_NETDLG_H_

#include "td/defines.h"
#include "td/ipxaddr.h"

bool Init_Network(void);
void Shutdown_Network(void);
bool Remote_Connect(void);
void Destroy_Connection(int id, int error);
bool Process_Global_Packet(GlobalPacketType *packet, IPXAddressClass *address);
unsigned long Compute_Name_CRC(char *name);
void Net_Reconnect_Dialog(int reconn, int fresh, int oldest_index,
                          unsigned long timeval);

#endif  // CNC_RED_ALERT_TD_NETDLG_H_
