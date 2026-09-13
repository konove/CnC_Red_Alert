/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\nullmgr.h_v   1.14   16 Oct 1995 16:45:26
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_NULLMGR_H_
#define CNC_RED_ALERT_TD_NULLMGR_H_

/*
********************************* Includes **********************************
*/
#include <limits>

#include "sdllib/keyboard.h"
#include "td/connmgr.h"
#include "td/defines.h"
#include "td/gadget.h"
#include "td/nullconn.h"

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
  enum SendModemEnum {
    MODEM_CMD_TIMEOUT = 0,
    MODEM_CMD_OK,
    MODEM_CMD_0,
    MODEM_CMD_ERROR
  };

  char* BuildBuf{nullptr};
  int MaxLen;

  char* EchoBuf{nullptr};
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
  NullModemClass(int numsend, int numreceive, int maxlen,
                 unsigned short magicnum);
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
  int Delete_Connection();
  int Num_Connections() override;
  int Connection_ID(int) override { return 0; }
  int Connection_Index(int) override { return 0; }
  int Init_Send_Queue();
  void Shutdown();

  void Set_Timing(unsigned long retrydelta, unsigned long maxretries,
                  unsigned long timeout) override;

  /*
  **	This is how the application sends & receives messages.
  */
  int Send_Message(void* buf, int buflen, int ack_req = 1);
  int Get_Message(void* buf, int* buflen);

  /*
  ** These are for compatibility
  */
  int Send_Private_Message(void* buf, int buflen, int ack_req = 1,
                           int = CONNECTION_NONE) override {
    return Send_Message(buf, buflen, ack_req);
  }
  int Get_Private_Message(void* buf, int* buflen, int*) override {
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
  long Response_Time() override;
  void Reset_Response_Time() override;
  void* Oldest_Send();
  void Configure_Debug(int index, int offset, int size, const char** names,
                       int maxnames) override;
  void Mono_Debug_Print(int index, int refresh = 0) override;

  /*
  ** These are for compatibility
  */
  int Global_Num_Send() override { return Num_Send(); }
  int Global_Num_Receive() override { return Num_Receive(); }
  int Private_Num_Send(int = CONNECTION_NONE) override { return Num_Send(); }
  int Private_Num_Receive(int = CONNECTION_NONE) override {
    return Num_Receive();
  }

  DetectPortType Detect_Port(SerialSettingsType* settings);
  int Detect_Modem(SerialSettingsType* settings, bool reconnect = false);
  DialStatusType Dial_Modem(char* string, DialMethodType method,
                            bool reconnect = false);
  DialStatusType Answer_Modem(bool reconnect = false);
  bool Hangup_Modem();
  void Setup_Modem_Echo(void (*func)(char c));
  void Remove_Modem_Echo();
  void Print_EchoBuf();
  void Reset_EchoBuf();
  // static int Abort_Modem(PORT *);
  static int Abort_Modem();
  void Setup_Abort_Modem();
  void Remove_Abort_Modem();

  int Change_IRQ_Priority(int irq);
  int Get_Modem_Status();
  int Send_Modem_Command(const char* command, char terminator, char* buffer,
                         int buflen, int delay, int retries);
  int Verify_And_Convert_To_Int(char* buffer);

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
  **	This is the Greenleaf port handle.
  */
  // PORT *Port;
  HANDLE PortHandle{nullptr};

  int NumSend;
  int NumReceive;
  unsigned short MagicNum;

  /*
  **	This is the staging buffer for parsing incoming packets.
  **	RXSize is the allocated size of the RX buffer.
  **	RXCount is the # of characters we currently have in our buffer.
  */
  char* RXBuf{nullptr};
  int RXSize = 0;
  int RXCount = 0;

  /*.....................................................................
  Timing parameters for all connections  // 60 ticks between retries
  .....................................................................*/  // disregard # retries
  unsigned long RetryDelta{60};  // report bad connection after 20 seconds
  unsigned long MaxRetries{
      std::numeric_limits<unsigned long>::max()};  // Retry forever.
  unsigned long Timeout{1200};

  /*
  **	Various Statistics
  */
  int SendOverflows{0};
  int ReceiveOverflows{0};
  int CRCErrors{0};
};

#endif  // CNC_RED_ALERT_TD_NULLMGR_H_
