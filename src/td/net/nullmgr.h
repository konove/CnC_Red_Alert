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

#ifndef CNC_RED_ALERT_TD_NET_NULLMGR_H_
#define CNC_RED_ALERT_TD_NET_NULLMGR_H_

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

/*
********************************* Includes **********************************
*/
#include <cstdint>

#include "engine/net/nullmgr.h"
#include "engine/window/keyboard.h"
#include "td/core/defines.h"
#include "td/ui/gadget.h"

namespace td {

/*
** Ugly hack: this string stores the string received from the modem
*/
inline char ModemRXString[80] = {};

/*
***************************** Class Declaration *****************************
*/
class NullModemClass : public engine::net::NullModemClass {
  /*
  ---------------------------- Public Interface ----------------------------
  */
 public:
  static constexpr int kModemCmdTimeout = 0;
  static constexpr int kModemCmdOk = 1;
  static constexpr int kModemCmd0 = 2;
  static constexpr int kModemCmdError = 3;

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
  static engine::window::KeyNumber Input;
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
  void Shutdown() override;

  static DetectPortType Detect_Port(SerialSettingsType* settings);
  int Detect_Modem(SerialSettingsType* settings, bool reconnect = false);
  DialStatusType Dial_Modem(char* string, DialMethodType method,
                            bool reconnect = false);
  DialStatusType Answer_Modem(bool reconnect = false);
  bool Hangup_Modem();
  static void Setup_Modem_Echo(void (*func)(char c));
  static void Remove_Modem_Echo();
  static void Print_EchoBuf();
  void Reset_EchoBuf();
  // static int Abort_Modem(PORT *);
  static int Abort_Modem();
  static void Setup_Abort_Modem();
  static void Remove_Abort_Modem();

  static int Change_IRQ_Priority(int irq);
  static unsigned Get_Modem_Status();
  static int Send_Modem_Command(const char* command, char terminator,
                                char* buffer, int buflen, int delay,
                                int retries);
  static int Verify_And_Convert_To_Int(char* buffer);
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_NET_NULLMGR_H_
