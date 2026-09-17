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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer/ WW Library *
 *                                                                                             *
 *                    File Name : WINCOMM.H *
 *                                                                                             *
 *                   Programmer : Steve Tall *
 *                                                                                             *
 *                   Start Date : 1/10/96 *
 *                                                                                             *
 *                  Last Update : January 10th 1996 [ST] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview: *
 *                                                                                             *
 *   These classes was created to replace the greenleaf comms functions used in
 *C&C DOS with   * WIN32 API calls. *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions: *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#ifndef CNC_RED_ALERT_SDLLIB_WINCOMM_H_
#define CNC_RED_ALERT_SDLLIB_WINCOMM_H_

#include <cstdint>

enum class WinCommDialMethodType { WC_TOUCH_TONE = 0, WC_PULSE };
using enum WinCommDialMethodType;

#define COMMSUCCESS 0
#define ASTIMEOUT (-10)
#define COMMUSERABORT (-16)
#define ASSUCCESS COMMSUCCESS
#define ASUSERABORT COMMUSERABORT
using HANDLE = void*;

// Modem status bits returned by Get_Modem_Status().
inline constexpr uint32_t kCtsSet = 0x10;
inline constexpr uint32_t kDsrSet = 0x20;
inline constexpr uint32_t kRiSet = 0x40;
inline constexpr uint32_t kCdSet = 0x80;

/*
** WinModemClass.
**
** This class provides access to modems under Win95. The functions are designed
*to be more or less
** drop in replacements for the Grenleaf comms functions.
*/

class WinModemClass {
 public:
  WinModemClass() = default;
  virtual ~WinModemClass() = default;
  WinModemClass(const WinModemClass&) = delete;
  WinModemClass& operator=(const WinModemClass&) = delete;
  WinModemClass(WinModemClass&&) = delete;
  WinModemClass& operator=(WinModemClass&&) = delete;

  /*
  ** Serial port open should be called to get a handle to the COM port
  ** This needs to be called first as other class members rely on the handle
  **
  ** Replacement for Greenleaf function: PortOpenGreenleafFast
  */
  // virtual	HANDLE	Serial_Port_Open (int port, int baud, int parity, int
  // wordlen, int stopbits);
  virtual HANDLE Serial_Port_Open(const char* device_name, int baud, int parity,
                                  int wordlen, int stopbits, int flowcontrol);

  /*
  ** This function releases the COM port handle and should be called after
  ** communications have finished
  **
  ** Replacement for Greenleaf function: PortClose
  */
  void Serial_Port_Close();

  /*
  ** This member copies up to buffer_len bytes waiting on the serial port
  ** into dest_ptr and returns how many were copied (0 if none).
  **
  ** Replacement for Greenleaf function: ReadBuffer
  */
  static int Read_From_Serial_Port(void* dest_ptr, int buffer_len);

  /*
  ** Writes length bytes from buffer to the serial port.
  **
  ** Replacement for Greenleaf function: WriteBuffer
  */
  void Write_To_Serial_Port(const void* buffer, int length);

  /*
  ** Wait for the outgoing buffer to empty
  */
  void Wait_For_Serial_Write();

  /*
  ** Set the dial type to DIAL_TOUCH_TONE or DIAL_PULSE
  **
  ** Replacement for Greenleaf function: HMSetDiallingMethod
  */
  virtual void Set_Modem_Dial_Type(WinCommDialMethodType method);

  /*
  ** Get the status of the modem control lines
  ** Possible flags are: kCtsSet kDsrSet kRiSet & kCdSet
  **
  ** Replacement for Greenleaf function: GetModemStatus
  */
  virtual unsigned Get_Modem_Status();

  /*
  ** Set the DTR line to the given state
  **
  ** Replacement for Greenleaf function: SetDtr
  */
  virtual void Set_Serial_DTR(bool state);

  /*
  ** Get the result code from the modem after issuing an 'AT' command
  **
  ** Replacement for Greenleaf function: HMInputLine
  */
  virtual int Get_Modem_Result(int delay, const char* buffer, int buffer_len);

  /*
  ** Issue a dial command to the modem.
  ** Use Set_Modem_Dial_Type to select pulse or tone dial
  **
  ** Replacement for Greenleaf function: HMDial
  */
  virtual void Dial_Modem(const char* dial_number);

  /*
  ** Send a command to the modem. This is usually an 'AT' command.
  ** Function will optionally retry until 'OK' is received.
  */
  virtual int Send_Command_To_Modem(const char* command, char terminator,
                                    char* buffer, int buflen, int delay,
                                    int retries);

  /*
  ** Sets a pointer to a function that will be called for each incoming serial
  *char
  **
  ** Replacement for Greenleaf function: HMSetUpEchoRoutine
  */
  virtual void Set_Echo_Function(void (*func)(char c));

  /*
  ** Sets a pointer to a function that will be called if ESC is pressed during a
  *dial
  **
  ** Replacement for Greenleaf function: HMSetUpAbortKey
  */
  virtual void Set_Abort_Function(int (*func)());

  /*
  ** Member to allow access to the serial port handle
  */
  [[nodiscard]] HANDLE Get_Port_Handle() const;

  // Modem send result codes.
  static constexpr int kModemCmdTimeout = 0;
  static constexpr int kModemCmdOk = 1;
  static constexpr int kModemCmd0 = 2;
  static constexpr int kModemCmdError = 3;

  // Modem status flags.
  static constexpr uint32_t kCtsSet = 0x10;
  static constexpr uint32_t kDsrSet = 0x20;
  static constexpr uint32_t kRiSet = 0x40;
  static constexpr uint32_t kCdSet = 0x80;

 protected:
  /*
  ** Pointer to the internal class circular buffer for incoming data
  */
  unsigned char* SerialBuffer = nullptr;

  /*
  ** Head and Tail pointers for our internal serial buffer
  */
  int SerialBufferReadPtr = 0;
  int SerialBufferWritePtr = 0;

  /*
  ** Windows handle to the COM port device
  */
  HANDLE PortHandle = nullptr;

  /*
  ** Dialing method - DIAL_TOUCH_TONE or DIAL_PULSE
  */
  WinCommDialMethodType DialingMethod = WC_TOUCH_TONE;
};

/*
** WinNullModemClass.
**
** This class provides access to serial ports under Win95. The functions are
*designed to be more or less
** drop in replacements for the Grenleaf comms functions.
**
** This class just overloads the WinModemClass members that arent required for
*direct serial communications
** via a 'null modem' cable.
*/
class WinNullModemClass : public WinModemClass {
 public:
  void Set_Modem_Dial_Type(WinCommDialMethodType /*method*/) override {}
  unsigned Get_Modem_Status() override { return 0; }
  void Set_Serial_DTR(bool /*state*/) override {}
  int Get_Modem_Result(int /*delay*/, const char* /*buffer*/,
                       int /*buffer_len*/) override {
    return 0;
  }
  void Dial_Modem(const char* /*dial_number*/) override {}
  int Send_Command_To_Modem(const char* /*command*/, char /*terminator*/,
                            char* /*buffer*/, int /*buflen*/, int /*delay*/,
                            int /*retries*/) override {
    return 0;
  }
  void Set_Echo_Function(void (* /*func*/)(char)) override {}
  void Set_Abort_Function(int (* /*func*/)()) override {}
};

extern WinModemClass* SerialPort;

#endif  // CNC_RED_ALERT_SDLLIB_WINCOMM_H_
