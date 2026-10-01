#ifndef CNC_RED_ALERT_TD_NET_NETDLG_H_
#define CNC_RED_ALERT_TD_NET_NETDLG_H_

#include <cstdint>

#include "td/core/defines.h"
#include "td/net/ipxaddr.h"

namespace td {

bool Init_Network();
void Shutdown_Network();
bool Remote_Connect();
void Destroy_Connection(int id, int error);
bool Process_Global_Packet(GlobalPacketType* packet, IPXAddressClass* address);
uint32_t Compute_Name_CRC(const char* name);
void Net_Reconnect_Dialog(bool reconn, bool fresh, int oldest_index,
                          int timeval);

bool Client_Remote_Connect();
bool Server_Remote_Connect();

}  // namespace td

#endif  // CNC_RED_ALERT_TD_NET_NETDLG_H_
