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
  enum SendModemEnum {
    MODEM_CMD_TIMEOUT = 0,
    MODEM_CMD_OK,
    MODEM_CMD_0,
    MODEM_CMD_ERROR
  };

  char* BuildBuf;
  int MaxLen;

  char* EchoBuf;
  int EchoSize;
  int EchoCount;

  int OldIRQPri;

  int ModemVerboseOn;
  int ModemEchoOn;
  int ModemWaitCarrier;
  int ModemCarrierDetect;
  int ModemCarrierLoss;
  int ModemHangupDelay;
  int ModemGuardTime;
  char ModemEscapeCode;

  static void (*OrigAbortModemFunc)(int);
  static KeyNumType Input;
  static GadgetClass* Commands;  // button list

  /*
  **	Constructor/destructor.
  */
  NullModemClass(int numsend, int numreceive, int maxlen,
                 unsigned short magicnum);
  ~NullModemClass() override;

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
  void Configure_Debug(int index, int type_offset, int type_size,
                       const char** names, int namestart,
                       int namecount) override;
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
  int Detect_Modem(SerialSettingsType* settings, bool reconnect = 0);
  DialStatusType Dial_Modem(const char* string, DialMethodType method,
                            bool reconnect = 0);
  DialStatusType Answer_Modem(bool reconnect = 0);
  bool Hangup_Modem();
  void Setup_Modem_Echo(void (*func)(char c));
  void Remove_Modem_Echo();
  void Print_EchoBuf();
  void Reset_EchoBuf();
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
  NullModemConnClass* Connection;
  int NumConnections;  // # connection objects in use

  /*
  ** This is the Win95 port handle
  */
  HANDLE PortHandle;

  int NumSend;
  int NumReceive;
  unsigned short MagicNum;

  /*
  **	This is the staging buffer for parsing incoming packets.
  **	RXSize is the allocated size of the RX buffer.
  **	RXCount is the # of characters we currently have in our buffer.
  */
  char* RXBuf;
  int RXSize;
  int RXCount;

  /*.....................................................................
  Timing parameters for all connections
  .....................................................................*/
  unsigned long RetryDelta;
  unsigned long MaxRetries;
  unsigned long Timeout;

  /*
  **	Various Statistics
  */
  int SendOverflows;
  int ReceiveOverflows;
  int CRCErrors;
};

#endif  // CNC_RED_ALERT_RA_NULLMGR_H_

/*************************** end of nullmgr.h ******************************/
