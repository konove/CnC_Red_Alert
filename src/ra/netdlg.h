#ifndef CNC_RED_ALERT_RA_NETDLG_H_
#define CNC_RED_ALERT_RA_NETDLG_H_
#include <cstdint>

#include "ra/ipxaddr.h"
#include "ra/session.h"

bool Init_Network();
void Shutdown_Network();
bool Remote_Connect();
void Destroy_Connection(int id, int error);
bool Process_Global_Packet(GlobalPacketType* packet, IPXAddressClass* address);
uint32_t Compute_Name_CRC(char* name);
void Net_Reconnect_Dialog(int reconn, int fresh, int oldest_index,
                          unsigned long timeval);

#endif  // CNC_RED_ALERT_RA_NETDLG_H_
