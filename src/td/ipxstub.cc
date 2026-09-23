#include <cstddef>
#include <cstdint>

#include "td/ipx95.h"

bool IPX_Initialise() { return false; }

bool IPX_Get_Outstanding_Buffer95(unsigned char* /*buffer*/) { return false; }

void IPX_Shut_Down95() {}

int IPX_Send_Packet95(unsigned char* /*unused*/, const std::byte* /*unused*/,
                      int /*unused*/, unsigned char* /*unused*/,
                      unsigned char* /*unused*/) {
  return 0;
}

int IPX_Broadcast_Packet95(const std::byte* /*unused*/, int /*unused*/) {
  return 0;
}

bool IPX_Start_Listening95() { return false; }

int IPX_Open_Socket95(int /*socket*/) { return 0; }

void IPX_Close_Socket95(int /*socket*/) {}

int IPX_Get_Connection_Number95() { return 0; }

int IPX_Get_Local_Target95(unsigned char* /*unused*/, unsigned char* /*unused*/,
                           uint16_t /*unused*/, unsigned char* /*unused*/) {
  return 0;
}
