#ifndef CNC_RED_ALERT_RA_NET_NETDLG_H_
#define CNC_RED_ALERT_RA_NET_NETDLG_H_

#include <cstdint>
#include <span>

#include "engine/net/ipxaddr.h"
#include "ra/net/session.h"

namespace ra {

bool Init_Network();
void Shutdown_Network();
bool Remote_Connect();
void Destroy_Connection(int id, int error);
bool Process_Global_Packet(GlobalPacketType* packet,
                           engine::net::IPXAddressClass* address);
uint32_t Compute_Name_CRC(const char* name);
void Net_Reconnect_Dialog(bool reconn, bool fresh, int oldest_index,
                          int timeval);

extern const std::span<const char* const> EngMisStr;

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_NET_NETDLG_H_
