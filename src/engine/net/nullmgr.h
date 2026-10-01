/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header: /CounterStrike/NULLMGR.H 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLMGR.H                                *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : April 3, 1995   [BR] *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * This is the Connection Manager for a NULL-Modem connection.
 **
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_ENGINE_NET_NULLMGR_H_
#define CNC_RED_ALERT_ENGINE_NET_NULLMGR_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/net/connmgr.h"
#include "engine/net/nullconn.h"

class ModemRegistryEntryClass;

namespace engine::net {

// The serial-line protocol: frames, checks and queues packets over one
// NullModemConnClass. Each game derives its modem dialing from it.
class NullModemClass : public ConnManClass {
 public:
  std::vector<std::byte> BuildBuf;
  int MaxLen;

  // What the modem echoed back, for the dialing screens.
  std::vector<char> EchoBuf;
  int EchoSize{500};
  int EchoCount = 0;

  // `divert` is handed to the connection; see NullModemConnClass.
  NullModemClass(int numsend, int numreceive, int maxlen, uint16_t magicnum,
                 NullModemConnClass::PacketDiverter divert
                     ABSL_ATTRIBUTE_LIFETIME_BOUND = nullptr);
  ~NullModemClass() override;
  NullModemClass(const NullModemClass&) = delete;
  NullModemClass& operator=(const NullModemClass&) = delete;
  NullModemClass(NullModemClass&&) = delete;
  NullModemClass& operator=(NullModemClass&&) = delete;

  // Returns the device to open for `port`: COM1-COM4 for their I/O
  // addresses, or for port 1 the device of the modem named `modem_name`
  // (leaving its entry in `registry`), or `modem_name` itself if no installed
  // modem has that name. Returns nullopt for any other port.
  static std::optional<std::string> Port_Device(
      int port, const char* modem_name,
      std::unique_ptr<ModemRegistryEntryClass>& registry);

  // Opens `device` and creates the connection. Returns 1 on success, 0 (after
  // Shutdown()) if the port does not open.
  int Open(const char* device, int baud, char parity, int wordlength,
           int stopbits, int flowcontrol);
  bool Delete_Connection();
  int Num_Connections() override;
  int Connection_ID(int /*index*/) override { return 0; }
  int Connection_Index(int /*id*/) override { return 0; }
  bool Init_Send_Queue();
  // Closes the port and drops the connection.
  virtual void Shutdown();

  void Set_Timing(int32_t retrydelta, int32_t maxretries,
                  int32_t timeout) override;

  /*
  **	This is how the application sends & receives messages.
  */
  int Send_Message(std::span<const std::byte> buf, int buflen, int ack_req = 1);
  int Get_Message(std::span<std::byte> buf, int* buflen);

  /*
  ** These are for compatibility
  */
  int Send_Private_Message(std::span<const std::byte> buf, int buflen,
                           int ack_req = 1,
                           int /*conn_id*/ = kConnectionNone) override {
    return Send_Message(buf, buflen, ack_req);
  }
  int Get_Private_Message(std::span<std::byte> buf, int* buflen,
                          int* /*conn_id*/) override {
    return Get_Message(buf, buflen);
  }

  /*
  **	The main polling routine; should be called as often as possible.
  */
  int Service() override;

  /*
  **	Queue utility routines.  The application can determine how many
  **	messages are in the send/receive queues, and the queue's average
  **	response time (in clock ticks).
  */
  int Num_Send();
  int Num_Receive();
  int32_t Response_Time() override;
  void Reset_Response_Time() override;
  std::span<const std::byte> Oldest_Send();

  /*
  ** These are for compatibility
  */
  int Global_Num_Send() override { return Num_Send(); }
  int Global_Num_Receive() override { return Num_Receive(); }
  int Private_Num_Send(int /*id*/ = kConnectionNone) override {
    return Num_Send();
  }
  int Private_Num_Receive(int /*id*/ = kConnectionNone) override {
    return Num_Receive();
  }

 protected:
  /*
  **	This is a pointer to the NULL-Modem Connection object.
  */
  std::unique_ptr<NullModemConnClass> Connection;  // null until Open
  int NumConnections{0};  // # connection objects in use

  /*
  ** This is the Win95 port handle
  */
  HANDLE PortHandle{nullptr};

 private:
  int NumSend;
  int NumReceive;
  uint16_t MagicNum;
  NullModemConnClass::PacketDiverter Divert;

  /*
  **	This is the staging buffer for parsing incoming packets.
  **	RXSize is the allocated size of the RX buffer.
  **	RXCount is the # of characters we currently have in our buffer.
  */
  std::vector<char> RXBuf;
  int RXSize = 0;
  int RXCount = 0;

  /*.....................................................................
  Timing parameters for all connections
  .....................................................................*/
  int32_t RetryDelta{60};  // ticks between retries
  int32_t MaxRetries{-1};  // -1 means no limit: retry forever
  int32_t Timeout{1200};   // report bad connection after 20 seconds
};

}  // namespace engine::net

#endif  // CNC_RED_ALERT_ENGINE_NET_NULLMGR_H_
