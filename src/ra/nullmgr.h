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
 *                    File Name : CONNECT.H                                *
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

#ifndef CNC_RED_ALERT_RA_NULLMGR_H_
#define CNC_RED_ALERT_RA_NULLMGR_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "ra/connmgr.h"
#include "ra/gadget.h"
#include "ra/nullconn.h"
#include "ra/session.h"
#include "sdllib/keyboard.h"

/*
** Ugly hack: this string stores the string received from the modem
*/
inline char ModemRXString[80] = {};

/*
***************************** Class Declaration *****************************
*/
class NullModemClass : public ConnManClass {
  /*
  ---------------------------- Public Interface ----------------------------
  */
 public:
  static constexpr int kModemCmdTimeout = 0;
  static constexpr int kModemCmdOk = kModemCmdTimeout + 1;
  static constexpr int kModemCmd0 = kModemCmdOk + 1;
  static constexpr int kModemCmdError = kModemCmd0 + 1;

  std::vector<std::byte> BuildBuf;
  int MaxLen;

  std::vector<char> EchoBuf;
  int EchoSize{500};
  int EchoCount = 0;

  int OldIRQPri{-1};  // default true

  bool ModemVerboseOn{false};   // default 50 * 1000ms = 50 secs
  bool ModemEchoOn{false};      // default 6  * 100ms  = .6 secs
  int ModemWaitCarrier{50000};  // default 14 * 100ms  = 1.4 secs
  int ModemCarrierDetect{600};  // default 20 * 1000ms = 20 secs
  int ModemCarrierLoss{1400};   // default 50 * 20ms   = 1 sec
  int ModemHangupDelay{20000};  // default ASCII 43
  int ModemGuardTime{1000};
  char ModemEscapeCode{'+'};

  static void (*OrigAbortModemFunc)(int);
  static KeyNumType Input;
  static GadgetClass* Commands;  // button list

  /*
  **	Constructor/destructor.
  */
  NullModemClass(int numsend, int numreceive, int maxlen, uint16_t magicnum);
  ~NullModemClass() override;
  NullModemClass(const NullModemClass&) = delete;
  NullModemClass& operator=(const NullModemClass&) = delete;
  NullModemClass(NullModemClass&&) = delete;
  NullModemClass& operator=(NullModemClass&&) = delete;

  /*
  **	This is the main initialization routine.
  */
  int Init(int port, int irq, char* dev_name, int baud, char parity,
           int wordlength, int stopbits, int flowcontrol);
  bool Delete_Connection();
  int Num_Connections() override;
  int Connection_ID(int /*index*/) override { return 0; }
  int Connection_Index(int /*id*/) override { return 0; }
  bool Init_Send_Queue();
  void Shutdown();

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
  void Configure_Debug(int index, int type_offset, int type_size,
                       const char** names, int namestart,
                       int namecount) override;
  void Mono_Debug_Print(int index, int refresh = 0) override;

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

  static DetectPortType Detect_Port(SerialSettingsType* settings);
  int Detect_Modem(SerialSettingsType* settings, bool reconnect = false);
  DialStatusType Dial_Modem(const char* string, DialMethodType method,
                            bool reconnect = false);
  DialStatusType Answer_Modem(bool reconnect = false);
  bool Hangup_Modem();
  static void Setup_Modem_Echo(void (*func)(char c));
  static void Remove_Modem_Echo();
  static void Print_EchoBuf();
  void Reset_EchoBuf();
  static int Abort_Modem();
  static void Setup_Abort_Modem();
  static void Remove_Abort_Modem();

  static int Change_IRQ_Priority(int irq);
  static uint32_t Get_Modem_Status();
  static int Send_Modem_Command(const char* command, char terminator,
                                char* buffer, int buflen, int delay,
                                int retries);
  static int Verify_And_Convert_To_Int(char* buffer);

  /*
  **	Private Interface.
  */
 private:
  /*
  **	This is a pointer to the NULL-Modem Connection object.
  */
  NullModemConnClass* Connection{nullptr};
  int NumConnections{0};  // # connection objects in use

  /*
  ** This is the Win95 port handle
  */
  HANDLE PortHandle{nullptr};

  int NumSend;
  int NumReceive;
  uint16_t MagicNum;

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

  /*
  **	Various Statistics
  */
  int SendOverflows{0};
  int ReceiveOverflows{0};
  int CRCErrors{0};
};

#endif  // CNC_RED_ALERT_RA_NULLMGR_H_

/*************************** end of nullmgr.h ******************************/
